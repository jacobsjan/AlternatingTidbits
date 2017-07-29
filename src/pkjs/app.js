try{
  window.location = {}; //shim for iOS
  document.createElement = null; //shim to trick the browserify shims, yay! (also for iOS, yay!)
} catch(err) {
  // While these are necessary for the weather lookup to work in iOS, they brake the emulator
}

var MessageQueue = require('./libs/js-message-queue');
var WeatherMan = require('./libs/weather-man');
var constants = require('./constants');
var Analytics = require('./libs/analytics.js');
var keys = require('message_keys');
var lastSubmittedAltitude = Number.MIN_VALUE;
var altitudeSubscription = false;
var heartrateAvailable = false;
var analytics = null;

function sendMessage(data) {
  MessageQueue.sendAppMessage(data, ack, nack);
}

function calculateTimezoneOffset(timezone) {
  var timezoneOffset = require('./timezoneOffset');
  var altTimezoneOffset = timezoneOffset(timezone);
  var localTimezoneOffset = new Date().getTimezoneOffset();
  return localTimezoneOffset - altTimezoneOffset;
}

//localStorage.removeItem('clay-settings');
// Initialize settings, don't know how to retrieve these neatly from clay
var settings = {};
try {
    settings = JSON.parse(localStorage.getItem('clay-settings'));
  
    if (!settings) {
      // No clay settings found, initializing settings to defaults
      settings = {
        "cfgEnableTimezone": false,
        "cfgTemperatureUnits": 'C',
        "cfgWeatherProvider": constants.YRNO,
        "cfgWeatherLocPhone": true
      };
    }
  } catch (e) {
    console.error(e.toString());
}

// Make the watchface configurable using Clay
var Clay = require('pebble-clay');

// Load our Clay configuration files
var clayConfig = require('./config');
var clayConfigHR = require('./config-hr');
var customClay = require('./clay-custom');

// Initialize Clay
var clay = new Clay(clayConfig, customClay, { autoHandleEvents: false } );
clay.registerComponent(require('./clay-component-masterkey'));
clay.registerComponent(require('./clay-component-dateformat'));
clay.registerComponent(require('./clay-component-timezone'));

Pebble.addEventListener('showConfiguration', function(e) {
  // Show HR or default config page?
  if (heartrateAvailable) {
    clay.config = clayConfigHR;
  } else {
    clay.config = clayConfig;
  }
  
  // Open configuration page
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (e && !e.response) {
    return;
  }
  
  // Get the keys and values from each config item
  var dict = clay.getSettings(e.response);
  
  // Remove invalid countdown information
  var countdowns = JSON.parse(dict[keys.cfgCountdownLabel]);
  countdowns = removeInvalidCountdowns(countdowns);
  clay.setSettings("cfgCountdownLabel", JSON.stringify(countdowns));
  
  // Add countdown count and index
  dict[keys.cfgCountdownCount] = countdowns.length;
  if (countdowns.length == 0) {
    // Disable countdown tidbit if there are no valid countdown targets
    dict[keys.cfgEnableCountdown] = 0;
    dict[keys.cfgCountdownIndex] = -1;
    clay.setSettings("cfgEnableCountdown", false);
  } 
   
  // Clear countdown info from main data sent to the watch
  delete dict[keys.cfgCountdownLabel];
  delete dict[keys.cfgCountdownTo];
  delete dict[keys.cfgCountdownTime];
  delete dict[keys.cfgCountdownDate];
  
  // Calculate alternative timezone offset
  var timezoneOffset = calculateTimezoneOffset(dict[keys.cfgTimeZoneCity]);
  dict[keys.cfgTimeZoneOffset] = timezoneOffset;
    
  // Convert health indicators to int
  dict[keys.cfgHealthNormalLine1] = parseInt(dict[keys.cfgHealthNormalLine1]);
  dict[keys.cfgHealthNormalLine2] = parseInt(dict[keys.cfgHealthNormalLine2]);
  dict[keys.cfgHealthNormalLine3] = parseInt(dict[keys.cfgHealthNormalLine3]);
  dict[keys.cfgHealthWalkLine1] = parseInt(dict[keys.cfgHealthWalkLine1]);
  dict[keys.cfgHealthWalkLine2] = parseInt(dict[keys.cfgHealthWalkLine2]);
  dict[keys.cfgHealthWalkLine3] = parseInt(dict[keys.cfgHealthWalkLine3]);
  dict[keys.cfgHealthRunLine1] = parseInt(dict[keys.cfgHealthRunLine1]);
  dict[keys.cfgHealthRunLine2] = parseInt(dict[keys.cfgHealthRunLine2]);
  dict[keys.cfgHealthRunLine3] = parseInt(dict[keys.cfgHealthRunLine3]);
  dict[keys.cfgHealthSleepLine1] = parseInt(dict[keys.cfgHealthSleepLine1]);
  dict[keys.cfgHealthSleepLine2] = parseInt(dict[keys.cfgHealthSleepLine2]);
  dict[keys.cfgHealthSleepLine3] = parseInt(dict[keys.cfgHealthSleepLine3]);
  
  // Convert weather refresh to int
  dict[keys.cfgWeatherRefresh] = parseInt(dict[keys.cfgWeatherRefresh]);
  
  // Determine alternative timezone offset based on timezone name
  var split = dict[keys.cfgTimeZoneCity].split('/');
  var city = split[split.length - 1].replace('_', ' ');
  dict[keys.cfgTimeZoneCity] = city;
  
  // Send settings values to watch side
  sendMessage(dict);
  
  // Send countdown information seperately to the watch
  sendCountdowns(countdowns);
  
  // Reload settings
  settings = JSON.parse(localStorage.getItem('clay-settings'));
  
  // Send settings set to analytics
  analytics.trackEvent('watchface', 'Settings');
});

function removeInvalidCountdowns(countdowns) {
  var result = [];
  
  for (var i = 0; i < countdowns.length; i++) {
    // Convert countdown time/date to integers, avoiding posix format due to DST issues over longer periods of time
    var timeElems = countdowns[i].time.split(':');
    var dateElems = countdowns[i].date.split('-');
    if (countdowns[i].countdownTo == 'D') {
      if (dateElems.length == 3) {
        result.push(countdowns[i]);
      }
    } else if (countdowns[i].countdownTo == 'T') {
      if (dateElems.length == 3 && timeElems.length == 2) {
        result.push(countdowns[i]);
      }
    } else {
      if (timeElems.length == 2) {
        result.push(countdowns[i]);
      }
    }
  }
  
  return result;
}

function sendCountdowns(countdowns) {  
  for (var i = 0; i < countdowns.length; i++) {
    var countdownDict = {};
    countdownDict[keys.cfgCountdownIndex] = i;
    countdownDict[keys.cfgCountdownLabel] = countdowns[i].label;
    countdownDict[keys.cfgCountdownTo] = countdowns[i].countdownTo;
    
    // Convert countdown time/date to integers, avoiding posix format due to DST issues over longer periods of time
    var timeElems = countdowns[i].time.split(':');
    var dateElems = countdowns[i].date.split('-');
    if (countdowns[i].countdownTo == 'D') {
      countdownDict[keys.cfgCountdownTime] = 0; 
      countdownDict[keys.cfgCountdownDate] = parseInt(dateElems[0]) * 100 * 100 + parseInt(dateElems[1]) * 100 + parseInt(dateElems[2]); 
    } else if (countdowns[i].countdownTo == 'T') {
      countdownDict[keys.cfgCountdownTime] = parseInt(timeElems[0]) * 100 + parseInt(timeElems[1]); 
      countdownDict[keys.cfgCountdownDate] = parseInt(dateElems[0]) * 100 * 100 + parseInt(dateElems[1]) * 100 + parseInt(dateElems[2]); 
    } else {
      countdownDict[keys.cfgCountdownTime] = parseInt(timeElems[0]) * 100 + parseInt(timeElems[1]); 
      countdownDict[keys.cfgCountdownDate] = 0;
    }
    
    // Send countdown to watch
    sendMessage(countdownDict);
  }
}

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

function pingAnalytics() {
  // Ping maximally every hour
  var lastPing = window.localStorage.getItem('analyticsLastPing');
  var mSinceLastPing = Math.floor((new Date() - new Date(lastPing)) / (60 * 1000));
  if (lastPing === undefined || mSinceLastPing >= 60) {
    console.log("Pinging, last analytics ping was " + mSinceLastPing + "m ago.");
    analytics.trackEvent('watchface', 'Alive');
    window.localStorage.setItem('analyticsLastPing', new Date());
  } else {
    console.log("Last analytics ping was only " + mSinceLastPing + "m ago.");
  }
}

Pebble.addEventListener('ready', function(e) {
  // Inform the watch that the phone is ready for communication
  sendMessage({
    'JSReady': 0
  });
  
  // Recalculate the alternative timezone offset
  if (settings.cfgEnableTimezone) {
    sendMessage({
      'cfgTimeZoneOffset': calculateTimezoneOffset(settings.cfgTimeZoneCity)
    });
  }
  
  // Initialize analytics
  analytics = new Analytics('UA-89182749-1', 'Alternating Tidbits', '1.5');

  // Signal analytics
  pingAnalytics();
});

Pebble.addEventListener('appmessage', function(e) {
  try {
    console.log('Received message: ' + JSON.stringify(e.payload));
    if (e.payload.Fetch) {
      fetchLocation();
    } else if (e.payload.FetchMoonphase) {
      fetchMoonphase();
    } else if (e.payload.SubscribeAltitude) {
      // Look up altitude every 60 seconds 
      fetchAltitude();
      altitudeSubscription = window.setInterval(fetchAltitude, 60000);  
    } else if (e.payload.UnsubscribeAltitude) {
      window.clearInterval(altitudeSubscription);
      lastSubmittedAltitude = Number.MIN_VALUE;
    } else if (e.payload.HeartrateAvailable) {
      heartrateAvailable = true;
    } else if (e.payload.Exception) {
      analytics.trackException("C crash in zone " + e.payload.Exception + ".");
    } else if (e.payload.Fireworks) {
      analytics.trackEvent('watchface', 'Fireworks');
    } else if (e.payload.SleepTrust) {
      if (e.payload.SleepTrust == 1) {
        analytics.trackEvent('watchface', 'Sleep not trusted');
      } else if (e.payload.SleepTrust == 2) { 
        analytics.trackEvent('watchface', 'Sleep trusted again');
      } else { 
        analytics.trackEvent('watchface', 'Sleep trust unclear');
      }
    }
  }
  catch (err) {
    analytics.trackException(err);
  }
  
  pingAnalytics();
});

function fetchAltitude() {
  navigator.geolocation.getCurrentPosition( function (position) {
      // Succes, got location
      var currentAltitude = position.coords.altitude;
      
      // Validate that the altitude has changed before sending it to the watch
      if (Math.abs(currentAltitude - lastSubmittedAltitude) > position.coords.altitudeAccuracy) {
        lastSubmittedAltitude = currentAltitude;
        sendMessage({
          'Altitude': Math.round(currentAltitude)
        });
      }
    }, function(err) {
        console.warn('location error: ' + err.code + ' - ' + err.message); 
        sendMessage({
          'Err': constants.LOCATION_ERROR,
        });    
    }, { 
      enableHighAccuracy: true,
      maximumAge: 0,
      timeout: 59000
    } );
}

function geoloc(latitude, longitude)
{
    this.latitude=latitude;
    this.longitude=longitude;
}

function fetchLocation() {
    if (!settings.cfgWeatherLocPhone) {
      // Retrieve location from settings
      var loc = new geoloc(settings.cfgWeatherLocLat, settings.cfgWeatherLocLong);
      fetchWeather(loc);
    } else {
      // Retrieve location from phone
      window.navigator.geolocation.getCurrentPosition(function(pos) { //Success
        console.log('lat: ' + pos.coords.latitude);
        console.log('lng: ' + pos.coords.longitude);
        console.log('altitude: ' + pos.coords.altitude + 'm (accuracy ' + pos.coords.altitudeAccuracy + 'm)');
    
        var loc = new geoloc(pos.coords.latitude, pos.coords.longitude);
        fetchWeather(loc);
    
      }, function(err) { //Error
        console.warn('location error: ' + err.code + ' - ' + err.message);
        sendMessage({
          'Err': constants.LOCATION_ERROR,
        });    
      }, { //Options
        timeout: 30000, //30 seconds
        maximumAge: 300000, //5 minutes
      });
  }
}

function fetchWeather(loc) {
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
            var units = WeatherMan.KELVIN;
            if (settings.cfgTemperatureUnits == 'F') {
                units = WeatherMan.FAHRENHEIT;
            }
            else if (settings.cfgTemperatureUnits == 'C') {
                units = WeatherMan.CELCIUS;
            }

            var temp = result.getTemperature(units);
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

            sendMessage({
                'Temperature': temp,
                'Condition': cond,
                'Sunrise': result.getSunrise(),
                'Sunset': result.getSunset(),
            });
        }).catch(function(result) {
            console.warn('weather error: ' + JSON.stringify(result));

            sendMessage({
                'Err': constants.WEATHER_ERROR,
            });
        });
    }
    else {
        sendMessage({
            'Err': constants.WEATHER_ERROR,
        });
    }
}

function fetchMoonphase() {
  var moonphase = require('./moonphase');  
  var mp = moonphase();
  console.log('Moon phase: ' + mp.phase + ", illumination: " + mp.illumination);
  
  sendMessage({
    'Moonphase': mp.phase,
    'Moonillumination': mp.illumination
  });
}