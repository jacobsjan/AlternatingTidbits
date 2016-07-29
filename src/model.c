#include <pebble.h>
#include "model.h"

// Initialize the model (see https://forums.pebble.com/t/question-regarding-struct/13104/4)
struct Model actual_model;
struct Model* model = &actual_model;

void model_set_error(enum ErrorCodes error) {
  enum ErrorCodes prevError = model->error;
  model->error = error;
    
  if (model->on_error_change) {
    model->on_error_change(prevError);
  }
}

void model_set_time(struct tm *time) {
  model->time = time;
  
  if (model->on_time_change) {
    model->on_time_change();
  }
}

void model_set_weather_condition(enum WeatherCondition condition) {
  model->weather_condition = condition;
    
  if (model->on_weather_condition_change) {
    model->on_weather_condition_change();
  }
}

void model_set_weather_temperature(int temperature) {
  model->weather_temperature = temperature;
    
  if (model->on_weather_temperature_change) {
    model->on_weather_temperature_change();
  }
}

void model_set_sunrise(int sunrise) {
  model->sunrise = sunrise;
    
  if (model->on_sunrise_change) {
    model->on_sunrise_change();
  }
}

void model_set_sunset(int sunset) {
  model->sunset = sunset;
    
  if (model->on_sunset_change) {
    model->on_sunset_change();
  }
}

void model_set_battery(uint8_t charge, bool charging, bool plugged) {
  model->battery_charge = charge;
  model->battery_charging = charging;
  model->battery_plugged = plugged;
    
  if (model->on_battery_change) {
    model->on_battery_change();
  }
}

#if defined(PBL_HEALTH)
void model_set_activity(enum Activities activity) {
  model->activity = activity;
    
  if (model->on_activity_change) {
    model->on_activity_change();
  }
}

void model_set_activity_counters(int total_step_count, int duration, int distance) {
  model->activity_total_step_count = total_step_count;
  model->activity_duration = duration;
  model->activity_distance = distance;
    
  if (model->on_activity_counters_change) {
    model->on_activity_counters_change();
  }
}
#endif