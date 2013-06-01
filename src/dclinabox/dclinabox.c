/*****************************************************************************/
#ifdef COMMENTS_WITH_COMMENTS
/*
                                  DCLinabox.c

JavaScript based VT102 terminal emulation using WASD WebSocket communication
with a pseudo-terminal connected VMS process.

Based on JavaScript code from the ShellInABox project:

  Copyright (C) 2008-2009 Markus Gutschke <markus@shellinabox.com>.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 2 as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

http://code.google.com/p/shellinabox/

For all things VT100 (et.al.) ... http://vt100.net


CAUTION
-------
This WebSocket application allows remote login from a Web browser to the server
system.  This could be a security issue and so the script disables itself by
default.  Logical name value DCLINABOX_ENABLE controls whether this script can
be used.   Define this system-wide using a value of "*" to simply allow
experimentation.  Alternatively provide one or more comma-separated,
dotted-decimal IP address to specify one or more hosts allowed to use the
script, and/or one or more comman-separated IP addresses and CIDR subnet mask
to specify a range of hosts.  IPv4 only!  For example

  $ DEFINE /SYSTEM DCLINABOX_ENABLE "*"
  $ DEFINE /SYSTEM DCLINABOX_ENABLE "192.168.1.2"
  $ DEFINE /SYSTEM DCLINABOX_ENABLE "192.168.1.2,192.168.1.3"
  $ DEFINE /SYSTEM DCLINABOX_ENABLE "192.168.1.0/24"
  $ DEFINE /SYSTEM DCLINABOX_ENABLE "192.168.1.0/24,192.168.2.2"

By default the WebSocket, and hence all traffic to and from the DCLinabox login
and session, is only allowed over Secure Sockets Layer.  To allow access via
clear-text connections add "ws:" somewhere in the logical name value.

  $ DEFINE /SYSTEM DCLINABOX_ENABLE "ws:,*"
  $ DEFINE /SYSTEM DCLINABOX_ENABLE "192.168.1.0/24,ws:,192.168.2.2"


SINGLE SIGN-ON
--------------
By default, DCLinabox terminal sessions prompt for a username and password. 
For sites where WASD SYSUAF authentication is available, or where the
authenticated remote user string is the equivalent of a VMS username, DCLinabox
can use that authenticated VMS username to establish a terminal session without
further credential input (i.e. the terminal just displays the DCL prompt, ready
to go).

THIS IS OBVIOUSLY VERY POWERFUL AND SHOULD ONLY BE USED WITH DUE CAUTION!

The functionality is enabled using the DCLINABOX_SSO logical name.  This
logical name is multi-valued, allowing considerable granularity in establishing
allowed use of the facility.  Each value begins with the name of the realm
associated with authentication of the VMS username.  This is separated by an
equate symbol and then one or more comma-separated usernames and/or wildcard
allowed to single sign-on.  Preceding a username with a '!' (exclamation point)
disallows the matching username to SSO.  All string matches are
case-insensitive.

Account restrictions (e.g. times) are not evaluated.  If a specific username
matches it is permitted regardless of the account privileges.  If a '**'
(double asterisk) is specified any username is permitted regardless of the
account privileges.  If a '*' (single asterisk) is specified any non-privileged
account is permitted to SSO.  If '!*' (exclamation point then asterisk) is
specified DCLinabox cannot be used except if permitted by SSO.

For example, the following authentication rule

  ["VMS credentials"=WASD_VMS_RW=id]
  /cgi*-bin/dclinabox* r+w,https:

would require the logical name defintion

  $ DEFINE /SYSTEM DCLINABOX_SSO "WASD_VMS_RW=*"

to allow any such non-privileged authenticated user to create a logged-in
terminal session, while the logical name definition

  $ DEFINE /SYSTEM DCLINABOX_SSO "WASD_VMS_RW=REN,STIMPY"

would allow only users REN and STIMPY to do so.  The logical name definition

  $ DEFINE /SYSTEM DCLINABOX_SSO "WASD_VMS_RW=**"

would allow any account (privileged or non-privileged) to SSO, and

  $ DEFINE /SYSTEM DCLINABOX_SSO "WASD_VMS_RW=REN,!STIMPY,*"

allows (perhaps privileged) REN but not STIMPY, and then any other
non-privileged account.

If a matching authentication realm is not present, or a matching username in a
matched realm is not found, or a disabling account status, then single sign-on
does not occur and the terminal session just prompts for credentials as usual. 
Of course, even if the logical name does not allow SSO, the access to DCLinabox
is still controlled by the web server authentication and authorisation.

The logical name DCLINABOX_ANNOUNCE allows an SSO session establishment
announcement to be displayed in the terminal window.  This multi-valued logical
name appends carriage-control to each value displaying it as separate line.

Single sign-on requires the executable image to be installed with privileges to
allow UAI and persona services to be used.  System startup requires (includes
WORLD required for process name update)

  $  INSTALL ADD CGI-BIN:[000000]DCLINABOX.EXE /AUTH=(DETACH,SYSPRV,WORLD)

and on executable image update

  $  INSTALL REPLACE CGI-BIN:[000000]DCLINABOX.EXE


TERMINAL TITLE
--------------
By default the terminal title bar displays the DCLinabox host name, VMS node
name and username.  To display the process name in addition (periodically
updated if changes) the executable image needs to be installed with WORLD
privilege.  System startup requires

  $  INSTALL ADD CGI-BIN:[000000]DCLINABOX.EXE /AUTH=(WORLD)

and on executable image update

  $  INSTALL REPLACE CGI-BIN:[000000]DCLINABOX.EXE


IDLE SESSION
------------
An idle session is one not having terminal input for a given period.  By
default idle sessions are disconnected after two hours with a five minute
warning.  The logical name DCLINABOX_IDLE allows the number of minutes before
client disconnection to be specified, the number of minutes warning (delivered
in a browser alert), and the warning message (allowing language customisation). 
Each of these elements is delimited by a comma.  Idle session management may be
changed at any time and is propagated to new and existing sessions.

Define to -1 to to disable idle disconnection: 

  $ DEFINE /SYSTEM DCLINABOX_IDLE -1

To specify a one hour idle period with 5 minute warning:

  $ DEFINE /SYSTEM DCLINABOX_IDLE "60,5"

To specify a six hour idle period with ten minute warning and local warning
message (which may contain just one "%d" to substitute the minutes warning):

  $ DEFINE /SYSTEM DCLINABOX_IDLE -
  "360,10,WARNING - disconnection in %d minutes!"


SESSION ANNOUNCEMENT
--------------------
The logical name DCLINABOX_ALERT results in an announcement being displayed in
a browser alert dialog.  This alert will be delivered at session establishment
if it exists at the time, perhaps as a permanent announcement, otherwise will
be alerted within a minute of it first being defined.  If an ephemeral
announcement it should be undefined when no longer relevant.  For example

  $ DEFINE /SYSTEM DCLINABOX_ALERT -
  "*** DCLinabox restart shortly - PLEASE LOG OFF ***"


RENAMING THE INTERFACE
-----------------------
The DCLINABOX.EXE file may be alternately named.  With a public system this may
be useful for reducing nuisance-value access attempts and/or an obvious attack
vector by obscuring the web interface.  Just use an obscured (or
access-controlled) HTML terminal page, a different script executable file name,
and add that script name to the <head>..</head> section of the HTML terminal
file as with other per-terminal configuration vardisableds.

  <script type="text/javascript"><!--
  DCLinaboxScriptName = "anexample";               
  //--></script>

The script accessed will then be /cgiplus-bin/anexample.

The logical names used would then be ANEXAMPLE_ENABLE, etc.


COPYRIGHT
---------
Copyright (C) 2011,2012 Mark G.Daniel
This program comes with ABSOLUTELY NO WARRANTY.
This is free software, and you are welcome to redistribute it under the
conditions of the GNU GENERAL PUBLIC LICENSE, version 3, or any later version.
http://www.gnu.org/licenses/gpl.txt


VERSION HISTORY
---------------
08-DEC-2012  MGD  v1.1.1, tidied some #includes
                          bugfix; SessionManagement() NULL pointer
01-OCT-2012  MGD  v1.1.0, single sign-on (no-password required terminal)
                          dynamic terminal resize
                          set terminal title to include logon detail
                          DCLINABOX.EXE/JavaScript IPC 'escape' sequences
                          refine idle session management
                          refine process termination signaling
                          bugfix; PtdClose() queued read and *write*
21-JUL-2012  MGD  v1.0.1, bugfix; ptd$delete() during client removal
                          bugfix; AddClient() lib$free_vm_page() memory
04-DEC-2011  MGD  v1.0.0, initial development
*/
#endif /* COMMENTS_WITH_COMMENTS */
/*****************************************************************************/

#define SOFTWAREVN "1.1.1"
/*                  ^^^^^ don't forget to update DCLINABOX.JS compliance! */
#define SOFTWARENM "DCLINABOX"
#ifdef __ALPHA
#  define SOFTWAREID SOFTWARENM " AXP-" SOFTWAREVN
#endif
#ifdef __ia64
#  define SOFTWAREID SOFTWARENM " IA64-" SOFTWAREVN
#endif
#ifdef __VAX
#  define SOFTWAREID SOFTWARENM " VAX-" SOFTWAREVN
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <inet.h>
#include <netdb.h>
#include <unixlib.h>

#include <cmbdef.h>
#include <descrip.h>
#include <dvidef.h>
#include <lib$routines.h>
#include <iodef.h>
#include <issdef.h>
#include <jpidef.h>
#include <lnmdef.h>
#include <prcdef.h>
#include <prvdef.h>
#include <ssdef.h>
#include <starlet.h>
#include <stsdef.h>
#include <syidef.h>
#include <ttdef.h>
#include <tt2def.h>
#include <uaidef.h>

#include "wslib.h"

#define DC$_TERM 6

#define VMSok(x) ((x) & STS$M_SUCCESS)
#define VMSnok(x) !(((x) & STS$M_SUCCESS))

#define Debug 0

#define FI_LI "DCLINABOX", __LINE__
#define EXIT_FI_LI(status) { printf ("[%s:%d]", FI_LI); exit(status); }

/* basically the page size on the architecture */
#ifdef VAX
#define PTD_READ_SIZE 512
#define PTD_WRITE_SIZE 512
#else
#define PTD_READ_SIZE 8192
#define PTD_WRITE_SIZE 8192
#endif

#define DEFAULT_IDLE_MINS    120
#define DEFAULT_WARN_MINS      5
#define DEFAULT_WARN_MESSAGE "This idle terminal will be disconnected " \
                             "in %d minutes!"

int  ConnectedCount,
     UsageCount,
     VmsVersionInteger = 720;

unsigned long  ScriptUic;

/* an unlikely sequence for end-use terminal output (avoid nulls) */
#define DCLINABOX_ESCAPE "\r\x02" "DCLinabox\x03\r\\"

char  AlertLogicalName [128],
      AnnounceLogicalName [128],
      EnableLogicalName [128],
      IdleLogicalName [128],
      SingleLogicalName [128],
      DCLinaboxEscape [] = DCLINABOX_ESCAPE,
      AlertEscape [] =     DCLINABOX_ESCAPE "6", /* plus message string */
      LogoutEscape [] =    DCLINABOX_ESCAPE "5",
      TermSizeEscape [] =  DCLINABOX_ESCAPE "4", /* plus WxH string */
      TerminateEscape [] = DCLINABOX_ESCAPE "3",
      TitleEscape [] =     DCLINABOX_ESCAPE "2", /* plus title string */
      VersionEscape [] =   DCLINABOX_ESCAPE "1" SOFTWAREVN;

struct PtdClient {

   /* keep these adjacent and aligned on a page boundary */
   char  PtdReadBuffer [PTD_READ_SIZE],
         PtdWriteBuffer [PTD_WRITE_SIZE];

   int  Alerted,
        IdleMins,
        LogoutResponse,
        ProcessPid,
        PtdQueuedRead,
        PtdQueuedWrite,
        PtdReadCount,
        PtdWriteCount,
        WarnMins;

   unsigned long  ClientCount,
                  DviOwnUic,
                  DviPid,
                  IdleCount,
                  IdleTime,
                  WarnTime;

   unsigned short  ptdchan;

   char  DviHostName [8+1],
         InputBuffer [256],
         HttpHost [64],
         JpiPrcNam [15+1],
         OwnIdent [31+1],
         PtdDevName [64],
         VmsUserName [12];

   struct dsc$descriptor_s  PtdDevNameDsc;

   struct WsLibStruct  *WsLibPtr;
};

long  PtdClientPages = (sizeof(struct PtdClient) / 512 ) + 1;

long  CharBuf [3];

/* function prototypes */
void AddClient ();
void AdviseClientTermSize (struct PtdClient*);
void ClientEscape (struct PtdClient*, char*, int);
int DCLinaboxEnable ();
int DCLinaboxSingleSignOn (struct PtdClient*);
int PtdOpen (struct PtdClient*);
void PtdClose (struct PtdClient*);
int PtdCrePrc (struct PtdClient*);
void PtdRead (struct PtdClient*);
void PtdReadClient (struct WsLibStruct*);
void PtdRemoveClient (struct WsLibStruct *wsptr);
void PtdTerminateAst (struct PtdClient*);
void PtdReadAst (struct PtdClient*);
void PtdReadWriteAst (struct WsLibStruct*);
void PtdWrite (struct PtdClient*, char*, int);
void PtdWriteAst (struct PtdClient*);
void SessionManagement ();
char* SysTrnLnm (char*, char*, int);

/*****************************************************************************/
/*
AST delivery is disabled during client acceptance and the add-client function
is deferred using an AST to help minimise the client setup window with a
potentially busy WebSocket application.
*/

main (int argc, char *argv[])

{
   static unsigned long  JpiUicItem = JPI$_UIC;

   int  len;
   char  *aptr, *cptr, *sptr, *zptr;

   /*********/
   /* begin */
   /*********/

   if (argc > 1 && !strcasecmp(argv[1],"/VERSION"))
   {
      fprintf (stdout, "%%DCLINABOX-I-VERSION, %s %s\n",
               SOFTWAREID, WsLibVersion());
      exit (SS$_NORMAL);
   }

   /* don't want the C-RTL fiddling with the carriage control */
   stdout = freopen ("SYS$OUTPUT", "w", stdout, "ctx=bin");

   if (!WsLibIsCgiPlus())
   {
      fprintf (stdout, "Status: 500\n\nMust be CGIplus!\n");
      exit (SS$_NORMAL);
   }

   /* note the scripting account's UIC */
   lib$getjpi (&JpiUicItem, 0, 0, &ScriptUic, 0, 0);

   /* set the terminal characteristics */
   CharBuf[0] = (80 << 16) | (TT$_LA100 << 8) | DC$_TERM;
   CharBuf[1] = (24 << 24) |
                TT$M_EIGHTBIT | TT$M_SCOPE | TT$M_WRAP |
                TT$M_MECHTAB | TT$M_LOWER | TT$M_TTSYNC;
   CharBuf[2] = TT2$M_EDIT | TT2$M_DRCS | TT2$M_EDITING | TT2$M_HANGUP;

   /* parse out the executable file name */
   for (aptr = argv[0]; *aptr; aptr++);
   while (aptr > argv[0] && *aptr != ']') aptr--;
   if (*aptr++ != ']') exit (SS$_BUGCHECK);

   /* generate the logical names from the executable file name */
   zptr = (sptr = AlertLogicalName) + sizeof(AlertLogicalName)-16;
   for (cptr = aptr;
        *cptr && *cptr != '.' && sptr < zptr;
        *sptr++ = toupper(*cptr++));
   len = sptr - AlertLogicalName;
   strcpy (AlertLogicalName+len, "_ALERT");
   strncpy (AnnounceLogicalName, AlertLogicalName, len);
   strcpy (AnnounceLogicalName+len, "_ANNOUNCE");
   strncpy (EnableLogicalName, AlertLogicalName, len);
   strcpy (EnableLogicalName+len, "_ENABLE");
   strncpy (IdleLogicalName, AlertLogicalName, len);
   strcpy (IdleLogicalName+len, "_IDLE");
   strncpy (SingleLogicalName, AlertLogicalName, len);
   strcpy (SingleLogicalName+len, "_SSO");

   /* no clients is two minutes in seconds */
   WsLibSetLifeSecs (2*60);

   SessionManagement ();

   while (WsLibIsCgiPlus())
   {
      WsLibCgiVar ("");

      sys$setast (0);

      UsageCount++;

      if (DCLinaboxEnable()) AddClient ();

      WsLibCgiPlusEof ();

      sys$setast (1);
   }

   exit (SS$_NORMAL);
}

/*****************************************************************************/
/*
Allocate a client structure and add it to the head of the list.  Establish the
WebSocket IPC, create the user termina (and process if SSO) and begin
processing.
*/

void AddClient ()

{
   int  idx, len, sso, status;
   short int  slen;
   char  *aptr, *cptr, *sptr, *zptr;
   char  AlertMsg [sizeof(AlertEscape)+256],
         AnnounceLine [256+2];
   struct PtdClient  *clptr;
   $DESCRIPTOR (AlertMsgDsc, AlertMsg);

   /*********/
   /* begin */
   /*********/

   /* PTD$ buffers must be page aligned */
   status = lib$get_vm_page (&PtdClientPages, &clptr);
   if (VMSnok(status)) EXIT_FI_LI (status);
   memset (clptr, 0, sizeof(struct PtdClient));

   if (cptr = WsLibCgiVarNull("HTTP_HOST"))
   {
      zptr = (sptr = clptr->HttpHost) + sizeof(clptr->HttpHost)-1;
      while (*cptr && sptr < zptr) *sptr++ = *cptr++;
      *sptr = '\0';
   }

   /* create a WebSocket library structure for the client */
   if (!(clptr->WsLibPtr = WsLibCreate (clptr, PtdRemoveClient)))
   {
      /* failed, commonly on some WebSocket protocol issue */
      status = lib$free_vm_page (&PtdClientPages, &clptr);
      if (VMSnok(status)) EXIT_FI_LI (status);
      return;
   }

   /* open the IPC to the WebSocket (mailboxes) */
   status = WsLibOpen (clptr->WsLibPtr);
   if (VMSnok(status)) EXIT_FI_LI(status);

   WsLibWatchScript (clptr->WsLibPtr, FI_LI, "!AZ", SOFTWAREID);

   if ((status = DCLinaboxSingleSignOn (clptr)) == SS$_NORMAL)
      status = PtdCrePrc (clptr);
   else
   if (VMSok(status))
      status = PtdOpen (clptr);

   /* inform the JavaScript which version executable it's dealing with */
   WsLibWrite (clptr->WsLibPtr, VersionEscape,
               sizeof(VersionEscape)-1, WSLIB_ASYNCH);

   if (VMSnok (status))
   {
      /* unsuccessful create alert */
      zptr = (sptr = AlertMsg) + sizeof(AlertMsg)-1;
      for (cptr = AlertEscape; *cptr && sptr < zptr; *sptr++ = *cptr++);
      if (sptr < zptr) *sptr++ = '\"';
      AlertMsgDsc.dsc$a_pointer = sptr;
      AlertMsgDsc.dsc$w_length = sizeof(AlertMsg) - (sptr - AlertMsg);
      sys$getmsg (status, &slen, &AlertMsgDsc, 1, 0); 
      sptr += slen;
      if (sptr < zptr) *sptr++ = '\"';
      WsLibWrite (clptr->WsLibPtr, AlertMsg, sptr-AlertMsg, WSLIB_ASYNCH);
      WsLibClose (clptr->WsLibPtr, 0, NULL);
      return;
   }
   else
   if (aptr = SysTrnLnm (AlertLogicalName, NULL, 0))
   {
      /* session alert */
      zptr = (sptr = AlertMsg) + sizeof(AlertMsg)-1;
      for (cptr = AlertEscape; *cptr && sptr < zptr; *sptr++ = *cptr++);
      for (cptr = aptr; *cptr && sptr < zptr; *sptr++ = *cptr++);
      WsLibWrite (clptr->WsLibPtr, AlertMsg, sptr-AlertMsg, WSLIB_ASYNCH);
      clptr->Alerted = 1;
   }

   if (VMSok (status) && clptr->VmsUserName[0])
   {
      /* successful single sign-on terminal */
      for (idx = 0; idx <= 127; idx++)
      {
         /* single sign-on session announcement */
         if (!SysTrnLnm (AnnounceLogicalName, AnnounceLine, idx)) break;
         slen = strlen(AnnounceLine);
         AnnounceLine[slen++] = '\r';
         AnnounceLine[slen++] = '\n';
         WsLibWrite (clptr->WsLibPtr, AnnounceLine, slen, WSLIB_ASYNCH);
      }
   }

   /* queue an asynchronous read from the client */
   WsLibRead (clptr->WsLibPtr,
              clptr->InputBuffer,
              sizeof(clptr->InputBuffer),
              PtdReadClient);

   ConnectedCount++;
}

/*****************************************************************************/
/*
Remove the client structure from the list and free the memory.
*/

void PtdRemoveClient (struct WsLibStruct *wsptr)

{
   int  status;
   struct PtdClient  *clptr;

   /*********/
   /* begin */
   /*********/

   clptr = WsLibGetUserData(wsptr);

   if (clptr->ptdchan) status = ptd$delete (clptr->ptdchan);

   status = lib$free_vm_page (&PtdClientPages, &clptr);
   if (VMSnok(status)) EXIT_FI_LI (status);

   if (ConnectedCount) ConnectedCount--;
}

/*****************************************************************************/
/*
Create the pseudo-terminal and begin reading from it.
*/

int PtdOpen (struct PtdClient *clptr)

{
   int  status;
   long  InAdr [2];

   /*********/
   /* begin */
   /*********/

   InAdr[0] = (int)(clptr->PtdReadBuffer);
   InAdr[1] = (int)(clptr->PtdReadBuffer +
                    sizeof(clptr->PtdReadBuffer) +
                    sizeof(clptr->PtdWriteBuffer)-1);

   status = ptd$create (&clptr->ptdchan, 0, CharBuf, sizeof(CharBuf),
                        PtdTerminateAst, clptr, 0, InAdr);
   if (VMSnok (status)) return (status);

   /* unsolicited input to get LOGINOUT to prompt for username/password */
   clptr->PtdWriteBuffer[sizeof(short)+sizeof(short)] = '\r';
   ptd$write (clptr->ptdchan, 0, 0, clptr->PtdWriteBuffer, 1, 0, 0);

   clptr->PtdQueuedRead++;
   status = ptd$read (0, clptr->ptdchan, &PtdReadAst, clptr,
                      clptr->PtdReadBuffer, sizeof(clptr->PtdReadBuffer));

   return (status);
}

/*****************************************************************************/
/*
Open a pseudo-terminal attached to a detached process created as the specified
VMS user account.
*/

#define ISS$C_ID_NATURAL 1
#define IMP$M_ASSUME_SECURITY 1
#define IMP$M_ASSUME_DEFPRIV  8

int PtdCrePrc (struct PtdClient *clptr)

{
   static unsigned long  CrePrcFlags = PRC$M_DETACH |
                                       PRC$M_INTER |
                                       PRC$M_NOPASSWORD,
                         DevNamItem = DVI$_DEVNAM,
                         UnitItem = DVI$_UNIT;
   static unsigned long  NeedPrvMask [2] = { PRV$M_SYSPRV | PRV$M_DETACH, 0 };
   static $DESCRIPTOR (LoginOutDsc, "SYS$SYSTEM:LOGINOUT.EXE");

   int  status, ptatus,
        PersonaHandle = 0;
   short  sLength;
   long  InAdr [2];
   $DESCRIPTOR (PrcNamDsc, clptr->PtdDevName);
   $DESCRIPTOR (PtdDevNameDsc, clptr->PtdDevName);
   $DESCRIPTOR (UserNameDsc, clptr->VmsUserName);

   /*********/
   /* begin */
   /*********/

   if (!clptr->VmsUserName[0]) EXIT_FI_LI (SS$_BUGCHECK);

   UserNameDsc.dsc$w_length = strlen(clptr->VmsUserName);

   ptatus = sys$setprv (1, &NeedPrvMask, 0, 0);
   if (VMSnok (ptatus)) EXIT_FI_LI(ptatus);

#ifdef __VAX
   status = sys$persona_create (&PersonaHandle, &UserNameDsc,
                                IMP$M_ASSUME_DEFPRIV);
#else
   status = sys$persona_create (&PersonaHandle, &UserNameDsc,
                                ISS$M_CREATE_AUTHPRIV, 0, 0);
#endif

   if (VMSok (status))
   {
#ifdef __VAX
      status = sys$persona_assume (&PersonaHandle, IMP$M_ASSUME_SECURITY, 0, 0);
#else
      if (VmsVersionInteger >= 720)
         status = sys$persona_assume (&PersonaHandle, 0, 0, 0);
      else
         status = sys$persona_assume (&PersonaHandle, IMP$M_ASSUME_SECURITY, 0, 0);
#endif
   }

   /**************************/
   /* create pseudo terminal */
   /**************************/

   if (VMSok (status))
   {
      InAdr[0] = (int)(clptr->PtdReadBuffer);
      InAdr[1] = (int)(clptr->PtdReadBuffer +
                       sizeof(clptr->PtdReadBuffer) +
                       sizeof(clptr->PtdWriteBuffer)-1);

      status = ptd$create (&clptr->ptdchan, 0, CharBuf, sizeof(CharBuf),
                           PtdTerminateAst, clptr, 0, InAdr);

      if (VMSok (status))
      {
         status = lib$getdvi (&DevNamItem, &clptr->ptdchan,
                              0, 0, &PtdDevNameDsc, &sLength);
         if (VMSok (status))
         {
            clptr->PtdDevName[PtdDevNameDsc.dsc$w_length = sLength] = '\0';
            if ((PrcNamDsc.dsc$w_length = sLength) > 15)
               PrcNamDsc.dsc$w_length = 15;
         }
      }
   }

   /******************/
   /* create process */
   /******************/

   if (VMSok (status))
      status = sys$creprc (&clptr->ProcessPid,
                           &LoginOutDsc,
                           &PtdDevNameDsc,
                           &PtdDevNameDsc,
                           &PtdDevNameDsc,
                           0, 0,
                           &PrcNamDsc,
                           4,
                           0, 0,
                           CrePrcFlags,
                           0, 0);


   /******************/
   /* revert persona */
   /******************/

   ptatus = sys$persona_delete (&PersonaHandle);
   if (VMSnok(ptatus))
      WsLibWatchScript (clptr->WsLibPtr, FI_LI, "$PERSONA_DELETE %X!8XL",
                        ptatus);

   PersonaHandle = ISS$C_ID_NATURAL;

#ifdef __VAX
   ptatus = sys$persona_assume (&PersonaHandle, IMP$M_ASSUME_SECURITY, 0, 0);
#else
   if (VmsVersionInteger >= 720)
      ptatus = sys$persona_assume (&PersonaHandle, 0, 0, 0);
   else
      ptatus = sys$persona_assume (&PersonaHandle, IMP$M_ASSUME_SECURITY, 0, 0);
#endif
   if (VMSnok (ptatus)) EXIT_FI_LI(ptatus);

   ptatus = sys$setprv (0, &NeedPrvMask, 0, 0);
   if (VMSnok (ptatus)) EXIT_FI_LI(ptatus);
   
   /****************/
   /* initial read */
   /****************/

   if (VMSok (status))
   {
      clptr->PtdQueuedRead++;
      status = ptd$read (0, clptr->ptdchan, &PtdReadAst, clptr,
                         clptr->PtdReadBuffer, sizeof(clptr->PtdReadBuffer));
   }

   return (status);
}

/*****************************************************************************/
/*
Called when the process terminates.
Escape character sequence is detected by DCLINABOX.JS. 
*/

void PtdTerminateAst (struct PtdClient *clptr)

{
   /*********/
   /* begin */
   /*********/

   if (clptr->LogoutResponse)
      WsLibWrite (clptr->WsLibPtr, LogoutEscape,
                  sizeof(LogoutEscape)-1, WSLIB_ASYNCH);
   else
      WsLibWrite (clptr->WsLibPtr, TerminateEscape,
                  sizeof(TerminateEscape)-1, WSLIB_ASYNCH);
}

/*****************************************************************************/
/*
Cancel any outstanding terminal I/O.
*/

void PtdClose (struct PtdClient *clptr)

{
   /*********/
   /* begin */
   /*********/

   if (clptr->PtdQueuedRead || clptr->PtdQueuedWrite)
   {
      ptd$cancel (clptr->ptdchan);
      return;
   }

   WsLibClose (clptr->WsLibPtr, 0 , NULL);
}

/*****************************************************************************/
/*
Data has been read from the PTD (i.e. from the system).
*/

void PtdReadAst (struct PtdClient *clptr)

{
   int  bcnt, status;
   char  *bptr, *cptr, *zptr;

   /*********/
   /* begin */
   /*********/

   if (clptr->PtdQueuedRead) clptr->PtdQueuedRead--;

   status = *(short*)clptr->PtdReadBuffer;
   if (VMSok(status))
   {
      bptr = clptr->PtdReadBuffer + sizeof(short)+sizeof(short);
      bcnt = *(short*)(clptr->PtdReadBuffer + sizeof(short));

      /*
         Check if it looks like a LOGOUT response.
         e.g. "\r  SYSTEM       logged out at 21-JUL-2012 22:03:31.08\r"
           or "\r  SYSTEM       logged out at 21-JUL-2012 22:03\r"
      */
      if (bcnt == 48 || bcnt == 54)
      {
         zptr = (cptr = bptr) + bcnt;
         if (*cptr == '\r' || *cptr == '\n') cptr++;
         while (cptr < zptr && *cptr == ' ') cptr++;
         while (cptr < zptr && *cptr != ' ') cptr++;
         while (cptr < zptr && *cptr == ' ') cptr++;
         if (cptr == bptr+16 && !strncmp (cptr, "logged out at", 13))
         {
            cptr += 13;
            while (cptr < zptr && *cptr == ' ') cptr++;
            if (cptr < zptr && isdigit(*cptr)) cptr++;
            if (cptr < zptr && isdigit(*cptr)) cptr++;
            if (cptr < zptr && *cptr == '-') cptr++;
            if (cptr < zptr && isalpha(*cptr)) cptr++;
            if (cptr < zptr && isalpha(*cptr)) cptr++;
            if (cptr < zptr && isalpha(*cptr)) cptr++;
            if (cptr < zptr && *cptr == '-') cptr++;
            if (cptr < zptr && isdigit(*cptr)) cptr++;
            if (cptr < zptr && isdigit(*cptr)) cptr++;
            if (cptr < zptr && isdigit(*cptr)) cptr++;
            if (cptr < zptr && isdigit(*cptr)) cptr++;
            while (cptr < zptr && *cptr == ' ') cptr++;
            while (cptr < zptr && (isdigit(*cptr) ||
                                   *cptr == ':' ||
                                   *cptr == '.')) cptr++;
            /* if termination does not happen 'immediately' this gets reset */
            if (cptr == zptr-1 && (*cptr == '\r' || *cptr == '\n'))
               clptr->LogoutResponse = 10;
         }
      }

      WsLibWrite (clptr->WsLibPtr, bptr, bcnt, PtdReadWriteAst);
   }
   else
      PtdClose (clptr);
}

/*****************************************************************************/
/*
Data read from the PTD (system) has been written to the WebSocket client. 
Check status and if OK queue another read from the PTD.
*/

void PtdReadWriteAst (struct WsLibStruct *wsptr)

{
   int  status;
   struct PtdClient  *clptr;

   /*********/
   /* begin */
   /*********/

   clptr = WsLibGetUserData(wsptr);

   status = WsLibWriteStatus (wsptr);
   if (VMSok (status))
   {
      clptr->PtdQueuedRead++;
      ptd$read (0, clptr->ptdchan, &PtdReadAst, clptr,
                clptr->PtdReadBuffer, sizeof(clptr->PtdReadBuffer));
   }
   else
      WsLibClose (wsptr, 0, NULL);
}

/*****************************************************************************/
/*
Asynchronous read from a WebSocket client has concluded.
*/

void PtdReadClient (struct WsLibStruct *wsptr)

{
   int  cnt;
   struct PtdClient  *clptr;

   /*********/
   /* begin */
   /*********/

   if (VMSnok (WsLibReadStatus(wsptr)))
   {
      /* WEBSOCKET_INPUT read error (can be EOF) */
      WsLibClose (wsptr, 0, NULL);
      return;
   }

   clptr = WsLibGetUserData(wsptr);

   if (cnt = WsLibReadCount(wsptr))
   {
      if (!memcmp (clptr->InputBuffer,
                   DCLinaboxEscape,
                   sizeof(DCLinaboxEscape)-1))
      {
         ClientEscape (clptr, clptr->InputBuffer, cnt);

         /* queue the next read from the client */
         WsLibRead (wsptr,
                    clptr->InputBuffer,
                    sizeof(clptr->InputBuffer),
                    PtdReadClient);
      }
      else
         PtdWrite (clptr, clptr->InputBuffer, cnt);

      /* keep track of client input (for idle timeout) */
      clptr->ClientCount++;

      /* reset on continued client (keyboard) input */
      if (clptr->LogoutResponse) clptr->LogoutResponse--;
   }
   else
      /* otherwise queue the next read from the client */
      WsLibRead (wsptr,
                 clptr->InputBuffer,
                 sizeof(clptr->InputBuffer),
                 PtdReadClient);
}

/*****************************************************************************/
/*
Write the supplied data to the PTD (i.e. to the system).
*/

void PtdWrite
(
struct PtdClient *clptr,
char *DataPtr,
int DataCount
)
{
   int  status;
   char  *bptr, *cptr, *sptr, *zptr;

   /*********/
   /* begin */
   /*********/

   sptr = bptr = clptr->PtdWriteBuffer + sizeof(short)+sizeof(short);
   zptr = sptr + sizeof(clptr->PtdWriteBuffer) - sizeof(short)-sizeof(short);
   if (DataCount < 0) DataCount = strlen(DataPtr);
   for (cptr = DataPtr; DataCount-- && sptr < zptr; *sptr++ = *cptr++);
   clptr->PtdWriteCount = sptr - bptr;

   clptr->PtdQueuedWrite++;
   ptd$write (clptr->ptdchan, PtdWriteAst, clptr,
              clptr->PtdWriteBuffer, clptr->PtdWriteCount, 0, 0);
}

/*****************************************************************************/
/*
PTD write (to system) has completed.  If OK read from the WebSocket client.
*/

void PtdWriteAst (struct PtdClient *clptr)

{
   int  status;

   /*********/
   /* begin */
   /*********/

   if (clptr->PtdQueuedWrite) clptr->PtdQueuedWrite--;

   status = *(short*)clptr->PtdWriteBuffer;
   if (VMSok(status) ||
       status == SS$_DATAOVERUN ||
       status == SS$_DATALOST)
      WsLibRead (clptr->WsLibPtr,
                 clptr->InputBuffer,
                 sizeof(clptr->InputBuffer),
                 PtdReadClient);
   else
      PtdClose (clptr);
}

/*****************************************************************************/
/*
Client has sent a DCLinabox escape sequence.
*/

void ClientEscape
(
struct PtdClient *clptr,
char *DataPtr,
int DataCount
)
{
   unsigned int  cols, rows;
   char  *cptr, *zptr;

   /*********/
   /* begin */
   /*********/

   zptr = (cptr = DataPtr) + DataCount;
   if (!memcmp (cptr, TermSizeEscape, sizeof(TermSizeEscape)-1))
   {
      /* resize terminal sequence */
      cols = rows = -1;
      cptr += sizeof(TermSizeEscape)-1;
      cols = atoi(cptr);
      while (isdigit(*cptr) && cptr < zptr) cptr++;
      if (*cptr == 'x') cptr++;
      rows = atoi(cptr);
      while (isdigit(*cptr) && cptr < zptr) cptr++;
      if (*cptr) cols = rows = -1;
      if (cols < 48 || cols > 511) cols = (unsigned int)-1;
      if (rows < 10 || rows > 255) rows = (unsigned int)-1;

      ptd$decterm_set_page_size (clptr->ptdchan, rows, cols);

      AdviseClientTermSize (clptr);
   }
}

/*****************************************************************************/
/*
GETDVI the terminal width and height and advise the client using the
appropriate DCLinabox escape sequence.
*/

void AdviseClientTermSize (struct PtdClient *clptr)

{
   static unsigned long  DevBufSizItem = DVI$_DEVBUFSIZ,
                         TtPageItem = DVI$_TT_PAGE;

   int  cnt;
   unsigned long  DevBufSiz,
                  TtPage;
   char  *cptr, *sptr, *zptr;
   char  TermSize [sizeof(TermSizeEscape)+32];

   /*********/
   /* begin */
   /*********/

   lib$getdvi (&TtPageItem, &clptr->ptdchan, 0, &TtPage, 0, 0);
   lib$getdvi (&DevBufSizItem, &clptr->ptdchan, 0, &DevBufSiz, 0, 0);

   zptr = (sptr = TermSize) + sizeof(TermSize)-32;
   for (cptr = TermSizeEscape; *cptr && sptr < zptr; *sptr++ = *cptr++);
   sptr += sprintf (sptr, "%dx%d", DevBufSiz, TtPage);

   WsLibWrite (clptr->WsLibPtr, TermSize, sptr-TermSize, WSLIB_ASYNCH);
}

/*****************************************************************************/
/*
Logical name value DCLINABOX_ENABLE controls whether this script can be used. 
Make the value "*" to allow all remote hosts.  Alternatively provide one or
more comma-separated, dotted-decimal IP address to specify one or more hosts
allowed to use the script, and/or one or more comman-separated IP addresses and
CIDR subnet mask to specify a range of hosts.  IPv4 only!
*/

int DCLinaboxEnable ()

{
   unsigned int  IPaddr, IPnet, mask;
   char  ch;
   char  *aptr, *cptr, *sptr, *zptr;

   /*********/
   /* begin */
   /*********/

   if (!(cptr = SysTrnLnm (EnableLogicalName, NULL, 0)))
   {
      fprintf (stdout, "Status: 403 \"%s\" undefined\r\n\r\n",
               EnableLogicalName);
      exit (1);
   }

   if (!(aptr = WsLibCgiVarNull("REMOTE_ADDR"))) return (0);
   /* inet_aton() is not available on VMS V7.2 */
   IPaddr = inet_addr (aptr);
   if (IPaddr == 0xffffffff) return (0);

   if (!strstr (cptr, "ws:"))
   {
      if (!(sptr = WsLibCgiVarNull ("REQUEST_SCHEME"))) return (0);
      if (strcmp(sptr,"wss:") && strcmp(sptr,"https:"))
      {
         fprintf (stdout, "Status: 403 Must be SSL\r\n\r\n");
         return (0);
      }
   }

   if (strchr (cptr, '*')) return (1);

   while (*cptr && *sptr)
   {
      while (*cptr && *cptr != ',') cptr++;
      if (ch = *cptr) *cptr = '\0';
      if (zptr = strchr (sptr, '/'))
      {
         /* subnet mask */
         *zptr = '\0';
         /* inet_aton() is not available on VMS V7.2 */
         IPnet = inet_addr (sptr);
         if (IPnet == 0xffffffff) return (0);
         mask = atoi(zptr+1);
         mask = 0xffffffff >> (32 - mask); 
         /* if both addresses are valid and masked client address matches */
         if (IPaddr && IPnet && (IPnet == (IPaddr & mask))) return (1);
         *zptr = '/';
      }
      else
      if (!strcmp (sptr, aptr))
         return (1);
      if (*cptr = ch) cptr++;
      sptr = cptr;
   }

   fprintf (stdout, "Status: 403 Not Permitted\r\n\r\n");

   return (0);
}

/*****************************************************************************/
/*
See description in code prologue.  Returns SS$_NORMAL if single sign-on has
been validated and should be performed, SS$_NOMOREITEMS (still a success
status) if DCLinabox without SSO is permitted, and SS$_INVLOGIN if DCLinabox
usage is not permitted without SSO.
*/

int DCLinaboxSingleSignOn (struct PtdClient *clptr)

{
   static unsigned long  SysPrvMask [2] = { PRV$M_SYSPRV, 0 };
   static unsigned long  UaiFlags;
   static unsigned long  UaiPriv [2];
   static struct {
      short int  buf_len;
      short int  item;
      void  *buf_addr;
      unsigned short  *ret_len;
   } UaiItems [] =
   {
      { sizeof(UaiFlags), UAI$_FLAGS, &UaiFlags, 0 },
      { sizeof(UaiPriv), UAI$_PRIV, &UaiPriv, 0 },
      { 0,0,0,0 }
   };
   static $DESCRIPTOR (UserNameDsc, "");

   int  idx, ptatus, status,
        NotUserName = 0;
   char  *cptr, *sptr, *zptr,
         *AuthRealm,
         *RemoteUser;
   struct WsLibStruct  *wsptr;

   /*********/
   /* begin */
   /*********/

   if (WsLibCgiVarNull("WWW_PAPI_ASSERT"))
   {
      /* PAPI SSO environment */
      if (!(AuthRealm = WsLibCgiVarNull("WWW_PAPI_CN")))
         return (SS$_NOMOREITEMS);
      while (*AuthRealm && *AuthRealm != '@') AuthRealm++;
      if (*AuthRealm) AuthRealm++;
      if (!*AuthRealm) return (SS$_NOMOREITEMS);
   }
   else
   {
      if (!(AuthRealm = WsLibCgiVarNull("WWW_AUTH_REALM")))
         return (SS$_NOMOREITEMS);
      if (!*AuthRealm) return (SS$_NOMOREITEMS);
   }

   if (!(RemoteUser = WsLibCgiVarNull("WWW_REMOTE_USER")))
      return (SS$_NOMOREITEMS);
   if (!*RemoteUser) return (SS$_NOMOREITEMS);

   UserNameDsc.dsc$a_pointer = RemoteUser;
   UserNameDsc.dsc$w_length = strlen(RemoteUser);

   wsptr = clptr->WsLibPtr;

   for (idx = 0; idx <= 127; idx++)
   {
      if (!(cptr = SysTrnLnm (SingleLogicalName, NULL, idx))) break;

      WsLibWatchScript (wsptr, FI_LI, "\"!AZ\"", cptr);
      for (sptr = cptr; *sptr && *sptr != '='; sptr++);
      if (*sptr) *sptr++ = '\0';

      /* if the realm name does not match then look for the next */
      WsLibWatchScript (wsptr, FI_LI, "\"!AZ\" \"!AZ\"", AuthRealm, cptr);
      if (!*cptr || strcasecmp (cptr, AuthRealm)) continue;

      while (*cptr)
      {
         for (cptr = sptr; *sptr && *sptr != ','; sptr++);
         if (!*cptr) break;
         if (*sptr) *sptr++ = '\0';
         if (!*cptr) continue;

         WsLibWatchScript (wsptr, FI_LI, "\"!AZ\" \"!AZ\"", RemoteUser, cptr);

         if (*cptr == '!') cptr++;

         if (strcasecmp (cptr, RemoteUser) && *cptr != '*') continue;

         /* check the account status */
         ptatus = sys$setprv (1, &SysPrvMask, 0, 0);
         if (VMSnok (ptatus)) EXIT_FI_LI(ptatus);

         status = sys$getuai (0, 0, &UserNameDsc, &UaiItems, 0, 0, 0);

         ptatus = sys$setprv (0, &SysPrvMask, 0, 0);
         if (VMSnok (ptatus)) EXIT_FI_LI(ptatus);

         if (VMSnok (status))
         {
            WsLibWatchScript (wsptr, FI_LI, "$GETUAI %X!8XL", status);
            return (SS$_NOMOREITEMS);
         }

         if (UaiFlags & UAI$M_DISACNT)
         {
            WsLibWatchScript (wsptr, FI_LI, "UAI flags !8XL", UaiFlags);
            return (SS$_NOMOREITEMS);
         }

         /* wildcard match (allows privileged) */
         if (*(USHORTPTR)cptr == '**') break;

         /* if the user name matches */
         if (!strcasecmp (cptr, RemoteUser))
         {
            if (*(cptr-1) == '!')
            {
               NotUserName = 1;
               continue;
            }
            NotUserName = 0;
            break;
         }

         /* check for vanilla user */
         if ((UaiPriv[0] & ~(PRV$M_NETMBX | PRV$M_TMPMBX)) || UaiPriv[1])
         {
            WsLibWatchScript (wsptr, FI_LI, "UAI priv !8XL !8XL",
                              UaiPriv[0], UaiPriv[1]);
            return (SS$_NOMOREITEMS);
         }

         break;
      }

      if (*cptr) break;
   }

   if (cptr && *cptr && *(cptr-1) != '!' && !NotUserName)
   {
      zptr = (sptr = clptr->VmsUserName) + sizeof(clptr->VmsUserName)-1;
      for (cptr = RemoteUser; *cptr && sptr < zptr; *sptr++ = *cptr++);
      if (sptr > zptr)
      {
         /* hmmm, something's askew! */
         clptr->VmsUserName[0] = '\0';
         return (SS$_RESULTOVF);
      }
      *sptr = '\0';
      return (SS$_NORMAL);
   }

   if (cptr && *(USHORTPTR)(cptr-1) == '!*')
   {
      /* only available to SSO */
      return (SS$_INVLOGIN);
   }

   return (SS$_NOMOREITEMS);
}

/*****************************************************************************/
/*
Timer-driven function, called once every fifteen seconds to 1) set the title of
any new terminal window(s) and any idle timeout, 2) every sixty seconds (four
iterations) check the process name associated with the terminal and reset the
title if necessary (if INSTALLed with WORLD privilege), and 3) manage idle
terminals (if configured).
*/

void SessionManagement ()

{
   /* fifteen seconds */
   static unsigned long  TimerDelta [2] = { -150000000, -1 };
   static unsigned long  WorldMask [2] = { PRV$M_WORLD, 0 };
   static unsigned long  DviOwnUic,
                         DviPid;
   static int  AlertMsgLen,
               IdleMins,
               WarnMins;
   static unsigned short  DviHostNameLen,
                          JpiPrcNamLen;
   static int  GetPrcNam = 1,
               WaitForIt;
   static char  *WarnMsgPtr;
   static char  AlertMsg [sizeof(AlertEscape)+256],
                DviDevNam [64+1],
                DviHostName [8+1],
                IdleLogicalValue [256],
                IdentString [64],
                JpiPrcNam [15+1];
   static $DESCRIPTOR (UicFaoDsc, "!%I\0");
   static $DESCRIPTOR (IdentStringDsc, IdentString);
   static struct {
      short int  buf_len;
      short int  item;
      void  *buf_addr;
      unsigned short  *ret_len;
   }
   DviItems [] =
   {
      { sizeof(DviPid), DVI$_PID, &DviPid, 0 },
      { sizeof(DviOwnUic), DVI$_OWNUIC, &DviOwnUic, 0 },
      { sizeof(DviHostName), DVI$_HOST_NAME, &DviHostName, &DviHostNameLen },
      { 0,0,0,0 }
   },
   JpiItems [] =
   {
      { sizeof(JpiPrcNam)-1, JPI$_PRCNAM, &JpiPrcNam, &JpiPrcNamLen },
      { 0,0,0,0 }
   };

   int  ptatus, status,
        NewSession,
        NewTitle;
   unsigned long  CurrentTime;
   unsigned long  CurrentBinTime [2];
   char  *aptr, *cptr, *sptr, *zptr;
   char  EscapeBuffer [sizeof(DCLinaboxEscape)+256+16];
   struct PtdClient  *clptr;
   struct WsLibStruct  *wsptr = NULL;

   /*********/
   /* begin */
   /*********/

   sys$gettim (&CurrentBinTime);
   CurrentTime = decc$fix_time (&CurrentBinTime);

   /* only do some things every 60 (4 x 15) seconds or so */
   if (WaitForIt)
      WaitForIt--;
   else
      WaitForIt = 4;

   if (!WaitForIt)
   {
      /****************/
      /* every minute */
      /****************/

      /* idle session management can be changed at any point */
      IdleMins = WarnMins = 0;
      WarnMsgPtr = NULL;
      if (cptr = SysTrnLnm (IdleLogicalName, IdleLogicalValue, 0))
      {
         IdleMins = atoi(cptr);
         while (*cptr && *cptr != ',') cptr++;
         if (*cptr) cptr++;
         WarnMins = atoi(cptr);
         while (*cptr && *cptr != ',') cptr++;
         if (*cptr) cptr++;
         if (!*(WarnMsgPtr = cptr)) WarnMsgPtr = DEFAULT_WARN_MESSAGE;
      }
      /* defining idle minutes to -1 disables idle session management */
      if (IdleMins >= 0)
      {
         if (!IdleMins) IdleMins = DEFAULT_IDLE_MINS;
         if (!WarnMins) WarnMins = DEFAULT_WARN_MINS;
         if (IdleMins <= WarnMins) IdleMins = WarnMins + DEFAULT_WARN_MINS;
         if (!WarnMsgPtr) WarnMsgPtr = DEFAULT_WARN_MESSAGE;
      }

      /* check for the presence of an ALERT logical name and value */
      if (aptr = SysTrnLnm (AlertLogicalName, NULL, 0))
      {
         if (!AlertMsg[0] || strcmp (aptr, AlertMsg+sizeof(AlertEscape)-1))
         {
            /* value has been defined/changed since last time */
            while (WsLibNext(&wsptr))
            {
               /* if not a new session reset alerted flag */
               clptr = WsLibGetUserData(wsptr);
               if (clptr->DviOwnUic) clptr->Alerted = 0;
            }
            zptr = (sptr = AlertMsg) + sizeof(AlertMsg)-1;
            for (cptr = AlertEscape; *cptr && sptr < zptr; *sptr++ = *cptr++);
            while (*aptr && sptr < zptr) *sptr++ = *aptr++;
            *sptr = '\0';
            AlertMsgLen = sptr - AlertMsg;
         }
      }
      else
         AlertMsg[0] = '\0';
   }

   /****************/
   /* all sessions */
   /****************/

   while (WsLibNext(&wsptr))
   {
      clptr = WsLibGetUserData(wsptr);

      NewSession = 0;
      if (!clptr->DviOwnUic)
      {
         /***************/
         /* new session */
         /***************/

         NewSession = 1;

         status = sys$getdviw (0, clptr->ptdchan, 0, &DviItems, 0, 0, 0, 0, 0);
         if (VMSnok (status)) continue;

         /* for a LOGINOUT terminal, ownership is changed after login */
         if (DviOwnUic == ScriptUic) continue;

         clptr->DviOwnUic = DviOwnUic;
         clptr->DviPid = DviPid;
         DviHostName[DviHostNameLen] = '\0';
         strcpy (clptr->DviHostName, DviHostName);

         sys$fao (&UicFaoDsc, 0, &IdentStringDsc, clptr->DviOwnUic);
         /* strip the [] from the identifier */
         zptr = (sptr = clptr->OwnIdent) + sizeof(clptr->OwnIdent)-1;
         if (*(cptr = IdentString) == '[') cptr++;
         while (*cptr && *cptr != ']' && sptr < zptr) *sptr++ = *cptr++;
         *sptr = '\0';
      }

      /*****************/
      /* session title */
      /*****************/

      NewTitle = 0;
      if (!WaitForIt || NewSession)
      {
         if (GetPrcNam)
         {
            ptatus = sys$setprv (1, &WorldMask, 0, 0);
            if (VMSnok (ptatus)) EXIT_FI_LI(ptatus);

            status = sys$getjpiw (0, &clptr->DviPid, 0, &JpiItems, 0, 0, 0);

            ptatus = sys$setprv (0, &WorldMask, 0, 0);
            if (VMSnok (ptatus)) EXIT_FI_LI(ptatus);

            if (VMSok(status))
            {
               JpiPrcNam[JpiPrcNamLen] = '\0';
               if (strcmp (JpiPrcNam, clptr->JpiPrcNam))
               {
                  strcpy (clptr->JpiPrcNam, JpiPrcNam);
                  NewTitle = 1;
               }
            }
            else
            {
               /* presumably not installed with WORLD privilege */
               GetPrcNam = 0;
               JpiPrcNam[0] = '\0';
            }
         }
         else
            JpiPrcNam[0] = '\0';
      }

      if (NewTitle || NewSession)
      {
         zptr = (sptr = EscapeBuffer) + sizeof(EscapeBuffer)-1;
         for (cptr = TitleEscape; *cptr && sptr < zptr; *sptr++ = *cptr++);
         for (cptr = "DCLinabox: "; *cptr && sptr < zptr; *sptr++ = *cptr++);
         for (cptr = clptr->HttpHost; *cptr && sptr < zptr; *sptr++ = *cptr++);
         if (sptr < zptr) *sptr++ = ' ';
         for (cptr = clptr->DviHostName; *cptr && sptr < zptr; *sptr++ = *cptr++);
         for (cptr = ":: "; *cptr && sptr < zptr; *sptr++ = *cptr++);
         for (cptr = clptr->OwnIdent; *cptr && sptr < zptr; *sptr++ = *cptr++);
         if (JpiPrcNam[0])
         {
            for (cptr = " \""; *cptr && sptr < zptr; *sptr++ = *cptr++);
            for (cptr = JpiPrcNam; *cptr && sptr < zptr; *sptr++ = *cptr++);
            if (sptr < zptr) *sptr++ = '\"';
         }

         WsLibWrite (wsptr, EscapeBuffer, sptr-EscapeBuffer, WSLIB_ASYNCH);
      }

      /****************/
      /* idle session */
      /****************/

      if (IdleMins != clptr->IdleMins ||
          WarnMins != clptr->WarnMins)
      {
         /* (re)set and (re)calculate */
         clptr->IdleMins = IdleMins;
         clptr->WarnMins = WarnMins;
         if (IdleMins > 0)
         {
            clptr->IdleTime = CurrentTime + (clptr->IdleMins * 60);
            clptr->WarnTime = clptr->IdleTime - (clptr->WarnMins * 60);
            clptr->IdleCount = clptr->ClientCount;
         }
         else
           clptr->IdleTime = clptr->WarnTime = 0;
      }
      else
      if (clptr->IdleTime && clptr->ClientCount > clptr->IdleCount)
      {
         /* there has been client input since last time - reset timeout */
         clptr->IdleCount = clptr->ClientCount;
         clptr->IdleTime = CurrentTime + (clptr->IdleMins * 60);
         clptr->WarnTime = clptr->IdleTime - (clptr->WarnMins * 60);
      }
      else
      if (clptr->IdleTime && clptr->IdleTime < CurrentTime)
      {
         clptr->IdleTime = clptr->WarnTime = 0;
         clptr->Alerted = 1;
         WsLibClose (wsptr, 0, NULL);

         /* avoid trying to bang out an alert message after closure */
         continue;
      }
      else
      if (clptr->WarnTime && clptr->WarnTime < CurrentTime)
      {
         clptr->WarnTime = 0;
         zptr = (sptr = EscapeBuffer) + sizeof(EscapeBuffer)-16;
         for (cptr = AlertEscape; *cptr && sptr < zptr; *sptr++ = *cptr++);
         for (cptr = WarnMsgPtr;
              *cptr && *(USHORTPTR)cptr != '%d' && sptr < zptr;
              *sptr++ = *cptr++);
         if (*(USHORTPTR)cptr == '%d')
         {
            cptr += 2;
            sprintf (sptr, "%d", WarnMins);
            while (*sptr && sptr < zptr) sptr++;
            while (*cptr && sptr < zptr) *sptr++ = *cptr++;
         }
         WsLibWrite (wsptr, EscapeBuffer, sptr-EscapeBuffer, WSLIB_ASYNCH);

         /* avoid banging out an alert message at the same time */
         continue;
      }

      /*****************/
      /* alert message */
      /*****************/

      if (AlertMsg[0] && !clptr->Alerted)
      {
         clptr->Alerted = 1;
         WsLibWrite (clptr->WsLibPtr, AlertMsg, AlertMsgLen, WSLIB_ASYNCH);
      }
   }

   status = sys$setimr (0, &TimerDelta, SessionManagement, 0, 0);
   if (VMSnok(status)) EXIT_FI_LI (status);
}

/*****************************************************************************/
/*
Translate a logical name using LNM$FILE_DEV.  Returns a pointer to the value
string, or NULL if the name does not exist.  If 'LogValue' is supplied the
logical name is translated into that (assumed to be large enough), otherwise
it's translated into an internal static buffer.  'IndexValue' should be zero
for a 'flat' logical name, or 0..127 for interative translations.
*/

char* SysTrnLnm
(
char *LogName,
char *LogValue,
int IndexValue
)
{
   static unsigned short  ValueLength;
   static unsigned long  LnmAttributes,
                         LnmIndex;
   static char  StaticLogValue [256];
   static $DESCRIPTOR (LogNameDsc, "");
   static $DESCRIPTOR (LnmTableDsc, "LNM$FILE_DEV");
   static struct {
      short int  buf_len;
      short int  item;
      void  *buf_addr;
      unsigned short  *ret_len;
   } LnmItems [] =
   {
      { sizeof(LnmIndex), LNM$_INDEX, &LnmIndex, 0 },
      { sizeof(LnmAttributes), LNM$_ATTRIBUTES, &LnmAttributes, 0 },
      { 255, LNM$_STRING, 0, &ValueLength },
      { 0,0,0,0 }
   };

   int  status;
   char  *cptr;

   /*********/
   /* begin */
   /*********/

   LnmIndex = IndexValue;

   LogNameDsc.dsc$a_pointer = LogName;
   LogNameDsc.dsc$w_length = strlen(LogName);
   if (LogValue)
      cptr = LnmItems[2].buf_addr = LogValue;
   else
      cptr = LnmItems[2].buf_addr = StaticLogValue;

   status = sys$trnlnm (0, &LnmTableDsc, &LogNameDsc, 0, &LnmItems);
   if (!(status & 1) || !(LnmAttributes & LNM$M_EXISTS))
   {
      if (LogValue) LogValue[0] = '\0';
      return (NULL);
   }

   cptr[ValueLength] = '\0';
   return (cptr);
}

/*****************************************************************************/

