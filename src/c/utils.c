#include <pebble.h>
#include "utils.h"
#include "storage.h"
#include "messagequeue.h"

#if defined(PBL_HEALTH) 
time_t last_recorded_awake;
#endif

bool is_asleep() {
  #if defined(PBL_HEALTH) 
  // Some watches are stuck on forever sleeping,
  // do not trust sleep if witnessed awake phase in last 24hrs
  HealthActivityMask activities = health_service_peek_current_activities();
  bool sleeping = activities & (HealthActivitySleep | HealthActivityRestfulSleep);
  time_t now = time(NULL);
  if (sleeping) {
    return now - last_recorded_awake < SECONDS_PER_DAY; // Only trust sleep if awake in the last 24hrs
  } else {
    last_recorded_awake = now;
    return false;
  }
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
  last_recorded_awake = persist_exists(STORAGE_LAST_RECORDED_AWAKE) ? persist_read_int(STORAGE_LAST_RECORDED_AWAKE) : 0;
  #endif
}

void utils_deinit() {
  #if defined(PBL_HEALTH) 
  persist_write_int(STORAGE_LAST_RECORDED_AWAKE, last_recorded_awake);
  #endif
}