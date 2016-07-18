// To escape/unescape template and style use http://www.freeformatter.com/javascript-escape.html
module.exports = {
  name: 'dateformat',
  template: '<div class=\"component-dateformat component\">\r\n  <label class=\"tap-highlight\">\r\n    <input\r\n      data-manipulator-target\r\n      type=\"hidden\"\r\n    \/>\r\n    <span class=\"label\">{{{label}}}<\/span>\r\n    <span class=\"value\"><\/span>\r\n  <\/label>\r\n  {{if description}}\r\n    <div class=\"description\">{{{description}}}<\/div>\r\n  {{\/if}}\r\n  <div class=\"dateformat-overlay\">\r\n    <div class=\"dateformat-popup\">\r\n      <h2>Date Format<\/h2>\r\n      <div class=\"targetZone\" id=\"targetZone\"><\/div> \r\n      Drag elements from below to the zone above to form your custom date format.\r\n      <table>\r\n        <tr>\r\n          <td>Day<\/td>\r\n          <td id=\"sourceZone\"><span class=\"element\" value=\"e\">1<\/span><span class=\"element\" value=\"d\">01<\/span><span class=\"element\" value=\"a\">Fri<\/span><span class=\"element\" value=\"A\">Friday<\/span><\/td>\r\n        <\/tr>\r\n        <tr>\r\n          <td>Month<\/td>\r\n          <td id=\"sourceZone\"><span class=\"element\" value=\"n\">3<\/span><span class=\"element\" value=\"m\">03<\/span><span class=\"element\" value=\"b\">Mar<\/span><span class=\"element\" value=\"B\">March<\/span><\/td>\r\n        <tr>\r\n        <tr>\r\n          <td>Year<\/td>\r\n          <td id=\"sourceZone\"><span class=\"element\" value=\"y\">16<\/span><span class=\"element\" value=\"Y\">2016<\/span><\/td>\r\n        <tr>\r\n        <tr>\r\n          <td>Separator<\/td>\r\n          <td id=\"sourceZone\"><span class=\"element\" value=\" \">&nbsp;<\/span><span class=\"element\" value=\".\">.<\/span><span class=\"element\" value=\"-\">-<\/span><span class=\"element\" value=\"\/\">\/<\/span><span class=\"element\" value=\"\'\">\'<\/span><span class=\"element\" value=\"|\">|<\/span><span class=\"element\" value=\"(\">(<\/span><span class=\"element\" value=\")\">)<\/span><\/td>\r\n        <tr>\r\n        <tr>\r\n          <td class=\"or\">OR<\/td>\r\n          <td id=\"sourceZone\"><span class=\"element bar\" value=\"z\">MO TU WE <u>FR<\/u> <b>SA SU<\/b><\/span><\/td>\r\n        <tr>\r\n        <tr>\r\n          <td class=\"or\">OR<\/td>\r\n          <td id=\"sourceZone\"><span class=\"element bar\" value=\"Z\"><b>SU<\/b> MO TU WE <u>FR<\/u> <b>SA<\/b><\/span><\/td>\r\n        <tr>\r\n      <\/table>\r\n      <div class=\"footer\">\r\n        <button type=\"button\" class=\"primary button\" id=\"setButton\">Set<\/button>\r\n        <button type=\"button\" class=\"button\" id=\"cancelButton\">Cancel<\/button>\r\n      <\/div>\r\n  <\/div>\r\n<\/div>', 
  style: '.section .component-dateformat {\r\n    padding: 0;\r\n}\r\n\r\n.component-dateformat .value {\r\n    background-color: #333333;\r\n    padding: 0rem 0.4rem;\r\n    width: 11rem;\r\n    height: 1.4rem;\r\n    border-radius: 0.7rem;\r\n    box-shadow: 0 0.1rem 0.1rem #2f2f2f;\r\n    display: block;\r\n    text-align: center;\r\n    overflow: hidden;\r\n}\r\n\r\n.component-dateformat .value b {\r\n    font-weight: bold;\r\n}\r\n\r\n.dateformat-overlay {\r\n    left: 0;\r\n    top: 0;\r\n    right: 0;\r\n    bottom: 0;\r\n    position: fixed;\r\n    padding: 0.7rem 0.375rem;\r\n    background: rgba(0, 0, 0, 0.65);\r\n    opacity: 0;\r\n    -webkit-transition: opacity 100ms ease-in 175ms;\r\n    transition: opacity 100ms ease-in 175ms;\r\n    pointer-events: none;\r\n    z-index: 100;\r\n    display: -webkit-box;\r\n    display: -webkit-flex;\r\n    display: flex;\r\n    -webkit-box-orient: vertical;\r\n    -webkit-box-direction: normal;\r\n    -webkit-flex-direction: column;\r\n    flex-direction: column;\r\n    -webkit-box-pack: center;\r\n    -webkit-justify-content: center;\r\n    justify-content: center;\r\n    -webkit-box-align: center;\r\n    -webkit-align-items: center;\r\n    align-items: center;\r\n}\r\n\r\n.dateformat-popup {\r\n    padding: 0.7rem 0.75rem;\r\n    background: #484848;\r\n    box-shadow: 0 0.17647rem 0.88235rem rgba(0, 0, 0, 0.4);\r\n    border-radius: 0.25rem;\r\n    width: 100%;\r\n    max-width: 26rem;\r\n    overflow: auto;\r\n    text-align: left;\r\n}\r\n\r\ntable {\r\n    margin-top: 0.7rem;\r\n}\r\n\r\n.dateformat-overlay.show {\r\n    -webkit-transition-delay: 0ms;\r\n    transition-delay: 0ms;\r\n    pointer-events: auto;\r\n    opacity: 1;\r\n}\r\n\r\n.component-dateformat .dateformat-popup .targetZone {\r\n    background-color: #333333;\r\n    margin: 0.7rem 0px;\r\n    padding-bottom: 0.25rem;\r\n    min-height: 2.30rem;\r\n    border-radius: 0.25rem;\r\n}\r\n\r\n.component-dateformat .dateformat-popup .or {\r\n    text-align: center;    \r\n}\r\n\r\n.component-dateformat .dateformat-popup .element {\r\n    background-color: #0e90d2; \/* #00B293; *\/\r\n    border-radius: 0.25rem;\r\n    padding: 0rem 0.5rem;\r\n    cursor: move;\r\n    cursor: grab;\r\n    cursor: -moz-grab;\r\n    cursor: -webkit-grab;\r\n    margin-left: 0.25rem;\r\n    margin-top: 0.25rem;\r\n    font-size: 120%;\r\n    display: inline-block;\r\n    white-space: nowrap;\r\n}\r\n\r\n.component-dateformat .dateformat-popup .element.hide {\r\n    display: none;\r\n}\r\n\r\n.component-dateformat .dateformat-popup .element b {\r\n    font-weight: bold;\r\n}\r\n\r\n\r\n.component-dateformat .dateformat-popup .bar {\r\n    font-size: 100%;\r\n}\r\n\r\n.component-dateformat .dateformat-popup #targetZone .bar {\r\n    font-size: 120%;\r\n}\r\n\r\n.component-dateformat .dateformat-popup button {\r\n    margin-top: 0.7rem;\r\n    min-width: 8rem;\r\n}\r\n\r\n.component-dateformat .footer {\r\n    text-align: center;\r\n}\r\n\r\n\/* Dragula CSS *\/\r\n.gu-mirror {\r\n  position: fixed !important;\r\n  margin: 0 !important;\r\n  z-index: 9999 !important;\r\n  opacity: 0.8;\r\n  -ms-filter: \"progid:DXImageTransform.Microsoft.Alpha(Opacity=80)\";\r\n  filter: alpha(opacity=80);\r\n}\r\n.gu-hide {\r\n  display: none !important;\r\n}\r\n.gu-unselectable {\r\n  -webkit-user-select: none !important;\r\n  -moz-user-select: none !important;\r\n  -ms-user-select: none !important;\r\n  user-select: none !important;\r\n}\r\n.gu-transit {\r\n  opacity: 0.2;\r\n  -ms-filter: \"progid:DXImageTransform.Microsoft.Alpha(Opacity=20)\";\r\n  filter: alpha(opacity=20);\r\n}', 
  defaults: {
    description: ""
  },
  manipulator: {     
    fillTargetAndPreview: function() {
      var value = this.get();
      var $targetZone = this.$element.select("#targetZone")[0];
      
      // Clear target zone
      $targetZone.innerHTML = "";
      
      // Fill target zone 
      var i;
      function testTargetValue(el, index) { 
        if (value.charAt(i) == el.getAttribute('value')) {
          $targetZone.appendChild(el.cloneNode(true));
        }
      }  
      for ( i = 0; i < value.length; i++ )
      {
        this.$element.select("#sourceZone .element").each(testTargetValue);
      }
      
      // Fill preview
      var preview = "";
      this.$element.select("#targetZone .element").each(function(el, index) { preview = preview + el.innerHTML; });
      this.$element.select(".value")[0].innerHTML = preview;
    },
    get: function() {
      return this.$manipulatorTarget.get('value')  || "";
    },
    set: function(value) {
      this.$manipulatorTarget.set('value', value); 
      this.fillTargetAndPreview();
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
    var $popupoverlay = $elem.select('.dateformat-overlay');    
    var drake; // Dragula object
    
    function hidePopup() {
      $popupoverlay.set('-show');
      if (drake) drake.destroy(); 
    }
        
    // Hide when clicked on the overlay
    $popupoverlay.on('click', hidePopup);
    
    // Ignore clicks on the popup
    var $popup = $popupoverlay.select('.dateformat-popup');        
    $popup.on('click', function() {  });
    
    // Show popup when component is clicked
    var $popupButton = $elem.select('label'); 
    $popupButton.on('click', function() {   
      // Show popup
      $popupoverlay.set('show');  
      
      // Webpack version of Dragula
      var dragula=function(e){function n(r){if(t[r])return t[r].exports;var o=t[r]={exports:{},id:r,loaded:!1};return e[r].call(o.exports,o,o.exports,n),o.loaded=!0,o.exports}var t={};return n.m=e,n.c=t,n.p="",n(0)}([function(e,n,t){(function(n){"use strict";function r(e,n){function t(e){return fe.containers.indexOf(e)!==-1||le.isContainer(e)}function r(e){var n=e?"remove":"add";o(x,n,"mousedown",C),o(x,n,"mouseup",P)}function c(e){var n=e?"remove":"add";o(x,n,"mousemove",I)}function p(e){var n=e?"remove":"add";b[n](x,"selectstart",S),b[n](x,"click",S)}function g(){r(!0),P({})}function S(e){ce&&e.preventDefault()}function C(e){ne=e.clientX,te=e.clientY;var n=1!==i(e)||e.metaKey||e.ctrlKey;if(!n){var t=e.target,r=N(t);r&&(ce=r,c(),"mousedown"===e.type&&(m(t)?t.focus():e.preventDefault()))}}function I(e){if(ce){if(0===i(e))return void P({});if(void 0===e.clientX||e.clientX!==ne||void 0===e.clientY||e.clientY!==te){if(le.ignoreInputTextSelection){var n=y("clientX",e),t=y("clientY",e),r=E.elementFromPoint(n,t);if(m(r))return}var o=ce;c(!0),p(),A(),X(o);var a=u(W);Z=y("pageX",e)-a.left,ee=y("pageY",e)-a.top,T.add(ie||W,"gu-transit"),z(),j(e)}}}function N(e){if(!(fe.dragging&&J||t(e))){for(var n=e;d(e)&&t(d(e))===!1;){if(le.invalid(e,n))return;if(e=d(e),!e)return}var r=d(e);if(r&&!le.invalid(e,n)){var o=le.moves(e,r,n,h(e));if(o)return{item:e,source:r}}}}function O(e){return!!N(e)}function _(e){var n=N(e);n&&X(n)}function X(e){q(e.item,e.source)&&(ie=e.item.cloneNode(!0),fe.emit("cloned",ie,e.item,"copy")),Q=e.source,W=e.item,re=oe=h(e.item),fe.dragging=!0,fe.emit("drag",W,Q)}function Y(){return!1}function A(){if(fe.dragging){var e=ie||W;L(e,d(e))}}function B(){ce=!1,c(!0),p(!0)}function P(e){if(B(),fe.dragging){var n=ie||W,t=y("clientX",e),r=y("clientY",e),o=a(J,t,r),i=M(o,t,r);i&&(ie&&le.copySortSource||!ie||i!==Q)?L(n,i):le.removeOnSpill?D():R()}}function L(e,n){var t=d(e);ie&&le.copySortSource&&n===Q&&t.removeChild(W),F(n)?fe.emit("cancel",e,Q,Q):fe.emit("drop",e,n,Q,oe),k()}function D(){if(fe.dragging){var e=ie||W,n=d(e);n&&n.removeChild(e),fe.emit(ie?"cancel":"remove",e,n,Q),k()}}function R(e){if(fe.dragging){var n=arguments.length>0?e:le.revertOnSpill,t=ie||W,r=d(t),o=F(r);o===!1&&n&&(ie?r.removeChild(ie):Q.insertBefore(t,re)),o||n?fe.emit("cancel",t,Q,Q):fe.emit("drop",t,r,Q,oe),k()}}function k(){var e=ie||W;B(),H(),e&&T.rm(e,"gu-transit"),ue&&clearTimeout(ue),fe.dragging=!1,ae&&fe.emit("out",e,ae,Q),fe.emit("dragend",e),Q=W=ie=re=oe=ue=ae=null}function F(e,n){var t;return t=void 0!==n?n:J?oe:h(ie||W),e===Q&&t===re}function M(e,n,r){function o(){var o=t(i);if(o===!1)return!1;var u=V(i,e),c=$(i,u,n,r),a=F(i,c);return!!a||le.accepts(W,i,Q,c)}for(var i=e;i&&!o();)i=d(i);return i}function j(e){function n(e){fe.emit(e,l,ae,Q)}function t(){v&&n("over")}function r(){ae&&n("out")}if(J){e.preventDefault();var o=y("clientX",e),i=y("clientY",e),u=o-Z,c=i-ee;J.style.left=u+"px",J.style.top=c+"px";var l=ie||W,f=a(J,o,i),s=M(f,o,i),v=null!==s&&s!==ae;(v||null===s)&&(r(),ae=s,t());var m=d(l);if(s===Q&&ie&&!le.copySortSource)return void(m&&m.removeChild(l));var p,g=V(s,f);if(null!==g)p=$(s,g,o,i);else{if(le.revertOnSpill!==!0||ie)return void(ie&&m&&m.removeChild(l));p=re,s=Q}(null===p&&v||p!==l&&p!==h(l))&&(oe=p,s.insertBefore(l,p),fe.emit("shadow",l,s,Q))}}function K(e){T.rm(e,"gu-hide")}function U(e){fe.dragging&&T.add(e,"gu-hide")}function z(){if(!J){var e=W.getBoundingClientRect();J=W.cloneNode(!0),J.style.width=s(e)+"px",J.style.height=v(e)+"px",T.rm(J,"gu-transit"),T.add(J,"gu-mirror"),le.mirrorContainer.appendChild(J),o(x,"add","mousemove",j),T.add(le.mirrorContainer,"gu-unselectable"),fe.emit("cloned",J,W,"mirror")}}function H(){J&&(T.rm(le.mirrorContainer,"gu-unselectable"),o(x,"remove","mousemove",j),d(J).removeChild(J),J=null)}function V(e,n){for(var t=n;t!==e&&d(t)!==e;)t=d(t);return t===x?null:t}function $(e,n,t,r){function o(){var n,o,i,u=e.children.length;for(n=0;n<u;n++){if(o=e.children[n],i=o.getBoundingClientRect(),c&&i.left+i.width/2>t)return o;if(!c&&i.top+i.height/2>r)return o}return null}function i(){var e=n.getBoundingClientRect();return u(c?t>e.left+s(e)/2:r>e.top+v(e)/2)}function u(e){return e?h(n):n}var c="horizontal"===le.direction,a=n!==e?i():o();return a}function q(e,n){return"boolean"==typeof le.copy?le.copy:le.copy(e,n)}var G=arguments.length;1===G&&Array.isArray(e)===!1&&(n=e,e=[]);var J,Q,W,Z,ee,ne,te,re,oe,ie,ue,ce,ae=null,le=n||{};void 0===le.moves&&(le.moves=f),void 0===le.accepts&&(le.accepts=f),void 0===le.invalid&&(le.invalid=Y),void 0===le.containers&&(le.containers=e||[]),void 0===le.isContainer&&(le.isContainer=l),void 0===le.copy&&(le.copy=!1),void 0===le.copySortSource&&(le.copySortSource=!1),void 0===le.revertOnSpill&&(le.revertOnSpill=!1),void 0===le.removeOnSpill&&(le.removeOnSpill=!1),void 0===le.direction&&(le.direction="vertical"),void 0===le.ignoreInputTextSelection&&(le.ignoreInputTextSelection=!0),void 0===le.mirrorContainer&&(le.mirrorContainer=E.body);var fe=w({containers:le.containers,start:_,end:A,cancel:R,remove:D,destroy:g,canMove:O,dragging:!1});return le.removeOnSpill===!0&&fe.on("over",K).on("out",U),r(),fe}function o(e,t,r,o){var i={mouseup:"touchend",mousedown:"touchstart",mousemove:"touchmove"},u={mouseup:"pointerup",mousedown:"pointerdown",mousemove:"pointermove"},c={mouseup:"MSPointerUp",mousedown:"MSPointerDown",mousemove:"MSPointerMove"};n.navigator.pointerEnabled?b[t](e,u[r],o):n.navigator.msPointerEnabled?b[t](e,c[r],o):(b[t](e,i[r],o),b[t](e,r,o))}function i(e){if(void 0!==e.touches)return e.touches.length;if(void 0!==e.which&&0!==e.which)return e.which;if(void 0!==e.buttons)return e.buttons;var n=e.button;return void 0!==n?1&n?1:2&n?3:4&n?2:0:void 0}function u(e){var n=e.getBoundingClientRect();return{left:n.left+c("scrollLeft","pageXOffset"),top:n.top+c("scrollTop","pageYOffset")}}function c(e,t){return"undefined"!=typeof n[t]?n[t]:x.clientHeight?x[e]:E.body[e]}function a(e,n,t){var r,o=e||{},i=o.className;return o.className+=" gu-hide",r=E.elementFromPoint(n,t),o.className=i,r}function l(){return!1}function f(){return!0}function s(e){return e.width||e.right-e.left}function v(e){return e.height||e.bottom-e.top}function d(e){return e.parentNode===E?null:e.parentNode}function m(e){return"INPUT"===e.tagName||"TEXTAREA"===e.tagName||"SELECT"===e.tagName||p(e)}function p(e){return!!e&&("false"!==e.contentEditable&&("true"===e.contentEditable||p(d(e))))}function h(e){function n(){var n=e;do n=n.nextSibling;while(n&&1!==n.nodeType);return n}return e.nextElementSibling||n()}function g(e){return e.targetTouches&&e.targetTouches.length?e.targetTouches[0]:e.changedTouches&&e.changedTouches.length?e.changedTouches[0]:e}function y(e,n){var t=g(n),r={pageX:"clientX",pageY:"clientY"};return e in r&&!(e in t)&&r[e]in t&&(e=r[e]),t[e]}var w=t(1),b=t(7),T=t(10),E=document,x=E.documentElement;e.exports=r}).call(n,function(){return this}())},function(e,n,t){"use strict";var r=t(2),o=t(3);e.exports=function(e,n){var t=n||{},i={};return void 0===e&&(e={}),e.on=function(n,t){return i[n]?i[n].push(t):i[n]=[t],e},e.once=function(n,t){return t._once=!0,e.on(n,t),e},e.off=function(n,t){var r=arguments.length;if(1===r)delete i[n];else if(0===r)i={};else{var o=i[n];if(!o)return e;o.splice(o.indexOf(t),1)}return e},e.emit=function(){var n=r(arguments);return e.emitterSnapshot(n.shift()).apply(this,n)},e.emitterSnapshot=function(n){var u=(i[n]||[]).slice(0);return function(){var i=r(arguments),c=this||e;if("error"===n&&t["throws"]!==!1&&!u.length)throw 1===i.length?i[0]:i;return u.forEach(function(r){t.async?o(r,i,c):r.apply(c,i),r._once&&e.off(n,r)}),e}},e}},function(e,n){e.exports=function(e,n){return Array.prototype.slice.call(e,n)}},function(e,n,t){"use strict";var r=t(4);e.exports=function(e,n,t){e&&r(function(){e.apply(t||null,n||[])})}},function(e,n,t){(function(n){var t,r="function"==typeof n;t=r?function(e){n(e)}:function(e){setTimeout(e,0)},e.exports=t}).call(n,t(5).setImmediate)},function(e,n,t){(function(e,r){function o(e,n){this._id=e,this._clearFn=n}var i=t(6).nextTick,u=Function.prototype.apply,c=Array.prototype.slice,a={},l=0;n.setTimeout=function(){return new o(u.call(setTimeout,window,arguments),clearTimeout)},n.setInterval=function(){return new o(u.call(setInterval,window,arguments),clearInterval)},n.clearTimeout=n.clearInterval=function(e){e.close()},o.prototype.unref=o.prototype.ref=function(){},o.prototype.close=function(){this._clearFn.call(window,this._id)},n.enroll=function(e,n){clearTimeout(e._idleTimeoutId),e._idleTimeout=n},n.unenroll=function(e){clearTimeout(e._idleTimeoutId),e._idleTimeout=-1},n._unrefActive=n.active=function(e){clearTimeout(e._idleTimeoutId);var n=e._idleTimeout;n>=0&&(e._idleTimeoutId=setTimeout(function(){e._onTimeout&&e._onTimeout()},n))},n.setImmediate="function"==typeof e?e:function(e){var t=l++,r=!(arguments.length<2)&&c.call(arguments,1);return a[t]=!0,i(function(){a[t]&&(r?e.apply(null,r):e.call(null),n.clearImmediate(t))}),t},n.clearImmediate="function"==typeof r?r:function(e){delete a[e]}}).call(n,t(5).setImmediate,t(5).clearImmediate)},function(e,n){function t(){s&&l&&(s=!1,l.length?f=l.concat(f):v=-1,f.length&&r())}function r(){if(!s){var e=u(t);s=!0;for(var n=f.length;n;){for(l=f,f=[];++v<n;)l&&l[v].run();v=-1,n=f.length}l=null,s=!1,c(e)}}function o(e,n){this.fun=e,this.array=n}function i(){}var u,c,a=e.exports={};!function(){try{u=setTimeout}catch(e){u=function(){throw new Error("setTimeout is not defined")}}try{c=clearTimeout}catch(e){c=function(){throw new Error("clearTimeout is not defined")}}}();var l,f=[],s=!1,v=-1;a.nextTick=function(e){var n=new Array(arguments.length-1);if(arguments.length>1)for(var t=1;t<arguments.length;t++)n[t-1]=arguments[t];f.push(new o(e,n)),1!==f.length||s||u(r,0)},o.prototype.run=function(){this.fun.apply(null,this.array)},a.title="browser",a.browser=!0,a.env={},a.argv=[],a.version="",a.versions={},a.on=i,a.addListener=i,a.once=i,a.off=i,a.removeListener=i,a.removeAllListeners=i,a.emit=i,a.binding=function(e){throw new Error("process.binding is not supported")},a.cwd=function(){return"/"},a.chdir=function(e){throw new Error("process.chdir is not supported")},a.umask=function(){return 0}},function(e,n,t){(function(n){"use strict";function r(e,n,t,r){return e.addEventListener(n,t,r)}function o(e,n,t){return e.attachEvent("on"+n,l(e,n,t))}function i(e,n,t,r){return e.removeEventListener(n,t,r)}function u(e,n,t){var r=f(e,n,t);if(r)return e.detachEvent("on"+n,r)}function c(e,n,t){function r(){var e;return m.createEvent?(e=m.createEvent("Event"),e.initEvent(n,!0,!0)):m.createEventObject&&(e=m.createEventObject()),e}function o(){return new v(n,{detail:t})}var i=d.indexOf(n)===-1?o():r();e.dispatchEvent?e.dispatchEvent(i):e.fireEvent("on"+n,i)}function a(e,t,r){return function(t){var o=t||n.event;o.target=o.target||o.srcElement,o.preventDefault=o.preventDefault||function(){o.returnValue=!1},o.stopPropagation=o.stopPropagation||function(){o.cancelBubble=!0},o.which=o.which||o.keyCode,r.call(e,o)}}function l(e,n,t){var r=f(e,n,t)||a(e,n,t);return g.push({wrapper:r,element:e,type:n,fn:t}),r}function f(e,n,t){var r=s(e,n,t);if(r){var o=g[r].wrapper;return g.splice(r,1),o}}function s(e,n,t){var r,o;for(r=0;r<g.length;r++)if(o=g[r],o.element===e&&o.type===n&&o.fn===t)return r}var v=t(8),d=t(9),m=n.document,p=r,h=i,g=[];n.addEventListener||(p=o,h=u),e.exports={add:p,remove:h,fabricate:c}}).call(n,function(){return this}())},function(e,n){(function(n){function t(){try{var e=new r("cat",{detail:{foo:"bar"}});return"cat"===e.type&&"bar"===e.detail.foo}catch(n){}return!1}var r=n.CustomEvent;e.exports=t()?r:"function"==typeof document.createEvent?function(e,n){var t=document.createEvent("CustomEvent");return n?t.initCustomEvent(e,n.bubbles,n.cancelable,n.detail):t.initCustomEvent(e,!1,!1,void 0),t}:function(e,n){var t=document.createEventObject();return t.type=e,n?(t.bubbles=Boolean(n.bubbles),t.cancelable=Boolean(n.cancelable),t.detail=n.detail):(t.bubbles=!1,t.cancelable=!1,t.detail=void 0),t}}).call(n,function(){return this}())},function(e,n){(function(n){"use strict";var t=[],r="",o=/^on/;for(r in n)o.test(r)&&t.push(r.slice(2));e.exports=t}).call(n,function(){return this}())},function(e,n){"use strict";function t(e){var n=i[e];return n?n.lastIndex=0:i[e]=n=new RegExp(u+e+c,"g"),n}function r(e,n){var r=e.className;r.length?t(n).test(r)||(e.className+=" "+n):e.className=n}function o(e,n){e.className=e.className.replace(t(n)," ").trim()}var i={},u="(?:^|\\s)",c="(?:\\s|$)";e.exports={add:r,rm:o}}]);
      
      // Set up drag-drop using Dragula
      drake = dragula({
        isContainer: function(el) {
          return el.id === 'sourceZone' || (el.id === 'targetZone');
        },
        copy: function (el, source) {
          return source.id === 'sourceZone';
        },
        accepts: function(el, target, source, sibling) {
          return target.id === 'targetZone' && target.querySelectorAll(':not(.gu-transit)').length < 10;
        },
        removeOnSpill: true,
        mirrorContainer: $popup[0]
      }).on('over', function (el, container) {
        // Make sure bars are alone in the targetzone
        if (container.id === 'targetZone') {
          if (el.classList.contains('bar')) {
            // Inserting bar, hiding others
            for (var i = 0; i < container.children.length; i++) {
              if (container.children[i] !== el && !container.children[i].classList.contains('hide')) 
                container.children[i].classList.add('hide');
            }
          } else {
            // Inserting others, hiding bar 
            for (var i = 0; i < container.children.length; i++) {
              if (container.children[i].classList.contains('bar') && !container.children[i].classList.contains('hide')) 
                container.children[i].classList.add('hide');
            }    
          }
        }
      }).on('drop', function (el) {
        // Succesfull drop, removing hidden items
        var container = self.$element.select('#targetZone')[0];
        var i = 0;
        while (i < container.children.length) {
          if (container.children[i].classList.contains('hide')) 
            container.removeChild(container.children[i]);
          else
            i++;
        } 
      }).on('out', function (el, container) {
        // No drop, showing hidden items
        for (var i = 0; i < container.children.length; i++) {
          if (container.children[i].classList.contains('hide')) 
            container.children[i].classList.remove('hide');
        } 
      });
    });    
          
    // Set button                                                                                                                                                                                                                       
    var $receiveButton = $popup.select('#setButton');        
    $receiveButton.on('click', function() { 
      // Set value
      var value = "";
      $elem.select("#targetZone .element").each(function(el, index) { 
        value = value + el.getAttribute('value'); 
      });
      self.$manipulatorTarget.set('value', value);

      // Hide popup
      self.fillTargetAndPreview();
      hidePopup();                                                                                                                                                                                                        
    });
                                                                                                                                                                                                                         
    // Cancel button                                                                                                                                                                                                                         
    var $receiveButton = $popup.select('#cancelButton');        
    $receiveButton.on('click', function() {  
      // Hide popup
      hidePopup(); 
      self.fillTargetAndPreview();                                                                                                                                                                                                       
    });
  }
};
