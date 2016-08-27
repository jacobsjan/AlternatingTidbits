#pragma once
#include <pebble.h>

#include "model.h"

char* ICON_BLUETOOTH = "\uf282"; // bluetooth (Material design font)
char* ICON_NO_BLUETOOTH = "\uf27f"; // bluetooth_disabled (Material design font)
char* ICON_NO_LOCATION = "\uf1aa"; // location_off (Material design font)
char* ICON_NO_WEATHER = "\uf21b"; // cloud_off (Material design font)
char* ICON_FETCH_ERROR = "\uf03e"; // wi-cloud-refresh (Weather Icons)
char* ICON_VIBRATION_OVERLOAD = "\uf09e"; //wi-earthquake (Weather Icons)

char* ICON_TIMEZONE = "\uf2a7"; 
char* ICON_ALTITUDE = "\uf1bc"; 

#if defined(PBL_COMPASS)
char* ICON_COMPASS_ROTATE = "\uf0c1"; 
#endif

char* ICON_SUNRISE = "\uf051"; // wi-sunrise (Weather Icons)
char* ICON_SUNSET = "\uf052"; // wi-sunset (Weather Icons)

char* ICON_STEP = "\uf2ab";
char* ICON_WALK = "\uf2b1";
char* ICON_RUN = "\uf2ae";
char* ICON_SLEEP = "\uf2af";

char* icons_get_weather_condition_symbol(enum WeatherCondition code, bool is_day);
char* icons_get_battery_symbol(uint8_t charge, bool charging, bool plugged);
#if defined(PBL_COMPASS)
char* icons_get_compass(int degrees);
#endif
char* icons_get_moonphase(int degrees);