#pragma once
#include <pebble.h>

struct Config {
  GColor color_background;
  GColor color_primary;
  GColor color_secondary;
  GColor color_accent;
  
  bool date_hours_leading_zero;
  char date_format_top[11];
  char date_format_bottom[11];
  
  bool enable_timezone;
  bool enable_battery;
  bool enable_error;
  bool enable_sun;
  bool enable_weather;
  
  int timezone_offset;
  char timezone_city[20];
  
  int battery_show_from;
  int battery_accent_from;
  
  int weather_refresh;
  
  #if defined(PBL_HEALTH)
  #endif
};

bool parse_configuration_messages(DictionaryIterator*);
void config_init();
void config_deinit();

extern struct Config* config;