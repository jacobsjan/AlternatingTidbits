#pragma once
#include <pebble.h>

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
  ERROR_VIBRATION_OVERLOAD,
  ERROR_ALTITUDE,
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
  HEALTH_AVG_TIME_TOTAL_SLEEP,
  HEALTH_CLIMB_DESCEND,
  #if defined(PBL_PLATFORM_DIORITE) || defined(PBL_PLATFORM_EMERY) 
  HEALTH_HEARTRATE
  #endif
};
#endif

enum ModelUpdates {
  UPDATE_SECOND_TICKS = 1 << 0,
  UPDATE_FLICKS = 1 << 1,
  UPDATE_TAPS = 1 << 2,
  UPDATE_ALTITUDE = 1 << 3,
  UPDATE_ALTITUDE_CONTINUOUS = 1 << 4,
  UPDATE_COMPASS = 1 << 5,
  UPDATE_MOONPHASE = 1 << 6,
  UPDATE_WEATHER = 1 << 7,
  UPDATE_SUN = 1 << 8,
  UPDATE_BATTERY = 1 << 9,
  UPDATE_HEALTH = 1 << 10,
  UPDATE_LOCATION = 1 << 11,
};

struct ModelEvents {
  void (*on_update_req_change)(enum ModelUpdates prev_req);
  void (*on_error_change)(enum ErrorCodes prevError);
  void (*on_time_change)(TimeUnits units_changed);
  void (*on_weather_change)();
  void (*on_sun_change)();
  void (*on_battery_change)();
  #if defined(PBL_COMPASS)
  void (*on_compass_heading_change)();
  #endif
  #if defined(PBL_HEALTH)
  void (*on_activity_change)();
  void (*on_activity_counters_change)();
  #endif
  void (*on_moonphase_change)();
  void (*on_altitude_change)();
  void (*on_location_change)();
  
  void (*on_flick)();
  void (*on_tap)();
};

struct Model {
  enum ModelUpdates update_req;
  enum ErrorCodes error;
  struct tm *time;
  enum WeatherCondition weather_condition;
  int weather_temperature;
  int dawn;
  int sunrise;
  int sunset;
  int dusk;
  #if defined(PBL_COMPASS)
  CompassHeadingData compass_heading;
  #endif
  uint8_t battery_charge;
  bool battery_charging;
  bool battery_plugged;
  #if defined(PBL_HEALTH)
  enum Activities activity;
  int activity_calories;
  int activity_duration;
  int activity_distance;
  int activity_step_count;
  int activity_climb;
  int activity_descend;
  #endif
  int moonphase;
  int moonillumination;
  int altitude;
  int altitude_accuracy;
  char location1[30];
  char location2[30];
  char location3[30];
  
  struct ModelEvents events;
};

void model_add_update_req(enum ModelUpdates requirements);
void model_remove_update_req(enum ModelUpdates requirements);
void model_reset_events();

void model_set_error(enum ErrorCodes error);
void model_set_time(struct tm *tick_time, TimeUnits units_changed);
void model_set_weather(int weather_temperature, enum WeatherCondition condition);
void model_set_sun(int dawn, int sunrise, int sunset, int dusk);
void model_set_battery(uint8_t charge, bool charging, bool plugged);
#if defined(PBL_COMPASS)
void model_set_compass_heading(CompassHeadingData compass_heading);
#endif
#if defined(PBL_HEALTH)
void model_set_activity(enum Activities activity);
void model_set_activity_counters(int calories, int duration, int distance, int step_count, int climb, int descend);
#endif
void model_set_moon(int moonphase, int moonillumination);
void model_set_altitude(int altitude, int accuracy);
void model_set_location(char* location1, char* location2, char* location3);

void model_signal_flick();
void model_signal_tap();

extern struct Model* model;