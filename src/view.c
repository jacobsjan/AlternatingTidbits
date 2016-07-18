#include <pebble.h>
#include "view.h"
#include "config.h"
#include "model.h"
#include "icons.h"

enum SuspensionReasons {
  SUSPENSION_NONE = 0,
  SUSPENSION_ERROR_ALERT = 1,
  SUSPENSION_ACTIVITY = 2,
};

struct Layers {
  // Always shown layers
  TextLayer *hour;
  TextLayer *colon;
  TextLayer *minute;
  Layer *date_top;
  Layer *date_bottom;
  
  // Alternating layers, only one visible at a time
  Layer *error;
  Layer *weather;
  Layer *sunrise_sunset;
  Layer *battery;
  #if defined(PBL_HEALTH)
  Layer *health;
  #endif
};

struct Fonts {
  GFont time;
  GFont weekday;
  GFont icons;
};

struct Layers dummyLayers; // See nasty comment below
struct View {
  Window *window;
  struct Layers layers;
  struct Fonts fonts;
  
  // Alternating layer info
  struct Layer *alt_layers[sizeof(dummyLayers) / 4 - 5]; // Very nasty trick to avoid forgetting to increase the array size
  int alt_layer_count;
  int alt_layer_visible;
  
  // Suspension info
  enum SuspensionReasons suspension_reason;
  int suspension_return_layer;
  
  // Alert info
  AppTimer *alert_timeout_handler;
  enum ErrorCodes prev_error;
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
    GSize text_size = graphics_text_layout_get_content_size(texts[i], view.fonts.weekday, bounds, GTextOverflowModeWordWrap, GTextAlignmentLeft);
    result.w += text_size.w;
    result.h = text_size.h > result.h ? text_size.h : result.h;
  } 
  
  return result;
}

void draw_alternating_text(GContext *ctx, GRect bounds, int text_count, char *texts[]) {
  for (int i = 0; i < text_count; ++i) {
    // Draw the text
    graphics_context_set_text_color(ctx, i % 2 == 0 ? config->color_secondary : config->color_accent); // Alternate color
    graphics_draw_text(ctx, texts[i], view.fonts.weekday, bounds, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    
    // Shrink bounds to the right
    GSize text_size = graphics_text_layout_get_content_size(texts[i], view.fonts.weekday, bounds, GTextOverflowModeWordWrap, GTextAlignmentLeft);
    bounds.origin.x += text_size.w;
    bounds.size.w -= text_size.w;
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
  draw_bounds = GRect(text_left, bounds.origin.y + PBL_IF_ROUND_ELSE(12, 10), text_size.w, text_size.h);
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
      text = "Weather issue";
      break; 
    default:
      // Bleutooth ok once again alert
      symbol = ICON_BLUETOOTH;
      text = "Bluetooth ok";
      break; 
  }
  
  // Draw
  char *texts[] = { text };
  GColor symbol_color = model->error == ERROR_NONE ? config->color_secondary : config->color_accent;
  draw_centered(layer, ctx, symbol, symbol_color, sizeof(texts) / sizeof(texts[0]), texts);
}

void weather_update_proc(Layer *layer, GContext *ctx) {
  char* condition = icons_get_weather_condition_symbol(model->weather_condition, is_daytime());
  char temperature[6];
  snprintf(temperature, sizeof(temperature), "%d", model->weather_temperature);
  
  // Draw
  char *texts[] = { " ", "", temperature, "\u00b0"}; // Degree symbol
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
  char *texts[] = { " ", "", hour, ":", minute}; 
  draw_centered(layer, ctx, symbol, config->color_secondary, sizeof(texts) / sizeof(texts[0]), texts);
}

void battery_update_proc(Layer *layer, GContext *ctx) {
  char* battery_icon = icons_get_battery_symbol(model->battery_charge, model->battery_charging, model->battery_plugged);
  char charge[5];
  snprintf(charge, sizeof(charge), "%d", model->battery_charge);
  
  // Draw
  char *texts[] = { charge, "%" };
  GColor battery_color = model->battery_charge <= 30 ? config->color_accent : config->color_secondary;
  draw_centered(layer, ctx, battery_icon, battery_color, sizeof(texts) / sizeof(texts[0]), texts);
}

#if defined(PBL_HEALTH)
char** health_distance_texts() {
  static char before_point[5];
  static char after_point[2];
  
  if (model->activity_distance < 1000) {
    snprintf(before_point, sizeof(before_point), "%d", model->activity_distance);    
    static char* texts[] = { before_point, "m", "", "" };
    return texts;
  } else {
    snprintf(before_point, sizeof(before_point), "%d", (model->activity_distance + 50) / 1000);
    snprintf(after_point, sizeof(after_point), "%d", ((model->activity_distance + 50) % 1000) / 100);
    static char* texts[] = { before_point, ",", after_point, "km" };
    return texts;
  }  
}

char** health_activity_duration_texts() {
  static char before_point[4];  
  static char after_point[3];
  
  snprintf(before_point, sizeof(before_point), "%d", model->activity_duration / 60);
  snprintf(after_point, sizeof(after_point), "%02d", model->activity_duration % 60);
  static char* texts[] = { before_point, ":", after_point, "" };
  return texts;
}

char** health_activity_speed_texts() {
  static char before_point[4];  
  static char after_point[2];
  
  int m_per_hour = (model->activity_distance * 60) / model->activity_duration;
  snprintf(before_point, sizeof(before_point), "%d", (m_per_hour + 50) / 1000);
  snprintf(after_point, sizeof(after_point), "%d", ((m_per_hour + 50) % 1000) / 100);
  static char* texts[] = { before_point, ",", after_point, "km/h" };
  return texts;
}

char** health_total_steps_texts() {
  static char before_point[5];  
  static char after_point[4];
  
  if (model->activity_total_step_count < 1000) {
    snprintf(before_point, sizeof(before_point), "%d", model->activity_total_step_count);
    static char* texts[] = { before_point, "", "", "" };
    return texts;
  } else {
    snprintf(before_point, sizeof(before_point), "%d", model->activity_total_step_count / 1000);
    snprintf(after_point, sizeof(after_point), "%03d", model->activity_total_step_count % 1000);
    static char* texts[] = { before_point, ".", after_point, "" };
    return texts;
  }  
}

void health_update_proc(Layer *layer, GContext *ctx) {
  const int SEPARATOR = 4;
  const char* health_icon;
  
  char** bottom_texts;
  int bottom_text_count = 4;
  char** middle_texts = NULL;
  int middle_text_count = 0;
  char** top_texts = NULL;
  int top_text_count = 0;
  
  switch (model->activity) {    
    case ACTIVITY_WALK:
      health_icon = ICON_WALK;
      middle_texts = health_distance_texts();
      middle_text_count = 4;
      bottom_texts = health_total_steps_texts();
      break;
    
    case ACTIVITY_RUN:
      health_icon = ICON_RUN;
      top_texts = health_activity_duration_texts();
      top_text_count = 4;
      middle_texts = health_distance_texts();
      middle_text_count = 4;
      bottom_texts = health_activity_speed_texts();
      break;
    
    case ACTIVITY_SLEEP:
      health_icon = ICON_SLEEP;
      bottom_texts = health_activity_duration_texts();
      break;
    
    case ACTIVITY_CALM:
    default:
      health_icon = ICON_STEP;
      bottom_texts = health_total_steps_texts();
      break;
  }
  
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
  draw_bounds = GRect(text_left, bounds.origin.y + PBL_IF_ROUND_ELSE(12, 10) + PBL_IF_ROUND_ELSE(34, 30) - 40, top_text_size.w, top_text_size.h);
  draw_alternating_text(ctx, draw_bounds, top_text_count, top_texts);
  
  // Draw the middle text
  draw_bounds = GRect(text_left, bounds.origin.y + PBL_IF_ROUND_ELSE(12, 10) + PBL_IF_ROUND_ELSE(34, 30) - 20, middle_text_size.w, middle_text_size.h);
  draw_alternating_text(ctx, draw_bounds, middle_text_count, middle_texts);
    
  // Draw the bottom text
  draw_bounds = GRect(text_left, bounds.origin.y + PBL_IF_ROUND_ELSE(12, 10) + PBL_IF_ROUND_ELSE(34, 30), bottom_text_size.w, bottom_text_size.h);
  draw_alternating_text(ctx, draw_bounds, bottom_text_count, bottom_texts);
}
#endif

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
      normalText = (char*)malloc(12 * sizeof(char));
      accentText = (char*)malloc(1 * sizeof(char));
      char strfmt[] = "%x";
      strfmt[1] = element;
      strftime(normalText, 12, strfmt, model->time);
      
      if ((format[i] == 'n' && normalText[0] == '0') || (element == 'e' && normalText[0] == ' ')) {
        // Remove leading 0 of month or leading space of day
        normalText[0] = normalText[1];
        normalText[1] = normalText[2];
      }
      accentText[0] = 0;
    } else {
      // Separartor element
      normalText = (char*)malloc(1 * sizeof(char));
      accentText = (char*)malloc(2 * sizeof(char));
      normalText[0] = 0;
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
  char day_texts[5][7];
  GSize day_sizes[7];
  GFont day_fonts[7];
  
  GSize text_size = GSize(0, 0);
  for (struct tm day = { .tm_wday = 0 }; day.tm_wday <= 6; day.tm_wday++) {
    int i = day.tm_wday;
    strftime(day_texts[i], sizeof(day_texts[i]), "%a ", &day);
    day_texts[i][1] += 'A' - 'a'; // Capitalize seconds letter
    day_texts[i][2] = ' '; // Abreviate to two characters per day
    day_texts[i][3] = 0;
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

int alternating_layers_add(Layer* layer) {
  // Retrieve and increase layer count
  int index = view.alt_layer_count++; 
  
  // Add the layer
  view.alt_layers[index] = layer;
  
  // Immediately show it if it was the first available layer
  if (index == 0) {
    alternating_layers_show(index);
  }
  
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
    } 
    
    // Make sure the layer is no longer shown
    if (view.alt_layer_visible == index) {
      alternating_layers_show(index - 1);      
    } 
  } 
}

void remove_error_layer() {
  // Hide error layer
  alternating_layers_remove(view.layers.error);
  layer_destroy(view.layers.error);
  view.layers.error = NULL;
}

static void alert_timeout_handler(void *context) {
  // Clear the suspension
  view.suspension_reason &= ~SUSPENSION_ERROR_ALERT;
  
  // Show the layer hidden by the alert
  alternating_layers_show(view.suspension_return_layer);
  
  // Deregister timeout handler
  view.alert_timeout_handler = NULL;
}

static void alert_and_remove_timeout_handler(void *context) {
  // Remove suspension
  alert_timeout_handler(context);
  
  // Remove error layer
  remove_error_layer();
}

void alert_error(AppTimerCallback callback) {
  // Set suspension
  view.suspension_reason |= SUSPENSION_ERROR_ALERT;
  view.suspension_return_layer = view.alt_layer_visible;
  
  // Show error layer
  alternating_layers_show_layer(view.layers.error);
  
  // Cancel previous timeout if there is one
  if (view.alert_timeout_handler) {
    app_timer_cancel(view.alert_timeout_handler);
  }
  
  // Shedule timeout handler to stop suspending for alert
  view.alert_timeout_handler = app_timer_register(15000, callback, NULL); // Show as an alert for 15 seconds
}

void error_changed() {
  if (model->error != ERROR_NONE) {
    // Check if error layer is allready visible
    if (view.layers.error == NULL) {
      // Create error layer
      Layer *window_layer = window_get_root_layer(view.window);
      GRect bounds = layer_get_bounds(window_layer);

      view.layers.error = layer_create(GRect(0, PBL_IF_ROUND_ELSE(34, 30), bounds.size.w, 60));
      layer_set_update_proc(view.layers.error, error_update_proc);
      alternating_layers_add(view.layers.error);
    }

    // Suspend alternation and show error alert
    alert_error(alert_timeout_handler);
  } else if (view.prev_error == ERROR_CONNECTION) {
    // Connection error solved, show error layer and suspend alternation
    alert_error(alert_and_remove_timeout_handler);
  } else {
    // Don't remove layer when in suspension
    if (view.layers.error != NULL && !(view.suspension_reason & SUSPENSION_ERROR_ALERT)) {
      // Remove error layer
      remove_error_layer();
    }
  }
  
  view.prev_error = model->error;
}

void time_changed() {
  // Write the current time, weekday and date into buffers
  static char s_hour_buffer[3];
  strftime(s_hour_buffer, sizeof(s_hour_buffer), clock_is_24h_style() ? "%H" : "%I", model->time);
  static char s_minute_buffer[8];
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
  if (view.suspension_reason == SUSPENSION_NONE && view.alt_layer_count > 0) { 
    // Not on suspension, alternate layer
    alternating_layers_show((view.alt_layer_visible + 1) % view.alt_layer_count);
  }
}

void weather_changed() {
  if (view.layers.weather == NULL) {
    // Create weather layer
    Layer *window_layer = window_get_root_layer(view.window);
    GRect bounds = layer_get_bounds(window_layer);
    
    view.layers.weather = layer_create(GRect(0, PBL_IF_ROUND_ELSE(34, 30), bounds.size.w, 60));
    layer_set_update_proc(view.layers.weather, weather_update_proc);
    alternating_layers_add(view.layers.weather);
  }
}

void sunrise_sunset_changed() {
  if (view.layers.sunrise_sunset == NULL) {
    // Create sunrise layer
    Layer *window_layer = window_get_root_layer(view.window);
    GRect bounds = layer_get_bounds(window_layer);
    
    view.layers.sunrise_sunset = layer_create(GRect(0, PBL_IF_ROUND_ELSE(34, 30), bounds.size.w, 60));
    layer_set_update_proc(view.layers.sunrise_sunset, sunrise_sunset_update_proc);
    alternating_layers_add(view.layers.sunrise_sunset);
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
    alternating_layers_add(view.layers.health);
  }
  
  // Suspend alternating when walking, running or sleeping
  if (model->activity == ACTIVITY_CALM) {
    // Stop suspending alternating layers
    view.suspension_reason &= ~SUSPENSION_ACTIVITY;
  } else {
    // Suspend alternating layers and show health layer
    view.suspension_reason |= SUSPENSION_ACTIVITY;
    alternating_layers_show_layer(view.layers.health);
  }
}
#endif

void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create GFonts
  view.fonts.time = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SPARKLER_54));
  view.fonts.weekday = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_EXPRESSWAY_18));
  view.fonts.icons = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ICONS_30));

  // Create and add the hour layer
  view.layers.hour = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w / 2 - 4, 54));
  text_layer_set_background_color(view.layers.hour, GColorClear);
  text_layer_set_text_color(view.layers.hour, config->color_primary);
  text_layer_set_text_alignment(view.layers.hour, GTextAlignmentRight);
  text_layer_set_font(view.layers.hour, view.fonts.time);
  layer_add_child(window_layer, text_layer_get_layer(view.layers.hour));
  
  // Create and add the colon layer, I like my colon exactly in the middle
  view.layers.colon = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 54));
  text_layer_set_background_color(view.layers.colon, GColorClear);
  text_layer_set_text_color(view.layers.colon, config->color_accent);
  text_layer_set_text_alignment(view.layers.colon, GTextAlignmentCenter);
  text_layer_set_font(view.layers.colon, view.fonts.time);
  text_layer_set_text(view.layers.colon, ":");
  layer_add_child(window_layer, text_layer_get_layer(view.layers.colon));
  
  // Create and add the minute layer
  view.layers.minute = text_layer_create(
      GRect(bounds.size.w / 2 + 4, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w / 2 - 4, 54));
  text_layer_set_background_color(view.layers.minute, GColorClear);
  text_layer_set_text_color(view.layers.minute, config->color_primary);
  text_layer_set_text_alignment(view.layers.minute, GTextAlignmentLeft);
  text_layer_set_font(view.layers.minute, view.fonts.time);
  layer_add_child(window_layer, text_layer_get_layer(view.layers.minute));
  
  // Create and add the top date layer
  view.layers.date_top = layer_create(GRect(0, PBL_IF_ROUND_ELSE(118, 108), bounds.size.w, 24));
  LayerUpdateProc update_proc = config->date_format_top[0] == 'z' ? week_bar_monday_update_proc : config->date_format_top[0] == 'Z' ? week_bar_sunday_update_proc : date_top_update_proc;
  layer_set_update_proc(view.layers.date_top, update_proc);
  layer_add_child(window_layer, view.layers.date_top); 
    
  // Create and add the bottom date layer
  view.layers.date_bottom = layer_create(GRect(0, PBL_IF_ROUND_ELSE(136, 126), bounds.size.w, 24));
  update_proc = config->date_format_bottom[0] == 'z' ? week_bar_monday_update_proc : config->date_format_bottom[0] == 'Z' ? week_bar_sunday_update_proc : date_bottom_update_proc;
  layer_set_update_proc(view.layers.date_bottom, update_proc);
  layer_add_child(window_layer, view.layers.date_bottom);
  
  // Update time text
  time_changed();
  
  // Create battery layer
  view.layers.battery = layer_create(GRect(0, PBL_IF_ROUND_ELSE(34, 30), bounds.size.w, 60));
  layer_set_update_proc(view.layers.battery, battery_update_proc);
  alternating_layers_add(view.layers.battery);
}

void main_window_unload(Window *window) {
  // Destroy TextLayers
  text_layer_destroy(view.layers.hour);
  text_layer_destroy(view.layers.colon);
  text_layer_destroy(view.layers.minute);
  layer_destroy(view.layers.date_top);
  layer_destroy(view.layers.date_bottom);
  layer_destroy(view.layers.battery);
  if (view.layers.error) layer_destroy(view.layers.error);
  if (view.layers.weather) layer_destroy(view.layers.weather);
  if (view.layers.sunrise_sunset) layer_destroy(view.layers.sunrise_sunset);
  #if defined(PBL_HEALTH)
  if (view.layers.health) layer_destroy(view.layers.health);
  #endif
  
  // Unload GFonts
  fonts_unload_custom_font(view.fonts.time);
  fonts_unload_custom_font(view.fonts.weekday);
  fonts_unload_custom_font(view.fonts.icons);
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
  model->on_error_change = error_changed;
  model->on_time_change = time_changed;
  model->on_weather_temperature_change = weather_changed;
  model->on_weather_condition_change = weather_changed;
  model->on_sunrise_change = sunrise_sunset_changed;
  model->on_sunset_change = sunrise_sunset_changed;
  #if defined(PBL_HEALTH)
  model->on_activity_change = activity_changed;
  #endif
}

void view_deinit() {  
  // Hide window
  window_stack_pop(true);
  
  // Destroy Window
  window_destroy(view.window);
}
