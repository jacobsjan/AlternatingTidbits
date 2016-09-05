#pragma once
#include <pebble.h>
#include "model.h"

struct Config {
  GColor color_background;
  GColor color_primary;
  GColor color_secondary;
  GColor color_accent;
  
  bool date_hours_leading_zero;
  char date_format_top[11];
  char date_format_bottom[11];
  
  bool vibrate_bluetooth;
  bool vibrate_hourly;
  
  bool enable_timezone;
  bool enable_altitude;
  bool enable_battery;
  #if defined(PBL_COMPASS)
  bool enable_compass;
  #endif
  bool enable_countdown;
  bool enable_error;
  #if defined(PBL_HEALTH)
  bool enable_health;
  #endif
  bool enable_moonphase;
  bool enable_sun;
  bool enable_weather;
  char alternate_mode;
  bool switcher_animate;
  
  int timezone_offset;
  char timezone_city[20];
  
  char altitude_unit;
  
  int battery_show_from;
  int battery_accent_from;
  
  #if defined(PBL_COMPASS)
  bool compass_switcher_only;
  #endif
  
  char countdown_label[30];
  char countdown_to;
  time_t countdown_target;
  char countdown_display;
  
  char health_number_format;
  #if defined(PBL_HEALTH)
  bool health_stick;
  char health_distance_unit;
  enum HealthIndicator health_normal_top;
  enum HealthIndicator health_normal_middle;
  enum HealthIndicator health_normal_bottom;
  enum HealthIndicator health_walk_top;
  enum HealthIndicator health_walk_middle;
  enum HealthIndicator health_walk_bottom;
  enum HealthIndicator health_run_top;
  enum HealthIndicator health_run_middle;
  enum HealthIndicator health_run_bottom;
  enum HealthIndicator health_sleep_top;
  enum HealthIndicator health_sleep_middle;
  enum HealthIndicator health_sleep_bottom;
  #endif
  
  bool moonphase_night_only;
  
  int weather_refresh;
};

bool parse_configuration_messages(DictionaryIterator*);
void config_init();
void config_deinit();

extern struct Config* config;