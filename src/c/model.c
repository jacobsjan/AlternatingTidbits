#include <pebble.h>
#include "model.h"

// Initialize the model (see https://forums.pebble.com/t/question-regarding-struct/13104/4)
struct Model actual_model;
struct Model* model = &actual_model;

void model_update_update_req(enum ModelUpdates requirements, bool actionAdd) {
  enum ModelUpdates prevRequirements = model->update_req;
  if (actionAdd) {
    model->update_req |= requirements;
  } else {
    model->update_req &= ~requirements;  
  }
    
  if (model->events.on_update_req_change) {
    model->events.on_update_req_change(prevRequirements);
  } 
}

void model_add_update_req(enum ModelUpdates requirements) {
  model_update_update_req(requirements, true);
}

void model_remove_update_req(enum ModelUpdates requirements) {
  model_update_update_req(requirements, false);
}

void model_reset_events() {
  // Set the requirement change handler aside, don't reset it
  void* update_req_change_handler = model->events.on_update_req_change;
  
  // Reset all handlers
  struct ModelEvents emptyEvents = { 0 };
  model->events = emptyEvents;
  
  // Restore the requirement change handler
  model->events.on_update_req_change = update_req_change_handler;
}

void model_set_error(enum ErrorCodes error) {
  enum ErrorCodes prevError = model->error;
  model->error = error;
    
  if (model->events.on_error_change) {
    model->events.on_error_change(prevError);
  }
}

void model_set_time(struct tm *time, TimeUnits units_changed) {
  model->time = time;
  
  if ((units_changed & ~SECOND_UNIT) || (model->update_req & UPDATE_SECOND_TICKS)) {
    if (model->events.on_time_change) {
      model->events.on_time_change(units_changed);
    }
  }
}

void model_set_weather(int temperature, enum WeatherCondition condition) {
  model->weather_temperature = temperature;
  model->weather_condition = condition;
    
  if (model->update_req & UPDATE_WEATHER) {
    if (model->events.on_weather_change) {
      model->events.on_weather_change();
    }
  }
}

void model_set_sun(int dawn, int sunrise, int sunset, int dusk) {
  model->dawn = dawn;
  model->sunrise = sunrise;
  model->sunset = sunset;
  model->dusk = dusk;
    
  if (model->update_req & UPDATE_SUN) {
    if (model->events.on_sun_change) {
      model->events.on_sun_change();
    }
  }
}

void model_set_battery(uint8_t charge, bool charging, bool plugged) {
  model->battery_charge = charge;
  model->battery_charging = charging;
  model->battery_plugged = plugged;
    
  if (model->update_req & UPDATE_BATTERY) {
    if (model->events.on_battery_change) {
      model->events.on_battery_change();
    }
  }
}

#if defined(PBL_COMPASS)
void model_set_compass_heading(CompassHeadingData compass_heading) {
  model->compass_heading = compass_heading;
    
  if (model->update_req & UPDATE_COMPASS) {
    if (model->events.on_compass_heading_change) {
      model->events.on_compass_heading_change();
    }
  }
}
#endif

#if defined(PBL_HEALTH)
void model_set_activity(enum Activities activity) {
  model->activity = activity;
    
  if (model->update_req & UPDATE_HEALTH) {
    if (model->events.on_activity_change) {
      model->events.on_activity_change();
    }
  }
}

void model_set_activity_counters(int calories, int duration, int distance, int step_count, int climb, int descend) {
  model->activity_calories = calories;
  model->activity_duration = duration;
  model->activity_distance = distance;
  model->activity_step_count = step_count;
  model->activity_climb = climb;
  model->activity_descend = descend;
    
  if (model->update_req & UPDATE_HEALTH) {
    if (model->events.on_activity_counters_change) {
      model->events.on_activity_counters_change();
    }
  }
}
#endif

void model_set_moon(int moonphase, int moonillumination) {
  model->moonphase = moonphase;
  model->moonillumination = moonillumination;
    
  if (model->update_req & UPDATE_MOONPHASE) {
    if (model->events.on_moonphase_change) {
      model->events.on_moonphase_change();
    }
  }
}

void model_set_altitude(int altitude, int accuracy) {
  model->altitude = altitude;
  model->altitude_accuracy = accuracy;
    
  if (model->update_req & (UPDATE_ALTITUDE | UPDATE_ALTITUDE_CONTINUOUS)) {
    if (model->events.on_altitude_change) {
      model->events.on_altitude_change();
    }
  }
}

void model_set_location(char* location1, char* location2, char* location3) {
  if (location1) {
    strncpy(model->location1, location1, sizeof(model->location1));
  } else { 
    model->location1[0] = 0;
  }
  if (location2) {
    strncpy(model->location2, location2, sizeof(model->location2)); 
  } else { 
    model->location2[0] = 0;
  }
  if (location3) {
    strncpy(model->location3, location3, sizeof(model->location3));
  } else {
    model->location3[0] = 0;
  }
    
  if (model->update_req & UPDATE_LOCATION) {
    if (model->events.on_location_change) {
      model->events.on_location_change();
    }
  }
}

void model_signal_flick() {
  if (model->update_req & UPDATE_FLICKS) {
    if (model->events.on_flick) {
      model->events.on_flick();
    }
  }
}

void model_signal_tap() {
  if (model->update_req & UPDATE_TAPS) {
    if (model->events.on_tap) {
      model->events.on_tap();
    }
  }
}