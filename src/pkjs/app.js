try{
  window.location = {}; //shim for iOS
  document.createElement = null; //shim to trick the browserify shims, yay! (also for iOS, yay!)
} catch(err) {
  // While these are necessary for the weather lookup to work in iOS, they brake the emulator
}

var MessageQueue = require('./libs/js-message-queue');
var constants = require('./constants');
var Analytics = require('./libs/analytics.js');
var keys = require('message_keys');
var lastSubmittedAltitude = Number.MIN_VALUE;
var lastSubmittedAltitudeAccuracy = Number.MIN_VALUE;
var altitudeWatchId;
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
  
  // Calculate alternative timezone offset
  var timezoneOffset = calculateTimezoneOffset(dict[keys.cfgTimeZoneCity]);
  dict[keys.cfgTimeZoneOffset] = timezoneOffset;
  
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
   
  // Clear location config from data sent to the watch
  delete dict[keys.Location1];
  delete dict[keys.Location2];
  delete dict[keys.Location3];
  
  // Convert refreshes to int
  dict[keys.cfgLocationRefresh] = parseInt(dict[keys.cfgLocationRefresh]);
  dict[keys.cfgAltitudeRefresh] = parseInt(dict[keys.cfgAltitudeRefresh]);
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
    if (e.payload.FetchAltitude) fetchAltitude(); 
    if (e.payload.SubscribeAltitude) {
      lastSubmittedAltitude = Number.MIN_VALUE;
      lastSubmittedAltitudeAccuracy = Number.MIN_VALUE;
      altitudeWatchId = navigator.geolocation.watchPosition(processAltitude, failedAltitude);
    } 
    if (e.payload.UnsubscribeAltitude) navigator.geolocation.clearWatch(altitudeWatchId);
    if (e.payload.FetchLocation) findLocation(fetchLocation);
    if (e.payload.FetchMoonphase) fetchMoonphase();
    if (e.payload.FetchSun) findLocation(fetchSun);
    if (e.payload.FetchWeather) findLocation(fetchWeather);
    if (e.payload.HeartrateAvailable) heartrateAvailable = true;
    if (e.payload.Exception) analytics.trackException("C crash in zone " + e.payload.Exception + ".");
    if (e.payload.Fireworks) analytics.trackEvent('watchface', 'Fireworks');
  }
  catch (err) {
    console.warn('AddEventListener error: ' + err);
    analytics.trackException(err);
  }
  
  pingAnalytics();
});

function geoloc(latitude, longitude)
{
    this.latitude=latitude;
    this.longitude=longitude;
}

function findLocation(callback) {
    if (!settings.cfgWeatherLocPhone) {
      // Retrieve location from settings
      var loc = new geoloc(settings.cfgWeatherLocLat, settings.cfgWeatherLocLong);
      callback(loc);
    } else {
      // Retrieve location from phone
      navigator.geolocation.getCurrentPosition(function(pos) { //Success
        console.log('lat: ' + pos.coords.latitude);
        console.log('lng: ' + pos.coords.longitude);
    
        var loc = new geoloc(pos.coords.latitude, pos.coords.longitude);
        callback(loc);
    
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

function processAltitude(position) {
  // Was an altitude provided?
  if (!position.coords.altitude) {
    failedAltitude( {
      code: 0,
      message: "Altitude not available."
    });
    return;
  }
  
  // Succes, got location
  var currentAltitude = position.coords.altitude;
  var currentAccuracy = position.coords.altitudeAccuracy;

  // Validate that the altitude has changed before sending it to the watch
  if (Math.abs(currentAltitude - lastSubmittedAltitude) > currentAccuracy + lastSubmittedAltitudeAccuracy) {
    console.log('Altitude: ' + position.coords.altitude + 'm (accuracy ' + position.coords.altitudeAccuracy + 'm)');
    
    lastSubmittedAltitude = currentAltitude;
    lastSubmittedAltitudeAccuracy = currentAccuracy;
    sendMessage({
      'Altitude': Math.round(currentAltitude * 1000),
      'AltitudeAccuracy': Math.round(currentAccuracy * 1000)
    });
  }
}

function fetchAltitude() {
  navigator.geolocation.getCurrentPosition( function(position) {
      // Was an altitude provided?
      if (!position.coords.altitude) {
        failedAltitude( {
          code: 0,
          message: "Altitude not available."
        });
        return;
      }
    
      var currentAltitude = position.coords.altitude;
      var currentAccuracy = position.coords.altitudeAccuracy;
      console.log('Altitude: ' + position.coords.altitude + 'm (accuracy ' + position.coords.altitudeAccuracy + 'm)');
    
      sendMessage({
        'Altitude': Math.round(currentAltitude * 1000),
        'AltitudeAccuracy': Math.round(currentAccuracy * 1000)
      });
    } , failedAltitude , { 
      enableHighAccuracy: true,
      maximumAge: 300000,
      timeout: 59000
    } );
}

function failedAltitude(err) {
  console.warn('Altitude error: ' + err.code + ' - ' + err.message); 
  sendMessage({
    'Err': constants.ALTITUDE_ERROR,
  });    
}

function toDegreesMinutesAndSeconds(coordinate) {
    var absolute = Math.abs(coordinate);
    var degrees = Math.floor(absolute);
    var minutesNotTruncated = (absolute - degrees) * 60;
    var minutes = Math.floor(minutesNotTruncated);
    var seconds = Math.floor((minutesNotTruncated - minutes) * 60);

  return (degrees < 10 ? "0" : "") + degrees + "°" + (minutes < 10 ? "0" : "") + minutes + "'" + (seconds < 10 ? "0" : "") + seconds + '"';
}

function extractLocationProperty(location, property) {
  var point = settings.cfgHealthNumbers ? settings.cfgHealthNumbers == 'M' ? "," : "." : ",";
  
  switch (property) {
    case constants.ROAD:
      if (location.address.road) return location.address.road;
      break;
    case constants.CITY_TOWN_HAMLET:
      if (location.address.hamlet) return location.address.hamlet;
      if (location.address.town) return location.address.town;
      if (location.address.city) return location.address.city;
      break;
    case constants.STATE:
      if (location.address.state) return location.address.state;
      break;
    case constants.COUNTRY:
      if (location.address.country) return location.address.country;
      break;
    case constants.COUNTRY_CODE:
      if (location.address.country_code) return location.address.country_code.toUpperCase();
      break;
    case constants.LATITUDE_DEGREES:
      if (location.lat) {
        var latitude = toDegreesMinutesAndSeconds(location.lat);
        var latitudeCardinal = parseFloat(location.lat) >= 0 ? "N" : "S";
        return latitude + latitudeCardinal;
      }
      break;
    case constants.LONGITUDE_DEGREES:
      if (location.lon) {
        var longitude = toDegreesMinutesAndSeconds(location.lon);
        var longitudeCardinal = parseFloat(location.lon) >= 0 ? "E" : "W";
        return longitude + longitudeCardinal;
      }
      break;
    case constants.LATITUDE_DECIMAL:
      if (location.lat) {
        return Number(Math.round(location.lat + 'e3') + 'e-3').toString().replace(".", point);
      }
      break;
    case constants.LONGITUDE_DECIMAL:
      if (location.lon) {
        return Number(Math.round(location.lon + 'e3') + 'e-3').toString().replace(".", point);
      }
      break;
  }  
  
  return null;
}

function processLocation(location) {
  var location1, location2, location3;
  location1 = extractLocationProperty(location, settings.Location1 ? parseInt(settings.Location1) : constants.EMPTY);
  location2 = extractLocationProperty(location, settings.Location2 ? parseInt(settings.Location2) : constants.EMPTY);
  location3 = extractLocationProperty(location, settings.Location3 ? parseInt(settings.Location3) : constants.CITY_TOWN_HAMLET);
  
  var message = { };
  if (location1) message.Location1 = location1;
  if (location2) message.Location2 = location2;
  if (location3) message.Location3 = location3;
  
  console.log("Location: " + location1 + ", " + location2 + ", " + location3);
  
  sendMessage(message);
}

function fetchLocation(loc) {
  // Do we need an OpenStreetMap call or are only latitude and longitude required?
  var location1cfg, location2cfg, location3cfg;
  location1cfg = settings.Location1 ? parseInt(settings.Location1) : constants.EMPTY;
  location2cfg = settings.Location2 ? parseInt(settings.Location2) : constants.EMPTY;
  location3cfg = settings.Location3 ? parseInt(settings.Location3) : constants.CITY_TOWN_HAMLET;
  if ((location1cfg == constants.EMPTY || (location1cfg >= constants.LATITUDE_DEGREES && location1cfg <= constants.LONGITUDE_DECIMAL)) &&
      (location2cfg == constants.EMPTY || (location2cfg >= constants.LATITUDE_DEGREES && location2cfg <= constants.LONGITUDE_DECIMAL)) &&
      (location3cfg == constants.EMPTY || (location3cfg >= constants.LATITUDE_DEGREES && location3cfg <= constants.LONGITUDE_DECIMAL))) {
    // No OpenStreetMap call necessary 
    var location = {
      lat: loc.latitude,
      lon: loc.longitude
    };
    console.log("Keeping it local :-)");
    processLocation(location);
  }  else {  
    // Call to OpenStreetMap required
    var req = new XMLHttpRequest();
    req.open("GET", "http://nominatim.openstreetmap.org/reverse?format=json&lat=" + loc.latitude + "&lon=" + loc.longitude, true);
    req.setRequestHeader("User-Agent", "Alternating Tidbits Pebble watchface");
    req.onload = function(e) {
      if(req.status == 200) {
        var response = JSON.parse(req.responseText);
        processLocation(response);
      }
      else {
        failedLocation(req.status, "OpenStreetMap returned error");
      }
    };
    req.onerror = function(e) {
      failedLocation(e);
    };
    req.send(null);
  }
}

function failedLocation(err) {
  console.warn('Location error: ' + err.code + ' - ' + err.message); 
  sendMessage({
    'Err': constants.LOCATION_ERROR,
  });    
}

function fetchMoonphase() {
  var SunCalc = require('./libs/suncalc');
  var moon = SunCalc.getMoonIllumination(new Date());  
  console.log('Moon phase: ' + Math.round(moon.phase * 360) + ", illumination: " + Math.round(moon.fraction * 100));
  
  sendMessage({
    'Moonphase': Math.round(moon.phase * 360),
    'Moonillumination': Math.round(moon.fraction * 100)
  });
}

function fetchSun(loc) {
  var SunCalc = require('./libs/suncalc');
  var times = SunCalc.getTimes(new Date(), loc.latitude, loc.longitude);
  
  console.log('dawn: ' + times.dawn.getHours() + ':' + times.dawn.getMinutes());
  console.log('sunrise: ' + times.sunrise.getHours() + ':' + times.sunrise.getMinutes());
  console.log('sunset: ' + times.sunsetStart.getHours() + ':' + times.sunsetStart.getMinutes());
  console.log('dusk: ' + times.dusk.getHours() + ':' + times.dusk.getMinutes());
  
  sendMessage({
    'Dawn': times.dawn.getHours() * 60 + times.dawn.getMinutes(),
    'Sunrise': times.sunrise.getHours() * 60 + times.sunrise.getMinutes(),
    'Sunset': times.sunsetStart.getHours() * 60 + times.sunsetStart.getMinutes(),
    'Dusk': times.dusk.getHours() * 60 + times.dusk.getMinutes(),
  });
}

function fetchWeather(loc) {
    var WeatherMan = require('./libs/weather-man');
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

            sendMessage({
                'Temperature': temp,
                'Condition': cond,
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