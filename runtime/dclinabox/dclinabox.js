// DCLinabox.js
//
// Core JavaScript for DCLinabox.
//
// Copyright (C) 2011,2012 Mark G Daniel <mark.daniel@wasd.vsm.com.au>
// Based on ShellInABox ... http://shellinabox.com/
// Copyright (C) 2008,2009 Markus Gutschke <markus@shellinabox.com>
//
// 01-OCT-2012  MGD  v1.1.0, single sign-on (in DCLINABOX.EXE)
//                           dynamic terminal resize
//                           refine process termination reporting
//                           set terminal title to include logged-in username
//                           JavaScript/DCLINABOX.EXE IPC 'escape' sequences
// 21-JUL-2012  MGD  v1.0.2, dclws.onopen() MSIE (10) needs the try-catch
//                           plus VT100.JS MSIE (10) non-breaking 0xa0 space
//                           DCLinaboxLogoutClose configuration
// 28-APR-2012  MGD  v1.0.1, kludge for Firefox line height discrepencies
//                           DCLinaboxWxH configuration
// 04-DEC-2011  MGD  initial
var DCLinaboxVersion = "v1.1.1";
// versions of DCLBINABOX.EXE this JavaScript is compatible with
var compatibleVersions = new Array ("1.1.0","1.1.1"); 

/////////////////////////
// configuration settings
/////////////////////////

// DO NOT MODIFY THESE, follow the instructions in CONFIGINABOX.JS

// language configuration
var messageDefault = { NOTSUP : 'WebSocket not supported!', 
                       // buttons, etc.
                       CONNEC : 'CONNECT',
                       DISCON : 'DISCONNECT',
                       DISURE : 'DISCONNECT: Are you sure?',
                       PRINT  : 'Print',
                       // messages for connection status
                       FAILED : 'FAILED to connect',
                       NORESP : 'CONNECTED but no response',
                       BROKEN : 'CONNECTION broken',
                       DISCED : 'DISCONNECTED',
                       LOGOUT : 'LOGOUT',
                       TERMIN : 'TERMINATED',
                       // other
                       COMPAT : 'JavaScript and executable incompatible.\n' +
                                '(Expect quirky or broken behaviour!)',
                     };

getParameter('DCLinaboxMessage',messageDefault);

for (var key in messageDefault)
   if (typeof DCLinaboxMessage[key] == 'undefined')
      DCLinaboxMessage[key] = messageDefault[key];

var resizeDefault = new Array ("80x12","80x16","80x20","80x24","80x28","80x32",
                               "80x36","80x40","80x44","80x48",
                               "132x24","132x28","132x32","132x36","132x40",
                               "132x44","132x48","132x52","132x56","132x60",
                               "132x64",
                               "255x24","255x48","255x64","255x96","255x255");

// the above but user-supplied array of options
getParameter('DCLinaboxResizeOptions',resizeDefault);

// if true then resize dialogue is available embedded terminals
getParameter('DCLinaboxResizeEmbedded',false);

// always connect via "wss:" (i.e. via SSL, true or false)
getParameter('DCLinaboxSecureAlways',true);

// connect immediately the page opens
getParameter('DCLinaboxImmediate',false);

// by default the [^] button is available on a standalone terminal
getParameter('DCLinaboxAnother',false);

// by default a standalone terminal "LOGOUT" response closes window
getParameter('DCLinaboxLogoutClose',true);

// suppress scrollbar using 0 or set to number of lines in buffer (e.g. 500)
getParameter('DCLinaboxScroll',0);

// obscuring the application by using an alternate script name
getParameter('DCLinaboxScriptName','dclinabox');

// explicitly title the window
getParameter('DCLinaboxTitle','');

// get any explictly defined host name or default to the current host
getParameter('DCLinaboxHost',window.location.host);

// these are ShellInABox (vt100.js) configuration elements ...

getParameter('suppressAllAudio',true);
getParameter('linkifyURLs',1);
var userCSSListDefault = [ [ 'White on Black', true, false ],
                           [ 'Black on White', false, true ],
                           [ 'Monochrome', true, false ],
                           [ 'Color Terminal', false, true ] ];
getParameter('userCSSList',userCSSListDefault);

// terminal size
getSizeParameter ();

var charWidth = 0;
var charHeight = 0;
var scrollWidth = 0;
var dclws = null;
var thisDCLinabox = null;
var vtterm = null;
var vtstatus = null;
var vtinabox = null;
var esc = String.fromCharCode(27);
var compatibilityAlert = true;

// 0=unconnected,1=connecting,2=connected,3..n=data_rx,
// -1=[disconnect],-2=logout,-3=terminated
var connectionStatus = 0;

// character sequences emitted by DCLINABOX.EXE for signalling purposes
var substrEscape =    "\r\x02" + "DCLinabox\x03\r\\";
var alertEscape =     substrEscape + "6"; //(plus message string)
var logoutEscape =    substrEscape + "5";
var termSizeEscape =  substrEscape + "4"; //(plus WxH string)
var terminateEscape = substrEscape + "3";
var titleEscape =     substrEscape + "2"; //(plus title string)
var versionEscape =   substrEscape + "1"; //(plus trailing version string)

// from a bookmarklet if no parent with openDCLinabox() available
var bookmarkletTerminal =
    !((opener && typeof opener.openDCLinabox != 'undefined') ||
      (parent && typeof parent.openDCLinabox != 'undefined'));

/////////////////////////////////////////
// instantiate the DCLinabox/VT100 object
/////////////////////////////////////////

// monkey see, monkey do

function extend(subClass, baseClass) {
  function inheritance() { }
  inheritance.prototype          = baseClass.prototype;
  subClass.prototype             = new inheritance();
  subClass.prototype.constructor = subClass;
  subClass.prototype.superClass  = baseClass.prototype;
};

function DCLinabox() {

   thisDCLinabox = this;

   vtterm = document.getElementById ('vt100');
   vtstatus = document.getElementById ('vtstatus');
   vtinabox = document.getElementById ('vtinabox');

   // calculate a monospace character's width and height
   vtterm.innerHTML = '<div id="scrollor" ' +
      'style="width:10em;height:2em;overflow:scroll;"><br></div>' +
      '<table><tbody><td id="cursor" style="margin:0px;padding:0px;">' +
      '&nbsp;</td></tbody></table>';
   var cid = document.getElementById('cursor');

   var navualo = navigator.userAgent.toLowerCase();
   if (navualo.indexOf('firefox') > -1) {
      // Firefox (11.0 and 12.0 at least) seems to have a different (to Chrome)
      // line-height reported via .clientHeight, kludge an adjustment for this
      // on OS-X and Linux it's a single pixel
      var cch = cid.clientHeight - 1;
      // and on Windows it's three pixels!
      if (navualo.indexOf('windows') > -1) cch -= 2;
      vtterm.style.lineHeight = cch + 'px';
   }

   charWidth = cid.clientWidth;
   charHeight = cid.clientHeight;

   if (DCLinaboxScroll) {
      var scrollor = document.getElementById('scrollor');
      scrollWidth = scrollor.offsetWidth - scrollor.clientWidth;
   }

   // if it has come from a bookmarklet
   parseHash();

   resizeTerminal ();

   if (opener) makeTerminal();

   this.superClass.constructor.call(this, vtterm);

   this.maxScrollbackLines = DCLinaboxScroll;
                                                                   
   if ('WebSocket' in window) {
      if (DCLinaboxImmediate)
         webSocketOpen();
      else
         terminalStatus();
   }
   else
      terminalStatus(DCLinaboxNotSupportedMsg);
};
extend(DCLinabox, VT100);

// override the VT100.JS function
// transmit vt100.js key-press to PTD (via WebSocket)

DCLinabox.prototype.keysPressed = function(ch) {
  if (dclws)
     dclws.send(ch);
  else
  if (ch == esc)
     connTerm();
};

// override the VT100.JS function
// add our bell character to the VT status line

DCLinabox.prototype.beep = function() {

   bellChar(true);

   if (this.visualBell) {
      this.flashScreen();
   } else {
     try {
       this.beeper.Play();
     } catch (e) {
       try {
         this.beeper.src = 'beep.wav';
       } catch (e) {
       }
     }
   }
};

// override the VT100.JS function
// we'll never need to display that size

DCLinabox.prototype.showCurrentSize = function() {
};

////////////////////////////////
// open the WebSocket connection
////////////////////////////////

function webSocketOpen () {

   if (!('WebSocket' in window)) {
      terminalStatus(DCLinaboxMessage.NOTSUP);
      return;
   }

   // with dynamic host list it may have changed since the page was generated!
   getParameter('DCLinaboxHost',window.location.host);

   if (DCLinaboxTitle.length) {
      if (window.parent &&
          window.frameElement &&
          window.parent.setDCLinaboxTitle)
         window.parent.setDCLinaboxTitle(DCLinaboxTitle);
      else
         setDCLinaboxTitle(DCLinaboxTitle);
   }
   else
      setDCLinaboxTitle(DCLinaboxTitle);

   if (DCLinaboxHost.substr(0,6) == 'wss://') {
      var URL = 'wss://';
      DCLinaboxHost = DCLinaboxHost.substr(6);
   }
   else
   if (DCLinaboxHost.substr(0,5) == 'ws://') {
      var URL = 'ws://';
      DCLinaboxHost = DCLinaboxHost.substr(5);
      if (DCLinaboxSecureAlways) URL = 'wss://';
   }
   else
   if (DCLinaboxSecureAlways)
      var URL = 'wss://';
   else
   if (window.location.protocol == 'https:')
      var URL = 'wss://';
   else
      var URL = 'ws://';

   var socketPort = window.location.port;

   if (DCLinaboxHost.indexOf(':') >= 0) {
      // includes a specific port
      socketPort = DCLinaboxHost.split(':')[1];
      DCLinaboxHost = DCLinaboxHost.split(':')[0];
      // if only the port was supplied
      if (DCLinaboxHost == '') DCLinaboxHost = window.location.host;
   }
   URL += DCLinaboxHost;

   if (socketPort.length && socketPort != 80 && socketPort != 443)
      URL += ':' + socketPort;

   // path is optional
   if (DCLinaboxScriptName.indexOf('/') == -1)
      URL += '/cgiplus-bin/' + DCLinaboxScriptName;
   else
      URL += DCLinaboxScriptName;

   connectionStatus = 1;

   try { dclws = new WebSocket(URL) }
   catch (err) { alert(err); }

   if (typeof dclws.protocol == 'undefined') {
      // WebSocket API/version is not recent enough
      dclws.close();
      dclws = null;
      connectionStatus = 0;
      terminalStatus(DCLinaboxMessage.NOTSUP);
      return;
   }

   // WebSocket open

   dclws.onopen = function(evt) {
      connectionStatus = 2;
      DCLinaboxImmediate = true;
      // MSIE (10) needs the try-catch
      window.onbeforeunload = function() { 
         try { return dclws.close(); } catch (err) { return null; }
      };
      vtterm.style.backgroundColor = 'white';
      terminalStatus();
      thisDCLinabox.focusCursor();
      thisDCLinabox.input.focus();
   };

   // WebSocket close

   dclws.onclose = function(evt) {
      var msg = null;
      var reason = '';
      if (typeof evt != 'undefined') {
         var code = evt.code;
         var reason = ' (' + evt.code
         if (evt.reason.length) reason += ' ' + evt.reason;
         reason += ')';
      }
      var dt = new Date;
      var at = ' <span style="font-size:80%;">' +
               dt.toDateString() + ' ' +
               dt.toTimeString().substr(0,8) +
               '</span>';
      switch (connectionStatus) {
         case -3 : msg = DCLinaboxMessage.TERMIN + at; break;
         case -2 : msg = DCLinaboxMessage.LOGOUT + at; break;
         case -1 : msg = DCLinaboxMessage.DISCED + at; break;
         case  0 : msg = '?'; break;
         case  1 : msg = DCLinaboxMessage.FAILED + reason; break;
         case  2 : msg = DCLinaboxMessage.NORESP + reason; break;
         default : msg = DCLinaboxMessage.BROKEN + at + reason; break;
      }
      connectionStatus = 0;
      vtterm.style.backgroundColor = 'whitesmoke';
      terminalStatus(msg);
      dclws = null;
   };

   // WebSocket data from PTD

   dclws.onmessage = function (evt) { 
      if (evt.data.substr(0,substrEscape.length) == substrEscape) {
         // a DCLinabox 'escape' sequence emitted by the executable
         if (evt.data == logoutEscape) {
            if (DCLinaboxAnother && DCLinaboxLogoutClose)
               window.close();
            else {
               connectionStatus = -2;
               dclws.close();
            }
         }
         else
         if (evt.data == terminateEscape) {
            connectionStatus = -3;
            dclws.close();
         }
         else
         if (evt.data.substr(0,alertEscape.length) == alertEscape) {
            connectionStatus++;
            var msg = evt.data.substr(alertEscape.length);
            alert (msg);
         }
         else
         if (evt.data.substr(0,termSizeEscape.length) == termSizeEscape) {
            connectionStatus++;
            var termSize = evt.data.substr(termSizeEscape.length);
            var WxH = termSize.split('x');
            if (WxH.length == 2)
               resizeTerminal (parseInt(WxH[0]), parseInt(WxH[1]));
         }
         else
         if (evt.data.substr(0,titleEscape.length) == titleEscape) {
            connectionStatus++;
            var title = evt.data.substr(titleEscape.length);
            setDCLinaboxTitle (title);
         }
         else
         if (evt.data.substr(0,versionEscape.length) == versionEscape) {
            connectionStatus++;
            var version = evt.data.substr(versionEscape.length);
            if (compatibleVersions.indexOf(version) != -1)
               compatibilityAlert = false;
         }
         else
            alert ('Unknown DCLinabox escape!');
      }
      else {
         // end-use terminal output
         if (compatibilityAlert) {
            compatibilityAlert = false;
            alert(DCLinaboxMessage.COMPAT);
         }
         connectionStatus++;
         var termResponse = thisDCLinabox.vt100(evt.data);
         if (termResponse.length) dclws.send(termResponse); 
      }
   };
}

///////////////////////////
// get parameters from hash
///////////////////////////

function parseHash () {

   if (!window.location.hash.length) return;

   var hash = window.location.hash.substr(1);
   if (hash.substr(0,6) == 'wss://') {
      var protocol = 'wss://';
      hash = hash.substr(6);
   }
   else
   if (hash.substr(0,5) == 'ws://') {
      var protocol = 'ws://';
      hash = hash.substr(5);
   }
   else {
      alert('ERROR:'+hash);
      return;
   }

   var WxH = null;
   var sizeat = hash.indexOf(';');
   if (sizeat > 0) {
      WxH = hash.substr(sizeat+1)
      hash = hash.substr(0,sizeat);
   }

   var pathat = hash.indexOf('/');
   if (pathat > 0) {
      var host = hash.substr(0,pathat)
      var script = hash.substr(pathat);
   }
   else {
      alert('ERROR:'+hash);
      return;
   }

   DCLinaboxHost = protocol + host;
   DCLinaboxScriptName = script;
   if (WxH) {
      DCLinaboxWxH = WxH;
      getSizeParameter ();
   }
}

//////////////////////////////////
// get the terminal size parameter
//////////////////////////////////

function getSizeParameter () {

   // width x height
   getParameter('DCLinaboxWxH',null);
   if (DCLinaboxWxH != null) {
      var WxH = DCLinaboxWxH.split('x');
      if (WxH.length == 2) {
         DCLinaboxWidth = parseInt(WxH[0]);
         DCLinaboxHeight = parseInt(WxH[1]);
      }
   }

   // opening terminal width and height
   getParameter('DCLinaboxWidth',80);
   if (DCLinaboxWidth != 80 && DCLinaboxWidth != 132) DCLinaboxWidth = 80;
   getParameter('DCLinaboxHeight',24);
   if (DCLinaboxHeight < 12 || DCLinaboxHeight > 96) DCLinaboxHeight = 24;
}

////////////////////////////////////
// make the popup a terminal window
////////////////////////////////////

var makeTerminalSnap = null;
var makeTerminalPopup = false;

function makeTerminal () {

   makeTerminalPopup = true;
   DCLinaboxImmediate = true;
   DCLinaboxAnother = true;
   document.body.style.margin = 0;
   document.body.style.padding = 0;
   vtinabox.style.margin = 0;

   if (typeof window.innerWidth != 'undefined') {
      var width = window.innerWidth;
      var height = window.innerHeight;
   } else {
      var width = document.body.clientWidth; 
      var height = document.body.clientHeight;
   }
   width = parseInt(vtinabox.style.width) - width;
   height = parseInt(vtinabox.style.height) - height;
   if (DCLinaboxScroll) { width++; height++; }
   window.resizeBy(width,height);

   window.onresize = function () {
      clearTimeout(makeTerminalSnap);
      makeTerminalSnap = setTimeout('makeTerminalSnap=null;makeTerminal()',500);
      return true;
   };
}

/////////////////////////////////////////////
// resize a parent IFRAME to fit the terminal
/////////////////////////////////////////////

function makeIframe () {

   if (!(window.parent &&
         window.frameElement &&
         window.parent.resizeDCLinaboxIframe)) return;
         
   document.body.style.margin = 0;
   document.body.style.padding = 0;
   vtinabox.style.margin = 0;

   var width = parseInt(vtinabox.style.width) + 10;
   var height = parseInt(vtinabox.style.height) + 10;
   window.parent.resizeDCLinaboxIframe(window.frameElement,width,height);
}

//////////////////////
// resize the terminal
//////////////////////

function resizeTerminal (cols,rows) {

   if (cols == DCLinaboxWidth && rows == DCLinaboxHeight) return;

   if (typeof cols != 'undefined') DCLinaboxWidth = cols;
   if (typeof rows != 'undefined') DCLinaboxHeight = rows;

   // set the terminal elements' height
   vtterm.style.height = (charHeight * DCLinaboxHeight) + 2 + 'px';
   vtstatus.style.height = charHeight + 5 + 'px';
   vtinabox.style.height = vtterm.offsetHeight +
                           vtstatus.offsetHeight + 'px';

   // set the terminal elements' width
   vtterm.style.width = vtstatus.style.width =
      (charWidth * DCLinaboxWidth) + 2 + scrollWidth + 'px';
   vtinabox.style.width = vtterm.offsetWidth + 'px';

   if (makeTerminalPopup)
      makeTerminal();
   else
      makeIframe();

   if (typeof cols == 'undefined') {
      // initial terminal instantiation
      thisDCLinabox.initializeElements();
      thisDCLinabox.reset();
   }
   else
      thisDCLinabox.resizer();

   terminalStatus();

   thisDCLinabox.focusCursor();
   thisDCLinabox.input.focus();
};

//////////////////
// terminal status
//////////////////

var terminalStatusOpen1 = null;

function terminalStatus (msg) {

   var status = '&nbsp;';
   if (connectionStatus)
      status += '<span id="vtbtn" onclick="discTerm()">&nbsp;' +
                DCLinaboxMessage.DISCON + '&nbsp;</span>';
   else
      status += '<span id="vtbtn" onclick="connTerm()">&nbsp;' +
                DCLinaboxMessage.CONNEC + '&nbsp;</span>';

   if (DCLinaboxAnother)
      status += '<span id="vtbtn" onclick="openAnotherDCLinabox()' +
                '">&nbsp;^&nbsp;</span>';

   if (DCLinaboxAnother || DCLinaboxResizeEmbedded)
      status += '<span id="vtWxH"></span>';

   status += '<span id="vtbtn" onclick="thisDCLinabox.vt100(esc+\'[i\')"' +
             '>&nbsp;' + DCLinaboxMessage.PRINT + '&nbsp;</span>';

   if (typeof msg != 'undefined' && msg != null)
      status += '<span>&nbsp;' + msg + '&nbsp;</span>';

   status += '&nbsp;<span id="vtbell">&nbsp;</span>&nbsp;';

   status += '<span id="vanity1">DCLinabox<span id="vanity2">' +
             DCLinaboxVersion + '</span></span>';

   vtstatus.innerHTML = status;

   if (DCLinaboxAnother || DCLinaboxResizeEmbedded) buttonWxH();
}

// connect button clicked

function connTerm () {
   thisDCLinabox.initializeElements();
   thisDCLinabox.reset();
   terminalStatus();
   webSocketOpen ();
}

// disconnect button clicked

function discTerm () {
   if (confirm(DCLinaboxMessage.DISURE)) {
      connectionStatus = -1;
      dclws.close();
   }
}

// resize selection dialogue

var buttonWxHtimeout = null;

function selectWxH () {

   if (!dclws) return;

   var vtwxh = document.getElementById('vtWxH');
   var wxh = DCLinaboxWidth + 'x' + DCLinaboxHeight;
   var selected;
   var options = "";
   for (var idx = 0; idx < DCLinaboxResizeOptions.length; idx++) {
      if (DCLinaboxResizeOptions[idx] == wxh)
         selected = ' selected';
      else
         selected = '';
      options += '<option value="' + DCLinaboxResizeOptions[idx] + '"' +
                 selected + '>' + DCLinaboxResizeOptions[idx];
   }
   vtwxh.innerHTML = '<span id="selWxH" ' +
                     'style="margin-left:5px;margin-right:5px;">' +
                     '<select id="selectWxH" onchange="changedWxH()">' +
                     options + '</select></span>';

   var selectwxh = document.getElementById('selectWxH');
   selectwxh.focus();

   if (buttonWxHtimeout) clearTimeout(buttonWxHtimeout);
   buttonWxHtimeout = setTimeout("buttonWxH()",15000);
}

// resize selection changed

function changedWxH () {
   var selectwxh = document.getElementById('selectWxH');
   var wxh = selectwxh.options[selectwxh.selectedIndex].text;
   buttonWxH(wxh);
   if (dclws) dclws.send(termSizeEscape+wxh);
}

// make the WxH button

function buttonWxH (wxh) {
   if (typeof wxh == 'undefined') wxh = DCLinaboxWidth + 'x' + DCLinaboxHeight;
   var vtwxh = document.getElementById('vtWxH');
   vtwxh.innerHTML = '<span id="vtbtn" onclick="selectWxH()">&nbsp;' +
                     wxh + '&nbsp;</span>';
   clearTimeout(buttonWxHtimeout);
   buttonWxHtimeout = null;
}

// [^] button clicked

function openAnotherDCLinabox () {

   var specs = 'toolbar=0,location=0,directories=0,status=0,menubar=0,' +
               'scrollbars=0,resizable=0,copyhistory=0,width=100,height=100';

   window.open(window.location.href,'_blank',specs);
}

// set the title of a standalone terminal
// this function can also be found in OPENINABOX.JS

function setDCLinaboxTitle (title) {
   // do not override a configuration set title
   if (DCLinaboxTitle.length) return;
   if (typeof title == 'undefined' ||
       !title.length ||
       title.substr(0,1) == '?')
      title = 'DCLinabox: ' + DCLinaboxHost;
   document.getElementsByTagName('title')[0].innerHTML = title;
}

///////////////////////////////////////////
// "He's deaf. The bells have made him so."
///////////////////////////////////////////

var vtbell = null;
var bellCharCount = 0;
var utf237e = null;

function bellChar (ding) {
   if (!vtbell) vtbell = document.getElementById('vtbell');
   if (!utf237e) utf237e = '<img style="vertical-align:text-top; height:' +
                           charHeight + 'px;" src="utf237e.png">';
   if (ding) {
      if (bellCharCount > 10) return;
      bellCharCount++;
      setTimeout('bellChar(false)',900);
   }
   else {
      bellCharCount--;
   }
   var bellCharVisual = '';
   for (var cnt = bellCharCount; cnt; cnt--) bellCharVisual += utf237e;
   vtbell.innerHTML = bellCharVisual;
}

///////////////////
// inline execution
///////////////////

// create a style element and add some inline style

function addInlineStyle () {

   var style = '';

   style += '#vt100 { min-height: 16px; margin-bottom:0; ' +
            'height:4em; width:24em; } ';

   style += '#vtstatus { min-height: 16px; font-family: "DejaVu Sans Mono", ' +
            '"Everson Mono", FreeMono, "Lucida Console", ' +
            'monospace; font-size:90%; letter-spacing: -1px; margin-top:0; ' +
            'padding-top: 5px; border-top-width:0; height:1em; width:24em; ' +
            'overflow: hidden; white-space: nowrap; } ';

   style += '#vanity1 { position:absolute; right:0; margin:0 2em 0 2em; ' +
            'font-family:futura, helvetica, arial, sans; font-size:90%; ' +
            'line-height:80%; text-align:left; letter-spacing:0px; } ';

   style += '#vanity2 { display:block; text-align:center; font-size:80%; } ';

   style += '#vtbell { vertical-align:text-top; } ';

   if (typeof DCLinaboxScroll == "undefined" || !DCLinaboxScroll)
      style += '#vt100 #scrollable { overflow: hidden; }';

   // comment from original ShellInABox DEMO.HTML on why this was included

   // We would like to hide overflowing lines as this can lead to
   // visually jarring results if the browser substitutes oversized
   // Unicode characters from different fonts. Unfortunately, a bug
   // in Firefox prevents it from allowing multi-line text
   // selections whenever we change the "overflow" style. So, only
   // do so for non-Netscape browsers.

   if (typeof navigator.appName == 'undefined' ||
       navigator.appName != 'Netscape')
     style += '#vt100 #console div, #vt100 #alt_console div ' +
              '{ overflow: hidden; }';

   var styleobj = document.createElement('style');
   styleobj.setAttribute('type','text/css');
   styleobj.innerHTML = style;
   document.getElementsByTagName('head')[0].appendChild(styleobj);
}

addInlineStyle();

window.onload = function () {
   setTimeout('new DCLinabox()',100);            
}

// end
