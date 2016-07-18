// To escape/unescape template and style use http://www.freeformatter.com/javascript-escape.html
module.exports = {
  name: 'masterkey',
  template: '<div class=\"component-masterkey component\">\r\n  <button type=\"button\">Retrieve keys from Master Key<\/button>\r\n  <div class=\"description\">More info? See <a href=\"http:\/\/pmkey.xyz\" target=\"_blank\">pmkey.xyz<\/a><\/div>\r\n  <div class=\"masterkey-overlay\" data-manipulator-target>\r\n    <div class=\"masterkey-popup\">\r\n      <h2>Master Key<\/h2>\r\n      <label class=\"tap-highlight\">\r\n        <div><span class=\"label\">E-Mail<\/span><\/div>\r\n        <div>\r\n\t  <span class=\"input\">\r\n            <input placeholder=\"eg: john@doe.com\" type=\"email\" id=\"email\" \/>\r\n          <\/span>\r\n\t<\/div>\r\n      <\/label>\r\n      <label class=\"tap-highlight\">\r\n        <div><span class=\"label\">Pin Code<\/span><\/div>\r\n        <div>\r\n\t  <span class=\"input\">\r\n            <input placeholder=\"eg: 12345\" type=\"number\" id=\"pin\" \/>\r\n          <\/span>\r\n\t<\/div>\r\n      <\/label>\r\n      <div class=\"error\">Something went wrong. Sorry!<\/div>\r\n      <div class=\"footer\"><button type=\"button\" class=\"primary button\">Retrieve<\/button><\/div>\r\n    <\/div>\r\n  <\/div>\r\n<\/div>', 
  style: '.section .component-masterkey {\r\n    text-align: center;\r\n    padding-bottom: 0;\r\n}\r\n\r\n.component-masterkey .description {\r\n    padding-left: 0;\r\n    padding-right: 0;\r\n}\r\n\r\n.masterkey-overlay {\r\n    left: 0;\r\n    top: 0;\r\n    right: 0;\r\n    bottom: 0;\r\n    position: fixed;\r\n    padding: 0.7rem 0.375rem;\r\n    background: rgba(0, 0, 0, 0.65);\r\n    opacity: 0;\r\n    -webkit-transition: opacity 100ms ease-in 175ms;\r\n    transition: opacity 100ms ease-in 175ms;\r\n    pointer-events: none;\r\n    z-index: 100;\r\n    display: -webkit-box;\r\n    display: -webkit-flex;\r\n    display: flex;\r\n    -webkit-box-orient: vertical;\r\n    -webkit-box-direction: normal;\r\n    -webkit-flex-direction: column;\r\n    flex-direction: column;\r\n    -webkit-box-pack: center;\r\n    -webkit-justify-content: center;\r\n    justify-content: center;\r\n    -webkit-box-align: center;\r\n    -webkit-align-items: center;\r\n    align-items: center;\r\n}\r\n\r\n.masterkey-popup {\r\n    padding: 0.7rem 0.75rem;\r\n    background: #484848;\r\n    box-shadow: 0 0.17647rem 0.88235rem rgba(0, 0, 0, 0.4);\r\n    border-radius: 0.25rem;\r\n    width: 100%;\r\n    max-width: 26rem;\r\n    overflow: auto;\r\n    text-align: left;\r\n}\r\n\r\n.masterkey-overlay.show {\r\n    -webkit-transition-delay: 0ms;\r\n    transition-delay: 0ms;\r\n    pointer-events: auto;\r\n    opacity: 1;\r\n}\r\n\r\n.component-masterkey label {\r\n    display: block;\r\n}\r\n.component-masterkey .label {\r\n    padding-bottom: 0.7rem;\r\n}\r\n.component-masterkey .input {\r\n    position: relative;\r\n    min-width: 100%;\r\n    margin-top: 0.7rem;\r\n    margin-left: 0;\r\n}\r\n.component-masterkey input {\r\n    display: block;\r\n    width: 100%;\r\n    background: #333333;\r\n    border-radius: 0.25rem;\r\n    padding: 0.35rem 0.375rem;\r\n    border: none;\r\n    vertical-align: baseline;\r\n    color: #ffffff;\r\n    font-size: inherit;\r\n    -webkit-appearance: none;\r\n    appearance: none;\r\n    min-height: 2.1rem;\r\n}\r\n.component-masterkey input::-webkit-input-placeholder {\r\n    color: #858585;\r\n}\r\n.component-masterkey input::-moz-placeholder {\r\n    color: #858585;\r\n}\r\n.component-masterkey input:-moz-placeholder {\r\n    color: #858585;\r\n}\r\n.component-masterkey input:-ms-input-placeholder {\r\n    color: #858585;\r\n}\r\n.component-masterkey input:focus {\r\n    border: none;\r\n    box-shadow: none;\r\n}\r\n.component-masterkey input:focus::-webkit-input-placeholder {\r\n    color: #666666;\r\n}\r\n.component-masterkey input:focus::-moz-placeholder {\r\n    color: #666666;\r\n}\r\n.component-masterkey input:focus:-moz-placeholder {\r\n    color: #666666;\r\n}\r\n.component-masterkey input:focus:-ms-input-placeholder {\r\n    color: #666666;\r\n}\r\n\r\n.component-masterkey .error {\r\n    color: red;\r\n    padding: 0.35rem 0.375rem;\r\n    display: none;\r\n}\r\n\r\n.masterkey-overlay .error.show {\r\n    display: block;\r\n}\r\n\r\n.component-masterkey .popup button {\r\n    margin-top: 0.7rem;\r\n}\r\n\r\n.component-masterkey .footer {\r\n    text-align: center;\r\n}', 
  manipulator: {     
    get: function() {
      var $email = this.$element.select('#email');
      var $pin = this.$element.select('#pin');
      return $email.get('value') + ';' + $pin.get('value');
    },
    set: function(value) {
      if (value.length > 0 && value.indexOf(';') > -1) {
        var $email = this.$element.select('#email');
        var $pin = this.$element.select('#pin');
        var split = value.split(';');
        $email.set('value', split[0]);
        $pin.set('value', split[1]);
      }
      return this; 
    },
    hide: function(value) {
      if (this.$element[0].classList.contains('hide')) { return this; }
      this.$element.set('+hide');
      return this.trigger('hide');
    },
    show: function(value) {
      if (!this.$element[0].classList.contains('hide')) { return this; }
      this.$element.set('-hide');
      return this.trigger('show');
    },
  },
  initialize: function(minified, clay) {
    var self = this;
    var $elem = self.$element;
    var $popupoverlay = $elem.select('.masterkey-overlay');    
    
    // Hide when clicked on the overlay
    $popupoverlay.on('click', function() {
      $popupoverlay.set('-show');
    });
    
    // Ignore clicks on the popup
    var $popup = $popupoverlay.select('.masterkey-popup');        
    $popup.on('click', function() {  });
    
    // Show popup when button is clicked
    var $popupButton = $elem.select('button', true); 
    $popupButton.on('click', function() {   
      $popupoverlay.set('show');    
    });    
        
    // Receive keys from MasterKey when button clicked
    var $email = $popup.select('#email');
    var $pin = $popup.select('#pin');
    var $receiveButton = $popup.select('button');        
    $receiveButton.on('click', function() {  
      function showError(error) {
        var $error = $popup.select('.error');  
        $error.set('innerHTML', error);
        $error.set('+show');    
      }
      
      function hideError() {
        var $error = $popup.select('.error');  
        $error.set('-show');
      }
      
      var email = $email.get('value');
      var pin = $pin.get('value');
      hideError();
      
      if (email.length === 0 || pin.length === 0) {
        showError("Please provide email and pin");
        return;
      }  
        
      // Send request to master key service
      $receiveButton.set('innerHTML', 'Retrieving ...');
      var xhr = new XMLHttpRequest();
      var url = "https://pmkey.xyz/search/?email=" + email + "&pin=" + pin;
      xhr.open("GET", url, true);
      xhr.onreadystatechange = function(){        
        if(xhr.readyState == 4) {  
          // The results are in         
          if (xhr.status == 200 ){
            var result = JSON.parse(xhr.responseText);
            if(result.success)
            {
              // Return results in receive event
              self.trigger('receive', result);
              
              // Hide the popup
              $popupoverlay.set('-show');
            } else {
              showError("Keys not found for these credentials.");
              $receiveButton.set('innerHTML', 'Retrieve');
            }
          } else {            
              showError("Error in the Master Key service.");
              $receiveButton.set('innerHTML', 'Retrieve');
          }
        }
      };
      xhr.send();            
    });
  }
};
