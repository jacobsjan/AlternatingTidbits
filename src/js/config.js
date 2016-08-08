var constants = require('./constants');

module.exports = [
  {
    "type": "heading",
    "defaultValue": "General Settings"
  }, 
  { 
    "type": "text", 
    "defaultValue": "Press 'Save Settings' at the bottom to commit changes to your Pebble." 
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Colors"
      },
      {
        "type": "color",
        "messageKey": "cfgColorBackground",
        "defaultValue": "0xFFFFFF",
        "label": "Background",
        "sunlight": false
      },
      {
        "type": "color",
        "messageKey": "cfgColorPrimary",
        "defaultValue": "0x000000",
        "label": "Primary",
        "sunlight": false,
        "capabilities": ["COLOR"]
      },
      {
        "type": "color",
        "messageKey": "cfgColorPrimary",
        "defaultValue": "0x000000",
        "label": "Foreground",
        "sunlight": false,
        "capabilities": ["BW"]
      },
      {
        "type": "color",
        "messageKey": "cfgColorSecondary",
        "defaultValue": "0x555555",
        "label": "Secondary",
        "sunlight": false,
        "capabilities": ["COLOR"]
      },
      {
        "type": "color",
        "messageKey": "cfgColorAccent",
        "defaultValue": "0xAA0000",
        "label": "Accent",
        "sunlight": false,
        "capabilities": ["COLOR"]
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Date and Time Format"
      },
      {
        "type": "toggle",
        "messageKey": "cfgDateHoursLeadingZero",
        "label": "Hour has a leading zero",
        "description": "08:00 vs. 8:00",
        "defaultValue": false
      },
      {
        "type": "dateformat",
        "messageKey": "cfgDateFormatTop",
        "label": "Line 1",
        "defaultValue": "z"
      },
      {
        "type": "dateformat",
        "messageKey": "cfgDateFormatBottom",
        "label": "Line 2",
        "defaultValue": "B e"
      }
    ]
  },
  {
    "type": "heading",
    "defaultValue": "Alternating Tidbits"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Tidbits"
      },
      { 
        "type": "text", 
        "defaultValue": "Select the tidbits of info to alternate between every minute." 
      },
      {
        "type": "toggle",
        "messageKey": "cfgEnableTimezone",
        "label": "Alternative Time Zone",
        "defaultValue": false
      },
      {
        "type": "toggle",
        "messageKey": "cfgEnableBattery",
        "label": "Battery",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "messageKey": "cfgEnableError",
        "label": "Error Messages",
        "defaultValue": false
      },
      {
        "type": "toggle",
        "messageKey": "cfgEnableHealth",
        "label": "Health",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "messageKey": "cfgEnableSun",
        "label": "Sunset/Sunrise",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "messageKey": "cfgEnableWeather",
        "label": "Weather",
        "defaultValue": true
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Alternative Timezone"
      },
      {
        "type": "timezone",
        "messageKey": "cfgTimeZoneCity",
        "defaultValue": "Europe/London"
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Battery"
      },
      {
        "type": "slider",
        "messageKey": "cfgBatteryShowFrom",
        "defaultValue": 100,
        "label": "Show from or below",
        "min": 0,
        "max": 100,
        "step": 10
      },
      {
        "type": "slider",
        "messageKey": "cfgBatteryAccentFrom",
        "defaultValue": 30,
        "label": "Accent color from or below",
        "min": 0,
        "max": 100,
        "step": 10,
        "capabilities": ["COLOR"]
      },
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Error messages"
      },
      { 
        "type": "text", 
        "defaultValue": "Error messages are shown for 15 sec on the watch anyway but enabling this repeats them every few minutes.",
      }
    ]
  },
  {
    "type": "section",
    "capabilities": ["HEALTH"],
    "items": [
      {
        "type": "heading",
        "defaultValue": "Health"
      },
      {
        "type": "toggle",
        "messageKey": "cfgHealthStick",
        "label": "Stick during activity",
        "description": "Alternating will be suspended during walks/runs/sleep.",
        "defaultValue": true
      },
      {
        "type": "select",
        "messageKey": "cfgHealthUnits",
        "defaultValue": 'M',
        "label": "Distance units",
        "options": [
          { 
            "label": "(Kilo)meters", 
            "value": 'M' 
          },
          { 
            "label": "Miles", 
            "value": 'I' 
          }
        ]
      },
      {
        "type": "select",
        "messageKey": "cfgHealthNumbers",
        "label": "Number format",
        "defaultValue": 'M',
        "options": [
          { 
            "label": "1.000,00", 
            "value": 'M' 
          },
          { 
            "label": "1,000.00", 
            "value": 'I' 
          }
        ]
      },
      {
        "type": "heading",
        "defaultValue": "During normal activity",
        "size": "5"
      },
      {
        "type": "select",
        "messageKey": "cfgHealthNormalLine1",
        "defaultValue": constants.EMPTY,
        "label": "Line 1",
        "options": [
          { 
            "label": "(Empty)",
            "value": constants.EMPTY
          },
          { 
            "label": "Avg. calories till now", 
            "value": constants.AVG_CALORIES_TILL_NOW 
          },
          { 
            "label": "Avg. distance till now", 
            "value": constants.AVG_DISTANCE_TILL_NOW 
          },
          { 
            "label": "Avg. steps till now", 
            "value": constants.AVG_STEPS_TILL_NOW 
          },
          { 
            "label": "Avg. total calories", 
            "value": constants.AVG_TOTAL_CALORIES 
          },
          { 
            "label": "Avg. total distance", 
            "value": constants.AVG_TOTAL_DISTANCE 
          },
          { 
            "label": "Avg. total steps", 
            "value": constants.AVG_TOTAL_STEPS 
          },
          { 
            "label": "Total calories", 
            "value": constants.TODAY_CALORIES 
          },
          { 
            "label": "Total distance", 
            "value": constants.TODAY_DISTANCE 
          },
          { 
            "label": "Total steps", 
            "value": constants.TODAY_STEPS 
          }
        ]
      },
      {
        "type": "select",
        "messageKey": "cfgHealthNormalLine2",
        "defaultValue": constants.EMPTY,
        "label": "Line 2",
        "options": [
          { 
            "label": "(Empty)",
            "value": constants.EMPTY
          },
          { 
            "label": "Avg. calories till now", 
            "value": constants.AVG_CALORIES_TILL_NOW 
          },
          { 
            "label": "Avg. distance till now", 
            "value": constants.AVG_DISTANCE_TILL_NOW 
          },
          { 
            "label": "Avg. steps till now", 
            "value": constants.AVG_STEPS_TILL_NOW 
          },
          { 
            "label": "Avg. total calories", 
            "value": constants.AVG_TOTAL_CALORIES 
          },
          { 
            "label": "Avg. total distance", 
            "value": constants.AVG_TOTAL_DISTANCE 
          },
          { 
            "label": "Avg. total steps", 
            "value": constants.AVG_TOTAL_STEPS 
          },
          { 
            "label": "Total calories", 
            "value": constants.TODAY_CALORIES 
          },
          { 
            "label": "Total distance", 
            "value": constants.TODAY_DISTANCE 
          },
          { 
            "label": "Total steps", 
            "value": constants.TODAY_STEPS 
          }
        ]
      },
      {
        "type": "select",
        "messageKey": "cfgHealthNormalLine3",
        "defaultValue": constants.TODAY_STEPS,
        "label": "Line 3",
        "options": [
          { 
            "label": "(Empty)",
            "value": constants.EMPTY
          },
          { 
            "label": "Avg. calories till now", 
            "value": constants.AVG_CALORIES_TILL_NOW 
          },
          { 
            "label": "Avg. distance till now", 
            "value": constants.AVG_DISTANCE_TILL_NOW 
          },
          { 
            "label": "Avg. steps till now", 
            "value": constants.AVG_STEPS_TILL_NOW 
          },
          { 
            "label": "Avg. total calories", 
            "value": constants.AVG_TOTAL_CALORIES 
          },
          { 
            "label": "Avg. total distance", 
            "value": constants.AVG_TOTAL_DISTANCE 
          },
          { 
            "label": "Avg. total steps", 
            "value": constants.AVG_TOTAL_STEPS 
          },
          { 
            "label": "Total calories", 
            "value": constants.TODAY_CALORIES 
          },
          { 
            "label": "Total distance", 
            "value": constants.TODAY_DISTANCE 
          },
          { 
            "label": "Total steps", 
            "value": constants.TODAY_STEPS 
          }
        ]
      },
      {
        "type": "heading",
        "defaultValue": "During walks",
        "size": "5"
      },
      {
        "type": "select",
        "messageKey": "cfgHealthWalkLine1",
        "defaultValue": constants.EMPTY,
        "label": "Line 1",
        "options": [
          { 
            "label": "(Empty)",
            "value": constants.EMPTY
          },
          { 
            "label": "Avg. calories till now", 
            "value": constants.AVG_CALORIES_TILL_NOW 
          },
          { 
            "label": "Avg. distance till now", 
            "value": constants.AVG_DISTANCE_TILL_NOW 
          },
          { 
            "label": "Avg. steps till now", 
            "value": constants.AVG_STEPS_TILL_NOW 
          },
          { 
            "label": "Avg. total calories", 
            "value": constants.AVG_TOTAL_CALORIES 
          },
          { 
            "label": "Avg. total distance", 
            "value": constants.AVG_TOTAL_DISTANCE 
          },
          { 
            "label": "Avg. total steps", 
            "value": constants.AVG_TOTAL_STEPS 
          },
          { 
            "label": "Total calories", 
            "value": constants.TODAY_CALORIES 
          },
          { 
            "label": "Total distance", 
            "value": constants.TODAY_DISTANCE 
          },
          { 
            "label": "Total steps", 
            "value": constants.TODAY_STEPS 
          },
          { 
            "label": "Walk calories", 
            "value": constants.ACTIVITY_CALORIES 
          },
          { 
            "label": "Walk distance", 
            "value": constants.ACTIVITY_DISTANCE 
          },
          { 
            "label": "Walk duration", 
            "value": constants.ACTIVITY_DURATION 
          },
          { 
            "label": "Walk pace", 
            "value": constants.ACTIVITY_PACE 
          },
          { 
            "label": "Walk speed", 
            "value": constants.ACTIVITY_SPEED 
          },
          { 
            "label": "Walk steps", 
            "value": constants.ACTIVITY_STEPS 
          }
        ]
      },
      {
        "type": "select",
        "messageKey": "cfgHealthWalkLine2",
        "defaultValue": constants.ACTIVITY_DISTANCE,
        "label": "Line 2",
        "options": [
          { 
            "label": "(Empty)",
            "value": constants.EMPTY
          },
          { 
            "label": "Avg. calories till now", 
            "value": constants.AVG_CALORIES_TILL_NOW 
          },
          { 
            "label": "Avg. distance till now", 
            "value": constants.AVG_DISTANCE_TILL_NOW 
          },
          { 
            "label": "Avg. steps till now", 
            "value": constants.AVG_STEPS_TILL_NOW 
          },
          { 
            "label": "Avg. total calories", 
            "value": constants.AVG_TOTAL_CALORIES 
          },
          { 
            "label": "Avg. total distance", 
            "value": constants.AVG_TOTAL_DISTANCE 
          },
          { 
            "label": "Avg. total steps", 
            "value": constants.AVG_TOTAL_STEPS 
          },
          { 
            "label": "Total calories", 
            "value": constants.TODAY_CALORIES 
          },
          { 
            "label": "Total distance", 
            "value": constants.TODAY_DISTANCE 
          },
          { 
            "label": "Total steps", 
            "value": constants.TODAY_STEPS 
          },
          { 
            "label": "Walk calories", 
            "value": constants.ACTIVITY_CALORIES 
          },
          { 
            "label": "Walk distance", 
            "value": constants.ACTIVITY_DISTANCE 
          },
          { 
            "label": "Walk duration", 
            "value": constants.ACTIVITY_DURATION 
          },
          { 
            "label": "Walk pace", 
            "value": constants.ACTIVITY_PACE 
          },
          { 
            "label": "Walk speed", 
            "value": constants.ACTIVITY_SPEED 
          },
          { 
            "label": "Walk steps", 
            "value": constants.ACTIVITY_STEPS 
          }
        ]
      },
      {
        "type": "select",
        "messageKey": "cfgHealthWalkLine3",
        "defaultValue": constants.TODAY_STEPS,
        "label": "Line 3",
        "options": [
          { 
            "label": "(Empty)",
            "value": constants.EMPTY
          },
          { 
            "label": "Avg. calories till now", 
            "value": constants.AVG_CALORIES_TILL_NOW 
          },
          { 
            "label": "Avg. distance till now", 
            "value": constants.AVG_DISTANCE_TILL_NOW 
          },
          { 
            "label": "Avg. steps till now", 
            "value": constants.AVG_STEPS_TILL_NOW 
          },
          { 
            "label": "Avg. total calories", 
            "value": constants.AVG_TOTAL_CALORIES 
          },
          { 
            "label": "Avg. total distance", 
            "value": constants.AVG_TOTAL_DISTANCE 
          },
          { 
            "label": "Avg. total steps", 
            "value": constants.AVG_TOTAL_STEPS 
          },
          { 
            "label": "Total calories", 
            "value": constants.TODAY_CALORIES 
          },
          { 
            "label": "Total distance", 
            "value": constants.TODAY_DISTANCE 
          },
          { 
            "label": "Total steps", 
            "value": constants.TODAY_STEPS 
          },
          { 
            "label": "Walk calories", 
            "value": constants.ACTIVITY_CALORIES 
          },
          { 
            "label": "Walk distance", 
            "value": constants.ACTIVITY_DISTANCE 
          },
          { 
            "label": "Walk duration", 
            "value": constants.ACTIVITY_DURATION 
          },
          { 
            "label": "Walk pace", 
            "value": constants.ACTIVITY_PACE 
          },
          { 
            "label": "Walk speed", 
            "value": constants.ACTIVITY_SPEED 
          },
          { 
            "label": "Walk steps", 
            "value": constants.ACTIVITY_STEPS 
          }
        ]
      },
      {
        "type": "heading",
        "defaultValue": "During runs",
        "size": "5"
      },
      {
        "type": "select",
        "messageKey": "cfgHealthRunLine1",
        "defaultValue": constants.ACTIVITY_DURATION,
        "label": "Line 1",
        "options": [
          { 
            "label": "(Empty)",
            "value": constants.EMPTY
          },
          { 
            "label": "Avg. calories till now", 
            "value": constants.AVG_CALORIES_TILL_NOW 
          },
          { 
            "label": "Avg. distance till now", 
            "value": constants.AVG_DISTANCE_TILL_NOW 
          },
          { 
            "label": "Avg. steps till now", 
            "value": constants.AVG_STEPS_TILL_NOW 
          },
          { 
            "label": "Avg. total calories", 
            "value": constants.AVG_TOTAL_CALORIES 
          },
          { 
            "label": "Avg. total distance", 
            "value": constants.AVG_TOTAL_DISTANCE 
          },
          { 
            "label": "Avg. total steps", 
            "value": constants.AVG_TOTAL_STEPS 
          },
          { 
            "label": "Run calories", 
            "value": constants.ACTIVITY_CALORIES 
          },
          { 
            "label": "Run distance", 
            "value": constants.ACTIVITY_DISTANCE 
          },
          { 
            "label": "Run duration", 
            "value": constants.ACTIVITY_DURATION 
          },
          { 
            "label": "Run pace", 
            "value": constants.ACTIVITY_PACE 
          },
          { 
            "label": "Run speed", 
            "value": constants.ACTIVITY_SPEED 
          },
          { 
            "label": "Run steps", 
            "value": constants.ACTIVITY_STEPS 
          },
          { 
            "label": "Total calories", 
            "value": constants.TODAY_CALORIES 
          },
          { 
            "label": "Total distance", 
            "value": constants.TODAY_DISTANCE 
          },
          { 
            "label": "Total steps", 
            "value": constants.TODAY_STEPS 
          }
        ]
      },
      {
        "type": "select",
        "messageKey": "cfgHealthRunLine2",
        "defaultValue": constants.ACTIVITY_DISTANCE,
        "label": "Line 2",
        "options": [
          { 
            "label": "(Empty)",
            "value": constants.EMPTY
          },
          { 
            "label": "Avg. calories till now", 
            "value": constants.AVG_CALORIES_TILL_NOW 
          },
          { 
            "label": "Avg. distance till now", 
            "value": constants.AVG_DISTANCE_TILL_NOW 
          },
          { 
            "label": "Avg. steps till now", 
            "value": constants.AVG_STEPS_TILL_NOW 
          },
          { 
            "label": "Avg. total calories", 
            "value": constants.AVG_TOTAL_CALORIES 
          },
          { 
            "label": "Avg. total distance", 
            "value": constants.AVG_TOTAL_DISTANCE 
          },
          { 
            "label": "Avg. total steps", 
            "value": constants.AVG_TOTAL_STEPS 
          },
          { 
            "label": "Run calories", 
            "value": constants.ACTIVITY_CALORIES 
          },
          { 
            "label": "Run distance", 
            "value": constants.ACTIVITY_DISTANCE 
          },
          { 
            "label": "Run duration", 
            "value": constants.ACTIVITY_DURATION 
          },
          { 
            "label": "Run pace", 
            "value": constants.ACTIVITY_PACE 
          },
          { 
            "label": "Run speed", 
            "value": constants.ACTIVITY_SPEED 
          },
          { 
            "label": "Run steps", 
            "value": constants.ACTIVITY_STEPS 
          },
          { 
            "label": "Total calories", 
            "value": constants.TODAY_CALORIES 
          },
          { 
            "label": "Total distance", 
            "value": constants.TODAY_DISTANCE 
          },
          { 
            "label": "Total steps", 
            "value": constants.TODAY_STEPS 
          }
        ]
      },
      {
        "type": "select",
        "messageKey": "cfgHealthRunLine3",
        "defaultValue": constants.ACTIVITY_SPEED,
        "label": "Line 3",
        "options": [
          { 
            "label": "(Empty)",
            "value": constants.EMPTY
          },
          { 
            "label": "Avg. calories till now", 
            "value": constants.AVG_CALORIES_TILL_NOW 
          },
          { 
            "label": "Avg. distance till now", 
            "value": constants.AVG_DISTANCE_TILL_NOW 
          },
          { 
            "label": "Avg. steps till now", 
            "value": constants.AVG_STEPS_TILL_NOW 
          },
          { 
            "label": "Avg. total calories", 
            "value": constants.AVG_TOTAL_CALORIES 
          },
          { 
            "label": "Avg. total distance", 
            "value": constants.AVG_TOTAL_DISTANCE 
          },
          { 
            "label": "Avg. total steps", 
            "value": constants.AVG_TOTAL_STEPS 
          },
          { 
            "label": "Run calories", 
            "value": constants.ACTIVITY_CALORIES 
          },
          { 
            "label": "Run distance", 
            "value": constants.ACTIVITY_DISTANCE 
          },
          { 
            "label": "Run duration", 
            "value": constants.ACTIVITY_DURATION 
          },
          { 
            "label": "Run pace", 
            "value": constants.ACTIVITY_PACE 
          },
          { 
            "label": "Run speed", 
            "value": constants.ACTIVITY_SPEED 
          },
          { 
            "label": "Run steps", 
            "value": constants.ACTIVITY_STEPS 
          },
          { 
            "label": "Total calories", 
            "value": constants.TODAY_CALORIES 
          },
          { 
            "label": "Total distance", 
            "value": constants.TODAY_DISTANCE 
          },
          { 
            "label": "Total steps", 
            "value": constants.TODAY_STEPS 
          }
        ]
      },
      {
        "type": "heading",
        "defaultValue": "During sleep",
        "size": "5"
      },
      {
        "type": "select",
        "messageKey": "cfgHealthSleepLine1",
        "defaultValue": constants.EMPTY,
        "label": "Line 1",
        "options": [
          { 
            "label": "(Empty)",
            "value": constants.EMPTY
          },
          { 
            "label": "Avg. total deep sleep", 
            "value": constants.AVG_TIME_DEEP_SLEEP 
          },
          { 
            "label": "Avg. total sleep", 
            "value": constants.AVG_TIME_TOTAL_SLEEP 
          },
          { 
            "label": "Time deep sleep", 
            "value": constants.TIME_DEEP_SLEEP 
          },
          { 
            "label": "Total time slept", 
            "value": constants.TIME_TOTAL_SLEEP 
          }
        ]
      },
      {
        "type": "select",
        "messageKey": "cfgHealthSleepLine2",
        "defaultValue": constants.EMPTY,
        "label": "Line 2",
        "options": [
          { 
            "label": "(Empty)",
            "value": constants.EMPTY
          },
          { 
            "label": "Avg. total deep sleep", 
            "value": constants.AVG_TIME_DEEP_SLEEP 
          },
          { 
            "label": "Avg. total sleep", 
            "value": constants.AVG_TIME_TOTAL_SLEEP 
          },
          { 
            "label": "Time deep sleep", 
            "value": constants.TIME_DEEP_SLEEP 
          },
          { 
            "label": "Total time slept", 
            "value": constants.TIME_TOTAL_SLEEP 
          }
        ]
      },
      {
        "type": "select",
        "messageKey": "cfgHealthSleepLine3",
        "defaultValue": constants.TIME_TOTAL_SLEEP,
        "label": "Line 3",
        "options": [
          { 
            "label": "(Empty)",
            "value": constants.EMPTY
          },
          { 
            "label": "Avg. total deep sleep", 
            "value": constants.AVG_TIME_DEEP_SLEEP 
          },
          { 
            "label": "Avg. total sleep", 
            "value": constants.AVG_TIME_TOTAL_SLEEP 
          },
          { 
            "label": "Time deep sleep", 
            "value": constants.TIME_DEEP_SLEEP 
          },
          { 
            "label": "Total time slept", 
            "value": constants.TIME_TOTAL_SLEEP 
          }
        ]
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Sunset/Sunrise"
      },
      { 
        "type": "text", 
        "defaultValue": "Sunset/sunrise is part of the weather refresh/location." 
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Weather"
      },
      {
        "type": "radiogroup",
        "messageKey": "cfgTemperatureUnits",
        "label": "Temperature Units",
        "defaultValue": 'C',
        "options": [
          { 
            "label": "Fahrenheit", 
            "value": 'F' 
          },
          { 
            "label": "Celcius", 
            "value": 'C' 
          }
        ]
      },
      {
        "type": "select",
        "messageKey": "cfgWeatherProvider",
        "defaultValue": constants.YRNO,
        "label": "Provider",
        "options": [
          { 
            "label": "YR.no", 
            "value": constants.YRNO 
          },
          { 
            "label": "OpenWeatherMap",
            "value": constants.OPENWEATHERMAP
          },
          { 
            "label": "Yahoo",
            "value": constants.YAHOO 
          },
          { 
            "label": "Forecast.io",
            "value": constants.FORECASTIO 
          }
        ]
      },
      {
        "type": "input",
        "messageKey": "cfgWeatherOWMKey",
        "defaultValue": "",
        "label": "OpenWeatherMap Key",
        "description": "Without a key the YR.no provider will be used.",
        "attributes": {
          "placeholder": "eg: ab123c456ef789a12bc345d6e78f9a12",
          "limit": 32
        }
      },
      {
        "type": "input",
        "messageKey": "cfgWeatherFIOKey",
        "defaultValue": "",
        "label": "Forecast.io Key",
        "description": "Without a key the YR.no provider will be used.",
        "attributes": {
          "limit": 32
        }
      },
      {
        "type": "masterkey",
        "messageKey": "cfgWeatherMasterKey"
      },
      {
        "type": "select",
        "messageKey": "cfgWeatherRefresh",
        "defaultValue": 30,
        "label": "Refresh every",
        "description":"Weather refresh is suspended during sleep.",
        "options": [
          { 
            "label": "10 mins", 
            "value": 10
          },
          { 
            "label": "30 mins",
            "value": 30
          },
          { 
            "label": "60 mins",
            "value": 60
          },
          { 
            "label": "2 hours",
            "value": 120
          },
          { 
            "label": "4 hours",
            "value": 240
          }
        ]
      },
      {
        "type": "toggle",
        "messageKey": "cfgWeatherLocPhone",
        "label": "Use phone location",
        "defaultValue": true
      },
      {
        "type": "input",
        "messageKey": "cfgWeatherLocLat",
        "defaultValue": "",
        "label": "Lattitude",
        "attributes": {
          "placeholder": "eg: 39.0437",
          "limit": 20
        }
      },
      {
        "type": "input",
        "messageKey": "cfgWeatherLocLong",
        "defaultValue": "",
        "label": "Longitude",
        "attributes": {
          "placeholder": "eg: -77.4875",
          "limit": 20
        }
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];