#include <pebble.h>

#include "config.h"
#include "model.h"
#include "view.h"
#include "storage.h"

#define FETCH_RETRIES 5
bool js_ready = false;
int weather_fetch_countdown = 0;
  
#if defined(PBL_HEALTH)
#define ACTIVITY_MONITOR_WINDOW 10
const int STEPS_PER_MINUTE_WALKING = 45;
const int STEPS_PER_MINUTE_RUNNING = 120;
  
struct ActivityStamp {
  int totalCalories;
  int totalDistance;
  int totalStepCount;
  time_t time;
};

struct ActivityStamp activity_start;
struct ActivityStamp activity_buffer[ACTIVITY_MONITOR_WINDOW];
int activity_buffer_index;
#endif

static bool is_asleep() {
  bool sleeping = false;
  #if defined(PBL_HEALTH) 
  HealthActivityMask activities = health_service_peek_current_activities();
  sleeping = activities & (HealthActivitySleep | HealthActivityRestfulSleep);
  #endif
  return sleeping;
}

static bool fetch_weather() {
  // Check configuration of weather/sunrise/sunset, bluetooth connection, JS Ready and sleeping
  if ((config->enable_sun || config->enable_weather) && connection_service_peek_pebble_app_connection() && js_ready && !is_asleep()) {
    // Bluetooth connected, prepare outgoing message
    DictionaryIterator *iter;
    AppMessageResult result = app_message_outbox_begin(&iter);
    if(result == APP_MSG_OK) { // Message could not be prepared
      // Send outgoing message
      Tuplet value = TupletInteger(MESSAGE_KEY_Fetch, 1);
      dict_write_tuplet(iter, &value);
      dict_write_end(iter);
      
      result = app_message_outbox_send();
    }
    
    return result == APP_MSG_OK;
  } else {
    // Did not fetch weather
    return false;
  }
}

static void msg_received_handler(DictionaryIterator *iter, void *context) {
  // Handle and incoming message from the phone
  Tuple *tuple = dict_find(iter, MESSAGE_KEY_JSReady);
  if(tuple) {
        js_ready = true;
        // Watch is ready for communication, is weather fetch wanted?  
        fetch_weather();
  }
  
  // Weather
  bool wtrChanged = false;    
  tuple = dict_find(iter, MESSAGE_KEY_Temperature);
  if(tuple && (wtrChanged = true)) {
    model_set_weather_temperature(tuple->value->int32);
    
    // Weather fetch was succesfull, reset fetch countdown
    weather_fetch_countdown = config->weather_refresh;
  }
  tuple = dict_find(iter, MESSAGE_KEY_Condition);
  if(tuple && (wtrChanged = true)) model_set_weather_condition(tuple->value->int32);
  tuple = dict_find(iter, MESSAGE_KEY_Sunrise);
  if(tuple && (wtrChanged = true)) model_set_sunrise(tuple->value->int32);
  tuple = dict_find(iter, MESSAGE_KEY_Sunset);
  if(tuple && (wtrChanged = true)) model_set_sunset(tuple->value->int32);
  
  // Error
  tuple = dict_find(iter, MESSAGE_KEY_Err);
  if(tuple) {
    // Error encountered
    model_set_error(tuple->value->int32);
  } else if (wtrChanged) {
    // Reset error if necessary
    if (model->error != ERROR_NONE) {
      model_set_error(ERROR_NONE);
    }
  } 
  
  // Configuration
  bool cfgChanged = parse_configuration_messages(iter);    
  if (cfgChanged) {
    // Hidden function to clear crash detection history
    if (gcolor_equal(config->color_background, GColorWhite) && gcolor_equal(config->color_primary, GColorWhite)) {
      persist_delete(STORAGE_CRASH_NUM);
      persist_delete(STORAGE_CRASH_TIMESTAMPS);
    }
    
    // Restart view  
    model_reset_events();
    view_deinit();
    view_init();
    
    // Refetch weather in case weather settings were changed
    fetch_weather();
  }
}

#if defined(PBL_HEALTH)
void update_health() {
  // Fill the activity buffer for this minute
  activity_buffer[activity_buffer_index].totalCalories = (int)health_service_sum_today(HealthMetricActiveKCalories);
  activity_buffer[activity_buffer_index].totalDistance = (int)health_service_sum_today(HealthMetricWalkedDistanceMeters);
  activity_buffer[activity_buffer_index].totalStepCount = (int)health_service_sum_today(HealthMetricStepCount);
  activity_buffer[activity_buffer_index].time = time(NULL);
  
  // Calculate the average step pace in the buffer
  int avg_steps_per_minute;
  int first_index;
  int last_index = activity_buffer_index;
  if (activity_buffer[ACTIVITY_MONITOR_WINDOW - 1].time == 0) {
    // Buffer isn't completely full yet
    first_index = 0;
  } else {
    first_index = (activity_buffer_index + 1) % ACTIVITY_MONITOR_WINDOW;
  }
  if (first_index != last_index) {
    // Check for counter reset at midnight
    if (activity_buffer[last_index].totalStepCount < activity_buffer[first_index].totalStepCount) {
      // Invert values in the buffer below zero, the last minute before midnight will seem like a pauze as no data of it is available
      int maxValuesIndex = (last_index - 1 + ACTIVITY_MONITOR_WINDOW) % ACTIVITY_MONITOR_WINDOW;
      for (int i = 0; i < ACTIVITY_MONITOR_WINDOW; ++i) {
        if (i != last_index) {
          activity_buffer[i].totalCalories -= activity_buffer[maxValuesIndex].totalCalories;
          activity_buffer[i].totalDistance -= activity_buffer[maxValuesIndex].totalDistance;
          activity_buffer[i].totalStepCount -= activity_buffer[maxValuesIndex].totalStepCount;          
        }
      }
    }
    
    // Calculate average step pace
    avg_steps_per_minute = activity_buffer[last_index].totalStepCount - activity_buffer[first_index].totalStepCount;
    avg_steps_per_minute /= (last_index - first_index + ACTIVITY_MONITOR_WINDOW) % ACTIVITY_MONITOR_WINDOW;
  } else {
    // Only one point in the buffer, we need at least two to compare them
    avg_steps_per_minute = 0;
  }
  
  // Determine the current activity
  enum Activities current_activity;
  if (is_asleep()) {
    current_activity = ACTIVITY_SLEEP;
  } else if (avg_steps_per_minute >= STEPS_PER_MINUTE_RUNNING) {
    current_activity = ACTIVITY_RUN;
  } else if (avg_steps_per_minute >= STEPS_PER_MINUTE_WALKING) {
    current_activity = ACTIVITY_WALK;
  } else {
    current_activity = ACTIVITY_NORMAL;
  }
  
  // Create sharp begin edge
  if (avg_steps_per_minute > 0 && (current_activity != model->activity || activity_start.time == 0)) {
    if (model->activity == ACTIVITY_RUN && current_activity == ACTIVITY_WALK) {
      // We stopped running, keep running as current activity, check at the sharp end check whether to switch back to walking or not
      current_activity = ACTIVITY_RUN;
    } else {
      if (current_activity == ACTIVITY_WALK || current_activity == ACTIVITY_RUN) {
        // Starting new walk or run, find exact activity start
        for (int index = first_index, next_index = (index + 1) % ACTIVITY_MONITOR_WINDOW; next_index != last_index; index = next_index, next_index = (index + 1) % ACTIVITY_MONITOR_WINDOW) {
          int steps_this_minute = activity_buffer[next_index].totalStepCount - activity_buffer[index].totalStepCount;
          if (steps_this_minute < (current_activity == ACTIVITY_WALK ? STEPS_PER_MINUTE_WALKING : STEPS_PER_MINUTE_RUNNING)) {
            // Activity had not yet started this minute
            first_index = next_index;
          } else {
            // Activity had started this minute, start_index found
            if (current_activity == ACTIVITY_WALK) {
              // Make sure we didn't start a run rather than a walk
              avg_steps_per_minute = activity_buffer[last_index].totalStepCount - activity_buffer[first_index].totalStepCount;
              avg_steps_per_minute /= (last_index - first_index + ACTIVITY_MONITOR_WINDOW) % ACTIVITY_MONITOR_WINDOW;
              if (avg_steps_per_minute >= STEPS_PER_MINUTE_RUNNING) {
                // Started a run rather than a walk
                current_activity = ACTIVITY_RUN;
                if (steps_this_minute < STEPS_PER_MINUTE_RUNNING) {
                  // But this minute wasn't all running, start at the next
                  first_index = next_index;
                }
              }
            }
            break;
          }
        }
      }
      
      // Set activity start
      activity_start = activity_buffer[first_index]; 
    }
  }
  
  // Create sharp end edge
  if (avg_steps_per_minute > 0 && (current_activity == ACTIVITY_WALK || current_activity == ACTIVITY_RUN)) {
    // Find the last minute we actually walked/ran
    bool foundActivity = false;
    for (int index = last_index, prev_index = (index - 1 + ACTIVITY_MONITOR_WINDOW) % ACTIVITY_MONITOR_WINDOW; prev_index != first_index; index = prev_index, prev_index = (index - 1 + ACTIVITY_MONITOR_WINDOW) % ACTIVITY_MONITOR_WINDOW) {
      int steps_this_minute = activity_buffer[index].totalStepCount - activity_buffer[prev_index].totalStepCount;
        if (steps_this_minute < (current_activity == ACTIVITY_WALK ? STEPS_PER_MINUTE_WALKING : STEPS_PER_MINUTE_RUNNING)) {
          // We were not walking/running this minute
          last_index = prev_index;
        } else {
          // Found the last minute we walked/ran
          foundActivity = true;
          break;
        }
    }
    
    // Make sure we didn't go from running to walking
    if (!foundActivity && current_activity == ACTIVITY_RUN) {
      // Didn't find any running, last running minuted dropped out of the buffer, presume started walking at start of buffer although a pauze is possible
      current_activity = ACTIVITY_WALK;
      last_index = activity_buffer_index;
      activity_start = activity_buffer[first_index];
    }
  }
  
  // Update activity counters
  int calories = activity_buffer[last_index].totalCalories - activity_start.totalCalories;
  int duration = (activity_buffer[last_index].time - activity_start.time + SECONDS_PER_MINUTE / 2) / SECONDS_PER_MINUTE;
  int distance = activity_buffer[last_index].totalDistance - activity_start.totalDistance;
  int step_count = activity_buffer[last_index].totalStepCount - activity_start.totalStepCount;
  
  // Update the model
  model_set_activity_counters(calories, duration, distance, step_count);
  if (avg_steps_per_minute > 0 && model->activity != current_activity) model_set_activity(current_activity);
    
  // Move index to next slot in buffer
  activity_buffer_index = (activity_buffer_index + 1) % ACTIVITY_MONITOR_WINDOW;
}
#endif

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (units_changed & MINUTE_UNIT) {
    // Update weather
    weather_fetch_countdown--;
    if (weather_fetch_countdown <= -FETCH_RETRIES) {
      // Ran out of retries, weather fetch failed
      model_set_error(ERROR_FETCH);   
      
      // Reset fetch countdown
      weather_fetch_countdown = config->weather_refresh;
    } else if (weather_fetch_countdown <= 0) {
      // Fetch weather
      if (!fetch_weather()) {
        // Fetching the weather was not tried, suspend countdown
        weather_fetch_countdown++;
      }      
    }
    
    // Update health
    #if defined(PBL_HEALTH)
    update_health();
    #endif
    
    // Update the displayed time
    static struct tm time_copy;
    time_copy = *tick_time;
    model_set_time(&time_copy);
  }
  
  if (units_changed & HOUR_UNIT) {
    // Vibrate on the hour 
    if (!is_asleep() && config->vibrate_hourly) {
      vibes_short_pulse();
    }
  }
}

static void connection_handler(bool connected) {
  // Vibrate if connection changed 
  if ((model->error == ERROR_CONNECTION) == connected) {
    // Check to make sure user is not sleeping and config asks for vibrations
    if (!is_asleep() && config->vibrate_bluetooth) {
      // Vibrate to indicate connection change
      vibes_double_pulse();
    }
  }
  
  // Update model error
  if (!connected) {
    model_set_error(ERROR_CONNECTION);
  } else if (model->error == ERROR_CONNECTION) {
    model_set_error(ERROR_NONE);
  }
}

void battery_handler(BatteryChargeState charge) {
  model_set_battery(charge.charge_percent, charge.is_charging, charge.is_plugged);
}

#if defined(PBL_HEALTH)
void health_init() {
  // Load stored health activity
  if (persist_exists(STORAGE_HEALTH_ACTIVITY) && persist_exists(STORAGE_HEALTH_ACTIVITY_START)) {
    enum Activities saved_activity = persist_read_int(STORAGE_HEALTH_ACTIVITY);
    struct ActivityStamp saved_activity_start;
    persist_read_data(STORAGE_HEALTH_ACTIVITY_START, &saved_activity_start, sizeof(saved_activity_start));
    
    // Validate that the activity is still ongoing
    if (saved_activity == ACTIVITY_SLEEP) {
      // Are we still sleeping?
      if (is_asleep()) {
        // Yep, resume activity
        model_set_activity(saved_activity);
        activity_start = saved_activity_start; 
      }
    } else if (saved_activity == ACTIVITY_WALK || saved_activity == ACTIVITY_RUN) {
      // Is the average #steps since saved activity start greater than the threshold?
      int duration_since_start = (time(NULL) - saved_activity_start.time + SECONDS_PER_MINUTE / 2) / SECONDS_PER_MINUTE;
      int steps_since_start = (int)health_service_sum_today(HealthMetricStepCount) - saved_activity_start.totalStepCount;
      int avg_steps_per_minute = steps_since_start / duration_since_start;
      if (avg_steps_per_minute >= (saved_activity == ACTIVITY_WALK ? STEPS_PER_MINUTE_WALKING : STEPS_PER_MINUTE_RUNNING)) {
        // Yep, resume activity
        activity_start = saved_activity_start; 
        
        int calories = (int)health_service_sum_today(HealthMetricActiveKCalories) - activity_start.totalCalories;
        int duration = duration_since_start;
        int distance = (int)health_service_sum_today(HealthMetricWalkedDistanceMeters) - activity_start.totalDistance;
        int step_count = (int)health_service_sum_today(HealthMetricStepCount) - activity_start.totalStepCount;

        // Update the model
        model_set_activity_counters(calories, duration, distance, step_count);
        model_set_activity(saved_activity);
      }
    }
  }
}
#endif

static void app_init() {     
  // Initialize the configuration
  config_init();
  
  // Initialize the model
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  model_set_time(tick_time);
  if (!bluetooth_connection_service_peek()) model_set_error(ERROR_CONNECTION); // Avoid vibrate on initialize
  battery_handler(battery_state_service_peek());
  
  // Load health data
  #if defined(PBL_HEALTH)
  health_init();
  #endif
  
  // Initialize the view
  view_init();
  
  // Set up watch communication
  const uint32_t inbox_size = 640;
  const uint32_t outbox_size = 64;
  app_message_register_inbox_received(&msg_received_handler);
  app_message_open(inbox_size, outbox_size);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Register with ConnectionService
  connection_service_subscribe((ConnectionHandlers){
    .pebble_app_connection_handler = connection_handler,
    .pebblekit_connection_handler = NULL
  });
  
  // Register with BatteryService
  battery_state_service_subscribe(battery_handler);
}

static void app_deinit() {
  // Unsubscribe from services
  battery_state_service_unsubscribe();
  connection_service_unsubscribe();
  tick_timer_service_unsubscribe();
  app_message_deregister_callbacks();
  
  // De-initialize view
  view_deinit();
  
  // Save configuration
  config_deinit();
  
  // Save health activity
  #if defined(PBL_HEALTH)
  persist_write_int(STORAGE_HEALTH_ACTIVITY, model->activity);
  persist_write_data(STORAGE_HEALTH_ACTIVITY_START, &activity_start, sizeof(struct ActivityStamp)); 
  #endif
}

static void crash_detection_init() {
  // Read previous crash info
  #define MAX_TIMESTAMPS (PERSIST_DATA_MAX_LENGTH / sizeof(time_t))
  uint num_crashes = persist_exists(STORAGE_CRASH_NUM) ? persist_read_int(STORAGE_CRASH_NUM) : 0;
  time_t crash_timestamps[MAX_TIMESTAMPS] = { 0 };
  if (persist_exists(STORAGE_CRASH_TIMESTAMPS)) persist_read_data(STORAGE_CRASH_TIMESTAMPS, &crash_timestamps, sizeof(crash_timestamps));   
    
  // Detect new restart after crash
  if (persist_exists(STORAGE_CRASH_DETECTOR)) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Restarted after crash!");
    
    // Record this crash
    time_t now = time(NULL);
    crash_timestamps[num_crashes % MAX_TIMESTAMPS] = now;
    ++num_crashes;
    
    // Save crash info
    persist_write_int(STORAGE_CRASH_NUM, num_crashes);
    persist_write_data(STORAGE_CRASH_TIMESTAMPS, crash_timestamps, sizeof(crash_timestamps));
  }
  
  // Print crash history
  if (num_crashes > 0) {
    APP_LOG(APP_LOG_LEVEL_INFO, "%d Crashes detected. Restarts at:", num_crashes);
    char timestamp[30];
    for (uint i = 0; i < num_crashes % MAX_TIMESTAMPS; ++i) {
      const struct tm *crash_time = localtime(&crash_timestamps[i]);
      strftime(timestamp, sizeof(timestamp) / sizeof(char), "%F %T", crash_time);
      APP_LOG(APP_LOG_LEVEL_INFO, "[%s]", timestamp);
    }
  }
  
  // Mark app as running to enable crash detection
  persist_write_bool(STORAGE_CRASH_DETECTOR, true);
}

static void crash_detection_deinit() {
  // Mark no crash occured
  persist_delete(STORAGE_CRASH_DETECTOR);
}

int main(void) {
  crash_detection_init();
  app_init();
  app_event_loop(); // Enter the main event loop. This will block until the app is ready to exit.
  app_deinit();
  crash_detection_deinit();
}