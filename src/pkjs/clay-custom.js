module.exports = function(minified) {
  var clayConfig = this;
  
  function showSwitcherOptions() {
    if (this.get() === "M") { // Minute only
      clayConfig.getItemByMessageKey('cfgAnimateSwitcher').hide();
      this.$element.select('div').set('+hide'); // Hide description
    } else {
      clayConfig.getItemByMessageKey('cfgAnimateSwitcher').show();
      this.$element.select('div').set('-hide'); // Show description
    }
  }
  
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
  
  function countdownToChanged() {
    var value = this.options[this.selectedIndex].value;
    var text = this.options[this.selectedIndex].text;

    // Set label text
    this.previousElementSibling.innerHTML = text;
    
    // Show/hide date and time input
    var labelInput = this.parentNode.parentNode.previousElementSibling.getElementsByTagName("input")[0];
    var time = this.parentNode.parentNode.nextElementSibling;
    var date = time.nextElementSibling;
    if (value === "D") { // To date
      time.classList.add('hide');
      date.classList.remove('hide');
      labelInput.setAttribute('placeholder', 'eg: Holiday');
    } else if (value === "T") { // To time and date 
      time.classList.remove('hide');
      date.classList.remove('hide');
      labelInput.setAttribute('placeholder', 'eg: Party!');
    } else { // To time every day 
      time.classList.remove('hide');
      date.classList.add('hide');
      labelInput.setAttribute('placeholder', 'eg: Go home');
    }
  }
  
  function showCountdownRemoveButtons() {
    var countdownSection = clayConfig.getItemById('CountdownAddButton').$element[0].parentNode;
    var show = Math.floor(countdownSection.childElementCount / 5) > 1;
    for (var i = 5; i < countdownSection.childElementCount; i = i + 5) {
      var removeButton = countdownSection.children[i];
      if (show) {
        removeButton.classList.remove('hide');
      } else {
        removeButton.classList.add('hide');
      }
    }
  }
  
  function removeCountdown() {
    var removeButtonElement = this.parentNode;
    var dateElement = removeButtonElement.previousSibling;
    var timeElement = dateElement.previousSibling;
    var countdownToElement = timeElement.previousSibling;
    var labelElement = countdownToElement.previousSibling;
    
    removeButtonElement.parentNode.removeChild(labelElement);
    removeButtonElement.parentNode.removeChild(countdownToElement);
    removeButtonElement.parentNode.removeChild(timeElement);
    removeButtonElement.parentNode.removeChild(dateElement);
    removeButtonElement.parentNode.removeChild(removeButtonElement);
    
    showCountdownRemoveButtons();
  }
  
  function addCountdown() {
    var addButtonElement = clayConfig.getItemById('CountdownAddButton').$element[0];
    var removeButtonElement = addButtonElement.previousSibling;
    var dateElement = removeButtonElement.previousSibling;
    var timeElement = dateElement.previousSibling;
    var countdownToElement = timeElement.previousSibling;
    var labelElement = countdownToElement.previousSibling;
    
    removeButtonElement = removeButtonElement.cloneNode(true);
    dateElement = dateElement.cloneNode(true);
    timeElement = timeElement.cloneNode(true);
    countdownToElement = countdownToElement.cloneNode(true);
    labelElement = labelElement.cloneNode(true);
    
    labelElement.getElementsByTagName("input")[0].value = "";
    countdownToElement.getElementsByTagName("select")[0].value = "D";
    countdownToElement.getElementsByTagName("select")[0].addEventListener('change', countdownToChanged); 
    timeElement.getElementsByTagName("input")[0].value = "";
    dateElement.getElementsByTagName("input")[0].value = "";
    removeButtonElement.getElementsByTagName("button")[0].addEventListener('click', removeCountdown); 
    
    addButtonElement.parentNode.insertBefore(labelElement, addButtonElement);
    addButtonElement.parentNode.insertBefore(countdownToElement, addButtonElement);
    addButtonElement.parentNode.insertBefore(timeElement, addButtonElement);    
    addButtonElement.parentNode.insertBefore(dateElement, addButtonElement);
    addButtonElement.parentNode.insertBefore(removeButtonElement, addButtonElement);
        
    countdownToChanged.call(countdownToElement.getElementsByTagName("select")[0]);
    showCountdownRemoveButtons();
  }
  
  function submitCountdown() {
    var countdownLabel = clayConfig.getItemByMessageKey('cfgCountdownLabel');
    
    var countdownSection = countdownLabel.$element[0].parentNode;
    var count = Math.floor(countdownSection.childElementCount / 5);
    var countdowns = [ ];
    for (var i = 0; i < count; ++i) {
      var labelElement = countdownSection.children[1 + i * 5].getElementsByTagName("input")[0];
      var countdownToElement = countdownSection.children[2 + i * 5].getElementsByTagName("select")[0];
      var timeElement = countdownSection.children[3 + i * 5].getElementsByTagName("input")[0];
      var dateElement = countdownSection.children[4 + i * 5].getElementsByTagName("input")[0];
      
      countdowns.push({
        label: labelElement.value,
        countdownTo: countdownToElement.options[countdownToElement.selectedIndex].value,
        time: timeElement.value,
        date: dateElement.value
      });
    }
    
    // Set value
    countdownLabel.$element.select("input")[0].removeAttribute("maxlength");
    countdownLabel.set(JSON.stringify(countdowns));
  }
  
  function loadCountdowns() {    
    try {
      // Parse countdown configuration from countdown label
      var countdownLabel = clayConfig.getItemByMessageKey('cfgCountdownLabel');
      var countdowns = JSON.parse(countdownLabel.get());
      countdownLabel.set("");
      
      var countdownAdd = clayConfig.getItemById('CountdownAddButton');
      var addButtonElement = countdownAdd.$element[0];
      
      for (var j = 0; j < countdowns.length; ++j) {
        var removeButtonElement = addButtonElement.previousSibling;
        var dateElement = removeButtonElement.previousSibling;
        var timeElement = dateElement.previousSibling;
        var countdownToElement = timeElement.previousSibling;
        var labelElement = countdownToElement.previousSibling;
            
        labelElement.getElementsByTagName("input")[0].value = countdowns[j].label;
        countdownToElement.getElementsByTagName("select")[0].value = countdowns[j].countdownTo; 
        countdownToChanged.call(countdownToElement.getElementsByTagName("select")[0]);
        timeElement.getElementsByTagName("input")[0].value = countdowns[j].time;
        dateElement.getElementsByTagName("input")[0].value = countdowns[j].date;
        
        if (j < countdowns.length - 1) {
          addCountdown();
        }          
      }
    } catch (e) {
      // The countdownlabel does not contain JSON, first time or previous version
    }
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
    // Enabling tidbits
    var enableSection = clayConfig.getItemByMessageKey('cfgEnableBattery').$element[0].parentElement;
    var enableCheckboxes = enableSection.querySelectorAll('.component-toggle input');
    var i = 0;
    var siblingSection = enableSection.nextElementSibling;
    var happyEnableDiv = clayConfig.getItemByMessageKey('cfgEnableHappy').$element[0];
    var now = new Date();
    var firstDayOfYear = now.getMonth() === 0 && now.getDate() === 1;
    while (siblingSection.classList.contains('section')) {
      var checkbox = enableCheckboxes[i];
      
      // Add event listener to each tidbit enable checkbox to hide the relevant tidbit section
      checkbox.addEventListener('change', (function(section, chkbox) {
        return function() {
          if (chkbox.checked) {
            section.classList.remove('hide');
          } else {
            section.classList.add('hide');
          }
        };
      })(siblingSection, checkbox));
      
      // Initialize section visibility
      if (checkbox.checked) {
        siblingSection.classList.remove('hide');
      } else {
        siblingSection.classList.add('hide');
      }
      
      // Only show Happy on the first day of the year
      if (checkbox == happyEnableDiv.querySelectorAll('input')[0] && !firstDayOfYear) {
        happyEnableDiv.classList.add('hide');
        siblingSection.classList.add('hide');
      }
      
      ++i;
      siblingSection = siblingSection.nextElementSibling;
    }
    
    // Show/hide swicher options
    var alternateMode = clayConfig.getItemByMessageKey('cfgAlternateMode');
    showSwitcherOptions.call(alternateMode);
    alternateMode.on('change', showSwitcherOptions);
        
    // Battery show from
    if (clayConfig.getItemByMessageKey('cfgBatteryAccentFrom')) { // Only available on color platforms
      var batteryShowFrom = clayConfig.getItemByMessageKey('cfgBatteryShowFrom');
      batteryShowFrom.on('change', resizeBatteryAccent);
      resizeBatteryAccent();
    }
    
    // Countdown show/hide of date and time
    var countdownTo = clayConfig.getItemByMessageKey('cfgCountdownTo');
    var countdownToSelect = countdownTo.$element[0].getElementsByTagName("select")[0];
    countdownToChanged.call(countdownToSelect);
    countdownToSelect.addEventListener('change', countdownToChanged); 
        
    // Countdown remove buttons
    var countdownRemove = clayConfig.getItemById('CountdownRemoveButton');
    countdownRemove.$element[0].getElementsByTagName("button")[0].addEventListener('click', removeCountdown); 
    showCountdownRemoveButtons();
    
    // Countdown add button
    var countdownAdd = clayConfig.getItemById('CountdownAddButton');
    countdownAdd.on('click', addCountdown); 
    
    // Countdown submit serialization
    var submitElement = countdownAdd.$element[0].ownerDocument.querySelector("button[type=submit]");
    submitElement.addEventListener("click", submitCountdown);
    
    // Load/parse countdowns
    loadCountdowns();
        
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