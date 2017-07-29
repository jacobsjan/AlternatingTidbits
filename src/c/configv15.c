#include <pebble.h>
#include "config.h"
#include "configv15.h"

void convert_config_v15(struct Config* toConfig, struct ConfigV15* fromConfig) {
  toConfig->color_background = fromConfig->color_background;
  toConfig->color_primary = fromConfig->color_primary;
  toConfig->color_secondary = fromConfig->color_secondary;
  toConfig->color_accent = fromConfig->color_accent;
  
  toConfig->date_hours_leading_zero = fromConfig->date_hours_leading_zero;
  strncpy(toConfig->date_format_top, fromConfig->date_format_top, sizeof(toConfig->date_format_top));
  strncpy(toConfig->date_format_bottom, fromConfig->date_format_bottom, sizeof(toConfig->date_format_bottom));
  
  toConfig->vibrate_bluetooth = fromConfig->vibrate_bluetooth;
  toConfig->vibrate_hourly = fromConfig->vibrate_hourly;
  
  toConfig->enable_timezone = fromConfig->enable_timezone;
  toConfig->enable_altitude = fromConfig->enable_altitude;
  toConfig->enable_battery = fromConfig->enable_battery;
  #if defined(PBL_COMPASS)
  toConfig->enable_compass = fromConfig->enable_compass;
  #endif
  toConfig->enable_countdown = fromConfig->enable_countdown;
  toConfig->enable_error = fromConfig->enable_error;
  toConfig->enable_happy = fromConfig->enable_happy;
  #if defined(PBL_HEALTH)
  toConfig->enable_health = fromConfig->enable_health;
  #endif
  toConfig->enable_moonphase = fromConfig->enable_moonphase;
  toConfig->enable_sun = fromConfig->enable_sun;
  toConfig->enable_weather = fromConfig->enable_weather;
  toConfig->alternate_mode = fromConfig->alternate_mode;
  toConfig->switcher_animate = fromConfig->switcher_animate;
  
  toConfig->timezone_offset = fromConfig->timezone_offset;
  strncpy(toConfig->timezone_city, fromConfig->timezone_city, sizeof(toConfig->timezone_city));
  
  toConfig->altitude_unit = fromConfig->altitude_unit;
  
  toConfig->battery_show_from = fromConfig->battery_show_from;
  toConfig->battery_accent_from = fromConfig->battery_accent_from;
  
  #if defined(PBL_COMPASS)
  toConfig->compass_switcher_only = fromConfig->compass_switcher_only;
  #endif
  
  toConfig->countdown_count = 1;
  toConfig->countdowns = malloc(sizeof(struct CountdownConfig));
  strncpy(toConfig->countdowns[0].label, fromConfig->countdown_label, sizeof(toConfig->countdowns->label));
  toConfig->countdowns[0].to = fromConfig->countdown_to;
  toConfig->countdowns[0].time = fromConfig->countdown_time;
  toConfig->countdowns[0].date = fromConfig->countdown_date;
  
  toConfig->countdown_display = fromConfig->countdown_display;
  
  toConfig->health_number_format = fromConfig->health_number_format;
  #if defined(PBL_HEALTH)
  toConfig->health_stick = fromConfig->health_stick;
  toConfig->health_distance_unit = fromConfig->health_distance_unit;
  toConfig->health_normal_top = fromConfig->health_normal_top;
  toConfig->health_normal_middle = fromConfig->health_normal_middle;
  toConfig->health_normal_bottom = fromConfig->health_normal_bottom;
  toConfig->health_walk_top = fromConfig->health_walk_top;
  toConfig->health_walk_middle = fromConfig->health_walk_middle;
  toConfig->health_walk_bottom = fromConfig->health_walk_bottom;
  toConfig->health_run_top = fromConfig->health_run_top;
  toConfig->health_run_middle = fromConfig->health_run_middle;
  toConfig->health_run_bottom = fromConfig->health_run_bottom;
  toConfig->health_sleep_top = fromConfig->health_sleep_top;
  toConfig->health_sleep_middle = fromConfig->health_sleep_middle;
  toConfig->health_sleep_bottom = fromConfig->health_sleep_bottom;
  #endif
  
  toConfig->moonphase_night_only = fromConfig->moonphase_night_only;
  
  toConfig->weather_refresh = fromConfig->weather_refresh;
}