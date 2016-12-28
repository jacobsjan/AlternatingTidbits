#include <pebble.h>
#include "utils.h"

bool is_asleep() {
    bool sleeping = false;
    #if defined(PBL_HEALTH) 
    HealthActivityMask activities = health_service_peek_current_activities();
    sleeping = activities & (HealthActivitySleep | HealthActivityRestfulSleep);
    #endif
    return sleeping;
}

bool should_keep_quiet() {
  if (quiet_time_is_active()) {
    return true;
  } else {
    return is_asleep();
  }
}