#include <pebble.h>
#include "utils.h"
#include "storage.h"
#include "messagequeue.h"

#if defined(PBL_HEALTH) 
bool sleep_trusted = true;
int last_seconds_slept = 0;
time_t last_sleep_update = 0;
#endif

bool is_asleep() {
  #if defined(PBL_HEALTH) 
  HealthActivityMask activities = health_service_peek_current_activities();
  bool sleeping = activities & (HealthActivitySleep | HealthActivityRestfulSleep);
  
  // Check whether sleeping according to peeking at activities can be trusted  
  if (!sleeping) {
    if (!sleep_trusted) {
      // Thought maybe that sleeping could not be trusted but apparantly it can
      sleep_trusted = true;
      last_sleep_update = 0;
      
      // Send sleep trust to analytics 
      message_queue_send_tuplet(TupletInteger(MESSAGE_KEY_SleepTrust, 2)); 
    }
  } else {
    // Check whether we appear to be sleeping while the sleep time is not increasing
    int seconds_slept = (int)health_service_sum_today(HealthMetricSleepSeconds);
    if (last_sleep_update == 0 || seconds_slept != last_seconds_slept) {
      // The sleep time is still changing
      last_seconds_slept = seconds_slept;
      last_sleep_update = time(NULL);
    } else {
      // The sleep time is not changing, since how long?
      long diff = time(NULL) - last_sleep_update;
      if (diff > SECONDS_PER_MINUTE * 10) {
        // Peek has been saying we are sleeping for more than 10 minutes while the sleep time is not increasing, don't trust it
        sleep_trusted = false;
        
        // Send sleep trust to analytics 
        message_queue_send_tuplet(TupletInteger(MESSAGE_KEY_SleepTrust, 1)); 
      }
    }
  }
  return sleeping && sleep_trusted;
  #else
  // If health is not supported never report sleeping
  return false;
  #endif  
}

bool should_keep_quiet() {
  if (quiet_time_is_active()) {
    return true;
  } else {
    return is_asleep();
  }
}

#if defined(PBL_PLATFORM_DIORITE) || defined(PBL_PLATFORM_EMERY) 
bool is_heartrate_available() {
  HealthServiceAccessibilityMask hr = health_service_metric_accessible(HealthMetricHeartRateBPM, time(NULL), time(NULL));
  if (hr & HealthServiceAccessibilityMaskAvailable) {
    HealthValue val = health_service_peek_current_value(HealthMetricHeartRateBPM);
    if(val > 0) {
      return true;
    }
  }  
  return false;
}
#endif

void utils_init() {  
  #if defined(PBL_HEALTH) 
  sleep_trusted = persist_exists(STORAGE_SLEEP_TRUSTED) ? persist_read_bool(STORAGE_SLEEP_TRUSTED) : true;
  #endif
}

void utils_deinit() {
  #if defined(PBL_HEALTH) 
  persist_write_bool(STORAGE_SLEEP_TRUSTED, sleep_trusted);
  #endif
}