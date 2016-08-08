#include <pebble.h>
#include "config.h"
#include "storage.h"

// Initialize the configuration (see https://forums.pebble.com/t/question-regarding-struct/13104/4)
struct Config actual_config;
struct Config* config = &actual_config;

bool parse_configuration_messages(DictionaryIterator* iter) {
  bool cfgChanged = false;
  
  // Colors
  Tuple* tuple = dict_find(iter, MESSAGE_KEY_cfgColorBackground);
  if(tuple && (cfgChanged = true)) config->color_background = GColorFromHEX(tuple->value->int32);
  tuple = dict_find(iter, MESSAGE_KEY_cfgColorPrimary);
  if(tuple && (cfgChanged = true)) {
    config->color_primary = GColorFromHEX(tuple->value->int32);
    #if defined(PBL_BW)
    config->color_secondary = config->color_primary;
    config->color_accent = config->color_primary;
    #endif
  }
  #if !defined(PBL_BW)
  tuple = dict_find(iter, MESSAGE_KEY_cfgColorSecondary);
  if(tuple && (cfgChanged = true)) config->color_secondary = GColorFromHEX(tuple->value->int32);
  tuple = dict_find(iter, MESSAGE_KEY_cfgColorAccent);
  if(tuple && (cfgChanged = true)) config->color_accent = GColorFromHEX(tuple->value->int32); 
  #endif
  
  // Time and date format
  tuple = dict_find(iter, MESSAGE_KEY_cfgDateHoursLeadingZero);
  if(tuple && (cfgChanged = true)) config->date_hours_leading_zero = tuple->value->int8; 
  tuple = dict_find(iter, MESSAGE_KEY_cfgDateFormatTop);
  if(tuple && (cfgChanged = true)) strncpy(config->date_format_top, tuple->value->cstring, sizeof(config->date_format_top)); 
  tuple = dict_find(iter, MESSAGE_KEY_cfgDateFormatBottom);
  if(tuple && (cfgChanged = true)) strncpy(config->date_format_bottom, tuple->value->cstring, sizeof(config->date_format_bottom));
  
  // Enabling tidbits
  tuple = dict_find(iter, MESSAGE_KEY_cfgEnableTimezone);
  if(tuple && (cfgChanged = true)) config->enable_timezone = tuple->value->int8; 
  tuple = dict_find(iter, MESSAGE_KEY_cfgEnableBattery);
  if(tuple && (cfgChanged = true)) config->enable_battery = tuple->value->int8;
  tuple = dict_find(iter, MESSAGE_KEY_cfgEnableError);
  if(tuple && (cfgChanged = true)) config->enable_error = tuple->value->int8;
  #if defined(PBL_HEALTH) 
  tuple = dict_find(iter, MESSAGE_KEY_cfgEnableHealth);
  if(tuple && (cfgChanged = true)) config->enable_health = tuple->value->int8;
  #endif
  tuple = dict_find(iter, MESSAGE_KEY_cfgEnableSun);
  if(tuple && (cfgChanged = true)) config->enable_sun = tuple->value->int8;
  tuple = dict_find(iter, MESSAGE_KEY_cfgEnableWeather);
  if(tuple && (cfgChanged = true)) config->enable_weather = tuple->value->int8;
  
  // Timezone
  tuple = dict_find(iter, MESSAGE_KEY_cfgTimeZoneOffset);
  if(tuple) config->timezone_offset = tuple->value->int32; // Timezone offset updates on itself are not considered configuration changes
  tuple = dict_find(iter, MESSAGE_KEY_cfgTimeZoneCity);
  if(tuple && (cfgChanged = true)) strncpy(config->timezone_city, tuple->value->cstring, sizeof(config->timezone_city));  
  
  // Battery
  tuple = dict_find(iter, MESSAGE_KEY_cfgBatteryShowFrom);
  if(tuple && (cfgChanged = true)) config->battery_show_from = tuple->value->int32; 
  #if !defined(PBL_BW)
  tuple = dict_find(iter, MESSAGE_KEY_cfgBatteryAccentFrom);
  if(tuple && (cfgChanged = true)) config->battery_accent_from = tuple->value->int32; 
  #endif 
  
  // Health
  #if defined(PBL_HEALTH)
  tuple = dict_find(iter, MESSAGE_KEY_cfgHealthStick);
  if(tuple && (cfgChanged = true)) config->health_stick = tuple->value->int8; 
  tuple = dict_find(iter, MESSAGE_KEY_cfgHealthUnits);
  if(tuple && (cfgChanged = true)) config->health_distance_unit = tuple->value->cstring[0]; 
  tuple = dict_find(iter, MESSAGE_KEY_cfgHealthNumbers);
  if(tuple && (cfgChanged = true)) config->health_number_format = tuple->value->cstring[0]; 
  tuple = dict_find(iter, MESSAGE_KEY_cfgHealthNormalLine1);
  if(tuple && (cfgChanged = true)) config->health_normal_top = tuple->value->int32; 
  tuple = dict_find(iter, MESSAGE_KEY_cfgHealthNormalLine2);
  if(tuple && (cfgChanged = true)) config->health_normal_middle = tuple->value->int32; 
  tuple = dict_find(iter, MESSAGE_KEY_cfgHealthNormalLine3);
  if(tuple && (cfgChanged = true)) config->health_normal_bottom = tuple->value->int32; 
  tuple = dict_find(iter, MESSAGE_KEY_cfgHealthWalkLine1);
  if(tuple && (cfgChanged = true)) config->health_walk_top = tuple->value->int32; 
  tuple = dict_find(iter, MESSAGE_KEY_cfgHealthWalkLine2);
  if(tuple && (cfgChanged = true)) config->health_walk_middle = tuple->value->int32; 
  tuple = dict_find(iter, MESSAGE_KEY_cfgHealthWalkLine3);
  if(tuple && (cfgChanged = true)) config->health_walk_bottom = tuple->value->int32;
  tuple = dict_find(iter, MESSAGE_KEY_cfgHealthRunLine1);
  if(tuple && (cfgChanged = true)) config->health_run_top = tuple->value->int32; 
  tuple = dict_find(iter, MESSAGE_KEY_cfgHealthRunLine2);
  if(tuple && (cfgChanged = true)) config->health_run_middle = tuple->value->int32; 
  tuple = dict_find(iter, MESSAGE_KEY_cfgHealthRunLine3);
  if(tuple && (cfgChanged = true)) config->health_run_bottom = tuple->value->int32;
  tuple = dict_find(iter, MESSAGE_KEY_cfgHealthSleepLine1);
  if(tuple && (cfgChanged = true)) config->health_sleep_top = tuple->value->int32; 
  tuple = dict_find(iter, MESSAGE_KEY_cfgHealthSleepLine2);
  if(tuple && (cfgChanged = true)) config->health_sleep_middle = tuple->value->int32; 
  tuple = dict_find(iter, MESSAGE_KEY_cfgHealthSleepLine3);
  if(tuple && (cfgChanged = true)) config->health_sleep_bottom = tuple->value->int32;
  
  #endif 
  
  // Weather
  tuple = dict_find(iter, MESSAGE_KEY_cfgWeatherRefresh);
  if(tuple && (cfgChanged = true)) config->weather_refresh = tuple->value->int32;
  
  return cfgChanged;
}

void config_init() {
  if (persist_exists(STORAGE_CONFIG) && persist_get_size(STORAGE_CONFIG) == sizeof(actual_config)) {
    // Read config from storage
    persist_read_data(STORAGE_CONFIG, config, sizeof(actual_config));
  } else {
    // Initialize with defaults
    config->color_background = GColorWhite;
    config->color_primary = GColorBlack;
    config->color_secondary = GColorDarkGray;
    config->color_accent = GColorDarkCandyAppleRed; 
    
    config->date_hours_leading_zero = false;   
    strncpy(config->date_format_top, "z", sizeof(config->date_format_top));
    strncpy(config->date_format_bottom, "B e", sizeof(config->date_format_bottom));
    
    config->enable_timezone = false;
    config->enable_battery = true;
    config->enable_error = false;
    #if defined(PBL_HEALTH)
    config->enable_health = true;
    #endif
    config->enable_sun = true;
    config->enable_weather = true;
    
    config->battery_show_from = 100;
    config->battery_accent_from = 30;
      
    #if defined(PBL_HEALTH)
    config->health_stick = true;
    config->health_distance_unit = 'M';
    config->health_number_format = 'M';
    config->health_normal_top = HEALTH_EMPTY;
    config->health_normal_middle = HEALTH_EMPTY;
    config->health_normal_bottom = HEALTH_TODAY_STEPS;
    config->health_walk_top = HEALTH_EMPTY;
    config->health_walk_middle = HEALTH_ACTIVITY_DISTANCE;
    config->health_walk_bottom = HEALTH_TODAY_STEPS;
    config->health_run_top = HEALTH_ACTIVITY_DURATION;
    config->health_run_middle = HEALTH_ACTIVITY_DISTANCE;
    config->health_run_bottom = HEALTH_ACTIVITY_SPEED;
    config->health_sleep_top = HEALTH_EMPTY;
    config->health_sleep_middle = HEALTH_EMPTY;
    config->health_sleep_bottom = HEALTH_TIME_TOTAL_SLEEP;
    #endif
    
    config->weather_refresh = 30; 
  }
}

void config_deinit() {
  // Save the configuration
  persist_write_data(STORAGE_CONFIG, config, sizeof(actual_config));
}