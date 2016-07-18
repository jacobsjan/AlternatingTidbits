#include <pebble.h>

#include "main.h"
#include "config.h"
#include "model.h"
#include "view.h"
#include "storage.h"

#if defined(PBL_HEALTH)
struct StepStamp {
  int totalStepCount;
  int totalDistance;
  time_t time;
};
#endif

bool is_js_ready = false;
int retry_times = MAX_RETRIES;
int elapsed_time_since_fetch = 0;
static AppTimer *s_timeout_timer;
#if defined(PBL_HEALTH)
#define ACTIVITY_MONITOR_WINDOW 10
struct StepStamp activity_start;
struct StepStamp activity_buffer[ACTIVITY_MONITOR_WINDOW];
int activity_buffer_index;
#endif

static void fetch_weather();
static void app_init();
static void app_deinit();

static bool is_asleep() {
  bool sleeping = false;
  #if defined(PBL_HEALTH) 
  HealthActivityMask activities = health_service_peek_current_activities();
  sleeping = activities & (HealthActivitySleep | HealthActivityRestfulSleep);
  #endif
  return sleeping;
}

static void fetch_failed_handler(void *context) {
  // The timer elapsed because no success was reported
  retry_times -= 1;
  if (retry_times > 0) {
    // Retry
    fetch_weather();
  } else {
    // Stop trying but prepare for the next try
    model_set_error(ERROR_FETCH);
    retry_times = MAX_RETRIES;
  }
}

static void fetch_weather() {
  // Check bluetooth connection
  if (!connection_service_peek_pebble_app_connection()) {
    //Bluetooth not connected
    model_set_error(ERROR_CONNECTION);
  }
  else {
    // Bluetooth connected, check the weather
    // Prepare outgoing message
    DictionaryIterator *iter;
    AppMessageResult result = app_message_outbox_begin(&iter);
    if(result != APP_MSG_OK) { // Message could not be prepared
      fetch_failed_handler(NULL); // Retry
    } else {
      // Send outgoing message
      Tuplet value = TupletInteger(MESSAGE_KEY_Fetch, 1);
      dict_write_tuplet(iter, &value);
      dict_write_end(iter);
      
      result = app_message_outbox_send();
      if(result != APP_MSG_OK) { // Message could not be sent
        fetch_failed_handler(NULL); // Retry
      }
    }
    
    // Cancel previour retry timeout timer
    if (s_timeout_timer) {
      app_timer_cancel(s_timeout_timer);
      s_timeout_timer = NULL;
    }  
    
    // Schedule retry timeout timer
    const int interval_ms = (MAX_RETRIES - retry_times + 1) * 10000; // Wait a bit longer with each retry
    s_timeout_timer = app_timer_register(interval_ms, fetch_failed_handler, NULL);
  }  
}

static void msg_received_handler(DictionaryIterator *iter, void *context) {
  // Handle and incoming message from the phone
  Tuple *tuple = dict_find(iter, MESSAGE_KEY_JSReady);
  if(tuple) {
        is_js_ready = true;
        // Watch is ready for communication, fetch weather data  
        fetch_weather();
  }
  
  // Weather
  tuple = dict_find(iter, MESSAGE_KEY_Temperature);
  if(tuple) model_set_weather_temperature(tuple->value->int32);
  tuple = dict_find(iter, MESSAGE_KEY_Condition);
  if(tuple) model_set_weather_condition(tuple->value->int32);
  tuple = dict_find(iter, MESSAGE_KEY_Sunrise);
  if(tuple) model_set_sunrise(tuple->value->int32);
  tuple = dict_find(iter, MESSAGE_KEY_Sunset);
  if(tuple) model_set_sunset(tuple->value->int32);
  tuple = dict_find(iter, MESSAGE_KEY_Err);
  if(tuple) {
    // Error encountered
    model_set_error(tuple->value->int32);
    if (model->error == ERROR_FETCH) {
      fetch_failed_handler(NULL); // Retry
    }
  } else  {
    // Fetch was succesfull, cancel retry timer and reset retry count
    retry_times = MAX_RETRIES;
    if (s_timeout_timer) {
      app_timer_cancel(s_timeout_timer);
      s_timeout_timer = NULL;
    }
    
    // Reset error if necessary
    if (model->error != ERROR_NONE) {
      model_set_error(ERROR_NONE);
    }
  } 
  
  // Configuration
  bool cfgChanged = false;
  tuple = dict_find(iter, MESSAGE_KEY_cfgColorBackground);
  if(tuple && (cfgChanged = true)) config->color_background = GColorFromHEX(tuple->value->int32);
  tuple = dict_find(iter, MESSAGE_KEY_cfgColorPrimary);
  if(tuple && (cfgChanged = true)) config->color_primary = GColorFromHEX(tuple->value->int32);
  tuple = dict_find(iter, MESSAGE_KEY_cfgColorSecondary);
  if(tuple && (cfgChanged = true)) config->color_secondary = GColorFromHEX(tuple->value->int32);
  tuple = dict_find(iter, MESSAGE_KEY_cfgColorAccent);
  if(tuple && (cfgChanged = true)) config->color_accent = GColorFromHEX(tuple->value->int32); 
  tuple = dict_find(iter, MESSAGE_KEY_cfgWeatherRefresh);
  if(tuple && (cfgChanged = true)) config->weather_refresh = tuple->value->int32; 
  tuple = dict_find(iter, MESSAGE_KEY_cfgDateHoursLeadingZero);
  if(tuple && (cfgChanged = true)) config->date_hours_leading_zero = tuple->value->int8; 
  tuple = dict_find(iter, MESSAGE_KEY_cfgDateFormatTop);
  if(tuple && (cfgChanged = true)) strncpy(config->date_format_top, tuple->value->cstring, sizeof(config->date_format_top)); 
  tuple = dict_find(iter, MESSAGE_KEY_cfgDateFormatBottom);
  if(tuple && (cfgChanged = true)) strncpy(config->date_format_bottom, tuple->value->cstring, sizeof(config->date_format_bottom)); 
  
  if (cfgChanged) {
    // Hiddden function to clear crash detection history
    if (gcolor_equal(config->color_background, GColorWhite) && gcolor_equal(config->color_primary, GColorWhite)) {
      persist_delete(STORAGE_CRASH_NUM);
      persist_delete(STORAGE_CRASH_TIMESTAMPS);
    }
    
    // Restart view  
    view_deinit();
    view_init();
    
    // Refetch weather in case weather settings were changed
    fetch_weather();
  }
}

static void msg_failed_handler(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    fetch_failed_handler(NULL); // Retry
}

#if defined(PBL_HEALTH)
void update_health() {
  const int STEPS_PER_MINUTE_WALKING = 45;
  const int STEPS_PER_MINUTE_RUNNING = 110;
  
  // Fill the activity buffer for this minute
  activity_buffer[activity_buffer_index].totalStepCount = (int)health_service_sum_today(HealthMetricStepCount);
  activity_buffer[activity_buffer_index].totalDistance = (int)health_service_sum_today(HealthMetricWalkedDistanceMeters);
  activity_buffer[activity_buffer_index].time = time(NULL);
  
  // Calculate the average step pace in the buffer
  int avg_steps_per_minute;
  int first_index;
  int last_index = activity_buffer_index;
  if (activity_buffer[ACTIVITY_MONITOR_WINDOW - 1].time == 0) {
    // Buffer isn't completey full yet
    first_index = 0;
  } else {
    first_index = (activity_buffer_index + 1) % ACTIVITY_MONITOR_WINDOW;
  }
  if (first_index != last_index) {
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
    current_activity = ACTIVITY_CALM;
  }
  
  if (current_activity != model->activity || activity_start.time == 0) {
    // The activity is changing
    model_set_activity(current_activity);
    activity_start = activity_buffer[first_index]; // Crude, can be improved
  }
  
  // Update activity counters
  int total_step_count = activity_buffer[last_index].totalStepCount;
  int duration = (activity_buffer[last_index].time - activity_start.time) / SECONDS_PER_MINUTE;
  int distance = activity_buffer[last_index].totalDistance - activity_start.totalDistance;
  model_set_activity_counters(total_step_count, duration, distance);
  
  // Move index to next slot in buffer
  activity_buffer_index = (activity_buffer_index + 1) % ACTIVITY_MONITOR_WINDOW;
}
#endif

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (units_changed & MINUTE_UNIT) {
    // Update weather
    elapsed_time_since_fetch++;
    if (is_js_ready && elapsed_time_since_fetch >= config->weather_refresh && !is_asleep()) {
      elapsed_time_since_fetch = 0;
      fetch_weather();
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
}

static void connection_handler(bool connected) {
  // Vibrate if connection changed 
  if ((model->error == ERROR_CONNECTION) == connected) {
    // Check to make sure user is not sleeping
    if (!is_asleep()) {
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

static void app_init() {     
  // Initialize the configuration
  config_init();
  
  // Initialize the model
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  model_set_time(tick_time);
  connection_handler(bluetooth_connection_service_peek());
  battery_handler(battery_state_service_peek());
  
  // Initialize the view
  view_init();
  
  // Set up watch communication
  const uint32_t inbox_size = 640;
  const uint32_t outbox_size = 64;
  app_message_register_outbox_failed(&msg_failed_handler);
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