#include <pebble.h>
#include <limits.h>

#include "config.h"
#include "model.h"
#include "utils.h"
#include "view.h"
#include "storage.h"
#include "messagequeue.h"

#define FETCH_RETRIES 5
int weather_fetch_countdown = 0;
AppTimer* vibration_overload_timer = NULL;
AppTimer* accel_unsubscribe_timer = NULL;
bool tapping = false;

void altitude_req_changed(bool required);
  
#if defined(PBL_HEALTH)
#define ACTIVITY_MONITOR_WINDOW 10
const int STEPS_PER_MINUTE_WALKING = 45;
const int STEPS_PER_MINUTE_RUNNING = 120;
  
struct ActivityStamp {
  int totalCalories;
  int totalDistance;
  int totalStepCount;
  int totalClimb;
  int totalDescend;
  time_t time;
};

struct ActivityStamp activity_start;
struct ActivityStamp activity_buffer[ACTIVITY_MONITOR_WINDOW];
int activity_buffer_index;

int altitude_climb = 0;
int altitude_descend = 0;
#endif

static bool fetch_weather() {
  // Check configuration of weather/sunrise/sunset and sleeping
  if ((model->update_req & (UPDATE_WEATHER | UPDATE_SUN)) && !is_asleep()) {
    message_queue_send_tuplet(TupletInteger(MESSAGE_KEY_Fetch, 1));    
    return true;
  } else {
    // Did not fetch weather
    return false;
  }
}

static bool fetch_moonphase() {  
  // Check configuration of moonphase, bluetooth connection
  if (model->update_req & UPDATE_MOONPHASE) {
    message_queue_send_tuplet(TupletInteger(MESSAGE_KEY_FetchMoonphase, 1));    
    return true;
  } else {
    // Did not fetch moonphase
    return false;
  }
}

static void msg_received_handler(DictionaryIterator *iter, void *context) {
  // Handle and incoming message from the phone
  Tuple *tuple = dict_find(iter, MESSAGE_KEY_JSReady);
  if(tuple) {
    message_queue_js_is_ready();
    
    // Watch is ready for communication, is weather fetch wanted?  
    fetch_weather();
    
    // Is moonphase fetch wanted?
    fetch_moonphase();
    
    // Subscribe to altitude
    if (model->update_req & UPDATE_ALTITUDE) altitude_req_changed(true);
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
  
  // Moonphase
  bool moonChanged = false;
  int moonphase = 0;
  int moonillumination = 0;
  tuple = dict_find(iter, MESSAGE_KEY_Moonphase);
  if(tuple && (moonChanged = true)) moonphase = tuple->value->int32;
  tuple = dict_find(iter, MESSAGE_KEY_Moonillumination);
  if(tuple && (moonChanged = true)) moonillumination = tuple->value->int32;
  if (moonChanged) model_set_moon(moonphase, moonillumination);
  
  // Altitude 
  tuple = dict_find(iter, MESSAGE_KEY_Altitude);
  if(tuple) {
    int altitude = tuple->value->int32;
#if defined(PBL_HEALTH)
    if (model->altitude != INT_MIN) {
      if (altitude > model->altitude) 
      {
          altitude_climb += altitude - model->altitude;
      } else {
          altitude_descend += model->altitude - altitude;
      }
    }
#endif    
    model_set_altitude(altitude);
  }
  
  // Configuration
  bool cfgChanged = parse_configuration_messages(iter);    
  if (cfgChanged) {
    // Hidden function to clear crash detection history
    if (gcolor_equal(config->color_background, GColorWhite) && gcolor_equal(config->color_primary, GColorWhite)) {
      persist_delete(STORAGE_CRASH_NUM);
      persist_delete(STORAGE_CRASH_TIMESTAMPS);
    }
    
    // Hidden function to crash the watchface
    /*if (gcolor_equal(config->color_background, GColorBlack) && gcolor_equal(config->color_primary, GColorBlack)) {
      void (*nullPtr)();
      nullPtr = 0;
      nullPtr();
    }*/
    
    // Restart view  
    view_deinit();
    view_init();
    
    // Refetch weather and moonphase in case settings were changed
    fetch_weather();
    fetch_moonphase();
    
    // Save configuration
    config_save();
  }
  
  // Send unsent messages
  message_queue_send_next();
}

#if defined(PBL_HEALTH)
void update_health() {
  // Fill the activity buffer for this minute
  activity_buffer[activity_buffer_index].totalCalories = (int)health_service_sum_today(HealthMetricActiveKCalories);
  activity_buffer[activity_buffer_index].totalDistance = (int)health_service_sum_today(HealthMetricWalkedDistanceMeters);
  activity_buffer[activity_buffer_index].totalStepCount = (int)health_service_sum_today(HealthMetricStepCount);
  activity_buffer[activity_buffer_index].totalClimb = altitude_climb;
  activity_buffer[activity_buffer_index].totalDescend = altitude_descend;
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
      struct ActivityStamp maxValues = activity_buffer[maxValuesIndex];
      for (int i = 0; i < ACTIVITY_MONITOR_WINDOW; ++i) {
        if (i != last_index) {
          activity_buffer[i].totalCalories -= maxValues.totalCalories;
          activity_buffer[i].totalDistance -= maxValues.totalDistance;
          activity_buffer[i].totalStepCount -= maxValues.totalStepCount;     
          activity_buffer[i].totalClimb -= maxValues.totalClimb;
          activity_buffer[i].totalDescend -= maxValues.totalDescend;       
        }
      }
      
      // Invert values in allready active activity
      activity_start.totalCalories -= maxValues.totalCalories;
      activity_start.totalDistance -= maxValues.totalDistance;
      activity_start.totalStepCount -= maxValues.totalStepCount;     
      activity_start.totalClimb -= maxValues.totalClimb;
      activity_start.totalDescend -= maxValues.totalDescend;    
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
  int climb = activity_buffer[last_index].totalClimb - activity_start.totalClimb;
  int descend = activity_buffer[last_index].totalDescend - activity_start.totalDescend;
  
  // Update the model
  model_set_activity_counters(calories, duration, distance, step_count, climb, descend);
  if (avg_steps_per_minute > 0 && model->activity != current_activity) model_set_activity(current_activity);
    
  // Move index to next slot in buffer
  activity_buffer_index = (activity_buffer_index + 1) % ACTIVITY_MONITOR_WINDOW;
}

void health_init() {
  // Load stored health activity
  if (persist_exists(STORAGE_HEALTH_ACTIVITY) && persist_exists(STORAGE_HEALTH_ACTIVITY_START)) {
    enum Activities saved_activity = persist_read_int(STORAGE_HEALTH_ACTIVITY);
    struct ActivityStamp saved_activity_start;
    persist_read_data(STORAGE_HEALTH_ACTIVITY_START, &saved_activity_start, sizeof(saved_activity_start));
    
    // Validate that the activity is still ongoing
    bool resume = false;
    int duration_since_start = (time(NULL) - saved_activity_start.time + SECONDS_PER_MINUTE / 2) / SECONDS_PER_MINUTE;
    if (saved_activity == ACTIVITY_SLEEP) {
      // Are we still sleeping?
      if (is_asleep()) {
        // Yep, resume activity
        resume = true;
      }
    } else if (saved_activity == ACTIVITY_WALK || saved_activity == ACTIVITY_RUN) {
      // Is the average #steps since saved activity start greater than the threshold?
      int steps_since_start = (int)health_service_sum_today(HealthMetricStepCount) - saved_activity_start.totalStepCount;
      int avg_steps_per_minute = steps_since_start / duration_since_start;
      if (avg_steps_per_minute >= (saved_activity == ACTIVITY_WALK ? STEPS_PER_MINUTE_WALKING : STEPS_PER_MINUTE_RUNNING)) {
        // Yep, resume activity
        resume = true;
      }
    }
    
    // Resume activity
    if (resume) {
        activity_start = saved_activity_start; 
        
        int calories = (int)health_service_sum_today(HealthMetricActiveKCalories) - activity_start.totalCalories;
        int duration = duration_since_start;
        int distance = (int)health_service_sum_today(HealthMetricWalkedDistanceMeters) - activity_start.totalDistance;
        int step_count = (int)health_service_sum_today(HealthMetricStepCount) - activity_start.totalStepCount;
        
        // Read climb and descend from storage
        altitude_climb = activity_start.totalClimb;
        if (persist_exists(STORAGE_ALTITUDE_CLIMB)) altitude_climb = persist_read_int(STORAGE_ALTITUDE_CLIMB);
        altitude_descend = activity_start.totalDescend;
        if (persist_exists(STORAGE_ALTITUDE_DESCEND)) altitude_descend = persist_read_int(STORAGE_ALTITUDE_DESCEND);
        int climb = altitude_climb - activity_start.totalClimb;
        int descend = altitude_descend - activity_start.totalDescend;

        // Update the model
        model_set_activity_counters(calories, duration, distance, step_count, climb, descend);
        model_set_activity(saved_activity);
    }
  }
}
#endif

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (units_changed & MINUTE_UNIT) {
    // Send unsent messages
    message_queue_send_next();
    
    // Update weather and sunset/sunrise
    if (model->update_req & (UPDATE_WEATHER | UPDATE_SUN)) {
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
    }
    
    // Update health
    #if defined(PBL_HEALTH)
    if (model->update_req & UPDATE_HEALTH) update_health();
    #endif
  }
  
  if (units_changed & HOUR_UNIT) {
    // Vibrate on the hour 
    if (!should_keep_quiet() && config->vibrate_hourly) {
      vibes_short_pulse();
    }
    
    // Refresh moonphase if required
    fetch_moonphase();
  }
      
  // Update the displayed time
  static struct tm time_copy;
    time_copy = *tick_time;
  model_set_time(&time_copy, units_changed);
}

void second_ticks_req_changed(bool required) {
  if (required) {
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  } else {
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  }
}

static void connection_handler(bool connected) {
  // Vibrate if connection changed 
  if ((model->error == ERROR_CONNECTION) == connected) {
    // Check to make sure user is not sleeping and config asks for vibrations
    if (!should_keep_quiet() && config->vibrate_bluetooth) {
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

static void battery_handler(BatteryChargeState charge) {
  model_set_battery(charge.charge_percent, charge.is_charging, charge.is_plugged);
}

void battery_req_changed(bool required) {
  if (required) {    
    // Initialize the model
    battery_handler(battery_state_service_peek());
    
    // Register with BatteryService
    battery_state_service_subscribe(battery_handler);
  } else {
    // Unsubscribe from battery service
    battery_state_service_unsubscribe();
  }
}

void vibration_overload_timeout_callback(void *data) {
  // Reset vibration overload error
  model_set_error(ERROR_NONE);  
  vibration_overload_timer = NULL;
}

static void accel_flick_handler(AccelAxisType axis, int32_t direction) {
  // Check whether not in vibration overload
  if (model->error != ERROR_VIBRATION_OVERLOAD) { 
    // Flick detected
    model_signal_flick();
  } else {
    // Prolong vibration overload
    if (vibration_overload_timer) {
      app_timer_reschedule(vibration_overload_timer, 15000);
    }
    else {
      vibration_overload_timer = app_timer_register(15000, vibration_overload_timeout_callback, NULL);
    }
  }
}

void flick_req_changed(bool required) {
  if (required) {    
    accel_tap_service_subscribe(accel_flick_handler);
  } else {
    accel_tap_service_unsubscribe();
  }
}

void accel_handler(AccelData *data, uint32_t num_samples) {
  // Check whether tap events are still required
  if (!(model->update_req & UPDATE_TAPS)) APP_LOG(APP_LOG_LEVEL_WARNING, "Accel handling while no longer required"); //return;
  
  // Detect minor taps
  bool also_calm = false;
  bool tapped = false;
  for (uint32_t i = 0; i < num_samples - 1; ++i) {
    int xDiff = abs(data[i + 1].x - data[i].x);
    int yDiff = abs(data[i + 1].y - data[i].y);
    int zDiff = abs(data[i + 1].z - data[i].z);
    int diff = (xDiff + yDiff + zDiff) / 8; // Accelerator samples are always multiples of 8?
    
    if (!(data[i].did_vibrate || data[i + 1].did_vibrate) && diff >= 200) {
      tapping = true; // tap started
    } else if (diff < 50 && tapping) {
      tapping = false; // tap ended
      tapped = true;
    }
    
    if (diff < 50) also_calm = true;
  }
  
  if (!also_calm) {    
    // Set error
    model_set_error(ERROR_VIBRATION_OVERLOAD);
    
    // Timeout error after 15sec
    if (vibration_overload_timer) {
      app_timer_reschedule(vibration_overload_timer, 15000);
    }
    else {
      vibration_overload_timer = app_timer_register(15000, vibration_overload_timeout_callback, NULL);
    }
  } else if (tapped) {
    // Signal tap detected
    model_signal_tap();
  }
}

void accel_unsubscribe_callback(void *data) {  
  //accel_service_set_sampling_rate(ACCEL_SAMPLING_25HZ);
  accel_data_service_unsubscribe(); 
  accel_unsubscribe_timer = NULL;
}

void tap_req_changed(bool required) {
  if (required) {    
    // Subscribe to accelerator data to detect minor taps
    accel_data_service_subscribe(25, accel_handler); // Callback every 25 samples, 4 times per second
    accel_service_set_sampling_rate(ACCEL_SAMPLING_100HZ);
    tapping = false;
  } else {
    // Unsubscribe from accelerator data via timer. 
    // Unsubscribing here, often in an accel event,
    // causes the app to crash.
    if (!accel_unsubscribe_timer){
      accel_unsubscribe_timer = app_timer_register(0, accel_unsubscribe_callback, NULL); 
    }
  }
}

#if defined(PBL_COMPASS)
void compass_heading_handler(CompassHeadingData heading) {
  model_set_compass_heading(heading);
}

void compass_req_changed(bool required) {
  if (required) {
    compass_service_subscribe(&compass_heading_handler);
    compass_service_set_heading_filter(TRIG_MAX_ANGLE / 32);    
  } else {
    compass_service_unsubscribe();    
  }
}
#endif

void altitude_req_changed(bool required) {
  if (required) {
    model->altitude = INT_MIN;
    message_queue_send_tuplet(TupletInteger(MESSAGE_KEY_SubscribeAltitude, 1));      
  } else {
    message_queue_send_tuplet(TupletInteger(MESSAGE_KEY_UnsubscribeAltitude, 1));       
  }
}

void update_requirements_changed(enum ModelUpdates prev_req) {
  enum ModelUpdates changes = model->update_req ^ prev_req;
  
  if (changes & UPDATE_SECOND_TICKS) second_ticks_req_changed(model->update_req & UPDATE_SECOND_TICKS);
  if (changes & UPDATE_FLICKS) flick_req_changed(model->update_req & UPDATE_FLICKS);
  if (changes & UPDATE_TAPS) tap_req_changed(model->update_req & UPDATE_TAPS);
  #if defined(PBL_COMPASS)
  if (changes & UPDATE_COMPASS) compass_req_changed(model->update_req & UPDATE_COMPASS);
  #endif
  if (changes & UPDATE_ALTITUDE) altitude_req_changed(model->update_req & UPDATE_ALTITUDE);
  if (changes & UPDATE_MOONPHASE) fetch_moonphase();
  if (changes & UPDATE_WEATHER || changes & UPDATE_SUN) fetch_moonphase();
  if (changes & UPDATE_BATTERY) battery_req_changed(model->update_req & UPDATE_BATTERY);
}

static void app_init() {     
  // Initialize the configuration
  config_load();
  
  // Initialize the model
  model->events.on_update_req_change = &update_requirements_changed;
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  static struct tm time_copy;
  time_copy = *tick_time;
  model_set_time(&time_copy, (TimeUnits) -1);
  if (!bluetooth_connection_service_peek()) model_set_error(ERROR_CONNECTION); // Avoid vibrate on initialize
  
  // Load health data
  #if defined(PBL_HEALTH)
  health_init();
  #endif
  
  // Initialize localization of time and date info
  setlocale(LC_TIME, i18n_get_system_locale());
    
  // Initialize the view
  view_init();
  
  // Set up watch communication
  const uint32_t inbox_size = 700;
  const uint32_t outbox_size = 64;
  app_message_register_inbox_received(&msg_received_handler);
  app_message_register_outbox_sent(&message_queue_sent_handler);
  app_message_register_outbox_failed(&message_queue_failed_handler);
  app_message_open(inbox_size, outbox_size);
  
  // Register with TickTimerService
  second_ticks_req_changed(model->update_req & UPDATE_SECOND_TICKS);
  
  // Register with ConnectionService
  connection_service_subscribe((ConnectionHandlers){
    .pebble_app_connection_handler = connection_handler,
    .pebblekit_connection_handler = NULL
  });
}

static void app_deinit() {
  // Stop timers
  if (vibration_overload_timer) app_timer_cancel(vibration_overload_timer);
    
  // Unsubscribe from services
  connection_service_unsubscribe();
  tick_timer_service_unsubscribe();
  app_message_deregister_callbacks();
  
  // De-initialize view
  view_deinit();
  
  // Clear messages left in the message queue
  message_queue_deinit();
  
  // Save configuration
  config_save();
  
  // Save health activity
  #if defined(PBL_HEALTH)
  persist_write_int(STORAGE_HEALTH_ACTIVITY, model->activity);
  persist_write_data(STORAGE_HEALTH_ACTIVITY_START, &activity_start, sizeof(struct ActivityStamp)); 
  
  // Save climb and descend
  persist_write_int(STORAGE_ALTITUDE_CLIMB, altitude_climb);
  persist_write_int(STORAGE_ALTITUDE_DESCEND, altitude_descend); 
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
    
    // Send crash to analytics 
    message_queue_send_tuplet(TupletInteger(MESSAGE_KEY_Exception, num_crashes));  
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