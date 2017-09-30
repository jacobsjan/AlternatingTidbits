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

void check_storage_status(enum StatusCode status, int key, char* operation) {
  switch(status) {
    default:
    case S_SUCCESS:
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "%s of %d - Operation completed successfully.", operation, key);
      break;
    case E_ERROR:
      APP_LOG(APP_LOG_LEVEL_ERROR, "%s of %d - An error occurred (no description).", operation, key);
      break;
    case E_UNKNOWN:
      APP_LOG(APP_LOG_LEVEL_ERROR, "%s of %d - No idea what went wrong.", operation, key);
      break; 
    case E_INTERNAL:
      APP_LOG(APP_LOG_LEVEL_ERROR, "%s of %d - There was a generic internal logic error.", operation, key);
      break; 
    case E_INVALID_ARGUMENT:
      APP_LOG(APP_LOG_LEVEL_ERROR, "%s of %d - The function was not called correctly.", operation, key);
      break; 
    case E_OUT_OF_MEMORY:
      APP_LOG(APP_LOG_LEVEL_ERROR, "%s of %d - Insufficient allocatable memory available.", operation, key);
      break; 
    case E_OUT_OF_STORAGE:
      APP_LOG(APP_LOG_LEVEL_ERROR, "%s of %d - Insufficient long-term storage available.", operation, key);
      break; 
    case E_OUT_OF_RESOURCES:
      APP_LOG(APP_LOG_LEVEL_ERROR, "%s of %d - Insufficient resources available.", operation, key);
      break; 
    case E_RANGE:
      APP_LOG(APP_LOG_LEVEL_ERROR, "%s of %d - Argument out of range (may be dynamic).", operation, key);
      break;  
    case E_DOES_NOT_EXIST:
      APP_LOG(APP_LOG_LEVEL_ERROR, "%s of %d - Target of operation does not exist.", operation, key);
      break; 
    case E_INVALID_OPERATION:
      APP_LOG(APP_LOG_LEVEL_ERROR, "%s of %d - Operation not allowed (may depend on state).", operation, key);
      break; 
    case E_BUSY:
      APP_LOG(APP_LOG_LEVEL_ERROR, "%s of %d - Another operation prevented this one.", operation, key);
      break;  
    case E_AGAIN:
      APP_LOG(APP_LOG_LEVEL_ERROR, "%s of %d - Operation not completed; try again.", operation, key);
      break; 
  }
}


void check_write_status(enum StatusCode status, int key) {
  check_storage_status(status, key, "Write");
}

void utils_init() {  
  #if defined(PBL_HEALTH) 
  last_recorded_awake = persist_exists(STORAGE_LAST_RECORDED_AWAKE) ? persist_read_int(STORAGE_LAST_RECORDED_AWAKE) : 0;
  #endif
}

void utils_deinit() {
  #if defined(PBL_HEALTH) 
  check_write_status(persist_write_int(STORAGE_LAST_RECORDED_AWAKE, last_recorded_awake), STORAGE_LAST_RECORDED_AWAKE);
  #endif
}