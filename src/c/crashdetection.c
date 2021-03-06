#include <pebble.h>
#include "crashdetection.h"
#include "messagequeue.h"
#include "storage.h"
#include "utils.h"

enum CrashZone crash_zone;

#ifndef PBL_PLATFORM_APLITE
void crash_enter_zone(enum CrashZone zone) {
  crash_zone |= zone;
  persist_write_int(STORAGE_CRASH_DETECTOR_ZONE, crash_zone);
}

void crash_leave_zone(enum CrashZone zone) {
  crash_zone &= ~zone;
  persist_write_int(STORAGE_CRASH_DETECTOR_ZONE, crash_zone);
}

void crash_detection_clear() {
  persist_delete(STORAGE_CRASH_NUM);
  persist_delete(STORAGE_CRASH_TIMESTAMPS);
  persist_delete(STORAGE_CRASH_DETECTOR_ZONE);
  persist_delete(STORAGE_CRASH_ZONES);
}

void crash_detection_init() {
  // Read previous crash info
  #define MAX_TIMESTAMPS 5 /* (PERSIST_DATA_MAX_LENGTH / sizeof(time_t)) */
  int num_crashes = persist_exists(STORAGE_CRASH_NUM) ? persist_read_int(STORAGE_CRASH_NUM) : 0;
  time_t crash_timestamps[MAX_TIMESTAMPS] = { 0 };
  if (persist_exists(STORAGE_CRASH_TIMESTAMPS)) {
    if (persist_get_size(STORAGE_CRASH_TIMESTAMPS) == sizeof(crash_timestamps)) {
      persist_read_data(STORAGE_CRASH_TIMESTAMPS, &crash_timestamps, sizeof(crash_timestamps)); 
    } else {
      persist_delete(STORAGE_CRASH_TIMESTAMPS);
    }    
  }
  enum CrashZone crash_zones[MAX_TIMESTAMPS] = { 0 };
  if (persist_exists(STORAGE_CRASH_ZONES)) {
    if (persist_get_size(STORAGE_CRASH_ZONES) == sizeof(crash_zones)) {
      persist_read_data(STORAGE_CRASH_ZONES, &crash_zones, sizeof(crash_zones));   
    } else {
      persist_delete(STORAGE_CRASH_ZONES);
    }    
  }
    
  // Detect new restart after crash
  if (persist_exists(STORAGE_CRASH_DETECTOR)) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Restarted after crash!");
    
    // Read crash zone
    enum CrashZone zone = (enum CrashZone)(persist_exists(STORAGE_CRASH_DETECTOR_ZONE) ? persist_read_int(STORAGE_CRASH_DETECTOR_ZONE) : 0);   
    
    // Record this crash
    time_t now = time(NULL);
    crash_timestamps[num_crashes % MAX_TIMESTAMPS] = now;
    crash_zones[num_crashes % MAX_TIMESTAMPS] = zone;
    ++num_crashes;
    
    // Save crash info
    persist_write_int(STORAGE_CRASH_NUM, num_crashes);
    persist_write_data(STORAGE_CRASH_TIMESTAMPS, crash_timestamps, sizeof(crash_timestamps));
    persist_write_int(STORAGE_CRASH_DETECTOR_ZONE, crash_zone); // Clearing
    persist_write_data(STORAGE_CRASH_ZONES, crash_zones, sizeof(crash_zones));
    
    // Send crash to analytics 
    message_queue_send_tuplet(TupletInteger(MESSAGE_KEY_Exception, zone));  
  }
  
  // Print crash history
  if (num_crashes > 0) {
    APP_LOG(APP_LOG_LEVEL_INFO, "%d Crashes detected. Restarts at:", num_crashes);
    char timestamp[30];
    for (int i = num_crashes - MAX_TIMESTAMPS > 0 ? num_crashes - MAX_TIMESTAMPS : 0; i < num_crashes; ++i) {
      const struct tm *crash_time = localtime(&crash_timestamps[i % MAX_TIMESTAMPS]);
      strftime(timestamp, sizeof(timestamp) / sizeof(char), "%F %T", crash_time);
      APP_LOG(APP_LOG_LEVEL_INFO, "[%s] Zone %d.", timestamp, crash_zones[i % MAX_TIMESTAMPS]);
    }
  }
  
  // Mark app as running to enable crash detection
  persist_write_bool(STORAGE_CRASH_DETECTOR, true);
}

void crash_detection_deinit() {
  // Mark no crash occured
  persist_delete(STORAGE_CRASH_DETECTOR);
}

#else 
void crash_detection_clear() { }
void crash_detection_deinit() { }
void crash_detection_init() { }
void crash_enter_zone(enum CrashZone zone) { }
void crash_leave_zone(enum CrashZone zone) { }
#endif