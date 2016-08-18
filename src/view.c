#include <pebble.h>
#include "view.h"
#include "config.h"
#include "model.h"
#include "icons.h"

enum SuspensionReasons {
  SUSPENSION_NONE = 0,
  SUSPENSION_ERROR_ALERT = 1,
  SUSPENSION_ACTIVITY = 2,
  SUSPENSION_SWITCHER = 4,
};

struct Layers {
  // Always shown layers
  TextLayer *hour;
  TextLayer *colon;
  TextLayer *minute;
  Layer *date_top;
  Layer *date_bottom;
  
  // Switcher
  Layer *switcher;
  
  // Alternating layers, only one visible at a time
  Layer *error;
  Layer *weather;
  Layer *sunrise_sunset;
  Layer *battery;
  #if defined(PBL_COMPASS)
  Layer *compass;
  #endif
  Layer *moonphase;
  Layer *timezone;
  #if defined(PBL_HEALTH)
  Layer *health;
  #endif
};

struct Fonts {
  GFont primary;
  GFont secondary;
  GFont icons;
  GFont icons_small;
  GFont icons_large;
};

struct View {
  Window *window;
  struct Layers layers;
  struct Fonts fonts;
  
  // Alternating layer info
  struct Layer *alt_layers[sizeof(struct Layers) / sizeof(Layer*) - 6]; // Very nasty trick to avoid forgetting to increase the array size
  char* alt_layer_icons[sizeof(struct Layers) / sizeof(Layer*) - 6];
  int alt_layer_count;
  int alt_layer_visible;
  
  // Suspension info
  enum SuspensionReasons suspension_reason;
  int suspension_return_layer;
  
  // Alert info
  AppTimer *alert_timeout_handler;
  
  // Compass info
  bool compass_subscribed;
  CompassHeadingData compass_heading_data;
  
  // Switcher animation
  Animation* switcher_animation;
  AnimationProgress switcher_animation_progress;
};

struct View view;

bool is_daytime() { 
  if (model->sunrise != 0 && model->sunset != 0) {
    int minutes = model->time->tm_hour * 60 + model->time->tm_min;
    if (model->sunrise <= minutes && minutes < model->sunset) {
      return true;
    }
    else {
      return false;
    }
  }
  else {
    // Default to daytime if sunrise/sunset is unknown
    return true;
  }
}

GSize calculate_total_size(GRect bounds, int text_count, char *texts[]) {
  GSize result = GSize(0, 0);
  
  for (int i = 0; i < text_count; ++i) {
    if (texts[i]) {
      GSize text_size = graphics_text_layout_get_content_size(texts[i], view.fonts.secondary, bounds, GTextOverflowModeWordWrap, GTextAlignmentLeft);
      result.w += text_size.w;
      result.h = text_size.h > result.h ? text_size.h : result.h;
    }
  } 
  
  return result;
}

void draw_alternating_text(GContext *ctx, GRect bounds, int text_count, char *texts[]) {
  for (int i = 0; i < text_count; ++i) {
    if (texts[i]) {
      // Draw the text
      graphics_context_set_text_color(ctx, i % 2 == 0 ? config->color_secondary : config->color_accent); // Alternate color
      graphics_draw_text(ctx, texts[i], view.fonts.secondary, bounds, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
      
      // Shrink bounds to the right
      GSize text_size = graphics_text_layout_get_content_size(texts[i], view.fonts.secondary, bounds, GTextOverflowModeWordWrap, GTextAlignmentLeft);
      bounds.origin.x += text_size.w;
      bounds.size.w -= text_size.w;
    }
  }
}

void draw_centered(Layer *layer, GContext *ctx, char* icon, GColor icon_color, int text_count, char *texts[]) { 
  // Calculate center alignment
  GRect bounds = layer_get_bounds(layer);
  GSize icon_size = graphics_text_layout_get_content_size(icon, view.fonts.icons, bounds, GTextOverflowModeWordWrap, GTextAlignmentRight);
  GSize text_size = calculate_total_size(bounds, text_count, texts);
  int total_width = icon_size.w + text_size.w;
  int icon_left = bounds.origin.x + (bounds.size.w - total_width) / 2;
  int text_left = icon_left + icon_size.w;
  
  // Draw icon
  GRect draw_bounds = GRect(icon_left, bounds.origin.y, icon_size.w, icon_size.h);
  graphics_context_set_text_color(ctx, icon_color);
  graphics_draw_text(ctx, icon, view.fonts.icons, draw_bounds, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    
  // Draw texts
  draw_bounds = GRect(text_left, bounds.origin.y + PBL_IF_ROUND_ELSE(6, 4), text_size.w, text_size.h);
  draw_alternating_text(ctx, draw_bounds, text_count, texts);
}

void error_update_proc(Layer *layer, GContext *ctx) { 
  // Determine symbol and text to be shown
  char* symbol;
  char* text;
  switch (model->error) {
     case ERROR_CONNECTION:
      symbol = ICON_NO_BLUETOOTH;
      text = "No bluetooth";
      break; 
    case ERROR_FETCH:
      symbol = ICON_FETCH_ERROR;
      text = "Signal issue";
      break; 
    case ERROR_LOCATION:
      symbol = ICON_NO_LOCATION;
      text = "No location";
      break; 
    case ERROR_WEATHER:
      symbol = ICON_NO_WEATHER;
      text = " Weather issue";
      break; 
    case ERROR_VIBRATION_OVERLOAD:
      symbol = ICON_VIBRATION_OVERLOAD;
      text = " Switcher pause";
      break; 
    default:
      // Bluetooth ok once again alert
      symbol = ICON_BLUETOOTH;
      text = "Bluetooth ok";
      break; 
  }
  
  // Draw
  char *texts[] = { text };
  GColor symbol_color = model->error == ERROR_NONE ? config->color_secondary : config->color_accent;
  draw_centered(layer, ctx, symbol, symbol_color, sizeof(texts) / sizeof(texts[0]), texts);
}

void timezone_update_proc(Layer *layer, GContext *ctx) {
  const int SEPARATOR = 4;
  
  // Format the time of the alternative timezone
  time_t altEpoch = time(NULL) + config->timezone_offset * SECONDS_PER_MINUTE;
  struct tm *altTime = localtime(&altEpoch);
    
  char hour[3];
  char minute[3];
  char ampm[3];
  strftime(hour, sizeof(hour), clock_is_24h_style() ? "%H" : "%I", altTime);
  strftime(minute, sizeof(minute), "%M", altTime);
  if (clock_is_24h_style()) {
    ampm[0] = 0;
  } else {
    strftime(ampm, sizeof(ampm), "%p", altTime);
    ampm[0] -= 'A' - 'a';
    ampm[1] -= 'A' - 'a';
  }
  
  // Remove hours leading zero
  if (hour[0] == '0' && !config->date_hours_leading_zero) {
    hour[0] = hour[1];
    hour[1] = hour[2];
  }
  
  // Make sure we leave the localtime behind in a good state
  altEpoch = time(NULL);
  localtime(&altEpoch);
  
  char* bottom_texts[] = { config->timezone_city };
  int bottom_text_count = 1;
  char* middle_texts[] = { hour, ":", minute, " ", ampm };
  int middle_text_count = 5;
  
  // Calculate center alignment
  GRect bounds = layer_get_bounds(layer);
  GSize icon_size = graphics_text_layout_get_content_size(ICON_TIMEZONE, view.fonts.icons, bounds, GTextOverflowModeWordWrap, GTextAlignmentRight);
  GSize bottom_text_size = calculate_total_size(bounds, bottom_text_count, bottom_texts);
  GSize middle_text_size = calculate_total_size(bounds, middle_text_count, middle_texts);
  int max_texts_width = bottom_text_size.w > middle_text_size.w ? bottom_text_size.w : middle_text_size.w;
  int total_width = icon_size.w + SEPARATOR + max_texts_width;
  int icon_left = bounds.origin.x + (bounds.size.w - total_width) / 2;
  int text_left = icon_left + SEPARATOR + icon_size.w;
  
  // Draw the timezone icon
  GRect draw_bounds = GRect(icon_left, bounds.origin.y + PBL_IF_ROUND_ELSE(34, 30), icon_size.w, icon_size.h);
  graphics_context_set_text_color(ctx, config->color_secondary);
  graphics_draw_text(ctx, ICON_TIMEZONE, view.fonts.icons, draw_bounds, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  
  // Draw the time
  draw_bounds = GRect(text_left, bounds.origin.y + PBL_IF_ROUND_ELSE(12, 10) + PBL_IF_ROUND_ELSE(28, 24) - 20, middle_text_size.w, middle_text_size.h);
  draw_alternating_text(ctx, draw_bounds, middle_text_count, middle_texts);
    
  // Draw the city
  draw_bounds = GRect(text_left, bounds.origin.y + PBL_IF_ROUND_ELSE(12, 10) + PBL_IF_ROUND_ELSE(28, 24), bottom_text_size.w, bottom_text_size.h);
  draw_alternating_text(ctx, draw_bounds, bottom_text_count, bottom_texts);
}

void weather_update_proc(Layer *layer, GContext *ctx) {
  char* condition = icons_get_weather_condition_symbol(model->weather_condition, is_daytime());
  char temperature[6];
  snprintf(temperature, sizeof(temperature), "%d", model->weather_temperature);
  
  // Draw
  char *texts[] = { " ", NULL, temperature, "\u00b0"}; // Degree symbol
  draw_centered(layer, ctx, condition, config->color_secondary, sizeof(texts) / sizeof(texts[0]), texts);
}

void sunrise_sunset_update_proc(Layer *layer, GContext *ctx) {
  bool daytime = is_daytime();
  char* symbol = daytime ? ICON_SUNSET : ICON_SUNRISE; 
  char hour[3];
  char minute[3];
  snprintf(hour, sizeof(hour), "%d", (daytime ? model->sunset : model->sunrise) / 60);
  snprintf(minute, sizeof(minute), "%02d", (daytime ? model->sunset : model->sunrise) % 60);
  
  // Draw
  char *texts[] = { " ", NULL, hour, ":", minute}; 
  draw_centered(layer, ctx, symbol, config->color_secondary, sizeof(texts) / sizeof(texts[0]), texts);
}

void battery_update_proc(Layer *layer, GContext *ctx) {
  int battery_charge = model->battery_charge;
  if (battery_charge == 0 && !model->battery_plugged) {
    battery_charge = 5; 
  } else if (battery_charge > 0 && model->battery_plugged) {
    battery_charge += 30;
  }
  
  char* battery_icon = icons_get_battery_symbol(battery_charge, model->battery_charging, model->battery_plugged);
  char charge[5];
  if (battery_charge == 0 && model->battery_plugged) {
    charge[0] = '<';
    charge[1] = ' ';
    charge[2] = '4';
    charge[3] = '0';
    charge[4] = 0;
  } else {
    snprintf(charge, sizeof(charge), "%d", battery_charge);
  }
  
  // Draw
  char *texts[] = { charge, "%" };
  GColor battery_color = model->battery_charge <= config->battery_accent_from ? config->color_accent : config->color_secondary;
  draw_centered(layer, ctx, battery_icon, battery_color, sizeof(texts) / sizeof(texts[0]), texts);
}

#if defined(PBL_COMPASS)
static void compass_heading_updated(CompassHeadingData heading) {
  // Update heading data
  view.compass_heading_data = heading;
    
  // Update layer or unsubscribe if it's no longer visible
  if (view.layers.compass == NULL || view.alt_layers[view.alt_layer_visible] != view.layers.compass) {
    // Unsubscribe from compass events
    if (view.compass_subscribed) {
      compass_service_unsubscribe();
      view.compass_subscribed = false;
    }
  } else {
    // Redraw the compass
    layer_mark_dirty(view.layers.compass);
  }
}

static void compass_update_proc(Layer *layer, GContext *ctx) {    
  if (!view.compass_subscribed) {
    // Subscribe to compass heading updates
    compass_service_subscribe(&compass_heading_updated);
    compass_service_set_heading_filter(TRIG_MAX_ANGLE / 32);
    view.compass_subscribed = true;  
  }
  
  // Read heading
  char* icon;
  if (view.compass_heading_data.compass_status != CompassStatusDataInvalid) {
    int degrees = TRIGANGLE_TO_DEG(view.compass_heading_data.magnetic_heading);
    icon = icons_get_compass(degrees);
  } else {
    icon = ICON_COMPASS_ROTATE;
  }
  
  // Calculate center alignment
  GRect bounds = layer_get_bounds(layer);
  GSize icon_size = graphics_text_layout_get_content_size(icon, view.fonts.icons_large, bounds, GTextOverflowModeWordWrap, GTextAlignmentRight);
  int icon_left = bounds.origin.x + (bounds.size.w - icon_size.w) / 2;
  
  // Draw the compass
  GRect draw_bounds = GRect(icon_left, bounds.origin.y + PBL_IF_ROUND_ELSE(8, 4), icon_size.w, icon_size.h);
  graphics_context_set_text_color(ctx, config->color_secondary);
  graphics_draw_text(ctx, icon, view.fonts.icons_large, draw_bounds, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
}
#endif

#if defined(PBL_HEALTH)
char* alloc_print_d(char* fmt, int i) {
  int size = snprintf(NULL, 0, fmt, i);
  char* result = (char*)malloc((size + 1) * sizeof(char));
  snprintf(result, size + 1, fmt, i);
  return result;
}

char* alloc_print_s(char* s) {
  int size = strlen(s);
  char* result = (char*)malloc((size + 1) * sizeof(char));
  return strncpy(result, s, size + 1);
}

char** health_generate_texts(enum HealthIndicator indicator) {
  char** result = (char**)malloc(4 * sizeof(char*));
  int metric = -1;
  int distance;
  switch (indicator) {
    case HEALTH_EMPTY:
      free(result);
      result = NULL;    
      break;
    case HEALTH_AVG_CALORIES_TILL_NOW:
      if (metric < 0) metric = (int)health_service_sum_averaged(HealthMetricRestingKCalories, time_start_of_today(), time(NULL), HealthServiceTimeScopeWeekly) + (int)health_service_sum_averaged(HealthMetricActiveKCalories, time_start_of_today(), time(NULL), HealthServiceTimeScopeWeekly);
    case HEALTH_AVG_TOTAL_CALORIES:
      if (metric < 0) metric = (int)health_service_sum_averaged(HealthMetricRestingKCalories, time_start_of_today(), time_start_of_today() + SECONDS_PER_DAY, HealthServiceTimeScopeWeekly) + (int)health_service_sum_averaged(HealthMetricActiveKCalories, time_start_of_today(), time_start_of_today() + SECONDS_PER_DAY, HealthServiceTimeScopeWeekly);
    case HEALTH_TODAY_CALORIES:
      if (metric < 0) metric = (int)health_service_sum_today(HealthMetricRestingKCalories) + (int)health_service_sum_today(HealthMetricActiveKCalories);
    case HEALTH_ACTIVITY_CALORIES:
      if (metric < 0) metric = model->activity_calories;
  
      if (metric < 1000) {
        result[0] = alloc_print_d("%d", metric);
        result[1] = alloc_print_s("kcal");
        result[2] = NULL;
        result[3] = NULL;
      } else {
        result[0] = alloc_print_d("%d", metric / 1000);
        result[1] = alloc_print_s(config->health_number_format == 'M' ? "." : ",");
        result[2] = alloc_print_d("%03d", metric % 1000);
        result[3] = alloc_print_s("kcal");
      }     
      break;
    case HEALTH_AVG_DISTANCE_TILL_NOW:
      if (metric < 0) metric = (int)health_service_sum_averaged(HealthMetricWalkedDistanceMeters, time_start_of_today(), time(NULL), HealthServiceTimeScopeWeekly);
    case HEALTH_AVG_TOTAL_DISTANCE:
      if (metric < 0) metric = (int)health_service_sum_averaged(HealthMetricWalkedDistanceMeters, time_start_of_today(), time_start_of_today() + SECONDS_PER_DAY, HealthServiceTimeScopeWeekly);
    case HEALTH_TODAY_DISTANCE:
      if (metric < 0) metric = (int)health_service_sum_today(HealthMetricWalkedDistanceMeters);
    case HEALTH_ACTIVITY_DISTANCE:
      if (metric < 0) metric = model->activity_distance;
  
      if (metric < 1000 && config->health_distance_unit == 'M') {
        result[0] = alloc_print_d("%d", metric);
        result[1] = alloc_print_s("m");
        result[2] = NULL;
        result[3] = NULL;
      } else {
        if (config->health_distance_unit == 'I') metric = (metric * 1000) / 1609;
        result[0] = alloc_print_d("%d", (metric + 50) / 1000);
        result[1] = alloc_print_s(config->health_number_format == 'M' ? "," : ".");
        result[2] = alloc_print_d("%d", ((metric + 50) % 1000) / 100); 
        result[3] = alloc_print_s(config->health_distance_unit == 'M' ? "km" : "mi");
      }      
      break;
    case HEALTH_AVG_STEPS_TILL_NOW:
      if (metric < 0) metric = (int)health_service_sum_averaged(HealthMetricStepCount, time_start_of_today(), time(NULL), HealthServiceTimeScopeWeekly);
    case HEALTH_AVG_TOTAL_STEPS:
      if (metric < 0) metric = (int)health_service_sum_averaged(HealthMetricStepCount, time_start_of_today(), time_start_of_today() + SECONDS_PER_DAY, HealthServiceTimeScopeWeekly);
    case HEALTH_TODAY_STEPS:
      if (metric < 0) metric = (int)health_service_sum_today(HealthMetricStepCount);
    case HEALTH_ACTIVITY_STEPS:
      if (metric < 0) metric = model->activity_step_count;
  
      if (metric < 1000) {
        result[0] = alloc_print_d("%d", metric);
        result[1] = NULL;
        result[2] = NULL;
        result[3] = NULL;
      } else {
        result[0] = alloc_print_d("%d", metric / 1000);
        result[1] = alloc_print_s(config->health_number_format == 'M' ? "." : ",");
        result[2] = alloc_print_d("%03d", metric % 1000);
        result[3] = NULL;
      }     
      break;
    case HEALTH_AVG_TIME_DEEP_SLEEP:
      if (metric < 0) metric = (int)health_service_sum_averaged(HealthMetricSleepRestfulSeconds, time_start_of_today(), time_start_of_today() + SECONDS_PER_DAY, HealthServiceTimeScopeWeekly);
    case HEALTH_AVG_TIME_TOTAL_SLEEP:
      if (metric < 0) metric = (int)health_service_sum_averaged(HealthMetricSleepSeconds, time_start_of_today(), time_start_of_today() + SECONDS_PER_DAY, HealthServiceTimeScopeWeekly);
    case HEALTH_TIME_DEEP_SLEEP:
      if (metric < 0) metric = (int)health_service_sum_today(HealthMetricSleepRestfulSeconds) / SECONDS_PER_MINUTE;
    case HEALTH_TIME_TOTAL_SLEEP:
      if (metric < 0) metric = (int)health_service_sum_today(HealthMetricSleepSeconds) / SECONDS_PER_MINUTE;
    case HEALTH_ACTIVITY_DURATION:
      if (metric < 0) metric = model->activity_duration;
      
      result[0] = alloc_print_d("%d", metric / 60);
      result[1] = alloc_print_s(":");
      result[2] = alloc_print_d("%02d", metric % 60);
      result[3] = NULL;
      break;
    case HEALTH_ACTIVITY_PACE:
      distance = config->health_distance_unit == 'M' ? model->activity_distance : (model->activity_distance * 1000) / 1609;
      int sec_per_dist = (model->activity_duration * 60 * 1000) / distance;
      
      result[0] = alloc_print_d("%d", sec_per_dist / 60);
      result[1] = alloc_print_s("m");
      result[2] = alloc_print_d("%02d", sec_per_dist % 60);
      result[3] = alloc_print_s(config->health_distance_unit == 'M' ? "s/km" : "s/mi");
      break;
    case HEALTH_ACTIVITY_SPEED:
      distance = config->health_distance_unit == 'M' ? model->activity_distance : (model->activity_distance * 1000) / 1609;
      int dist_per_hour = (distance * 60) / model->activity_duration;
    
      result[0] = alloc_print_d("%d", (dist_per_hour + 50) / 1000);
      result[1] = alloc_print_s(config->health_number_format == 'M' ? "," : ".");
      result[2] = alloc_print_d("%d", ((dist_per_hour + 50) % 1000) / 100);
      result[3] = alloc_print_s(config->health_distance_unit == 'M' ? "km/h" : "mi/h");
      break;
  }
  return result;
}

void health_update_proc(Layer *layer, GContext *ctx) {
  const int SEPARATOR = 4;
  const char* health_icon;  
  char** bottom_texts;
  char** middle_texts;
  char** top_texts;
  
  switch (model->activity) {    
    case ACTIVITY_WALK:
      health_icon = ICON_WALK;
      top_texts = health_generate_texts(config->health_walk_top);
      middle_texts = health_generate_texts(config->health_walk_middle);
      bottom_texts = health_generate_texts(config->health_walk_bottom);
      break;
    
    case ACTIVITY_RUN:
      health_icon = ICON_RUN;
      top_texts = health_generate_texts(config->health_run_top);
      middle_texts = health_generate_texts(config->health_run_middle);
      bottom_texts = health_generate_texts(config->health_run_bottom);
      break;
    
    case ACTIVITY_SLEEP:
      health_icon = ICON_SLEEP;
      top_texts = health_generate_texts(config->health_sleep_top);
      middle_texts = health_generate_texts(config->health_sleep_middle);
      bottom_texts = health_generate_texts(config->health_sleep_bottom);
      break;
    
    case ACTIVITY_NORMAL:
    default:
      health_icon = ICON_STEP;
      top_texts = health_generate_texts(config->health_normal_top);
      middle_texts = health_generate_texts(config->health_normal_middle);
      bottom_texts = health_generate_texts(config->health_normal_bottom);
      break;
  }
  
  int top_text_count = top_texts ? 4 : 0;
  int middle_text_count = middle_texts ? 4 : 0;
  int bottom_text_count = bottom_texts ? 4 : 0;
  
  // Calculate center alignment
  GRect bounds = layer_get_bounds(layer);
  GSize icon_size = graphics_text_layout_get_content_size(health_icon, view.fonts.icons, bounds, GTextOverflowModeWordWrap, GTextAlignmentRight);
  GSize bottom_text_size = calculate_total_size(bounds, bottom_text_count, bottom_texts);
  GSize middle_text_size = calculate_total_size(bounds, middle_text_count, middle_texts);
  GSize top_text_size = calculate_total_size(bounds, top_text_count, top_texts);
  int max_texts_width = bottom_text_size.w > middle_text_size.w ? bottom_text_size.w : middle_text_size.w;
  max_texts_width = top_text_size.w > max_texts_width ? top_text_size.w : max_texts_width;
  int total_width = icon_size.w + SEPARATOR + max_texts_width;
  int icon_left = bounds.origin.x + (bounds.size.w - total_width) / 2;
  int text_left = icon_left + SEPARATOR + icon_size.w;
  
  // Draw the health icon
  GRect draw_bounds = GRect(icon_left, bounds.origin.y + PBL_IF_ROUND_ELSE(34, 30), icon_size.w, icon_size.h);
  graphics_context_set_text_color(ctx, config->color_secondary);
  graphics_draw_text(ctx, health_icon, view.fonts.icons, draw_bounds, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  
  // Draw the top text
  draw_bounds = GRect(text_left, bounds.origin.y + PBL_IF_ROUND_ELSE(12, 10) + PBL_IF_ROUND_ELSE(28, 24) - 40, top_text_size.w, top_text_size.h);
  draw_alternating_text(ctx, draw_bounds, top_text_count, top_texts);
  
  // Draw the middle text
  draw_bounds = GRect(text_left, bounds.origin.y + PBL_IF_ROUND_ELSE(12, 10) + PBL_IF_ROUND_ELSE(28, 24) - 20, middle_text_size.w, middle_text_size.h);
  draw_alternating_text(ctx, draw_bounds, middle_text_count, middle_texts);
    
  // Draw the bottom text
  draw_bounds = GRect(text_left, bounds.origin.y + PBL_IF_ROUND_ELSE(12, 10) + PBL_IF_ROUND_ELSE(28, 24), bottom_text_size.w, bottom_text_size.h);
  draw_alternating_text(ctx, draw_bounds, bottom_text_count, bottom_texts);
  
  // Free allocated memory
  if (top_texts) {
    free(top_texts[0]);
    free(top_texts[1]);
    free(top_texts[2]);
    free(top_texts[3]);
    free(top_texts);
  }
  if (middle_texts) {
    free(middle_texts[0]);
    free(middle_texts[1]);
    free(middle_texts[2]);
    free(middle_texts[3]);
    free(middle_texts);
  }
  if (bottom_texts) {
    free(bottom_texts[0]);
    free(bottom_texts[1]);
    free(bottom_texts[2]);
    free(bottom_texts[3]);
    free(bottom_texts);
  }
}
#endif

void moonphase_update_proc(Layer *layer, GContext *ctx) {
  char* moon_icon = icons_get_moonphase(model->moonphase);
  char illumination[4];
  snprintf(illumination, sizeof(illumination), "%d", model->moonillumination);
  
  // Draw
  char *texts[] = { " ", NULL, illumination, "%"}; 
  draw_centered(layer, ctx, moon_icon, config->color_secondary, sizeof(texts) / sizeof(texts[0]), texts);
}

void date_update_proc(Layer *layer, GContext *ctx, char* format) {
  int element_count = strlen(format);
  int text_count = element_count * 2;
  char* texts[text_count];
  
  // Construct texts based on the provided format string
  for (int i = 0; i < element_count; ++i) {
    char element = format[i];
    if (element == 'n') element = 'm'; // Month without leading 0 is not provided by strftime
    
    char* normalText;
    char* accentText;
    if ((element >= 'a' && element <= 'z') || (element >= 'A' && element <= 'Z')) {
      // Date element
      normalText = (char*)malloc(20 * sizeof(char));
      accentText = NULL;
      char strfmt[] = "%x";
      strfmt[1] = element;
      strftime(normalText, 20, strfmt, model->time);
      
      if ((format[i] == 'n' && normalText[0] == '0') || (element == 'e' && normalText[0] == ' ')) {
        // Remove leading 0 of month or leading space of day
        normalText[0] = normalText[1];
        normalText[1] = normalText[2];
      }
    } else {
      // Separartor element
      normalText = NULL;
      accentText = (char*)malloc(2 * sizeof(char));
      accentText[0] = element;
      accentText[1] = 0;
    }
    
    texts[i * 2] = normalText;
    texts[i * 2 + 1] = accentText;
  }      
  
  // Calculate center alignment
  GRect bounds = layer_get_bounds(layer);
  GSize text_size = calculate_total_size(bounds, text_count, texts);
  int text_left = bounds.origin.x + (bounds.size.w - text_size.w) / 2;
  
  // Draw texts
  GRect draw_bounds = GRect(text_left, bounds.origin.y, text_size.w, text_size.h);
  draw_alternating_text(ctx, draw_bounds, text_count, texts);
  
  // Free memory of texts
  for (int i = 0; i < text_count; ++i) free(texts[i]);
}

void date_top_update_proc(Layer *layer, GContext *ctx) {
  date_update_proc(layer, ctx, config->date_format_top);
}

void date_bottom_update_proc(Layer *layer, GContext *ctx) {
  date_update_proc(layer, ctx, config->date_format_bottom);
}

void week_bar_update_proc(Layer* layer, GContext* ctx, bool start_with_monday) {
  // Calculate center alignmet
  GRect bounds = layer_get_bounds(layer);
  char day_texts[6][7]; // Large enough for unicode
  GSize day_sizes[7];
  GFont day_fonts[7];
  
  GSize text_size = GSize(0, 0);
  for (struct tm day = { .tm_wday = 0 }; day.tm_wday <= 6; day.tm_wday++) {
    int i = day.tm_wday;
    strftime(day_texts[i], sizeof(day_texts[i]), "%a ", &day);
    if (day_texts[i][0] >= 'a' && day_texts[i][0] <= 'z') day_texts[i][0] += 'A' - 'a'; // Capitalize first letter, e.g. in Spanish
    if (day_texts[i][1] >= 'a' && day_texts[i][1] <= 'z') day_texts[i][1] += 'A' - 'a'; // Capitalize second letter, e.g. in English
    if ((day_texts[i][0] & 0xc0) != 0xc0 && (day_texts[i][1] & 0xc0) != 0xc0) { // Check for unicode e.g. in Spanish
      day_texts[i][2] = ' '; // Abreviate to two characters per day 
      day_texts[i][3] = 0;
    }
    day_fonts[i] = fonts_get_system_font(day.tm_wday == 0 || day.tm_wday == 6 ? FONT_KEY_GOTHIC_14_BOLD : FONT_KEY_GOTHIC_14);
    day_sizes[i] = graphics_text_layout_get_content_size(day_texts[i], day_fonts[i], bounds, GTextOverflowModeWordWrap, GTextAlignmentLeft);
    text_size.w += day_sizes[i].w;
    text_size.h = text_size.h > day_sizes[i].h ? text_size.h : day_sizes[i].h;
  }
  GSize space_size = graphics_text_layout_get_content_size(" ", day_fonts[0], bounds, GTextOverflowModeWordWrap, GTextAlignmentLeft);
  text_size.w -= space_size.w;
  int text_left = bounds.origin.x + (bounds.size.w - text_size.w) / 2;
  
  // Draw days of the week
  int day_left = text_left;
  for (int i = 0; i < 7; ++i) {
    int j = start_with_monday ? (i + 1) % 7 : i; 
    char* day_text = day_texts[j];
    GSize day_size = day_sizes[j];
    GFont font = day_fonts[j];
    
    // Draw marker
    if (j == model->time->tm_wday) {
      graphics_context_set_fill_color(ctx, config->color_accent);
      GRect draw_bounds = GRect(day_left + 1, bounds.origin.y + day_size.h + 2, day_size.w - space_size.w - 2, 3);
      graphics_fill_rect(ctx, draw_bounds, 0, GCornerNone);
    }
    
    // Draw text
    GRect draw_bounds = GRect(day_left, bounds.origin.y, day_size.w, day_size.h);
    graphics_context_set_text_color(ctx, config->color_secondary);
    graphics_draw_text(ctx, day_text, font, draw_bounds, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    
    // Move to the right
    day_left += day_size.w;
  }
} 

void week_bar_monday_update_proc(Layer* layer, GContext* ctx) {
  week_bar_update_proc(layer, ctx, true);
}

void week_bar_sunday_update_proc(Layer* layer, GContext* ctx) {
  week_bar_update_proc(layer, ctx, false);
}

// From https://github.com/robisodd/concentricity/blob/master/src/rect2.c
#if defined(PBL_RECT)
GPoint get_point_on_rect(int angle, GRect rect) {                         // Returns a point on rect made from center of rect and angle
  int32_t sin = sin_lookup(angle), cos = cos_lookup(angle);               // Calculate once and store, to make quicker and cleaner
  int32_t dy = (sin > 0 ? (rect.size.h - 1) : (0 - rect.size.h)) / 2;     // Distance to top or bottom edge (from center)
  int32_t dx = (cos > 0 ? (rect.size.w - 1) : (0 - rect.size.w)) / 2;     // Distance to left or right edge (from center)
  if(abs(dx * sin) < abs(dy * cos))                                   // if (distance to vertical line) < (distance to horizontal line)
    dy = (dx * sin) / cos;                                                // calculate y position on left or right edge
   else                                                                   // else: (distance to top or bottom edge) < (distance to left or right edge)
    dx = (dy * cos) / sin;                                                // calculate x position on top or bottom line
  return GPoint(dx + rect.origin.x + (rect.size.w / 2), dy + rect.origin.y + (rect.size.h / 2));  // Return point on rect
}
#endif

void switcher_update_proc(Layer* layer, GContext* ctx) {
  GRect bounds = layer_get_bounds(layer);
  #if defined(PBL_RECT)
  GRect inner_rect = grect_inset(bounds, GEdgeInsets(14));
  #elif defined(PBL_ROUND)
  GRect inner_rect = grect_inset(bounds, GEdgeInsets(18));
  #endif
  
  // Draw layer queue
  for (int pos = 0, i = (view.alt_layer_visible + 1) % view.alt_layer_count; i != view.alt_layer_visible; ++pos, i = (i + 1) % view.alt_layer_count) {
    // Calculate icon position, possibly in animation
    int angle;
    if (pos < view.alt_layer_count - 2) {
      // Icons move up one place
      angle = 300 - pos * 20 - (ANIMATION_NORMALIZED_MAX - view.switcher_animation_progress) * 20 / ANIMATION_NORMALIZED_MAX;
    } else {
      // Last icon in the queue makes a long circle
      angle = 300 - pos * 20 - (ANIMATION_NORMALIZED_MAX - view.switcher_animation_progress) * (240 - pos * 20) / ANIMATION_NORMALIZED_MAX;  
    }
    #if defined(PBL_RECT)
    GPoint icon_point = icon_point = get_point_on_rect(DEG_TO_TRIGANGLE(angle - 90), inner_rect);
    #elif defined(PBL_ROUND)
    GPoint icon_point = icon_point = gpoint_from_polar(inner_rect, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(angle));
    #endif
    
    // Draw circle
    graphics_context_set_fill_color(ctx, config->color_secondary);
    graphics_fill_circle(ctx, icon_point, 10);
    
    // Draw icon
    char* icon = view.alt_layer_icons[i];      
    GSize icon_size = graphics_text_layout_get_content_size(icon, view.fonts.icons_small, bounds, GTextOverflowModeWordWrap, GTextAlignmentLeft);
    GRect icon_bounds = GRect(icon_point.x - icon_size.w / 2, icon_point.y - icon_size.h / 2, icon_size.w, icon_size.h);
    graphics_context_set_text_color(ctx, config->color_background);
    graphics_draw_text(ctx, icon, view.fonts.icons_small, icon_bounds, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  }
}

void alternating_layers_show(int index) {
  if (index < view.alt_layer_count) {
    // Hide the previously visible layer
    if (index != view.alt_layer_visible) {
      layer_remove_from_parent(view.alt_layers[view.alt_layer_visible]);
    }
    
    // Show this layer if there is a layer available
    if (index >= 0) {
      Layer *window_layer = window_get_root_layer(view.window);
      layer_add_child(window_layer, view.alt_layers[index]);
    }
    
    view.alt_layer_visible = index;
  }
}

void alternating_layers_show_layer(Layer* layer) {
  int index = 0;
  while(index < view.alt_layer_count && view.alt_layers[index] != layer) {
    ++index;
  }
  
  if (index < view.alt_layer_count) {
    // Show the layer
    alternating_layers_show(index);
  }
}

int alternating_layers_add(Layer* layer, char* icon) {
  // Retrieve and increase layer count
  int index = view.alt_layer_count++; 
  
  // Add the layer
  view.alt_layers[index] = layer;
  view.alt_layer_icons[index] = icon;
  
  // Immediately show it if it was the first available layer
  if (index == 0) {
    alternating_layers_show(index);
  }
  
  // Update switcher if active
  if (view.layers.switcher) layer_mark_dirty(view.layers.switcher);
  
  return index; 
}

void alternating_layers_remove(Layer* layer) {
  // Find the layer to remove
  int index = 0;
  while (index < view.alt_layer_count && view.alt_layers[index] != layer) {
    index++;
  }
  
  // Was layer found?
  if (index < view.alt_layer_count) {
    // Decrease layer count
    view.alt_layer_count--;   
    
    // Remove layer
    for (int i = index; i < view.alt_layer_count; ++i) {
      view.alt_layers[i] = view.alt_layers[i + 1];
      view.alt_layer_icons[i] = view.alt_layer_icons[i + 1];
    } 
    
    // Make sure the layer is no longer shown
    if (view.alt_layer_visible == index) {
      alternating_layers_show(index - 1);      
    } 
  } 
}

static void alert_timeout_handler(void *context) {
  // Clear the suspension
  view.suspension_reason &= ~SUSPENSION_ERROR_ALERT;
  
  // Show the layer hidden by the alert when not in the switcher
  if (!(view.suspension_reason & SUSPENSION_SWITCHER)) alternating_layers_show(view.suspension_return_layer);
  
  // Deregister timeout handler
  view.alert_timeout_handler = NULL;
    
  // Remove error layer when no longer in error or when errors are disabled
  if (model->error == ERROR_NONE || !config->enable_error) {
    alternating_layers_remove(view.layers.error);
    layer_destroy(view.layers.error);
    view.layers.error = NULL;
  }
}

void error_changed(enum ErrorCodes prevError) {
  // Check if error layer is allready visible
  if (view.layers.error == NULL) {
    // Create error layer
    Layer *window_layer = window_get_root_layer(view.window);
    GRect bounds = layer_get_bounds(window_layer);

    view.layers.error = layer_create(GRect(0, PBL_IF_ROUND_ELSE(34, 30), bounds.size.w, 60));
    layer_set_update_proc(view.layers.error, error_update_proc);
    alternating_layers_add(view.layers.error, ICON_BLUETOOTH);
  }
  
  // Set suspension
  if (!(view.suspension_reason & SUSPENSION_ERROR_ALERT)) {
    view.suspension_reason |= SUSPENSION_ERROR_ALERT;
    view.suspension_return_layer = view.alt_layer_visible;
  }
  
  // Show error layer
  alternating_layers_show_layer(view.layers.error);
  
  // Cancel previous timeout if there is one
  if (view.alert_timeout_handler) {
    app_timer_cancel(view.alert_timeout_handler);
  }
  
  if (model->error == ERROR_NONE && prevError != ERROR_CONNECTION) {
    // Stop suspending for alert
    alert_timeout_handler(NULL);
  } else {
    // Shedule timeout handler to stop suspending for alert
    view.alert_timeout_handler = app_timer_register(15000, alert_timeout_handler, NULL); // Show as an alert for 15 seconds
  }
}

void time_changed() {
  // Write the current time, weekday and date into buffers
  static char s_hour_buffer[3];
  strftime(s_hour_buffer, sizeof(s_hour_buffer), clock_is_24h_style() ? "%H" : "%I", model->time);
  static char s_minute_buffer[3];
  strftime(s_minute_buffer, sizeof(s_minute_buffer), "%M", model->time);
  static char s_weekday_buffer[10];
  strftime(s_weekday_buffer, sizeof(s_weekday_buffer), "%A", model->time);
  
  // Remove hours leading zero
  if (s_hour_buffer[0] == '0' && !config->date_hours_leading_zero) {
    s_hour_buffer[0] = s_hour_buffer[1];
    s_hour_buffer[1] = s_hour_buffer[2];
  }

  // Display time
  text_layer_set_text(view.layers.hour, s_hour_buffer);
  text_layer_set_text(view.layers.minute, s_minute_buffer);
  
  // Alternate every minute to the next available layer
  if (view.suspension_reason == SUSPENSION_NONE && view.alt_layer_count > 0 && config->alternate_mode != 'S') { 
    // Not on suspension, alternate layer
    alternating_layers_show((view.alt_layer_visible + 1) % view.alt_layer_count);
  }
}

void battery_changed() {
  if (view.layers.battery == NULL && model->battery_charge <= config->battery_show_from) {
    // Create battery layer
    Layer *window_layer = window_get_root_layer(view.window);
    GRect bounds = layer_get_bounds(window_layer);
    
    view.layers.battery = layer_create(GRect(0, PBL_IF_ROUND_ELSE(34, 30), bounds.size.w, 60));
    layer_set_update_proc(view.layers.battery, battery_update_proc);
    alternating_layers_add(view.layers.battery, icons_get_battery_symbol(70, false, false));
  } else if (view.layers.battery != NULL && model->battery_charge > config->battery_show_from) {
    // Destroy battery layer
    alternating_layers_remove(view.layers.battery);
    layer_destroy(view.layers.battery);
    view.layers.battery = NULL;
  }
}

#if defined(PBL_COMPASS)
void compass_changed() {
  if (view.layers.compass == NULL && config->enable_compass && (!config->compass_switcher_only || view.layers.switcher != NULL)) {    
    // Create compass layer
    Layer *window_layer = window_get_root_layer(view.window);
    GRect bounds = layer_get_bounds(window_layer);
    
    view.layers.compass = layer_create(GRect(0, 0, bounds.size.w, 60 +  PBL_IF_ROUND_ELSE(34, 30)));
    layer_set_update_proc(view.layers.compass, compass_update_proc);
    alternating_layers_add(view.layers.compass, icons_get_compass(0)); 
  } else if (view.layers.compass != NULL && (config->compass_switcher_only && view.layers.switcher == NULL)) {
    // Unsubscribe from compass events
    if (view.compass_subscribed) {
      compass_service_unsubscribe();
      view.compass_subscribed = false;
    }
    
    // Destroy compass layer
    alternating_layers_remove(view.layers.compass);
    layer_destroy(view.layers.compass);
    view.layers.compass = NULL;    
  }
}
#endif

void weather_changed() {
  if (view.layers.weather == NULL) {
    // Create weather layer
    Layer *window_layer = window_get_root_layer(view.window);
    GRect bounds = layer_get_bounds(window_layer);
    
    view.layers.weather = layer_create(GRect(0, PBL_IF_ROUND_ELSE(34, 30), bounds.size.w, 60));
    layer_set_update_proc(view.layers.weather, weather_update_proc);
    alternating_layers_add(view.layers.weather, icons_get_weather_condition_symbol(CONDITION_CLOUDY, true));
  }
}

void sunrise_sunset_changed() {
  if (view.layers.sunrise_sunset == NULL) {
    // Create sunrise layer
    Layer *window_layer = window_get_root_layer(view.window);
    GRect bounds = layer_get_bounds(window_layer);
    
    view.layers.sunrise_sunset = layer_create(GRect(0, PBL_IF_ROUND_ELSE(34, 30), bounds.size.w, 60));
    layer_set_update_proc(view.layers.sunrise_sunset, sunrise_sunset_update_proc);
    alternating_layers_add(view.layers.sunrise_sunset, ICON_SUNRISE);
  }
}

#if defined(PBL_HEALTH)
void activity_changed() {
  // Make sure the layer exists
  if (view.layers.health == NULL) {
    // Create health layer
    Layer *window_layer = window_get_root_layer(view.window);
    GRect bounds = layer_get_bounds(window_layer);
    
    view.layers.health = layer_create(GRect(0, 0, bounds.size.w, 60 + PBL_IF_ROUND_ELSE(34, 30)));
    layer_set_update_proc(view.layers.health, health_update_proc);
    alternating_layers_add(view.layers.health, ICON_WALK);
  }
  
  // Suspend alternating when walking, running or sleeping
  if (config->health_stick) {
      if (model->activity == ACTIVITY_NORMAL) {
      // Stop suspending alternating layers
      view.suspension_reason &= ~SUSPENSION_ACTIVITY;
    } else {
      // Suspend alternating layers and show health layer
      view.suspension_reason |= SUSPENSION_ACTIVITY;
      alternating_layers_show_layer(view.layers.health);
    }
  }
}
#endif

void moonphase_changed() {
  if (view.layers.moonphase == NULL && (!config->moonphase_night_only || !is_daytime())) {
    // Create moonphase layer
    Layer *window_layer = window_get_root_layer(view.window);
    GRect bounds = layer_get_bounds(window_layer);
    
    view.layers.moonphase = layer_create(GRect(0, PBL_IF_ROUND_ELSE(34, 30), bounds.size.w, 60));
    layer_set_update_proc(view.layers.moonphase, moonphase_update_proc);
    alternating_layers_add(view.layers.moonphase, icons_get_moonphase(120));
  } else if (view.layers.moonphase != NULL && (config->moonphase_night_only && is_daytime())) {
    // Destroy moonphase layer
    alternating_layers_remove(view.layers.moonphase);
    layer_destroy(view.layers.moonphase);
    view.layers.moonphase = NULL;
  }
}

void switcher_changed() {
  // Make sure the layer exists
  if (model->switcher && view.layers.switcher == NULL) {
    // Initialize switcher animation to finished
    view.switcher_animation_progress = ANIMATION_NORMALIZED_MAX;
    
    // Create switcher layer
    Layer *window_layer = window_get_root_layer(view.window);
    GRect bounds = layer_get_bounds(window_layer);

    view.layers.switcher = layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
    layer_set_update_proc(view.layers.switcher, switcher_update_proc);
    layer_add_child(window_layer, view.layers.switcher);
    
    // Create compass layer depending on configuration
    #if defined(PBL_COMPASS)
    compass_changed();
    #endif
    
    // Suspend alternating
    view.suspension_reason |= SUSPENSION_SWITCHER;
  } else if (!model->switcher && view.layers.switcher != NULL) {
    // Destroy the layer
    layer_destroy(view.layers.switcher);    
    view.layers.switcher = NULL;
    
    // Clear the suspension
    view.suspension_reason &= ~SUSPENSION_SWITCHER;
    
    // Destroy compass layer depending on configuration
    #if defined(PBL_COMPASS)
    compass_changed();
    #endif
    
    // Reactivate the health layer if on suspension
    #if defined(PBL_HEALTH)
    if (view.suspension_reason & SUSPENSION_ACTIVITY) alternating_layers_show_layer(view.layers.health);
    #endif
  }
}

static void switcher_animation_update(Animation *animation, const AnimationProgress progress) {
  view.switcher_animation_progress = progress;
  if (view.layers.switcher) layer_mark_dirty(view.layers.switcher);
}

static void switcher_animation_teardown(Animation *animation) {
  animation_destroy(animation);
  view.switcher_animation = NULL;
}

void tapped() {
  // Initialize switcher animation to start
  view.switcher_animation_progress = 0;
  
  // Tapped in the switcher, alternate to the next available layer
  if (view.alt_layer_count > 0) { 
    alternating_layers_show((view.alt_layer_visible + 1) % view.alt_layer_count);
  }
  
  // Animate the layer queue
  if (!view.switcher_animation) {
    view.switcher_animation = animation_create();
    animation_set_duration(view.switcher_animation, 250);
    static const AnimationImplementation implementation = {
      .update = switcher_animation_update,
      .teardown = switcher_animation_teardown
    };
    animation_set_implementation(view.switcher_animation, &implementation);
    animation_schedule(view.switcher_animation);
  }
}

void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create GFonts
  view.fonts.primary = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SPARKLER_54));
  view.fonts.secondary = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD); 
  view.fonts.icons = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ICONS_30));
  view.fonts.icons_small = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ICONS_16));
  view.fonts.icons_large = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ICONS_56));

  // Create and add the hour layer
  view.layers.hour = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w / 2 - 4, 54));
  text_layer_set_background_color(view.layers.hour, GColorClear);
  text_layer_set_text_color(view.layers.hour, config->color_primary);
  text_layer_set_text_alignment(view.layers.hour, GTextAlignmentRight);
  text_layer_set_font(view.layers.hour, view.fonts.primary);
  layer_add_child(window_layer, text_layer_get_layer(view.layers.hour));
  
  // Create and add the colon layer, I like my colon exactly in the middle
  view.layers.colon = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 54));
  text_layer_set_background_color(view.layers.colon, GColorClear);
  text_layer_set_text_color(view.layers.colon, config->color_accent);
  text_layer_set_text_alignment(view.layers.colon, GTextAlignmentCenter);
  text_layer_set_font(view.layers.colon, view.fonts.primary);
  text_layer_set_text(view.layers.colon, ":");
  layer_add_child(window_layer, text_layer_get_layer(view.layers.colon));
  
  // Create and add the minute layer
  view.layers.minute = text_layer_create(
      GRect(bounds.size.w / 2 + 4, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w / 2 - 4, 54));
  text_layer_set_background_color(view.layers.minute, GColorClear);
  text_layer_set_text_color(view.layers.minute, config->color_primary);
  text_layer_set_text_alignment(view.layers.minute, GTextAlignmentLeft);
  text_layer_set_font(view.layers.minute, view.fonts.primary);
  layer_add_child(window_layer, text_layer_get_layer(view.layers.minute));
  
  // Create and add the top date layer
  view.layers.date_top = layer_create(GRect(0, PBL_IF_ROUND_ELSE(112, 102), bounds.size.w, 30));
  LayerUpdateProc update_proc = config->date_format_top[0] == 'z' ? week_bar_monday_update_proc : config->date_format_top[0] == 'Z' ? week_bar_sunday_update_proc : date_top_update_proc;
  layer_set_update_proc(view.layers.date_top, update_proc);
  layer_add_child(window_layer, view.layers.date_top); 
    
  // Create and add the bottom date layer
  view.layers.date_bottom = layer_create(GRect(0, PBL_IF_ROUND_ELSE(132, 122), bounds.size.w, 30));
  update_proc = config->date_format_bottom[0] == 'z' ? week_bar_monday_update_proc : config->date_format_bottom[0] == 'Z' ? week_bar_sunday_update_proc : date_bottom_update_proc;
  layer_set_update_proc(view.layers.date_bottom, update_proc);
  layer_add_child(window_layer, view.layers.date_bottom);
  
  // Create the timezone layer
  if (config->enable_timezone) {    
    view.layers.timezone = layer_create(GRect(0, 0, bounds.size.w, 60 + PBL_IF_ROUND_ELSE(34, 30)));
    layer_set_update_proc(view.layers.timezone, timezone_update_proc);
    alternating_layers_add(view.layers.timezone, ICON_TIMEZONE);
  }
  
  // Create compass layer depending on configuration
  #if defined(PBL_COMPASS)
  compass_changed();
  #endif
  
  // Update time text
  time_changed();
}

void main_window_unload(Window *window) {
  // Destroy TextLayers
  text_layer_destroy(view.layers.hour);
  text_layer_destroy(view.layers.colon);
  text_layer_destroy(view.layers.minute);
  layer_destroy(view.layers.date_top);
  layer_destroy(view.layers.date_bottom);
  if (view.layers.battery) layer_destroy(view.layers.battery);
  #if defined(PBL_COMPASS)
  if (view.layers.compass) layer_destroy(view.layers.compass);
  #endif
  if (view.layers.error) layer_destroy(view.layers.error);
  if (view.layers.weather) layer_destroy(view.layers.weather);
  if (view.layers.sunrise_sunset) layer_destroy(view.layers.sunrise_sunset);
  if (view.layers.timezone) layer_destroy(view.layers.timezone);
  #if defined(PBL_HEALTH)
  if (view.layers.health) layer_destroy(view.layers.health);
  #endif
  if (view.layers.moonphase) layer_destroy(view.layers.moonphase);
  if (view.layers.switcher) layer_destroy(view.layers.switcher);
  
  // Unload custom fonts
  fonts_unload_custom_font(view.fonts.primary);
  fonts_unload_custom_font(view.fonts.icons);
  fonts_unload_custom_font(view.fonts.icons_small);
  fonts_unload_custom_font(view.fonts.icons_large);
  
  // Unsubscribe from compass events
  if (view.compass_subscribed) compass_service_unsubscribe();
}

void view_init() { 
  // Reset internals (especially after config update)
  struct View emptyView = { 0 };
  view = emptyView;
  
  // Create main Window element and assign to pointer
  view.window = window_create();
  window_set_background_color(view.window, config->color_background);

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(view.window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch
  window_stack_push(view.window, true);
  
  // Attach to model events
  model->events.on_error_change = error_changed;
  model->events.on_time_change = time_changed;
  if (config->alternate_mode != 'M') {
    model->events.on_switcher_change = switcher_changed;
    model->events.on_tap = tapped;
  }
  if (config->enable_battery) model->events.on_battery_change = battery_changed;
  if (config->enable_weather) model->events.on_weather_temperature_change = weather_changed;
  if (config->enable_weather) model->events.on_weather_condition_change = weather_changed;
  if (config->enable_sun) model->events.on_sunrise_change = sunrise_sunset_changed;
  if (config->enable_sun) model->events.on_sunset_change = sunrise_sunset_changed;
  #if defined(PBL_HEALTH)
  if (config->enable_health) model->events.on_activity_change = activity_changed;
  #endif
  if (config->enable_moonphase) model->events.on_moonphase_change = moonphase_changed;
  
  // Initialize some layers
  if (model->error != ERROR_NONE) error_changed(ERROR_NONE);
  if (config->enable_battery) battery_changed();
  #if defined(PBL_HEALTH)
  if (config->enable_health) activity_changed();
  #endif
}

void view_deinit() {  
  // Unregister from model events
  model_reset_events();
  
  // Hide window
  window_stack_pop(true);
  
  // Destroy Window
  window_destroy(view.window);
}
