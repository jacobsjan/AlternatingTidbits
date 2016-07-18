#pragma once
#include <pebble.h>

struct Config {
  GColor color_background;
  GColor color_primary;
  GColor color_secondary;
  GColor color_accent;
  int weather_refresh;
  bool date_hours_leading_zero;
  char date_format_top[11];
  char date_format_bottom[11];
  
  #if defined(PBL_HEALTH)
  #endif
};

void config_init();
void config_deinit();

extern struct Config* config;