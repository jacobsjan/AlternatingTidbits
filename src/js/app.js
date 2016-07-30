try{
  window.location = {}; //shim for iOS
  document.createElement = null; //shim to trick the browserify shims, yay! (also for iOS, yay!)
} catch(err) {
  // While these are necessary for the weather lookup to work in iOS, they brake the emulator
}

var MessageQueue = require('./js/libs/js-message-queue');
var WeatherMan = require('./js/libs/weather-man');
var logger = require('./js/logger');
var constants = require('./js/constants');
var customClay = require('./js/custom-clay');
var keys = require('message_keys');

//localStorage.removeItem('clay-settings');
// Initialize settings, don't know how to retrieve these neatly from clay
var settings = {};
try {
    settings = JSON.parse(localStorage.getItem('clay-settings'));
  
    if (!settings) {
      // No clay settings found, initializing settings to defaults
      settings = {
        "cfgTemperatureUnits": 'C',
        "cfgWeatherProvider": constants.YRNO,
        "cfgWeatherLocPhone": true
      };
    }
  } catch (e) {
    console.error(e.toString());
}


// Make the watchface configurable using Clay
// Import the Clay package
var Clay = require('pebble-clay');
// Load our Clay configuration file
var clayConfig = require('./config');
// Initialize Clay
var clay = new Clay(clayConfig, customClay, { autoHandleEvents: false } );
clay.registerComponent(require('./js/masterkey-clay-component'));
clay.registerComponent(require('./js/dateformat-clay-component'));

Pebble.addEventListener('showConfiguration', function(e) {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (e && !e.response) {
    return;
  }

  // Get the keys and values from each config item
  var dict = clay.getSettings(e.response);
  
  // Convert weather refresh to int
  dict[keys.cfgWeatherRefresh] = parseInt(dict[keys.cfgWeatherRefresh]);

  // Send settings values to watch side
  Pebble.sendAppMessage(dict, function(e) {
    console.log('Sent config data to Pebble');
  }, function(e) {
    console.log('Failed to send config data!');
    console.log(JSON.stringify(e));
  });
  
  // Reload settings
  settings = JSON.parse(localStorage.getItem('clay-settings'));
});

function ack(e) {
  try {
    console.log('Successfully delivered message with transactionId=' + e.data.transactionId);
  }
  catch(err) {}
}

function nack(e) {
  try {
    console.log('Unable to deliver message with transactionId=' + e.data.transactionId + ', error is: ' + e.error.message);
  }
  catch(err) {}
}

Pebble.addEventListener('ready', function(e) {
  // fetchLocation();
  MessageQueue.sendAppMessage({
    'JSReady': 0
  }, ack, nack);
});

Pebble.addEventListener('appmessage', function(e) {
    console.log('Received message: ' + JSON.stringify(e.payload));
    if (e.payload.Fetch) {
        logger.log(logger.FETCH_MESSAGE);

        fetchLocation();
    }
});

function geoloc(latitude, longitude)
{
    this.latitude=latitude;
    this.longitude=longitude;
}

function fetchLocation() {
    logger.log(logger.FETCH_LOCATION);

    if (!settings.cfgWeatherLocPhone) {
      // Retrieve location from settings
      var loc = new geoloc(settings.cfgWeatherLocLat, settings.cfgWeatherLocLong);
      fetchWeather(loc);
    } else {
      // Retrieve location from phone
      window.navigator.geolocation.getCurrentPosition(function(pos) { //Success
        logger.log(logger.LOCATION_SUCCESS);
    
        console.log('lat: ' + pos.coords.latitude);
        console.log('lng: ' + pos.coords.longitude);
    
        var loc = new geoloc(pos.coords.latitude, pos.coords.longitude);
        fetchWeather(loc);
    
      }, function(err) { //Error
        logger.log(logger.LOCATION_ERROR);
        logger.setLocationErrorCode(err.code);
    
        console.warn('location error: ' + err.code + ' - ' + err.message);
    
        MessageQueue.sendAppMessage({
          'Err': constants.LOCATION_ERROR,
        }, ack, nack);
    
      }, { //Options
        timeout: 30000, //30 seconds
        maximumAge: 300000, //5 minutes
      });
  }
}

function fetchWeather(loc) {
    logger.log(logger.FETCH_WEATHER);

    var wm = null;
    if (settings.cfgWeatherProvider == constants.OPENWEATHERMAP && settings.cfgWeatherOWMKey && settings.cfgWeatherOWMKey.length > 0) {
        wm = new WeatherMan(WeatherMan.OPENWEATHERMAP, settings.cfgWeatherOWMKey);
    }
    else if (settings.cfgWeatherProvider == constants.YAHOO) {
        wm = new WeatherMan(WeatherMan.YAHOO);
    }
    else if (settings.cfgWeatherProvider == constants.FORECASTIO && settings.cfgWeatherFIOKey && settings.cfgWeatherFIOKey.length > 0) {
      wm = new WeatherMan(WeatherMan.FORECASTIO, settings.cfgWeatherFIOKey);
    }
    else {
        wm = new WeatherMan(WeatherMan.YRNO);
    }

    if (wm) {
        wm.getCurrent(loc.latitude, loc.longitude).then(function(result) {
            logger.log(logger.WEATHER_SUCCESS);

            var units = WeatherMan.KELVIN;
            if (settings.cfgTemperatureUnits == 'F') {
                units = WeatherMan.FAHRENHEIT;
            }
            else if (settings.cfgTemperatureUnits == 'C') {
                units = WeatherMan.CELCIUS;
            }

            var temp = result.getTemperature(units);
            /*if (settings.feels_like == 1) {
                temp = result.getWindChill(units);
            }
            else if (settings.feels_like == 2) {
                temp = result.getHeatIndex(units);
            }*/

            var condition = result.getCondition();
            var conditions = {};
            conditions[WeatherMan.CLEAR] = constants.CLEAR;
            conditions[WeatherMan.CLOUDY] = constants.CLOUDY;
            conditions[WeatherMan.FOG] = constants.FOG;
            conditions[WeatherMan.LIGHT_RAIN] = constants.LIGHT_RAIN;
            conditions[WeatherMan.RAIN] = constants.RAIN;
            conditions[WeatherMan.THUNDERSTORM] = constants.THUNDERSTORM;
            conditions[WeatherMan.SNOW] = constants.SNOW;
            conditions[WeatherMan.HAIL] = constants.HAIL;
            conditions[WeatherMan.WIND] = constants.WIND;
            conditions[WeatherMan.EXTREME_WIND] = constants.EXTREME_WIND;
            conditions[WeatherMan.TORNADO] = constants.TORNADO;
            conditions[WeatherMan.HURRICANE] = constants.HURRICANE;
            conditions[WeatherMan.EXTREME_COLD] = constants.EXTREME_COLD;
            conditions[WeatherMan.EXTREME_HEAT] = constants.EXTREME_HEAT;
            conditions[WeatherMan.SNOW_THUNDERSTORM] = constants.SNOW_THUNDERSTORM;
            conditions[WeatherMan.LIGHT_CLOUDY] = constants.LIGHT_CLOUDY;
            conditions[WeatherMan.PARTLY_CLOUDY] = constants.PARTLY_CLOUDY;
          
            var cond = conditions[condition] ? conditions[condition] : constants.CLEAR;
            console.log('temp: ' + temp);
            console.log('condition: ' + cond + ' (' + condition + ')');
            console.log('sunrise: ' + result.getSunriseFormatted());
            console.log('sunset: ' + result.getSunsetFormatted());

            sendMessage(loc, {
                'Temperature': temp,
                'Condition': cond,
                'Sunrise': result.getSunrise(),
                'Sunset': result.getSunset(),
            });
        }).catch(function(result) {
            logger.log(logger.WEATHER_ERROR);
            console.warn('weather error: ' + JSON.stringify(result));

            if (result && result.status) {
                logger.setStatusCode(result.status);
            }

            sendMessage(loc, {
                'Err': constants.WEATHER_ERROR,
            });
        });
    }
    else {
        sendMessage(loc, {
            'Err': constants.WEATHER_ERROR,
        });
    }
}


function sendMessage(pos, data) {
  MessageQueue.sendAppMessage(data, ack, nack);
}
