#include <pebble.h>
#include "model.h"

// Initialize the model (see https://forums.pebble.com/t/question-regarding-struct/13104/4)
struct Model actual_model;
struct Model* model = &actual_model;

void model_reset_events() {
  struct ModelEvents emptyEvents = { 0 };
  model->events = emptyEvents;
}

void model_set_error(enum ErrorCodes error) {
  enum ErrorCodes prevError = model->error;
  model->error = error;
    
  if (model->events.on_error_change) {
    model->events.on_error_change(prevError);
  }
}

void model_set_time(struct tm *time) {
  model->time = time;
  
  if (model->events.on_time_change) {
    model->events.on_time_change();
  }
}

void model_set_weather_condition(enum WeatherCondition condition) {
  model->weather_condition = condition;
    
  if (model->events.on_weather_condition_change) {
    model->events.on_weather_condition_change();
  }
}

void model_set_weather_temperature(int temperature) {
  model->weather_temperature = temperature;
    
  if (model->events.on_weather_temperature_change) {
    model->events.on_weather_temperature_change();
  }
}

void model_set_sunrise(int sunrise) {
  model->sunrise = sunrise;
    
  if (model->events.on_sunrise_change) {
    model->events.on_sunrise_change();
  }
}

void model_set_sunset(int sunset) {
  model->sunset = sunset;
    
  if (model->events.on_sunset_change) {
    model->events.on_sunset_change();
  }
}

void model_set_battery(uint8_t charge, bool charging, bool plugged) {
  model->battery_charge = charge;
  model->battery_charging = charging;
  model->battery_plugged = plugged;
    
  if (model->events.on_battery_change) {
    model->events.on_battery_change();
  }
}

#if defined(PBL_HEALTH)
void model_set_activity(enum Activities activity) {
  model->activity = activity;
    
  if (model->events.on_activity_change) {
    model->events.on_activity_change();
  }
}

void model_set_activity_counters(int calories, int duration, int distance, int step_count) {
  model->activity_calories = calories;
  model->activity_duration = duration;
  model->activity_distance = distance;
  model->activity_step_count = step_count;
    
  if (model->events.on_activity_counters_change) {
    model->events.on_activity_counters_change();
  }
}
#endif

void model_set_switcher(bool active) {
  model->switcher = active;
      
  if (model->events.on_switcher_change) {
    model->events.on_switcher_change();
  }
}

void model_signal_tap() {
  if (model->events.on_tap) {
    model->events.on_tap();
  }
}