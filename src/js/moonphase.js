// Based on http://www.aphayes.16mb.com/library/moon.js
function dayno(year,month,day,hours) {
  // Day number is a modified Julian date, day 0 is 2000 January 0.0
  // which corresponds to a Julian date of 2451543.5
  var d= 367*year-Math.floor(7*(year+Math.floor((month+9)/12))/4)+Math.floor((275*month)/9)+day-730530+hours/24;
  return d;
}
function cosd(angle){return Math.cos((angle*Math.PI)/180.0);}
function sind(angle){return Math.sin((angle*Math.PI)/180.0);}
function rev(angle){return angle-Math.floor(angle/360.0)*360.0;}

function MoonPhase(year,month,day,hours) {
  // the illuminated percentage from Meeus chapter 46
  var j=dayno(year,month,day,hours)+2451543.5;
  var T=(j-2451545.0)/36525;
  var T2=T*T;
  var T3=T2*T;
  var T4=T3*T;
  // Moons mean elongation Meeus first edition
  // var D=297.8502042+445267.1115168*T-0.0016300*T2+T3/545868.0-T4/113065000.0;
  // Moons mean elongation Meeus second edition
  var D=297.8501921+445267.1114034*T-0.0018819*T2+T3/545868.0-T4/113065000.0; 
  // Moons mean anomaly M' Meeus first edition
  // var MP=134.9634114+477198.8676313*T+0.0089970*T2+T3/69699.0-T4/14712000.0;
  // Moons mean anomaly M' Meeus second edition
  var MP=134.9633964+477198.8675055*T+0.0087414*T2+T3/69699.0-T4/14712000.0;
  // Suns mean anomaly
  var M=357.5291092+35999.0502909*T-0.0001536*T2+T3/24490000.0;
  // phase angle
  var pa=180.0-D-6.289*sind(MP)+2.1*sind(M)-1.274*sind(2*D-MP)-0.658*sind(2*D)-0.214*sind(2*MP)-0.11*sind(D);
  return(rev(pa));
}

// Returns moonphase in degrees where 0 is full moon, 180 is new moon and 360 is once again full moon
function getMoonPhase() {
  var now = new Date();
  var year = now.getFullYear(now);
  var month = now.getMonth()+1;
  var day = now.getDate();
  var hour = now.getHours();
  var tz = now.getTimezoneOffset() / 60;
  
  var mp = MoonPhase(year, month, day, hour + tz);
  
  return { 
    phase: Math.round(mp),
    illumination: Math.round(100.0*(1.0+cosd(mp))/2.0)
  };
}

module.exports = getMoonPhase;  