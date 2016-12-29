#pragma once
#include <pebble.h>

#include "model.h"

static char* ICON_BLUETOOTH = "\uf282"; // bluetooth (Material design font)
static char* ICON_NO_BLUETOOTH = "\uf27f"; // bluetooth_disabled (Material design font)
static char* ICON_NO_LOCATION = "\uf1aa"; // location_off (Material design font)
static char* ICON_NO_WEATHER = "\uf21b"; // cloud_off (Material design font)
static char* ICON_FETCH_ERROR = "\uf03e"; // wi-cloud-refresh (Weather Icons)
static char* ICON_VIBRATION_OVERLOAD = "\uf09e"; //wi-earthquake (Weather Icons)

static char* ICON_TIMEZONE = "\uf2a7"; 
static char* ICON_ALTITUDE = "\uf1bc"; 

#if defined(PBL_COMPASS)
static char* ICON_COMPASS_ROTATE = "\uf0c1"; 
#endif

static char* ICON_COUNTDOWN_TO_TIME = "\uf150";
static char* ICON_COUNTDOWN_FROM_TIME = "\uf151";
static char* ICON_COUNTDOWN_TO_DATE = "\uf152";
static char* ICON_COUNTDOWN_FROM_DATE = "\uf153";

static char* ICON_HAPPY = "\uf283";

#if defined(PBL_HEALTH)
static char* ICON_STEP = "\uf2ab";
static char* ICON_WALK = "\uf2b1";
static char* ICON_RUN = "\uf2ae";
static char* ICON_SLEEP = "\uf2af";
#endif

static char* ICON_SUNRISE = "\uf051"; // wi-sunrise (Weather Icons)
static char* ICON_SUNSET = "\uf052"; // wi-sunset (Weather Icons)

char* icons_get_weather_condition_symbol(enum WeatherCondition code, bool is_day);
char* icons_get_battery_symbol(uint8_t charge, bool charging, bool plugged);
#if defined(PBL_COMPASS)
char* icons_get_compass(int degrees);
#endif
char* icons_get_moonphase(int degrees);