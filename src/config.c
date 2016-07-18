#include <pebble.h>
#include "config.h"
#include "storage.h"

// Initialize the configuration (see https://forums.pebble.com/t/question-regarding-struct/13104/4)
struct Config actual_config;
struct Config* config = &actual_config;

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
    config->weather_refresh = 30; 
    strncpy(config->date_format_top, "z", sizeof(config->date_format_top));
    strncpy(config->date_format_bottom, "B e", sizeof(config->date_format_bottom));
    config->date_hours_leading_zero = false;   
  }
}

void config_deinit() {
  // Save the configuration
  persist_write_data(STORAGE_CONFIG, config, sizeof(actual_config));
}