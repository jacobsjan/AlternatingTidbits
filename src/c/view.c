#include <pebble.h>
#include "view.h"
#include "config.h"
#include "model.h"
#include "icons.h"
#include "utils.h"
#include "messagequeue.h"

void evaluate_moonphase_req();

enum SuspensionReasons {
  SUSPENSION_NONE = 0,
  SUSPENSION_ERROR_ALERT = 1,
  SUSPENSION_ACTIVITY = 2,
  SUSPENSION_SWITCHER = 4,
};

#define FIREWORKS_NUM 3
#ifdef PBL_PLATFORM_APLITE
#define FIREWORKS_POINTS 100
#endif
#ifndef PBL_PLATFORM_APLITE
#define FIREWORKS_POINTS 300
#endif
#define FIREWORKS_FRAMES 256
#define FIREWORKS_TOTAL_DURATION 60
#define FIREWORKS_DURATION ((2000 * ANIMATION_NORMALIZED_MAX) / (FIREWORKS_TOTAL_DURATION * 1000))

struct Layers {
  // Always shown layers
  TextLayer *hour;
  TextLayer *colon;
  TextLayer *minute;
  Layer *date_top;
  Layer *date_bottom;
  
  // Full screen overlays
  Layer *switcher;
  Layer *fireworks;
  
  // Alternating layers, only one visible at a time
  Layer *timezone;
  Layer *altitude;
  Layer *battery;
  #if defined(PBL_COMPASS)
  Layer *compass;
  #endif
  Layer *countdown;
  Layer *error;
  Layer *happy;
  #if defined(PBL_HEALTH)
  Layer *health;
  #endif
  Layer *moonphase;
  Layer *sunrise_sunset;
  Layer *weather;
};

struct Fonts {
  GFont primary;
  GFont secondary;
  GFont icons;
  GFont icons_small;
  GFont icons_large;
};

struct Fireworks {  
  int origin_x[FIREWORKS_NUM];
  int origin_y[FIREWORKS_NUM];
  int starts[FIREWORKS_NUM];
  int sizes[FIREWORKS_NUM];
  bool vibrated[FIREWORKS_NUM];
  #if defined(PBL_COLOR)
  GColor colors1[FIREWORKS_NUM];
  GColor colors2[FIREWORKS_NUM];
  GColor colors3[FIREWORKS_NUM];
  #endif
  short vels_x[FIREWORKS_POINTS];
  short vels_y[FIREWORKS_POINTS];
  short accel_factors[FIREWORKS_FRAMES];
  Animation* animation;
  AnimationProgress animation_progress;
};

struct View {
  Window *window;
  struct Layers layers;
  struct Fonts fonts;
  
  // Alternating layer info
  struct Layer *alt_layers[(sizeof(struct Layers) / sizeof(Layer*)) - 7]; // Very nasty trick to avoid forgetting to increase the array size
  char* alt_layer_icons[(sizeof(struct Layers) / sizeof(Layer*)) - 7];
  int alt_layer_count;
  int alt_layer_visible;
  
  // Suspension info
  enum SuspensionReasons suspension_reason;
  int suspension_return_layer;
  
  // Alert info
  AppTimer *alert_timeout_handler;
  
  // Switcher 
  AppTimer* switcher_timeout_timer;
  Animation* switcher_animation;
  AnimationProgress switcher_animation_progress;
  
  // Fireworks
  struct Fireworks* fireworks;
  time_t fireworks_time;
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

void draw_multi_centered(Layer *layer, GContext *ctx, char* icon, GColor icon_color, int top_text_count, char *top_texts[], int middle_text_count, char *middle_texts[], int bottom_text_count, char *bottom_texts[]) {
  // Calculate center alignment
  GRect bounds = layer_get_bounds(layer);
  GSize icon_size = graphics_text_layout_get_content_size(icon, view.fonts.icons, bounds, GTextOverflowModeWordWrap, GTextAlignmentRight);
  GSize bottom_text_size = calculate_total_size(bounds, bottom_text_count, bottom_texts);
  GSize middle_text_size = calculate_total_size(bounds, middle_text_count, middle_texts);
  GSize top_text_size = calculate_total_size(bounds, top_text_count, top_texts);
  int max_texts_width = bottom_text_size.w > middle_text_size.w ? bottom_text_size.w : middle_text_size.w;
  max_texts_width = top_text_size.w > max_texts_width ? top_text_size.w : max_texts_width;
  int total_width = icon_size.w + max_texts_width;
  int icon_left = bounds.origin.x + (bounds.size.w - total_width) / 2;
  int text_left = icon_left + icon_size.w;
  
  // Draw the icon
  GRect draw_bounds = GRect(icon_left, bounds.origin.y + PBL_IF_ROUND_ELSE(34, 28), icon_size.w, icon_size.h);
  graphics_context_set_text_color(ctx, icon_color);
  graphics_draw_text(ctx, icon, view.fonts.icons, draw_bounds, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  
  // Draw the top text
  draw_bounds = GRect(text_left, bounds.origin.y + PBL_IF_ROUND_ELSE(40, 34) - 40, top_text_size.w, top_text_size.h);
  draw_alternating_text(ctx, draw_bounds, top_text_count, top_texts);
  
  // Draw the middle text
  draw_bounds = GRect(text_left, bounds.origin.y + PBL_IF_ROUND_ELSE(40, 34) - 20, middle_text_size.w, middle_text_size.h);
  draw_alternating_text(ctx, draw_bounds, middle_text_count, middle_texts);
    
  // Draw the bottom text
  draw_bounds = GRect(text_left, bounds.origin.y + PBL_IF_ROUND_ELSE(40, 34), bottom_text_size.w, bottom_text_size.h);
  draw_alternating_text(ctx, draw_bounds, bottom_text_count, bottom_texts);
}

void draw_centered(Layer *layer, GContext *ctx, char* icon, GColor icon_color, int text_count, char *texts[]) { 
  draw_multi_centered(layer, ctx, icon, icon_color, 0, NULL, 0, NULL, text_count, texts);
}

void timezone_update_proc(Layer *layer, GContext *ctx) {
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
  
  // Draw
  draw_multi_centered(layer, ctx, ICON_TIMEZONE, config->color_secondary, 0, NULL, middle_text_count, middle_texts, bottom_text_count, bottom_texts);
}

void altitude_update_proc(Layer *layer, GContext *ctx) { 
  static char before_point[4];
  static char after_point[4];
  char **texts;
  
  int metric = model->altitude;
  if (config->altitude_unit == 'I') metric = metric * 3048 / 1000;
  if (metric < 1000) {
    snprintf(before_point, sizeof(before_point), "%d", metric);
    if (config->altitude_unit == 'M') {
      static char *temp[] = { before_point,  "m", NULL, NULL };
      texts = temp;
    } else {
      static char *temp[] = { before_point,  "ft", NULL, NULL };
      texts = temp;
    }
  } else {
    snprintf(before_point, sizeof(before_point), "%d", metric / 1000);
    snprintf(after_point, sizeof(after_point), "%03d", metric % 1000);
    if (config->altitude_unit == 'M') {
      if (config->health_number_format == 'M') {
        static char *temp[] = { before_point, ",", after_point, "m" };
        texts = temp;
      } else {
        static char *temp[] = { before_point, ".", after_point, "m" };
        texts = temp;
      }
    } else {
      if (config->health_number_format == 'M') {
        static char *temp[] = { before_point, ",", after_point, "ft" };
        texts = temp;
      } else {
        static char *temp[] = { before_point, ".", after_point, "ft" };
        texts = temp;
      }
    }
  }  
  
  // Draw
  draw_centered(layer, ctx, ICON_ALTITUDE, config->color_secondary, 4, texts);
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
    strncpy(charge, "< 40", sizeof(charge));
  } else {
    snprintf(charge, sizeof(charge) / sizeof(charge[0]), "%d", battery_charge);
  }
  
  // Draw
  char *texts[] = { charge, "%" };
  GColor battery_color = battery_charge <= config->battery_accent_from ? config->color_accent : config->color_secondary;
  draw_centered(layer, ctx, battery_icon, battery_color, sizeof(texts) / sizeof(texts[0]), texts);
}

#if defined(PBL_COMPASS)
static void compass_update_proc(Layer *layer, GContext *ctx) {    
  // Subscribe to compass heading updates, unsubscibe in compass_heading_changed
  if (!(model->update_req & UPDATE_COMPASS)) {
    model_add_update_req(UPDATE_COMPASS);
  }
  
  // Decide on compass icon
  char* icon;
  if (model->compass_heading.compass_status != CompassStatusDataInvalid) {
    int degrees = TRIGANGLE_TO_DEG(model->compass_heading.magnetic_heading);
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

static bool is_leap (int yr) {
  return yr % 400 == 0 || (yr % 4 == 0 && yr % 100 != 0);
}

int days_per_month(int month, int year) {
  if (month == 1) {
    if (is_leap(year)) {
      return 29;
    } else {
      return 28;
    }
  } else {
    static const int months[] = { 31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    return months[month];
  }
}

static int months_to_days (int month) {
  return (month * 3057 - 3007) / 100;
}

static int years_to_days (int yr) {
  return yr * 365 + yr / 4 - yr / 100 + yr / 400;
}

static long ymd_to_days (int yr, int mo, int day) {
  long scalar;

  scalar = day + months_to_days(mo);
  if (mo > 2) /* adjust if past February */
    scalar -= is_leap(yr) ? 1 : 2;
  yr--;
  scalar += years_to_days(yr);
  return scalar;
}

static void countdown_update_proc(Layer *layer, GContext *ctx) {
  // As iOS javascript seems to disagree with Pebble on DST for further away dates
  // and mktime seems to completely fail on the Pebble we're doing all countdown
  // calculations in local time.
  
  // Convert target and now to tm formats
  time_t now = config->countdown_to == 'D' ? time_start_of_today() : (time(NULL) / SECONDS_PER_MINUTE) * SECONDS_PER_MINUTE;
  struct tm now_tm = *localtime(&now);
  struct tm target_tm;
  if (config->countdown_to != 'E') {
    target_tm.tm_year = config->countdown_date / 10000 - 1900;
    target_tm.tm_mon = (config->countdown_date % 10000) / 100 - 1;
    target_tm.tm_mday = config->countdown_date % 100;
  } else {
    target_tm.tm_year = now_tm.tm_year;
    target_tm.tm_mon = now_tm.tm_mon;
    target_tm.tm_mday = now_tm.tm_mday;
  }
  target_tm.tm_hour = config->countdown_time / 100;
  target_tm.tm_min = config->countdown_time % 100;
  target_tm.tm_sec = 0;
  
  // Convert target and now to comparable formats
  long target_comp = config->countdown_date * 10000L + config->countdown_time;
  long now_comp = now_tm.tm_year + 1900;
  now_comp = now_comp * 100L + now_tm.tm_mon + 1;
  now_comp = now_comp * 100L + now_tm.tm_mday;
  now_comp = now_comp * 100L + now_tm.tm_hour;
  now_comp = now_comp * 100L + now_tm.tm_min;
  
  // Choose icon, pre- and post-text, max_tm and min_tm
  char *countdown_icon;
  char *pre_text;
  char *post_text;
  struct tm max_tm, min_tm;
  if (target_comp > now_comp) {
    // Count down to
    pre_text = "in ";
    post_text = NULL;    
    if (config->countdown_to == 'D') {
      countdown_icon = ICON_COUNTDOWN_TO_DATE; 
    } else  {
      countdown_icon = ICON_COUNTDOWN_TO_TIME;
    }
    max_tm = target_tm;
    min_tm = now_tm;
  } else {
    // Count down to
    pre_text = NULL;
    post_text = "ago";
    if (config->countdown_to == 'D') {
      countdown_icon = ICON_COUNTDOWN_FROM_DATE;
    } else {
      countdown_icon = ICON_COUNTDOWN_FROM_TIME;
    } 
    max_tm = now_tm;
    min_tm = target_tm;
  }
  
  // Calculate differences in years, months, days, hours and minutes
  int received = min_tm.tm_min > max_tm.tm_min ? 60 : 0;
  int min_diff = max_tm.tm_min + received - min_tm.tm_min;
  int borrow = received > 0 ? 1 : 0;
  received = min_tm.tm_hour + borrow > max_tm.tm_hour ? 24 : 0;
  int hour_diff = max_tm.tm_hour + received - min_tm.tm_hour - borrow;
  borrow = received > 0 ? 1 : 0;
  received = min_tm.tm_mday + borrow > max_tm.tm_mday ? days_per_month((max_tm.tm_mon - 1 + 12) % 12, max_tm.tm_year - (max_tm.tm_mon == 0 ? 1 : 0)) : 0;
  int day_diff = max_tm.tm_mday + received - min_tm.tm_mday - borrow;
  borrow = received > 0 ? 1 : 0;
  received = min_tm.tm_mon + borrow > max_tm.tm_mon ? 12 : 0;
  int month_diff = max_tm.tm_mon + received - min_tm.tm_mon - borrow;
  borrow = received > 0 ? 1 : 0;
  int year_diff = max_tm.tm_year - min_tm.tm_year - borrow;
  
  // Calculate total difference in seconds
  long diff = ymd_to_days(max_tm.tm_year + 1900, max_tm.tm_mon + 1, max_tm.tm_mday) - ymd_to_days(min_tm.tm_year + 1900, min_tm.tm_mon + 1, min_tm.tm_mday);
  if (min_tm.tm_hour * 100 + min_tm.tm_min > max_tm.tm_hour * 100 + max_tm.tm_min) diff -= 1;
  diff = diff * 24 + hour_diff;
  diff = diff * 60 + min_diff;
  diff = diff * 60;
  
  // Display results
  char *middle_texts[] = { config->countdown_label }; 
  if (diff == 0 || (config->countdown_to == 'D' && diff == SECONDS_PER_DAY)) {
    // Display one word
    char* normal_text;
    char* accent_text;
    
    if (diff == 0) {
      normal_text = NULL;
      if (config->countdown_to == 'D') {
        accent_text = "Today";
      } else {
        accent_text = "Now";
      }
    } else {
      accent_text = NULL;
      if (target_comp > now_comp) {
        normal_text = "Tomorrow";
      } else {
        normal_text = "Yesterday";
      }
    }
    
    // Draw
    char *bottom_texts[] = { normal_text, accent_text }; 
    draw_multi_centered(layer, ctx, countdown_icon, config->color_secondary, 0, NULL, sizeof(middle_texts) / sizeof(middle_texts[0]), middle_texts, sizeof(bottom_texts) / sizeof(bottom_texts[0]), bottom_texts);
  } else if (config->countdown_display == 'F') {
    // Display full detail
    char year_count[3] = { 0 }, month_count[3] = { 0 }, day_count[3] = { 0 }, hour_count[3] = { 0 }, minute_count[3] = { 0 };
    char *year_label = NULL, *month_label = NULL, *day_label = NULL, *hour_label = NULL, *minute_label = NULL;
    
    if (year_diff > 0) {
      snprintf(year_count, sizeof(year_count), "%d", year_diff);
      year_label = "y ";
    } 
    if (month_diff > 0) {
      snprintf(month_count, sizeof(month_count), "%d", month_diff);
      month_label = "m ";
    }
    if (day_diff > 0) {
      snprintf(day_count, sizeof(day_count), "%d", day_diff);
      day_label = "d ";
    }
    if (hour_diff > 0) {
      snprintf(hour_count, sizeof(hour_count), "%d", hour_diff);
      hour_label = "h ";
    }
    if (min_diff > 0) {
      snprintf(minute_count, sizeof(minute_count), "%d", min_diff);
      minute_label = "m ";
    }
    
    // Draw
    char *bottom_texts[] = { pre_text, NULL, year_count, year_label, month_count, month_label, day_count, day_label, hour_count, hour_label, minute_count, minute_label, post_text }; 
    draw_multi_centered(layer, ctx, countdown_icon, config->color_secondary, 0, NULL, sizeof(middle_texts) / sizeof(middle_texts[0]), middle_texts, sizeof(bottom_texts) / sizeof(bottom_texts[0]), bottom_texts);
  } else {
    // Display incremental detail
    int count;
    char* unit;
    char count_text[6];
    if (year_diff > 3 ) {
      count = year_diff;
      unit = " years ";
    } else if (month_diff > 3) {
      count = month_diff;
      unit = " months ";
    } else if (diff > 3 * 7 * SECONDS_PER_DAY) {
      count = diff / (7 * SECONDS_PER_DAY);
      unit = " weeks ";
    } else if (diff > 3 * SECONDS_PER_DAY || config->countdown_to == 'D') {
      count = diff / SECONDS_PER_DAY;
      unit = " days ";
    } else if (diff > 3 * SECONDS_PER_HOUR) {
      count = diff / SECONDS_PER_HOUR;
      unit = " hours ";
    } else if (diff > SECONDS_PER_MINUTE) {
      count = diff / SECONDS_PER_MINUTE;
      unit = " minutes ";
    } else {
      count = 1;
      unit = " minute ";   
    }    
    snprintf(count_text, sizeof(count_text), "%d", count);    
  
    // Draw
    char *bottom_texts[] = { pre_text, count_text, unit, NULL, post_text };
    draw_multi_centered(layer, ctx, countdown_icon, config->color_secondary, 0, NULL, sizeof(middle_texts) / sizeof(middle_texts[0]), middle_texts, sizeof(bottom_texts) / sizeof(bottom_texts[0]), bottom_texts);
  }
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

void happy_update_proc(Layer *layer, GContext *ctx) { 
  // Determine text to be shown
  char year[5];
  strftime(year, sizeof(year), "%Y", model->time);
  
  // Draw
  char *middle_texts[] = { "Happy" };
  char *bottom_texts[] = { NULL, year, "!" };
  draw_multi_centered(layer, ctx, ICON_HAPPY, config->color_secondary, 0, 0, sizeof(middle_texts) / sizeof(middle_texts[0]), middle_texts, sizeof(bottom_texts) / sizeof(bottom_texts[0]), bottom_texts);
}

void happy_countdown_update_proc(Layer *layer, GContext *ctx) { 
  // Determine text to be shown
  time_t diff = view.fireworks_time - time(NULL);
  char count[3];
  snprintf(count, sizeof(count), "%d", (int)diff);
  
  // Determine text position
  GRect bounds = layer_get_bounds(layer);
  GSize text_size = graphics_text_layout_get_content_size(count, view.fonts.primary, bounds, GTextOverflowModeWordWrap, GTextAlignmentLeft);
  int text_left = bounds.origin.x + (bounds.size.w - text_size.w) / 2;
  int text_top = bounds.origin.y + (bounds.size.h - text_size.h) / 2;
  
  // Clear the background
  graphics_context_set_fill_color(ctx, config->color_background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  
  // Draw the text
  GRect draw_bounds = GRect(text_left, text_top, text_size.w, text_size.h);
  graphics_context_set_text_color(ctx, config->color_primary);
  graphics_draw_text(ctx, count, view.fonts.primary, draw_bounds, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);  
}

void initiate_fireworks(GRect bounds, int n, int start_base) {
  // Origin
  int cx = (rand() % (bounds.size.w / 2)) + (bounds.size.w / 4);
  int cy = (rand() % (bounds.size.h / 2)) + (bounds.size.h / 4);
  view.fireworks->origin_x[n] = cx * (1 << 16); // Convert to fixed point
  view.fireworks->origin_y[n] = cy * (1 << 16);

  // Start
  view.fireworks->starts[n] = start_base + rand() % (FIREWORKS_DURATION / 2);
  
  // Size
  view.fireworks->sizes[n] = 75 + rand() % 75;
  
  // Has been vibrated for
  view.fireworks->vibrated[n] = false;

  // Color
  #if defined(PBL_COLOR)
  switch (rand() % 6) {
    case 0:
      view.fireworks->colors1[n] = GColorPastelYellow;
      view.fireworks->colors2[n] = GColorYellow;
      view.fireworks->colors3[n] = GColorChromeYellow;
      break;
    case 1: 
      view.fireworks->colors1[n] = GColorMelon;
      view.fireworks->colors2[n] = GColorRed;
      view.fireworks->colors3[n] = GColorDarkCandyAppleRed;
      break;    
    case 2:
      view.fireworks->colors1[n] = GColorBabyBlueEyes;
      view.fireworks->colors2[n] = GColorBlue;
      view.fireworks->colors3[n] = GColorDukeBlue;
      break;    
    case 3:
      view.fireworks->colors1[n] = GColorMintGreen;
      view.fireworks->colors2[n] = GColorGreen;
      view.fireworks->colors3[n] = GColorIslamicGreen;
      break;    
    case 4:
      view.fireworks->colors1[n] = GColorCeleste;
      view.fireworks->colors2[n] = GColorCyan;
      view.fireworks->colors3[n] = GColorTiffanyBlue;
      break;    
    case 5:
    default:
      view.fireworks->colors1[n] = GColorRichBrilliantLavender;  
      view.fireworks->colors2[n] = GColorMagenta;  
      view.fireworks->colors3[n] = GColorPurple;   
  }
  #endif 
}

void fireworks_update_proc(Layer *layer, GContext *ctx) { 
  GRect bounds = layer_get_bounds(layer);  
  #if defined(PBL_BW)
  graphics_context_set_stroke_color(ctx, config->color_primary);
  #endif 
  
  for (int n = 0; n < FIREWORKS_NUM; ++n) {
    if (view.fireworks->animation_progress > view.fireworks->starts[n] + FIREWORKS_DURATION) {
      // Make sure the animation is not almost over
      if (view.fireworks->animation_progress < ANIMATION_NORMALIZED_MAX - FIREWORKS_DURATION) {
        // Choose new start for this firework
        initiate_fireworks(bounds, n, view.fireworks->animation_progress);
      }
    } else if (view.fireworks->animation_progress > view.fireworks->starts[n]) {
      // Show fireworks
      int frame = ((view.fireworks->animation_progress - view.fireworks->starts[n]) * FIREWORKS_FRAMES) / FIREWORKS_DURATION;
      if (frame >= FIREWORKS_FRAMES) frame = FIREWORKS_FRAMES - 1;
      int accel_factor = view.fireworks->accel_factors[frame];
      int fading = frame - (FIREWORKS_FRAMES / 2); // Start fading from the second half of the fireworks
         
      // Quick method for converting fixed point back to shorts
      union { 
        int32_t i;
        struct {
          uint16_t l;
          int16_t h;
        } s;
      } pos_x, pos_y;
      
      for (int i = 0; i < FIREWORKS_POINTS; ++i) {
        if (fading <= 0 || i % (FIREWORKS_FRAMES / 2) > fading) {                
          pos_x.i = view.fireworks->origin_x[n] + view.fireworks->sizes[n] * (frame * view.fireworks->vels_x[i] - (accel_factor * view.fireworks->vels_x[i]) / FIREWORKS_FRAMES) / 100;
          pos_y.i = view.fireworks->origin_y[n] + view.fireworks->sizes[n] * (frame * view.fireworks->vels_y[i] - (accel_factor * (view.fireworks->vels_y[i] - 50000)) / FIREWORKS_FRAMES) / 100; // - Gravity
    
          // Draw the spark
          GPoint p = GPoint(pos_x.s.h, pos_y.s.h);
          #if defined(PBL_COLOR)
          int x_dir = (view.fireworks->vels_x[i] < 0) ? 1 : -1;
          graphics_context_set_stroke_color(ctx, view.fireworks->colors3[n]);
          graphics_draw_pixel(ctx, p);
          graphics_context_set_stroke_color(ctx, view.fireworks->colors2[n]);
          graphics_draw_pixel(ctx, GPoint(p.x + x_dir, p.y));
          graphics_draw_pixel(ctx, GPoint(p.x, p.y - 1));
          graphics_context_set_stroke_color(ctx, view.fireworks->colors1[n]);
          graphics_draw_pixel(ctx, GPoint(p.x + x_dir, p.y - 1));
          #endif
          #if defined(PBL_BW)
          graphics_draw_pixel(ctx, p);
          graphics_draw_pixel(ctx, GPoint(p.x + 1, p.y));
          graphics_draw_pixel(ctx, GPoint(p.x, p.y + 1));
          graphics_draw_pixel(ctx, GPoint(p.x + 1, p.y + 1));
          #endif
        }
      }        
                
      // Vibrate 
      if (!view.fireworks->vibrated[n] && !should_keep_quiet()) {
        static const uint32_t const segments[] = { 50 };
        VibePattern pat = {
          .durations = segments,
          .num_segments = sizeof(segments) / sizeof(segments[0]),
        };
        vibes_enqueue_custom_pattern(pat);
        view.fireworks->vibrated[n] = true;
      }
    }
  }
}

static void fireworks_animation_update(Animation *animation, const AnimationProgress progress) {
  if (view.fireworks) view.fireworks->animation_progress = progress;
  if (view.layers.fireworks) layer_mark_dirty(view.layers.fireworks);
}

static void fireworks_animation_teardown(Animation *animation) {
  if (view.fireworks) {
    // Destroy animation
    if (view.fireworks->animation)
    { 
      animation_unschedule(view.fireworks->animation);
      animation_destroy(view.fireworks->animation);
      view.fireworks->animation = NULL;
    }
  
    // Free fireworks memory
    free(view.fireworks);
    view.fireworks = NULL;
  }
    
  // Destroy layer  
  if (view.layers.fireworks) {
    layer_remove_from_parent(view.layers.fireworks);
    layer_destroy(view.layers.fireworks);    
    view.layers.fireworks = NULL;
  }
}

void set_of_fireworks(int number_of_seconds) {
  // Based on EXPLOD.C by (C) 1989 Dennis Lo
  // Allocate fireworks memory
  view.fireworks = (struct Fireworks*)malloc(sizeof(struct Fireworks));
  if (!view.fireworks) return;
  
  // Initialize individual fireworks
  Layer *window_layer = window_get_root_layer(view.window);
  GRect bounds = layer_get_bounds(window_layer);
  for (int n = 0; n < FIREWORKS_NUM; ++n) {
    initiate_fireworks(bounds, n, 0);
  }
  
  // Initialize fireworks spark scatering
  GSize size = GSize(bounds.size.w / 4, bounds.size.h / 4);
  for (int i = 0; i < FIREWORKS_POINTS; i++) 
  {  
    // Randomly select a destination that is inside the ellipse with
    // X and Y radii of (Xsize, Ysize).
    int dest_x, dest_y;
    do 
    {
      dest_x = (rand() % (2 * size.w)) - size.w;
      dest_y = (rand() % (2 * size.h)) - size.h;
    } while (size.h * size.h * dest_x * dest_x + 
             size.w * size.w * dest_y * dest_y
             > size.h * size.h * size.w * size.w);
    
    // Convert to fixed pt. Can't use shifts because they are unsigned
    dest_x *= 1 << 16;
    dest_y *= 1 << 16;

    // accel = 2 * distance / #steps^2   (#steps is equivalent to time)
    // vel = accel * #steps 
    view.fireworks->vels_x[i] = (2 * dest_x) / FIREWORKS_FRAMES;
    view.fireworks->vels_y[i] = (2 * dest_y) / FIREWORKS_FRAMES; 
  }
  
  // Initialize the fireworks acceleration lookups
  if (view.fireworks->accel_factors[FIREWORKS_FRAMES - 1] == 0) {
    view.fireworks->accel_factors[0] = 0;
    for (int i = 1; i < FIREWORKS_FRAMES; ++i) {
      view.fireworks->accel_factors[i] = view.fireworks->accel_factors[i - 1] + i;
    }
  }
  
  // Create layer
  if (!view.layers.fireworks) {
    view.layers.fireworks = layer_create(bounds);
    layer_add_child(window_layer, view.layers.fireworks);
  }
  
  // Set the update_proc for fireworks layer, possibly switching from showing countdown
  layer_set_update_proc(view.layers.fireworks, fireworks_update_proc);
  
  // Animate the fireworks
  if (!view.fireworks->animation) {
    view.fireworks->animation_progress = 0;
    view.fireworks->animation = animation_create();
    animation_set_duration(view.fireworks->animation, number_of_seconds * 1000); 
    animation_set_curve(view.fireworks->animation, AnimationCurveLinear);
    static const AnimationImplementation implementation = {
      .update = fireworks_animation_update,
      .teardown = fireworks_animation_teardown
    };
    animation_set_implementation(view.fireworks->animation, &implementation);
    animation_schedule(view.fireworks->animation);
  } 
  
  // Send fireworks to analytics 
  message_queue_send_tuplet(TupletInteger(MESSAGE_KEY_Fireworks, 0)); 
}

#if defined(PBL_HEALTH)
char* alloc_print_d(char* fmt, int i) {
  int size = snprintf(NULL, 0, fmt, i);
  char* result = (char*)malloc((size + 1) * sizeof(char));
  if (result) snprintf(result, size + 1, fmt, i);
  return result;
}

char* alloc_print_s(char* s) {
  int size = strlen(s);
  char* result = (char*)malloc((size + 1) * sizeof(char));
  if (result) strncpy(result, s, size + 1);
  return result;
}

char** health_generate_texts(enum HealthIndicator indicator) {
  char** result = (char**)malloc(4 * sizeof(char*));
  if (!result) return NULL;
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
    case HEALTH_CLIMB_DESCEND:
      metric = model->activity_climb;
      if (config->altitude_unit == 'I') metric = metric * 3048 / 1000;
      result[0] = alloc_print_d("%d", metric);
      result[1] = alloc_print_s("|");
      metric = model->activity_descend;
      if (config->altitude_unit == 'I') metric = metric * 3048 / 1000;
      result[2] = alloc_print_d("%d", metric);
      result[3] = alloc_print_s(config->altitude_unit == 'M' ? "m" : "ft");
      break;
  }
  return result;
}

void health_update_proc(Layer *layer, GContext *ctx) {
  char* health_icon;  
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
  
  // Draw
  draw_multi_centered(layer, ctx, health_icon, config->color_secondary, top_text_count, top_texts, middle_text_count, middle_texts, bottom_text_count, bottom_texts);
  
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
      if (normalText) {
        char strfmt[] = "%x";
        strfmt[1] = element;
        strftime(normalText, 20, strfmt, model->time);
        
        if ((format[i] == 'n' && normalText[0] == '0') || (element == 'e' && normalText[0] == ' ')) {
          // Remove leading 0 of month or leading space of day
          normalText[0] = normalText[1];
          normalText[1] = normalText[2];
        }
      }
    } else {
      // Separartor element
      normalText = NULL;
      accentText = (char*)malloc(2 * sizeof(char));
      if (accentText) {
        accentText[0] = element;
        accentText[1] = 0;
      }
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
      GRect draw_bounds = GRect(day_left + 1, bounds.origin.y + day_size.h + 8, day_size.w - space_size.w - 2, 3);
      graphics_fill_rect(ctx, draw_bounds, 0, GCornerNone);
    }
    
    // Draw text
    GRect draw_bounds = GRect(day_left, bounds.origin.y + 6, day_size.w, day_size.h);
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
    graphics_context_set_stroke_color(ctx, config->color_background);
    graphics_draw_circle(ctx, icon_point, 11);
    
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
    // Make sure the layer is no longer shown
    if (view.alt_layer_visible == index) {
      if (view.alt_layer_count > 1) {
        alternating_layers_show((index + 1) % view.alt_layer_count);  
      } else {
        alternating_layers_show(-1);
      }
    } 
    
    // Decrease layer count
    view.alt_layer_count--;   
    
    // Remove layer
    for (int i = index; i < view.alt_layer_count; ++i) {
      view.alt_layers[i] = view.alt_layers[i + 1];
      view.alt_layer_icons[i] = view.alt_layer_icons[i + 1];
    } 
    
    // Decrease visible layer index
    if (view.alt_layer_visible >= index) {
      view.alt_layer_visible--;
    }
    
    // Update switcher if active
    if (view.layers.switcher) layer_mark_dirty(view.layers.switcher);
  } 
}

Layer* alternating_layers_create(LayerUpdateProc update_proc, char* icon) {
  Layer *window_layer = window_get_root_layer(view.window);
  GRect bounds = layer_get_bounds(window_layer);

  Layer *layer = layer_create(GRect(0, 0, bounds.size.w, PBL_IF_ROUND_ELSE(68, 62)));
  layer_set_update_proc(layer, update_proc);
  alternating_layers_add(layer, icon);
  
  return layer;
}

Layer* alternating_layers_destroy(Layer* layer) {
  alternating_layers_remove(layer);
  layer_destroy(layer);
  return NULL;
}

void evaluate_happiness() {
  if (config->enable_happy) {
    time_t diff = view.fireworks_time - time(NULL);
    
    // Check layer
    if (!view.layers.happy) {
      // The happy layer does not yet exist, create it if needed
      if (diff <= 0 && diff > -SECONDS_PER_DAY) {
        view.layers.happy = alternating_layers_create(happy_update_proc, ICON_HAPPY);
        alternating_layers_show_layer(view.layers.happy);
      } 
    } else {
      // Remove the happy layer if needed if needed
      if (diff > 0 || diff <= -SECONDS_PER_DAY) {
        alternating_layers_remove(view.layers.happy);
        view.layers.happy = NULL;
      } 
    }
    
    // Check countdown & fireworks
    if (diff <= SECONDS_PER_MINUTE && diff > 0) {
      // Less than a minute to go till fireworks
      // Ask for ticks every second 
      model_add_update_req(UPDATE_SECOND_TICKS);
      
      if (diff <= 10) {
        // Create layer if necessary
        if (!view.layers.fireworks) {
          Layer *window_layer = window_get_root_layer(view.window);
          GRect bounds = layer_get_bounds(window_layer);
          view.layers.fireworks = layer_create(bounds);
          layer_set_update_proc(view.layers.fireworks, happy_countdown_update_proc);
          layer_add_child(window_layer, view.layers.fireworks);
        }
        
        // Vibrate 
        if (!should_keep_quiet()) {
          static uint32_t segments[] = { 200 };
          VibePattern pat = {
            .durations = segments,
            .num_segments = sizeof(segments) / sizeof(segments[0]),
          };
          vibes_enqueue_custom_pattern(pat);
        }
        
        // Update countdown
        layer_mark_dirty(view.layers.fireworks);
      }
    } else if (diff <= 0 && diff > -FIREWORKS_TOTAL_DURATION){
      // Reset ticks to every minute
      model_remove_update_req(UPDATE_SECOND_TICKS);
      
      // Show fireworks 
      set_of_fireworks(FIREWORKS_TOTAL_DURATION - diff);
    }
  }
}

void time_changed(TimeUnits units_changed) {
  if (units_changed & MINUTE_UNIT) {
    // Write the current time, weekday and date into buffers
    static char s_hour_buffer[3];
    strftime(s_hour_buffer, sizeof(s_hour_buffer), clock_is_24h_style() ? "%H" : "%I", model->time);
    static char s_minute_buffer[3];
    strftime(s_minute_buffer, sizeof(s_minute_buffer), "%M", model->time);
    
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
    
    // Check visibility of moonphase based on day or night time
    if (config->enable_moonphase) evaluate_moonphase_req();
  }
  
  // Check whether a happy layer or fireworks should be shown
  // We evaluate this possibly every second right before and during countdown
  evaluate_happiness();
}

void evaluate_altitude_req() {
  if (config->enable_altitude 
#if defined(PBL_HEALTH)
      || (model->activity == ACTIVITY_WALK && (config->health_walk_top == HEALTH_CLIMB_DESCEND || config->health_walk_middle == HEALTH_CLIMB_DESCEND || config->health_walk_bottom == HEALTH_CLIMB_DESCEND)) 
      || (model->activity == ACTIVITY_RUN && (config->health_run_top == HEALTH_CLIMB_DESCEND || config->health_run_middle == HEALTH_CLIMB_DESCEND || config->health_run_bottom == HEALTH_CLIMB_DESCEND)) 
#endif
     ) {
    model_add_update_req(UPDATE_ALTITUDE);
  } else {
    model_remove_update_req(UPDATE_ALTITUDE);
  }
}

void altitude_changed() {
  if (view.layers.altitude == NULL) {
    // Create altitude layer
    view.layers.altitude = alternating_layers_create(altitude_update_proc, ICON_ALTITUDE);
  }   
}

void battery_changed() {
  if (view.layers.battery == NULL && model->battery_charge <= config->battery_show_from) {
    // Create battery layer
    view.layers.battery = alternating_layers_create(battery_update_proc, icons_get_battery_symbol(70, false, false));
  } else if (view.layers.battery != NULL && model->battery_charge > config->battery_show_from) {
    // Destroy battery layer
    view.layers.battery = alternating_layers_destroy(view.layers.battery);
  }
}

#if defined(PBL_COMPASS)
void compass_heading_changed() {
  // Update layer or unsubscribe if it's no longer visible
  if (view.layers.compass == NULL || view.alt_layers[view.alt_layer_visible] != view.layers.compass) {
    // Unsubscribe from compass events, subscibe in compass_update_proc
    model_remove_update_req(UPDATE_COMPASS);
  } else {
    // Redraw the compass
    layer_mark_dirty(view.layers.compass);
  }
}

void compass_enable_changed() {
  if (view.layers.compass == NULL && config->enable_compass && (!config->compass_switcher_only || view.layers.switcher != NULL)) {    
    // Create compass layer
    view.layers.compass = alternating_layers_create(compass_update_proc, icons_get_compass(0)); 
  } else if (view.layers.compass != NULL && (config->compass_switcher_only && view.layers.switcher == NULL)) {
    // Unsubscribe from compass events, subsbribe in compass_update_proc
    model_remove_update_req(UPDATE_COMPASS);
    
    // Destroy compass layer
    view.layers.compass = alternating_layers_destroy(view.layers.compass);   
  }
}
#endif

void weather_changed() {
  if (view.layers.weather == NULL) {
    // Create weather layer
    view.layers.weather = alternating_layers_create(weather_update_proc, icons_get_weather_condition_symbol(CONDITION_CLOUDY, true));
  }
}

void sunrise_sunset_changed() {
  if (view.layers.sunrise_sunset == NULL) {
    // Create sunrise layer
    view.layers.sunrise_sunset = alternating_layers_create(sunrise_sunset_update_proc, ICON_SUNRISE);
  }
}

#if defined(PBL_HEALTH)
void activity_changed() {
  // Make sure the layer exists
  if (view.layers.health == NULL) {
    // Create health layer
    view.layers.health = alternating_layers_create(health_update_proc, ICON_WALK);
  }
  
  // Suspend alternating when walking, running or sleeping
  if (config->health_stick) {
    if (model->activity == ACTIVITY_NORMAL) {
      // Stop suspending alternating layers
      view.suspension_reason &= ~SUSPENSION_ACTIVITY;
    } else {
      // Suspend alternating layers and show health layer
      view.suspension_reason |= SUSPENSION_ACTIVITY;
      if (!view.layers.switcher) alternating_layers_show_layer(view.layers.health);
    }
  }
  
  // Enable/disable altitude polling
  evaluate_altitude_req();
}
#endif

void evaluate_moonphase_req() {
  if (!config->moonphase_night_only || !is_daytime()) {
    model_add_update_req(UPDATE_MOONPHASE);
  } else {
    model_remove_update_req(UPDATE_MOONPHASE);
    
    if (view.layers.moonphase) {
      // Destroy moonphase layer
      view.layers.moonphase = alternating_layers_destroy(view.layers.moonphase);
    }
  }
}

void moonphase_changed() {
  if (view.layers.moonphase == NULL) {
    // Create moonphase layer
    view.layers.moonphase = alternating_layers_create(moonphase_update_proc, icons_get_moonphase(120));
  } 
}

static void switcher_animation_update(Animation *animation, const AnimationProgress progress) {
  view.switcher_animation_progress = progress;
  if (view.layers.switcher) layer_mark_dirty(view.layers.switcher);
}

static void switcher_animation_teardown(Animation *animation) {
  if (view.switcher_animation)
  { 
    animation_unschedule(view.switcher_animation);
    animation_destroy(view.switcher_animation);
    view.switcher_animation = NULL;
  }
}

void switcher_timeout_callback(void *data) {
  // Stop animations
  switcher_animation_teardown(NULL);
  
  // Destroy the layer
  if (view.layers.switcher) {
    layer_remove_from_parent(view.layers.switcher);
    layer_destroy(view.layers.switcher);    
    view.layers.switcher = NULL;
  }

  // Clear the suspension
  view.suspension_reason &= ~SUSPENSION_SWITCHER;
    
  // Unubscribe from tap events 
  model_remove_update_req(UPDATE_TAPS);
  
  // Reset timeout
  view.switcher_timeout_timer = NULL;

  // Destroy compass layer depending on configuration
  #if defined(PBL_COMPASS)
  compass_enable_changed();
  #endif

  // Reactivate the health layer if on suspension
  #if defined(PBL_HEALTH)
  if (view.suspension_reason & SUSPENSION_ACTIVITY) alternating_layers_show_layer(view.layers.health);
  #endif
}

void flicked() {
  // Make sure the switcher layer exists
  if (view.layers.switcher == NULL) {
    // Initialize switcher animation to finished
    view.switcher_animation_progress = ANIMATION_NORMALIZED_MAX;
    
    // Create switcher layer
    Layer *window_layer = window_get_root_layer(view.window);
    GRect bounds = layer_get_bounds(window_layer);
    view.layers.switcher = layer_create(bounds);
    layer_set_update_proc(view.layers.switcher, switcher_update_proc);
    layer_add_child(window_layer, view.layers.switcher);
    
    // Suspend alternating
    view.suspension_reason |= SUSPENSION_SWITCHER;
    
    // Subscribe to tap events
    model_add_update_req(UPDATE_TAPS);
    
    // Deactivate switcher after 30sec
    if (view.switcher_timeout_timer) app_timer_cancel(view.switcher_timeout_timer);
    view.switcher_timeout_timer = app_timer_register(30000, switcher_timeout_callback, NULL);
    
    // Create compass layer depending on configuration
    #if defined(PBL_COMPASS)
    compass_enable_changed();
    #endif
  } 
}

void tapped() {
  if (view.layers.switcher) {
    // Initialize switcher animation to start
    if (config->switcher_animate) view.switcher_animation_progress = 0;
    
    // Alternate to the next available layer
    if (view.alt_layer_count > 0) { 
      alternating_layers_show((view.alt_layer_visible + 1) % view.alt_layer_count);
    }
    
    // Animate the layer queue
    if (config->switcher_animate && !view.switcher_animation) {
      view.switcher_animation = animation_create();
      animation_set_duration(view.switcher_animation, 250);
      static const AnimationImplementation implementation = {
        .update = switcher_animation_update,
        .teardown = switcher_animation_teardown
      };
      animation_set_implementation(view.switcher_animation, &implementation);
      animation_schedule(view.switcher_animation);
    }
  
    // Prolong switcher
    if (view.switcher_timeout_timer) app_timer_reschedule(view.switcher_timeout_timer, 30000);
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
    view.layers.error = alternating_layers_destroy(view.layers.error);
  }
}

void error_changed(enum ErrorCodes prevError) {
  // Check if error layer is allready visible
  if (view.layers.error == NULL) {
    // Create error layer
    view.layers.error = alternating_layers_create(error_update_proc, ICON_BLUETOOTH);
  }
  
  // Set suspension
  if (!(view.suspension_reason & SUSPENSION_ERROR_ALERT)) {
    view.suspension_reason |= SUSPENSION_ERROR_ALERT;
    view.suspension_return_layer = view.alt_layer_visible;
  }
  
  // Stop switcher on vibration overload to avoid battery draining
  if (model->error == ERROR_VIBRATION_OVERLOAD) {
    if (view.switcher_timeout_timer) app_timer_cancel(view.switcher_timeout_timer);
    switcher_timeout_callback(NULL);
  }
  
  // Show error layer
  if (!view.layers.switcher) alternating_layers_show_layer(view.layers.error);
  
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
      GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w / 2 - 4, 60));
  text_layer_set_background_color(view.layers.hour, GColorClear);
  text_layer_set_text_color(view.layers.hour, config->color_primary);
  text_layer_set_text_alignment(view.layers.hour, GTextAlignmentRight);
  text_layer_set_font(view.layers.hour, view.fonts.primary);
  layer_add_child(window_layer, text_layer_get_layer(view.layers.hour));
  
  // Create and add the colon layer, I like my colon exactly in the middle
  view.layers.colon = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 60));
  text_layer_set_background_color(view.layers.colon, GColorClear);
  text_layer_set_text_color(view.layers.colon, config->color_accent);
  text_layer_set_text_alignment(view.layers.colon, GTextAlignmentCenter);
  text_layer_set_font(view.layers.colon, view.fonts.primary);
  text_layer_set_text(view.layers.colon, ":");
  layer_add_child(window_layer, text_layer_get_layer(view.layers.colon));
  
  // Create and add the minute layer
  view.layers.minute = text_layer_create(
      GRect(bounds.size.w / 2 + 4, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w / 2 - 4, 60));
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
}

void main_window_unload(Window *window) {
  // Destroy base layers
  text_layer_destroy(view.layers.hour);
  text_layer_destroy(view.layers.colon);
  text_layer_destroy(view.layers.minute);
  layer_destroy(view.layers.date_top);
  layer_destroy(view.layers.date_bottom);
  
  // Destroy alternating layers
  if (view.layers.battery) layer_destroy(view.layers.battery);
  if (view.layers.altitude) layer_destroy(view.layers.altitude);
  #if defined(PBL_COMPASS)
  if (view.layers.compass) layer_destroy(view.layers.compass);
  #endif
  if (view.layers.countdown) layer_destroy(view.layers.countdown);
  if (view.layers.error) layer_destroy(view.layers.error);
  if (view.layers.weather) layer_destroy(view.layers.weather);
  if (view.layers.sunrise_sunset) layer_destroy(view.layers.sunrise_sunset);
  if (view.layers.timezone) layer_destroy(view.layers.timezone);
  if (view.layers.happy) layer_destroy(view.layers.happy);
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
}

void view_init() { 
  // Reset internals (especially after config update)
  memset(&view, 0, sizeof(view));
    
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
  model->events.on_error_change = &error_changed;
  model->events.on_time_change = &time_changed;
  if (config->alternate_mode != 'M') {
    model->events.on_flick = &flicked;
    model->events.on_tap = &tapped;
    model_add_update_req(UPDATE_FLICKS);
  }
  if (config->enable_altitude) {
    model->events.on_altitude_change = &altitude_changed;
    evaluate_altitude_req();
  }
  if (config->enable_battery) {
    model->events.on_battery_change = &battery_changed;
    model_add_update_req(UPDATE_BATTERY);
  }
  #if defined(PBL_COMPASS)
  model->events.on_compass_heading_change = &compass_heading_changed; 
  compass_enable_changed();
  #endif
  if (config->enable_weather) {
    model->events.on_weather_temperature_change = &weather_changed;
    model->events.on_weather_condition_change = &weather_changed;
    model_add_update_req(UPDATE_WEATHER);
  }
  if (config->enable_sun) {
    model->events.on_sunrise_change = &sunrise_sunset_changed;
    model->events.on_sunset_change = &sunrise_sunset_changed;
    model_add_update_req(UPDATE_SUN);
  }
  #if defined(PBL_HEALTH)
  if (config->enable_health) {
    model->events.on_activity_change = &activity_changed;
    model_add_update_req(UPDATE_HEALTH);
  }
  #endif
  if (config->enable_moonphase) {
    model->events.on_moonphase_change = &moonphase_changed;
    evaluate_moonphase_req();
  }
  
  // Create non-model update based alternating layers
  if (config->enable_battery) battery_changed();
  if (config->enable_countdown) view.layers.countdown = alternating_layers_create(countdown_update_proc, ICON_COUNTDOWN_TO_TIME);
  if (model->error != ERROR_NONE) error_changed(ERROR_NONE);
  evaluate_happiness(); 
  #if defined(PBL_HEALTH)
  if (config->enable_health) activity_changed();
  #endif
  if (config->enable_timezone) view.layers.timezone = alternating_layers_create(timezone_update_proc, ICON_TIMEZONE);
  
  // Calculate time of next fireworks
  struct tm start_of_year = *model->time;/*
  start_of_year.tm_sec = 0;
  start_of_year.tm_min = 0;
  start_of_year.tm_hour = 0;
  start_of_year.tm_mday = 1;
  start_of_year.tm_mon = 0;
  start_of_year.tm_year += 1; // Next year*/
  
  start_of_year.tm_sec = 0;
  start_of_year.tm_min = 35;
  start_of_year.tm_hour = 23;
  
  view.fireworks_time = mktime(&start_of_year);
  
  // Update time text
  time_changed((TimeUnits)-1);
}

void view_deinit() {  
  // Stop animations
  switcher_animation_teardown(view.switcher_animation);
  fireworks_animation_teardown(NULL);
    
  // Stop timers
  if (view.alert_timeout_handler) app_timer_cancel(view.alert_timeout_handler);
  if (view.switcher_timeout_timer) app_timer_cancel(view.switcher_timeout_timer);
  
  // Unregister from model events
  model_remove_update_req(~0); // Remove all requirements
  model_reset_events();
  
  // Hide window
  window_stack_pop(true);
  
  // Destroy Window
  window_destroy(view.window);
}
