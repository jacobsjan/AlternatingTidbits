var constants = require('./js/constants');

module.exports = [
  {
    "type": "heading",
    "defaultValue": "General Settings"
  }, 
  { 
    "type": "text", 
    "defaultValue": "Press 'Save Settings' below to commit changes to your Pebble." 
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
        "sunlight": false
      },
      {
        "type": "color",
        "messageKey": "cfgColorSecondary",
        "defaultValue": "0x555555",
        "label": "Secondary",
        "sunlight": false
      },
      {
        "type": "color",
        "messageKey": "cfgColorAccent",
        "defaultValue": "0xAA0000",
        "label": "Accent",
        "sunlight": false
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
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Sunset/Sunrise"
      },
      { 
        "type": "text", 
        "defaultValue": "Sunset/sunrise is part of the weather refresh/location." 
      },
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
