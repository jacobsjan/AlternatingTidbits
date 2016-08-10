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
  ACTIVITY_NORMAL = 0,
  ACTIVITY_WALK,
  ACTIVITY_RUN,
  ACTIVITY_SLEEP,
};

#if defined(PBL_HEALTH)
enum HealthIndicator {
  HEALTH_EMPTY = 1,
  HEALTH_AVG_CALORIES_TILL_NOW,
  HEALTH_AVG_DISTANCE_TILL_NOW,
  HEALTH_AVG_STEPS_TILL_NOW,
  HEALTH_AVG_TOTAL_CALORIES,
  HEALTH_AVG_TOTAL_DISTANCE,
  HEALTH_AVG_TOTAL_STEPS,
  HEALTH_TODAY_CALORIES,
  HEALTH_TODAY_DISTANCE,
  HEALTH_TODAY_STEPS,
  HEALTH_ACTIVITY_CALORIES,
  HEALTH_ACTIVITY_DISTANCE,
  HEALTH_ACTIVITY_DURATION,
  HEALTH_ACTIVITY_PACE,
  HEALTH_ACTIVITY_SPEED,
  HEALTH_ACTIVITY_STEPS,
  HEALTH_TIME_DEEP_SLEEP,
  HEALTH_TIME_TOTAL_SLEEP,
  HEALTH_AVG_TIME_DEEP_SLEEP,
  HEALTH_AVG_TIME_TOTAL_SLEEP
};
#endif

struct ModelEvents {
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
  void (*on_switcher_change)();
  void (*on_tap)();
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
  int activity_calories;
  int activity_duration;
  int activity_distance;
  int activity_step_count;
  #endif
  bool switcher;
  
  struct ModelEvents events;
};

void model_reset_events();

void model_set_error(enum ErrorCodes error);
void model_set_time(struct tm *tick_time);
void model_set_weather_condition(enum WeatherCondition condition);
void model_set_weather_temperature(int weather_temperature);
void model_set_sunrise(int sunrise);
void model_set_sunset(int sunset);
void model_set_battery(uint8_t charge, bool charging, bool plugged);
#if defined(PBL_HEALTH)
void model_set_activity(enum Activities activity);
void model_set_activity_counters(int calories, int duration, int distance, int step_count);
#endif
void model_set_switcher(bool active);
void model_signal_tap();

extern struct Model* model;