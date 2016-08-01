#include <pebble.h>

#include "model.h"

//IDs from http://openweathermap.org/weather-conditions
//Icons from http://erikflowers.github.io/weather-icons/
char* icons_get_weather_condition_symbol(enum WeatherCondition code, bool is_day) {
  switch (code) {
    case CONDITION_CLEAR:
      return is_day ? "\uf00d" : "\uf02e"; // wi-day-sunny : wi-night-clear
    case CONDITION_CLOUDY:
      return "\uf013"; // wi-cloudy 
    case CONDITION_FOG:
      return is_day ? "\uf003" : "\uf04a"; // wi-day-fog : wi-night-fog
    case CONDITION_LIGHT_RAIN:
      return "\uf01c"; // wi-sprinkle 
    case CONDITION_RAIN:
      return "\uf019"; // wi-rain 
    case CONDITION_THUNDERSTORM:
      return "\uf01e"; // wi-thunderstorm 
    case CONDITION_SNOW:
      return "\uf01b"; // wi-snow 
    case CONDITION_HAIL:
      return "\uf015"; // wi-hail 
    case CONDITION_WIND:
      return "\uf021"; //wi-windy
    case CONDITION_EXTREME_WIND:
      return "\uf050"; //wi-strong-wind
    case CONDITION_TORNADO:
      return "\uf056"; //wi-tornado
    case CONDITION_HURRICANE:
      return "\uf073"; //wi-hurricane
    case CONDITION_EXTREME_COLD:
      return "\uf076"; //wi-snowflake-cold
    case CONDITION_EXTREME_HEAT:
      return "\uf072"; //wi-hot
    case CONDITION_SNOW_THUNDERSTORM:
      return "\uf01d"; // wi-storm-showers 
    case CONDITION_LIGHT_CLOUDY:
      return is_day ? "\uf002" : "\uf086"; // wi-day-cloudy : wi-night-alt-cloudy
    case CONDITION_PARTLY_CLOUDY:
      return "\uf041"; //wi-cloud
    default:
      return "\uf03e"; //wi-cloud-refresh
  }
}

char* icons_get_battery_symbol(uint8_t charge, bool charging, bool plugged) {
  if (plugged) {
    if(charge >= 70) {
      return "\uf0aa";
    } else if(charge >= 50) {
      return "\uf0b0";
    } else if(charge >= 60) {
      return "\uf0af";
    } else if(charge >= 40) {
      return "\uf0ae";
    } else if(charge >= 20) {
      return "\uf0ad";
    } else if(charge >= 10) {
      return "\uf0ac";
    } else {
      return "\uf0ab";
    }
  } else {
    if(charge >= 100) {
      return "\uf09f";
    } else if(charge >= 90) {
      return "\uf0a8";
    } else if(charge >= 80) {
      return "\uf0a7";
    } else if(charge >= 70) {
      return "\uf0a6";
    } else if(charge >= 60) {
      return "\uf0a5";
    } else if(charge >= 50) {
      return "\uf0a4";
    } else if(charge >= 40) {
      return "\uf0a3";
    } else if(charge >= 30) {
      return "\uf0a2";
    } else if(charge >= 20) {
      return "\uf0a1";
    } else {
      return "\uf0a0";
    }
  }
}