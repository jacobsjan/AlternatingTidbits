#include <pebble.h>

#include "model.h"

//IDs from http://openweathermap.org/weather-conditions
//Icons from http://erikflowers.github.io/weather-icons/
char* icons_get_weather_condition_symbol(enum WeatherCondition code, bool is_day) {
  switch (code) {
    case CONDITION_CLEAR:
      return is_day ? "\uf00d" : "\uf02e"; // wi-day-sunny : wi-night-clear
    case CONDITION_CLOUDY:
      return "\uf041"; // wi-cloudy 
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
      return "\uf013"; //wi-cloud
    default:
      return "\uf03e"; //wi-cloud-refresh
  }
}

char* icons_get_battery_symbol(uint8_t charge, bool charging, bool plugged) {
  if (plugged) {
    if(charge >= 70) {
      return "\uf0aa";
    } else if(charge >= 60) {
      return "\uf0b0";
    } else if(charge >= 50) {
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

#if defined(PBL_COMPASS)
char* icons_get_compass(int degrees) {
  int heading = ((degrees * 2 + 22) / 45) % 16;
  switch(heading) {
    case  1: return "\uf0b2";
    case  2: return "\uf0b3";
    case  3: return "\uf0b4";
    case  4: return "\uf0b5";
    case  5: return "\uf0b6";
    case  6: return "\uf0b7";
    case  7: return "\uf0b8";
    case  8: return "\uf0b9";
    case  9: return "\uf0ba";
    case 10: return "\uf0bb";
    case 11: return "\uf0bc";
    case 12: return "\uf0bd";
    case 13: return "\uf0be";
    case 14: return "\uf0bf";
    case 15: return "\uf0c0";
    default:
    case  0: return "\uf0b1";
  }
}
#endif

char* icons_get_moonphase(int degrees) {
  int phase = degrees * 28 / 360;
  switch(phase) {
    case  1: return "\uf0c5";
    case  2: return "\uf0c6";
    case  3: return "\uf0c7";
    case  4: return "\uf0c8";
    case  5: return "\uf0c9";
    case  6: return "\uf0ca";
    case  7: return "\uf0cb";
    case  8: return "\uf0cc";
    case  9: return "\uf0cd";
    case 10: return "\uf0ce";
    case 11: return "\uf0cf";
    case 12: return "\uf0d0";
    case 13: return "\uf0d1";
    case 14: return "\uf0d2";
    case 15: return "\uf0d3";
    case 16: return "\uf0d4";
    case 17: return "\uf0d5";
    case 18: return "\uf0d6";
    case 19: return "\uf0d7";
    case 20: return "\uf0d8";
    case 21: return "\uf0d9";
    case 22: return "\uf0da";
    case 23: return "\uf0db";
    case 24: return "\uf0dc";
    case 25: return "\uf0dd";
    case 26: return "\uf0de";
    case 27: return "\uf0df";
    default:
    case  0: return "\uf0c4";
  }
}