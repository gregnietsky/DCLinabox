// loadinabox.js
//
// Reads parameters and then dynamically loads JavaScript and style-sheet
// files from the <base href=..> location by appending script and style
// children to the <head>..</head> element in the required order.
//
// Copyright (C) 2011,2012 Mark G Daniel <mark.daniel@wasd.vsm.com.au>
//
// 10-JAN-2012  MGD  initial

////////////
// functions
////////////

// get the value from the parent/opener, this window (config), or default
// so local page-defined values override config file defined values
function getParameter (name,value) {
   // order is significant; opener then parent then window
   if (opener && typeof opener[name] != "undefined")
      window[name] = opener[name];
   else
   if (parent && typeof parent[name] != "undefined")
      window[name] = parent[name];
   else
   if (typeof window[name] == "undefined")
      window[name] = value;
}

// load one or more JavaScript files IN-ORDER!
// (dynamic scripting seems to load asynchronously and execute when fully 
// loaded making execution order indeterminate and unsuitable for this app)
function loadJS (nameList) {
   var names = nameList.split(',');
   var namesCount = names.length;
   var fileobj = document.createElement('script');
   fileobj.setAttribute('type','text/javascript');
   fileobj.setAttribute('src',names[0]);
   var onLoad = 'loadJS("';
   for (var cnt = 1; cnt < namesCount; cnt++) {
      if (cnt > 1) onLoad += ',';
      onLoad += names[cnt];
   }
   onLoad += '")';
   if (onLoad.length > 10) fileobj.setAttribute('onload',onLoad);
   document.getElementsByTagName('head')[0].appendChild(fileobj);
}

// load a CSS file
function loadCSS (filename) {
   var fileobj = document.createElement('link');
   fileobj.setAttribute('rel','stylesheet');
   fileobj.setAttribute('type','text/css');
   fileobj.setAttribute('href',filename);
   document.getElementsByTagName('head')[0].appendChild(fileobj);
}

///////////////
// in-line code
///////////////

// get any locally configured document base
getParameter('DCLinaboxBase','/dclinabox/-/');
// set the default base href dynamically
document.getElementsByTagName('base')[0].href = DCLinaboxBase;

// any locally specified configuration file
getParameter('DCLinaboxConfig','');

// any locally specified style file
getParameter('DCLinaboxStyle','');

// dynamically load the required JavaScipt and style files

var loadNames = '';

// if specified load the local configuration JavaScript
if (DCLinaboxConfig.length) loadNames += DCLinaboxConfig + ',';

// load the VT100 emulator core JavaScript
loadNames += 'vt100.js' + ',';

// load the DCLianbox core JavaScript
loadNames += 'dclinabox.js';

loadJS(loadNames);

// load the ShellInABox styles
loadCSS('styles.css');

// load the DCLinabox styles
loadCSS('dclinabox.css');

// if specified load the DCLinabox customised styles
if (DCLinaboxStyle.length) loadCSS(DCLinaboxStyle);

//////
// end
//////
