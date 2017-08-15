#pragma once

enum CrashZone {
  CRASH_ZONE_ACCEL_UNSUBSCRIBE = 1 << 0,
};

void crash_detection_clear();
void crash_detection_deinit();
void crash_detection_init();
void crash_enter_zone(enum CrashZone zone);
void crash_leave_zone(enum CrashZone zone);