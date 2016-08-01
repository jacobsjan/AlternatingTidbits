module.exports = function(minified) {
  var clayConfig = this;

  function resizeBatteryAccent() {
    // Resize the accent slider to match the from value
    var batteryShowFrom = clayConfig.getItemByMessageKey('cfgBatteryShowFrom');
    var batteryAccentFrom = clayConfig.getItemByMessageKey('cfgBatteryAccentFrom');
    var batteryAccentFromParentSpan = batteryAccentFrom.$element.select(".input");
    batteryAccentFromParentSpan.set('$min-width', batteryShowFrom.get() + '%');
    batteryAccentFromParentSpan.set('$width', batteryShowFrom.get() + '%');
      
    // Change max of accent slider
    if (batteryAccentFrom.get() > batteryShowFrom.get()) batteryAccentFrom.set(batteryShowFrom.get());
    batteryAccentFrom.$manipulatorTarget.set('@max', batteryShowFrom.get());
  }
  
  function showWeatherProviderKey() {
    clayConfig.getItemByMessageKey('cfgWeatherMasterKey').hide();
    
    if (this.get() === "0") { // OpenWeatherMap
      clayConfig.getItemByMessageKey('cfgWeatherOWMKey').show();
      clayConfig.getItemByMessageKey('cfgWeatherMasterKey').show();
    } else {
      clayConfig.getItemByMessageKey('cfgWeatherOWMKey').hide();
    }
        
    if (this.get() === "4") { // Forecast.io
      clayConfig.getItemByMessageKey('cfgWeatherFIOKey').show();
      clayConfig.getItemByMessageKey('cfgWeatherMasterKey').show();
    } else {
      clayConfig.getItemByMessageKey('cfgWeatherFIOKey').hide();
    }
  }
  
  function masterKeyReceived(result) {
    if (result.keys.weather.owm !== ""){
      clayConfig.getItemByMessageKey('cfgWeatherOWMKey').set(result.keys.weather.owm);
    }

    if (result.keys.weather.forecast !== ""){
      clayConfig.getItemByMessageKey('cfgWeatherFIOKey').set(result.keys.weather.forecast);
    }
  }

  function showLonLat() {
    if (this.get()) { 
      clayConfig.getItemByMessageKey('cfgWeatherLocLat').hide();
      clayConfig.getItemByMessageKey('cfgWeatherLocLong').hide();
    } else {
      clayConfig.getItemByMessageKey('cfgWeatherLocLat').show();
      clayConfig.getItemByMessageKey('cfgWeatherLocLong').show();
    }
  }

  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
    // Battery show from
    if (clayConfig.getItemByMessageKey('cfgBatteryAccentFrom')) { // Only available on color platforms
      var batteryShowFrom = clayConfig.getItemByMessageKey('cfgBatteryShowFrom');
      batteryShowFrom.on('change', resizeBatteryAccent);
      resizeBatteryAccent();
    }
    
    // Weather provider
    var weatherProvider = clayConfig.getItemByMessageKey('cfgWeatherProvider');
    showWeatherProviderKey.call(weatherProvider);
    weatherProvider.on('change', showWeatherProviderKey);
    
    // MasterKey popup
    var masterKeyPopup = clayConfig.getItemByMessageKey('cfgWeatherMasterKey');
    masterKeyPopup.on('receive', masterKeyReceived);
    
    // Constant location
    var phoneLocation = clayConfig.getItemByMessageKey('cfgWeatherLocPhone');
    showLonLat.call(phoneLocation);
    phoneLocation.on('change', showLonLat);
  });
};