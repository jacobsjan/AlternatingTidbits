#pragma once
#include<pebble.h>

enum WeatherCondition {
  CONDITION_CLEAR = 0,
  CONDITION_CLOUDY,
  CONDITION_FOG,
  CONDITION_LIGHT_RAIN,
  CONDITION_RAIN,
  CONDITION_THUNDERSTORM,
  CONDITION_SNOW,
  CONDITION_HAIL,
  CONDITION_WIND,
  CONDITION_EXTREME_WIND,
  CONDITION_TORNADO,
  CONDITION_HURRICANE,
  CONDITION_EXTREME_COLD,
  CONDITION_EXTREME_HEAT,
  CONDITION_SNOW_THUNDERSTORM,
  CONDITION_LIGHT_CLOUDY,
  CONDITION_PARTLY_CLOUDY,
};

enum ErrorCodes {
  ERROR_NONE = 0,
  ERROR_CONNECTION,
  ERROR_FETCH,
  ERROR_LOCATION,
  ERROR_WEATHER,
};

enum Activities {
  ACTIVITY_CALM = 0,
  ACTIVITY_WALK,
  ACTIVITY_RUN,
  ACTIVITY_SLEEP,
};

struct Model {
  enum ErrorCodes error;
  struct tm *time;
  enum WeatherCondition weather_condition;
  int weather_temperature;
  int sunrise;
  int sunset;
  uint8_t battery_charge;
  bool battery_charging;
  bool battery_plugged;
  #if defined(PBL_HEALTH)
  enum Activities activity;
  int activity_total_step_count;
  int activity_duration;
  int activity_distance;
  #endif
  void (*on_error_change)(enum ErrorCodes prevError);
  void (*on_time_change)();
  void (*on_weather_condition_change)();
  void (*on_weather_temperature_change)();
  void (*on_sunrise_change)();
  void (*on_sunset_change)();
  void (*on_battery_change)();
  #if defined(PBL_HEALTH)
  void (*on_activity_change)();
  void (*on_activity_counters_change)();
  #endif
};

void model_set_error(enum ErrorCodes error);
void model_set_time(struct tm *tick_time);
void model_set_weather_condition(enum WeatherCondition condition);
void model_set_weather_temperature(int weather_temperature);
void model_set_sunrise(int sunrise);
void model_set_sunset(int sunset);
void model_set_battery(uint8_t charge, bool charging, bool plugged);
#if defined(PBL_HEALTH)
void model_set_activity(enum Activities activity);
void model_set_activity_counters(int total_step_count, int duration, int distance);
#endif

extern struct Model* model;