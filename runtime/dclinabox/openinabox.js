// openinabox.js
//
// Opens a standalone terminal and provides a function called by the IFRAME
// child that resizes the IFRAME to contain the terminal in the parent window.
//
// Copyright (C) 2011,2012 Mark G Daniel <mark.daniel@wasd.vsm.com.au>
//
// 10-JAN-2012  MGD  initial

if (typeof DCLinaboxHost == 'undefined')
   var DCLinaboxHost = window.location.host;

///////////////////////////////////
// open a popup standalone terminal
///////////////////////////////////

var openDCLinaboxTop = 999;
var openDCLinaboxLeft = 999;
var openDCLinaboxHost = null;

function openDCLinabox(host) {

   if (host && host.length) {
      if (!openDCLinaboxHost) openDCLinaboxHost = DCLinaboxHost;
      DCLinaboxHost = host;
   }
   else
   if (openDCLinaboxHost) {
      DCLinaboxHost = openDCLinaboxHost;
      openDCLinaboxHost = null;
   }

   // get the terminal page from the local resources
   var URL = window.location.protocol + '//' + window.location.host;
   var port = window.location.port;
   if (!(port == 80 || port == 443)) URL += ':' + port; 
   URL += '/dclinabox/-/dclinabox.html';

   // each offset from the previous terminal
   if (openDCLinaboxTop > 500)
      openDCLinaboxTop = 200;
   else
      openDCLinaboxTop += 20;
   if (openDCLinaboxLeft > 500)
      openDCLinaboxLeft = 200;
   else
      openDCLinaboxLeft += 20;

   var specs = 'toolbar=0,location=0,directories=0,status=0,menubar=0,' +
               'scrollbars=0,resizable=0,copyhistory=0,width=100,height=100,' +
               'top=' + openDCLinaboxTop + ',left=' + openDCLinaboxLeft;

   window.open(URL,'_blank',specs);

   return false;
}

/////////////////////////
// resize (parent) iframe
/////////////////////////

// called by DCLINABOX.JS in a child frame when an embedded terminal page

function resizeDCLinaboxIframe(fobj,width,height) {
   fobj.style.borderWidth = 0;
   fobj.width = width;
   fobj.height = height;
}

////////////////////////
// (parent) window title
////////////////////////

// called by DCLINABOX.JS in a child frame when an embedded terminal page
// this function can also be found in DCLINABOX.JS

function setDCLinaboxTitle (title) {
   if (typeof title == 'undefined' ||
       !title.length ||
       title.substr(0,1) == '?')
      title = 'DCLinabox: ' + DCLinaboxHost;
   document.getElementsByTagName('title')[0].innerHTML = title;
}

/////////////////////
// dynamic hosts list
/////////////////////

function selectDCLinaboxHost (hosts) {
   hosts = hosts.split(',');
   if (hosts.length) {
      var select = '<select id="selectTheDCLinaboxHost" ' +
                   'onchange="setDCLinaboxHost()">\n';
      for (var idx = 0; idx < hosts.length; idx++) {
         // a host can include an optional, equate-delimited description
         var option = hosts[idx].split('=');
         if (!option[0].length) option[0] = window.location.host
         if (option.length == 1) option[1] = option[0];
         select += '<option value="' + option[0] + '">' + option[1] +
                   '</option>\n';
         if (idx == 0) DCLinaboxHost = option[0];
      }
      select += '</select>\n';
      document.getElementById('selectDCLinaboxHost').innerHTML = select;
   }
}

function setDCLinaboxHost () {
   var optionobj = document.getElementById('selectTheDCLinaboxHost');
   var idx = optionobj.selectedIndex;
   var opt = optionobj.options;
   DCLinaboxHost = opt[idx].value;
   return true;
}

/////////////////////////////
// dynamic terminal size list
/////////////////////////////

function selectDCLinaboxWxH (WxH) {
   WxH = WxH.split(',');
   if (WxH.length) {
      var select = '<select id="selectTheDCLinaboxWxH" ' +
                   'onchange="setDCLinaboxWxH()">\n';
      for (var idx = 0; idx < WxH.length; idx++) {
         if (!WxH[idx].length) continue;
         select += '<option value="' + WxH[idx] + '"';
         if (idx == 0) select += " selected";
         select += '>' + WxH[idx] + '</option>\n';
      }
      select += '</select>\n';
      document.getElementById('selectDCLinaboxWxH').innerHTML = select;
   }
}

function setDCLinaboxWxH () {
   var optionobj = document.getElementById('selectTheDCLinaboxWxH');
   var idx = optionobj.selectedIndex;
   var opt = optionobj.options;
   DCLinaboxWxH = opt[idx].value;
   return true;
}

//////
// end
//////
