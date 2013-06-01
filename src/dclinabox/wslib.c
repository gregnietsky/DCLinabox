/*****************************************************************************/
#ifdef COMMENTS_WITH_COMMENTS
/*
                                   wsLIB.c

WebSocket library for WASD persistent WebSocket applications (CGIplus scripts).

  http://tools.ietf.org/html/rfc6455

This library allows AST-driven, multi-client WebSocket applications.

It also abstracts as much of the required functionality into callable functions
using optional string descriptors so as to minimise dependency on the C
language and on knowing the internals of the library data structure. 
Supplementary but not necessarily general-purpose client functionality is
provided by WSLIBCL.C, and WSLIB.C contains some minor but necessary
functionality to support not only the primary IPC via the mailboxes of the WASD
CGIplus/WebSocket environment but also WebSocket comms via the TCP/IP socket
established by the client connection functionality.

The WebSocket read is best as a single thread (at least the client-driven
logic) within the processing and the writes  (and "WATCH:" callout) can support
multiple, concurrent I/O per thread.  Callout AST functions cannot themselves
call WsLibWatchScript() or WsLibCallout() (i.e. a callout inside a callout).

Check the application examples within the [SRC.WEBSOCKET] directory for
illustrations of use.

All sizes are unsigned longwords and allow ranges from 0 to 4294967295.

Any function name containing a double-underscore is for internal wsLIB purposes 
and NOT intended for application calls!

The default content (and frame type) is 8 bit ASCII text which is implicitly
converted to and from UTF-8 during reads and writes.  This can also be
explicitly set using WsLibSetAscii().  If the content is already UTF-8 or is
required to remain UTF-8 then using WsLibSetUtf8() sets the conversion off.  Of
course binary content, set using WsLibSetBinary(), is opaque.

The library contains WATCH points.  Network [x]Data and [x]Script provide a
useful combination of traffic data.  The library function WsLibWatchScript()
allows WebSocket applications (scripts) to provide additional WATCHable
information via the [x]Script item.


WEBSOCKET MESSAGES
------------------
WebSocket data are transfered as autonomous messages that have text (UTF-8) or
binary (opaque) content.  Messages may be transported as multiple frames of
data but this is transparent to the WebSocket application.  When writing
WebSocket messages the data must remain available and unchanged for the
duration of the transfer.  With synchronous (blocking) writes this is implicit;
for asynchronous (non-blocking) writes this must be assured.

For WebSocket reads a fixed buffer may be provided, with a SS$_RESULTOVF error
reported if the message is larger than the buffer provided. 

  WsLibRead (<wsptr>, <buffer-addr>, <buffer-size>, <AST-addr>);

Alternatively, the wsLIB library can allocate a dynamic buffer appropriate to
the size of the message being received.  This allows considerable flexibility
for applications.  If the <buffer-addr> is NULL (zero) then dynamic message
buffering occurs.

  WsLibRead (<wsptr>, NULL, 0, <AST-addr>);

The default is for there to be no limit on the size of the buffered data (well,
actually 2^32-1 but who's counting?)  To provide an application limit on the
size of the read buffer specify NULL for the <buffer-addr> and an integer for
the maximum number of bytes allowed for the buffer.

  WsLibRead (<wsptr>, NULL, <max-buffer-size>, <AST-addr>);

For asynchronous I/O the allocated buffer is automatically freed unless
WsLibReadGrab() is used during the AST delivery routine.  For synchronous I/O a
dynamic buffer remains allocated and must be grabbed and then explicitly freed
by the in-line code after the I/O completes.


BASE FRAMING PROTOCOL
---------------------
http://tools.ietf.org/html/draft-ietf-hybi-thewebsocketprotocol-10

Section 4.1

      0                   1                   2                   3
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     +-+-+-+-+-------+-+-------------+-------------------------------+
     |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
     |I|S|S|S|  (4)  |A|     (7)     |             (16/63)           |
     |N|V|V|V|       |S|             |   (if payload len==126/127)   |
     | |1|2|3|       |K|             |                               |
     +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
     |     Extended payload length continued, if payload len == 127  |
     + - - - - - - - - - - - - - - - +-------------------------------+
     |                               |Masking-key, if MASK set to 1  |
     +-------------------------------+-------------------------------+
     | Masking-key (continued)       |          Payload Data         |
     +-------------------------------- - - - - - - - - - - - - - - - +
     :                     Payload Data continued ...                :
     + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
     |                     Payload Data continued ...                |
     +---------------------------------------------------------------+


FUNCTIONS
---------
struct WsLibStruct* WsLibCreate (void *UserDataPtr,
                                 void *DestroyFunction)

   Allocates a websocket structure, sets the user data pointer to the
   supplied address, and returns a pointer to the new structure.  All
   subsequent associated websocket processing requires that pointer.


void WsLibDestroy (struct WsLibStruct *wsptr)

   Deprecated.  (From v1.0.1 handled internally.)

struct WsLibStruct* WsLibNext (struct WsLibStruct **WsLibCtx)

   Step through the list of structures.  WsLibCtx is the address of pointer to
   WsLibStruct used to hold the context.  Set to NULL to initialise.  Returns
   non-null pointers for each structure in the list, then NULL when exhausted.
   Care must be exercised that multiple calls are not preempted by list
   modification (i.e. use within AST delivery or with ASTs disabled).


int WsLibMsgDsc (struct WsLibStruct *wsptr,
                 struct dsc$descriptor_s *MsgDsc);

   Sets the supplied descriptor to the latest error string.


int WsLibMsgLineNumber (struct WsLibStruct *wsptr)

   Return the wsLIB source code line at which the last message occured.


char* WsLibMsgString (struct WsLibStruct *wsptr)

   Returns a pointer to the latest error string.


int WsLibOpen (struct WsLibStruct *wsptr,
               void *AstFunction)

   Using the device names from the CGI variables WEBSOCKET_INPUT and
   WEBSOCKET_OUTPUT assign channels in preparation for asynchronous I/O.
   The AST function is called at websocket close.


int WsLibClose (struct WsLibStruct* wsptr,
                int StatusCode,
                char *StatusString)

   Send a close opcode and then shut the WebSocket down.
   Optional status code; if zero then a "normal" close status is added.
   Optional status string (description).


int WsLibIsClosed (struct WsLibStruct* wsptr)

   Return true if the WebSocket has been closed.


int WsLibConnected (struct WsLibStruct *wsptr)

   Return true if the input/output channels are connected.


void WsLibFree (void *cptr)

   Free the allocated memory (currently C-RTL but may not remain so).
   To be used after WsLibReadGrab().


int WsLibRead (struct WsLibStruct *wsptr,
               char *DataPtr,
               int DataSize,
               void *AstFunction)

   Read data from the websocket into the supplied buffer.
   Function is non-blocking if AST function is supplied, otherwise blocking.
   When the read completes the status can be returned by WsLibReadStatus(),
   the read count by WsLibReadCount(), a pointer to the data buffer using
   WsLibReadData(), and a pointer to the internal read string descriptor
   using WsLibReadDataDsc().


int WsLibReadDsc (struct WsLibStruct *wsptr,
                  struct dsc$descriptor_s *DataDsc,
                  struct dsc$descriptor_s *ReadDsc,
                  void *AstFunction)

   Read data from the websocket into the buffer described by DataDsc.
   Function is non-blocking if AST function is supplied, otherwise blocking.
   When completed ReadDsc (if supplied) is modified to reflect the read.


int WsLibReadIsBinary (struct WsLibStruct *wsptr)

   Return true if the most recent read used the BINARY opcode, false otherwise.


int WsLibReadIsText (struct WsLibStruct *wsptr)

   Return true if the most recent read used the TEXT opcode, false otherwise.


int WsLibReadStatus (struct WsLibStruct *wsptr)

   Return the status value of the most recent read.


int WsLibReadCount (struct WsLibStruct *wsptr)

   Return the number of bytes in the most recent read.


char* WsLibReadData (struct WsLibStruct *wsptr)

   Return a pointer to char of the most recent read buffer.


struct dsc$descriptor_s* WsLibReadDataDsc (struct WsLibStruct *wsptr)

   Return a pointer to the internal descriptor for the most recent read.


char* WsLibReadGrab (struct WsLibStruct *wsptr)

   Return a pointer to char of the most recent read buffer.
   The read buffer is removed from the WebSocket structure and becomes the
   resource of the grabber.  Must be WsLibFree()ed when no longer required.


unsigned long* WsLibReadTotal (struct WsLibStruct *wsptr)

   Return a pointer to a quadword containing the total number of bytes read.


void WsLibResetMsg (struct WsLibStruct *wsptr)

   Reset the latest message data.


unsigned int WsLibTime ();

   Return the current wsLIB time in seconds (i.e. C-RTL, Unix time).


int WsLibWrite (struct WsLibStruct *wsptr,
                char *DataPtr,
                int DataCount,
                void *AstFunction)

   Write the supplied data to the websocket.
   Function is non-blocking if AST function is supplied, otherwise blocking.
   If the AST function is supplied as (void*)-1 a non-blocking I/O is
   generated that does not require a target AST function.
   If DataPtr is NULL then a close is sent to the websocket.


int WsLibWriteDsc (struct WsLibStruct *wsptr,
                   struct dsc$descriptor_s *DataDsc,
                   void *AstFunction)

   Write the data supplied by the descriptor to the websocket.
   Function is non-blocking if AST function is supplied, otherwise blocking.


int WsLibWriteStatus (struct WsLibStruct *wsptr)

   Return the status value of the most recent write.


int WsLibWriteCount (struct WsLibStruct *wsptr)

   Return the number of bytes in the most recent write.


struct dsc$descriptor_s* WsLibWriteDataDsc (struct WsLibStruct *wsptr)

   Return a pointer to the internal descriptor for the most recent write.


unsigned long* WsLibWriteTotal (struct WsLibStruct *wsptr)

   Return a pointer to a quadword containing the total number of bytes written.


int WsLibPing (struct WsLibStruct *wsptr,
               char *DataPtr,
               int DataCount)

   Ping the remote end with 0 to 125 bytes of opaque data by char pointer.


int WsLibPingDsc (struct WsLibStruct *wsptr,
                  struct dsc$descriptor_s *DataDsc)

   Ping the remote end with 0 to 125 bytes of opaque data by string descr.


int WsLibPong (struct WsLibStruct *wsptr,
               char *DataPtr,
               int DataCount)

   Ping the remote end with an unsolicted pong.


int WsLibSetAscii (struct WsLibStruct *wsptr)

   Set the message content to 8 bit ASCII (and yes, I realise ASCII is
   only 7 bit but I needed a representative yet intuitive name for the
   function that's less easily confused with WedSocket 'text' content).
   A WebSocket set WsLibSetAscii() is implicitly converted to and from
   UTF-8 during writes and reads with WebSocket messages sent as 'text'.


int WsLibSetBinary (struct WsLibStruct *wsptr)

   Set the WebSocket message content to 'binary' and treat all reads and
   writes as opaque data.


int WsLibSetUtf8 (struct WsLibStruct *wsptr)

   Set the WebSocket message content to 'text' and treat all reads and
   writes as UTF-8 data (no implicit conversions).  Data must be legal
   UTF-8 or will/should be rejected by the local and/or remote end.


void WsLibSetUserData (struct WsLibStruct *wsptr,
                       void *UserDataPtr)

   Set the websocket structure to point to the supplied user data.


void* WsLibGetUserData (struct WsLibStruct *wsptr)

   Return the current pointer to the supplied user data.


void* WsLibSetCallout (struct WsLibStruct *wsptr,
                       void *AstFunction)

   Set/reset the callout response AST.
   Returns the previous callout pointer.


void* WsLibSetMsgCallback (struct WsLibStruct *wsptr,
                           void *CallbackFunction)

   Set/reset the wsLIB error reporting callback.
   Returns the previous callback pointer.
   The error callback function is activated with a pointer to the WsLib
   data structure that has the MsgLineNumber and timestamp set and
   provides a descriptive string.


int WsLibSetFrameMax (struct WsLibStruct *wsptr,
                      int FrameMax);

   Set the maximum sized frame before a message is fragmented.


void WsLibSetCloseSecs (struct WsLibStruct *wsptr,
                        int CloseSecs);

   Set number of seconds before an unresponded-to WebSocket close is
   considered closed and disconnected.
   If a WebSocket is not specified then set global value.


void WsLibSetIdleSecs (struct WsLibStruct *wsptr,
                       int IdleSecs);

   Set number of seconds before a WebSocket is considered idle and closed.
   If a WebSocket is not specified then set global value.


void WsLibSetLifeSecs (int LifeSecs);

   Set number of seconds before the application is considered idle and exited.


void WsLibSetPingSecs (struct WsLibStruct *wsptr,
                       int IdleSecs);

   Set number of seconds for periodic ping (heartbeat) of remote end.
   If a WebSocket is not specified then set global value.
   Remote end received a string containing and unsigned int ping number
   (1..n) followed by an unsigned int time in seconds (U*x epoch).


void WsLibSetReadSecs (struct WsLibStruct *wsptr,
                       int IdleSecs);

   Set number of seconds to wait for a read from a WebSocket before close.
   If a WebSocket is not specified then set global value.


void* WsLibSetWakeCallback (struct WsLibStruct *wsptr,
                            void *CallbackFunction,
                            int WakesSecs)

   Set/reset the wsLIB periodic wakeup callback.
   Returns the previous callback pointer.
   Set the number of seconds between wakeup calls (zero defaults).


int WsLibFromUtf8 (char *UtfPtr,
                   int UtfCount,
                   char SubsChar)

   Given a buffer of UTF-8 convert in-situ to 8 bit ASCII.
   Ignore non- 8 bit ASCII characters.
   End-of-string is indicated by text-length not a null-character,
   however the resultant string is nulled.  If supplied the 8 bit
   character 'SubsChar' is substituted for any non 8 bit code in the string.  
   Return the number of converted characters or -1 if there is an error. 
   The input string is mangled if an error.


int WsLibFromUtf8Dsc (struct dsc$descriptor_s *InDsc,
                      struct dsc$descriptor_s *OutDsc,
                      char SubsChar)

   Given a descriptor of UTF-8 convert in-situ to 8 bit ASCII.
   The output descriptor must be provided even if it points to the same
   storage as the input descriptor.  Return the length of the converted
   string or -1 to indicated a conversion error.
   The input string is mangled if an error.


int WsLibToUtf8 (char *InPtr,
                 int InLength,
                 char *OutPtr,
                 int SizeOfOut)

   Given a buffer of 8 bit ASCII text convert it to UTF-8. 
   This can be done in-situ with the worst-case the buffer space needs
   to be two times in size (i.e. every character has the hi bit set requiring
   a leading UTF-8 byte).  End-of-string is indicated by text-length not a
   null-character, however the resultant string is nulled.
   Return the length of the converted strin or -1 if the insufficient buffer.


int WsLibToUtf8Dsc (struct dsc$descriptor_s *InDsc,
                    struct dsc$descriptor_s *OutDsc)

   Given a descriptor of 8 bit ASCII convert to 8 bit ASCII.
   Can be done in-situ.  The output descriptor must be provided even
   if it points to the same storage as the input descriptor.
   Return the length of the converted string or -1 to indicated a
   conversion error.


void WsLibWatchScript (struct WsLibStruct *wsptr,
                       char *SourceModuleName,
                       int SourceLineNumber,
                       char *FormatString,
                       ...)

   Outputs the $FAO-formatted string to the WASD WATCH [x]Script item.



COPYRIGHT
---------
Copyright (C) 2011,2012 Mark G.Daniel
This program comes with ABSOLUTELY NO WARRANTY.
This is free software, and you are welcome to redistribute it under the
conditions of the GNU GENERAL PUBLIC LICENSE, version 3, or any later version.
http://www.gnu.org/licenses/gpl.txt

Function WsLib__Utf8Legal() contains code ...
Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.


VERSION HISTORY
---------------
08-DEC-2012  MGD  tidied some #includes
23-SEP-2012  MGD  v1.0.4, "clean"-up response to client close
15-AUG-2012  MGD  v1.0.3, refine WRITEOF and channel destruction
                          bugfix; remove WATCH from WsLib_Shut() :-}
21-JUL-2012  MGD  v1.0.2, refine shut and destruction sequence
                          add WASD_WSLIB_WATCH_LOG mechanism
26-NOV-2011  MGD  v1.0.1, refine I/O removing all $QIOW
                          WsLibClose() on all I/O non-success status
30-SEP-2011  MGD  v1.0.0, draft-ietf-hybi-thewebsocketprotocol-17
31-AUG-2011  MGD  draft-ietf-hybi-thewebsocketprotocol-13
23-AUG-2011  MGD  draft-ietf-hybi-thewebsocketprotocol-11
11-JUL-2011  MGD  draft-ietf-hybi-thewebsocketprotocol-10
13-JUN-2011  MGD  draft-ietf-hybi-thewebsocketprotocol-09
07-JUN-2011  MGD  draft-ietf-hybi-thewebsocketprotocol-07 (BANG!)
25-FEB-2011  MGD  draft-ietf-hybi-thewebsocketprotocol-06
05-FEB-2011  MGD  draft-ietf-hybi-thewebsocketprotocol-05
11-JAN-2011  MGD  draft-ietf-hybi-thewebsocketprotocol-04
17-OCT-2010  MGD  draft-ietf-hybi-thewebsocketprotocol-03
24-SEP-2010  MGD  draft-ietf-hybi-thewebsocketprotocol-02
26-AUG-2010  MGD  draft-ietf-hybi-thewebsocketprotocol-01 (initial)
*/
#endif /* COMMENTS_WITH_COMMENTS */
/*****************************************************************************/

#define SOFTWAREVN "1.0.4"
#define SOFTWARENM "WSLIB"
#ifdef __ALPHA
#  define SOFTWAREID SOFTWARENM " AXP-" SOFTWAREVN
#endif
#ifdef __ia64
#  define SOFTWAREID SOFTWARENM " IA64-" SOFTWAREVN
#endif
#ifdef __VAX
#  define SOFTWAREID SOFTWARENM " VAX-" SOFTWAREVN
#endif

#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <descrip.h>
#include <iodef.h>
#include <lib$routines.h>
#include <ssdef.h>
#include <starlet.h>
#include <stsdef.h>
#include <unixlib.h>

#include "wslib.h"

/* from libmsg.h */
#define LIB$_INVSTRDES 1409572

#ifndef EFN$C_ENF
#  define EFN$C_ENF 128  /* Event No Flag (no stored state) */
#endif

#define AGN$M_READONLY 0x01
#define AGN$M_WRITEONLY 0x02

#define VMSok(x) ((x) & STS$M_SUCCESS)
#define VMSnok(x) !(((x) & STS$M_SUCCESS))

#define FI_LI "WSLIB", __LINE__

#if 1
#define WATCH_WSLIB if(wsptr->WatchScript)WsLibWatchScript
#else
#define WATCH_WSLIB if(0)WsLibWatchScript
#endif

/* used by WSLIBCL.C */
int  WsLibEfnWait,
     WsLibEfnNoWait;

static int  CgiPlusEofLength = 0,
            CgiPlusEotLength = 0,
            CgiPlusEscLength = 0;

static unsigned long  CurrentTime,
                      WatchDogWakeTime;
static unsigned long  CurrentBinTime [2];

#define DEFAULT_WATCHDOG_CLOSE_SECS  5
#define DEFAULT_WATCHDOG_IDLE_SECS 120
#define DEFAULT_WATCHDOG_LIFE_SECS 120
#define DEFAULT_WATCHDOG_PING_SECS 600
#define DEFAULT_WATCHDOG_READ_SECS  60
#define DEFAULT_WATCHDOG_WAKE_SECS  60

static unsigned long  WatchDogCloseSecs = DEFAULT_WATCHDOG_CLOSE_SECS,
                      WatchDogIdleSecs = DEFAULT_WATCHDOG_IDLE_SECS,
                      WatchDogLifeSecs = DEFAULT_WATCHDOG_LIFE_SECS,
                      WatchDogPingSecs = DEFAULT_WATCHDOG_PING_SECS,
                      WatchDogReadSecs = DEFAULT_WATCHDOG_READ_SECS,
                      WatchDogWakeSecs = DEFAULT_WATCHDOG_WAKE_SECS;

static char  *CgiPlusEofPtr = NULL,
             *CgiPlusEotPtr = NULL,
             *CgiPlusEscPtr = NULL;

static struct WsLibStruct *WsLibListHead; 

static void  (*PongCallbackFunction)(),
             (*WakeCallbackFunction)();

static char  SoftwareID [] = SOFTWAREID;

#define SYI$_VERSION 4096
int sys$getsyi(__unknown_params);

/***
#define __VAX
#undef __ALPHA
***/

/*****************************************************************************/
/*
Initialise the library.
*/

void WsLibInit ()

{
   static char  GetSyiVer [8];
   static struct
   {
      unsigned short  buf_len;
      unsigned short  item;
      void  *buf_addr;
      void  *ret_len;
   } SyiItem [] =
   {
     { sizeof(GetSyiVer)-1, SYI$_VERSION, &GetSyiVer, 0 },
     { 0,0,0,0 }
   };

   int  status,
        VersionInteger;

   /*********/
   /* begin */
   /*********/

   /* just the once! */
   if (WsLibEfnWait) return;

   status = sys$getsyiw (0, 0, 0, &SyiItem, 0, 0, 0);
   if (VMSnok (status)) WsLibExit (NULL, FI_LI, status);
   VersionInteger = ((GetSyiVer[1]-48) * 100) + ((GetSyiVer[3]-48) * 10);
   if (GetSyiVer[4] == '-') VersionInteger += GetSyiVer[5]-48;
   if (VersionInteger >= 700)
      WsLibEfnWait = WsLibEfnNoWait = EFN$C_ENF;
   else
   {
      if (VMSnok (status = lib$get_ef (&WsLibEfnWait)))
         WsLibExit (NULL, FI_LI, status);;
      if (VMSnok (status = lib$get_ef (&WsLibEfnNoWait)))
         WsLibExit (NULL, FI_LI, status);;
   }

   WsLib__WatchDog ();
}

/*****************************************************************************/
/*
Return a pointer to the wsLIB version string.
*/

char* WsLibVersion ()

{
   /*********/
   /* begin */
   /*********/

   return (SoftwareID);
}

/*****************************************************************************/
/*
Sanity check the incoming request.  Provide error or continue/upgrade response.
Allocate a WebSocket I/O structure and set the internal user data storage. 
Insert at the head of the list.  Return a pointer to the allocated structure.
Can only WATCH_WSLIB after WsLibOpen().
*/

struct WsLibStruct* WsLibCreate
(
void *UserDataPtr,
void *DestroyFunction
)
{
   int  astatus,
        SecWebSocketVersion;
   char  *cptr, *sptr;
   struct WsLibStruct  *wsptr;

   /*********/
   /* begin */
   /*********/

   if (!WsLibEfnWait) WsLibInit ();

   astatus = sys$setast (0); 

   wsptr = (struct WsLibStruct*) calloc (1, sizeof(struct WsLibStruct));
   if (!wsptr) WsLibExit (NULL, FI_LI, vaxc$errno);

   if (cptr = getenv ("WASD_WSLIB_WATCH_LOG"))
      if (!(wsptr->WatchLog = fopen (cptr, "w", "shr=get")))
         WsLibExit (NULL, FI_LI, vaxc$errno);

   /* if a scripting application running under the server */
   if (WsLibCgiVarNull ("SERVER_SOFTWARE"))
   {
      if (cptr = WsLibCgiVarNull("WEBSOCKET_INPUT_MRS"))
         wsptr->InputMrs = atoi(cptr);
      else
         WsLibExit (NULL, FI_LI, SS$_BUGCHECK);

      if (cptr = WsLibCgiVarNull("WEBSOCKET_OUTPUT_MRS"))
         wsptr->OutputMrs = atoi(cptr);
      else
         WsLibExit (NULL, FI_LI, SS$_BUGCHECK);

      if (!(cptr = WsLibCgiVarNull("HTTP_SEC_WEBSOCKET_VERSION")))
         WsLibExit (NULL, FI_LI, SS$_BUGCHECK);

      SecWebSocketVersion = atoi(cptr);
      if (SecWebSocketVersion <= 0) WsLibExit (NULL, FI_LI, SS$_BUGCHECK);

      /* this logical name is also detected by the WASD server */
      if (!(sptr = getenv("WASD_WEBSOCKET_VERSION")))
         /* this is the wsLIB header string */
         sptr = WSLIB_WEBSOCKET_VERSION;
      while (*sptr)
      {
         if (atoi(sptr) == SecWebSocketVersion) break;
         while (isdigit(*sptr)) sptr++;
         while (*sptr && !isdigit(*sptr)) sptr++;
      }

      if (!*sptr)
      {
         /* WebSockets version not supported */
         fprintf (stdout,
"Status: 426 Upgrade Required\r\n\
Sec-Websocket-Version: %s\r\n\
\r\n",
                  WSLIB_WEBSOCKET_VERSION);
         fflush (stdout);
         free (wsptr);
         return (NULL);
      }

      wsptr->WebSocketVersion = SecWebSocketVersion;

      /* connection acceptance response */
      fprintf (stdout, "Status: 101 Switching Protocols\r\n\r\n");
      fflush (stdout);
   }
   else
   {
      /* first number listed in the macro should be the current version */
      wsptr->WebSocketVersion = atoi (WSLIB_WEBSOCKET_VERSION);

      /* the maximum socket record size is the maximum $QIO size */
      wsptr->InputMrs = wsptr->OutputMrs = 65535;
   }

   wsptr->FrameMaxSize = 4294967295;
   wsptr->UserDataPtr = UserDataPtr;
   wsptr->DestroyAstFunction = DestroyFunction;
   wsptr->NextPtr = WsLibListHead;
   WsLibListHead = wsptr;

   if (astatus == SS$_WASSET) sys$setast (1);
   return (wsptr);
}

/*****************************************************************************/
/*
Deprecated.  Now is just a stub.
*/

void* WsLibDestroy (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   return (wsptr->UserDataPtr);
}

/*****************************************************************************/
/*
Remove from the list and free the allocated memory.  Return any user data.
*/

static void WsLib__Destroy (struct WsLibStruct *wsptr)

{
   int  astatus, cnt, status;
   struct WsLibStruct  *wslptr;
   void  *UserDataPtr;
   FILE  *WatchLog;

   /*********/
   /* begin */
   /*********/

   if (!wsptr) return;

   WATCH_WSLIB (wsptr, FI_LI, "DESTROY");

   WatchLog = wsptr->WatchLog;

   astatus = sys$setast (0); 
   UserDataPtr = wsptr->UserDataPtr;

   if (wsptr->InBufferSize) free (wsptr->InBufferPtr);
   if (wsptr->OutBufferSize) free (wsptr->OutBufferPtr);
   if (wsptr->MsgStringSize) free (wsptr->MsgStringPtr);
   if (wsptr->ClientHeaderSize)
   {
      /* free WLIBCL.C storage */
      free (wsptr->ClientHeaderPtr);
      if (wsptr->ClientAcceptSize) free (wsptr->ClientAcceptPtr);
      if (wsptr->ClientKeySize) free (wsptr->ClientKeyPtr);
      if (wsptr->ClientServerSize) free (wsptr->ClientServerPtr);
      if (wsptr->ClientUriSize) free (wsptr->ClientUriPtr);
   }

   if ((wslptr = WsLibListHead) == wsptr)
      WsLibListHead = wslptr->NextPtr;
   else
   {
      while (wslptr->NextPtr != wsptr) wslptr = wslptr->NextPtr;
      wslptr->NextPtr = wsptr->NextPtr;
   }

   if (!wsptr->SocketChannel && wsptr->OutputChannel)
      sys$dassgn (wsptr->OutputChannel);

   free (wsptr);
   if (astatus == SS$_WASSET) sys$setast (1);

   if (WatchLog) fclose (WatchLog);
}

/*****************************************************************************/
/*
Step through the list of structures.  WsLibCtx is the address of a pointer used
to hold the context.  Set to NULL to initialise.  Returns non-null pointers for
each structure in the list, then NULL when list exhausted, to begin non-null
pointers again.  Care must be exercised that multiple calls are not preempted
by a list modification (i.e. use within AST delivery or with ASTs disabled).
*/

struct WsLibStruct* WsLibNext (struct WsLibStruct **WsLibCtx)
                                                    
{
   int  astatus;
   struct WsLibStruct  *wsptr, *wslptr;

   /*********/
   /* begin */
   /*********/

   astatus = sys$setast (0); 
   /* let's be overcautious and make sure it's still in the list! */
   if (wsptr = *WsLibCtx)
   {
      for (wslptr = WsLibListHead; wslptr; wslptr = wslptr->NextPtr)
         if (wslptr == wsptr) break;
      if (!wslptr) WsLibExit (NULL, FI_LI, SS$_BUGCHECK);
      *WsLibCtx = wsptr->NextPtr;
   }
   else
      *WsLibCtx = WsLibListHead;
   if (astatus == SS$_WASSET) sys$setast (1);
   return (*WsLibCtx);
}

/*****************************************************************************/
/*
Using the device names from the CGI variables WEBSOCKET_INPUT and
WEBSOCKET_OUTPUT assign channels in preparation for asynchronous I/O.
The AST function is called at WebSocket close.
Default data is 8 bit ASCII (that requires implicit UTF-8 encoding).
*/

int WsLibOpen (struct WsLibStruct *wsptr)

{
   int  status;
   char  *cptr, *sptr, *zptr;
   $DESCRIPTOR (MbxDsc, "");

   /*********/
   /* begin */
   /*********/

   wsptr->InputDataDsc.dsc$b_class = 
      wsptr->OutputDataDsc.dsc$b_class = DSC$K_CLASS_S;
   wsptr->InputDataDsc.dsc$b_dtype =
      wsptr->OutputDataDsc.dsc$b_dtype = DSC$K_DTYPE_T;

   if (!(cptr = WsLibCgiVarNull("WEBSOCKET_INPUT"))) return (SS$_BUGCHECK);
   zptr = (sptr = wsptr->InputDevName) + sizeof(wsptr->InputDevName)-1;
   while (*cptr && sptr < zptr) *sptr++ = *cptr++;
   *sptr = '\0';

   wsptr->InputDevDsc.dsc$b_class = DSC$K_CLASS_S;
   wsptr->InputDevDsc.dsc$b_dtype = DSC$K_DTYPE_T;
   wsptr->InputDevDsc.dsc$a_pointer = wsptr->InputDevName;
   wsptr->InputDevDsc.dsc$w_length = sptr - wsptr->InputDevName;

   if (!(cptr = WsLibCgiVarNull("WEBSOCKET_OUTPUT"))) return (SS$_BUGCHECK);
   zptr = (sptr = wsptr->OutputDevName) + sizeof(wsptr->OutputDevName)-1;
   while (*cptr && sptr < zptr) *sptr++ = *cptr++;
   *sptr = '\0';

   wsptr->OutputDevDsc.dsc$b_class = DSC$K_CLASS_S;
   wsptr->OutputDevDsc.dsc$b_dtype = DSC$K_DTYPE_T;
   wsptr->OutputDevDsc.dsc$a_pointer = wsptr->OutputDevName;
   wsptr->OutputDevDsc.dsc$w_length = sptr - wsptr->OutputDevName;

   status = sys$assign (&wsptr->InputDevDsc, &wsptr->InputChannel, 0, 0,
                        AGN$M_READONLY);
   if (VMSnok (status)) return (status);

   status = sys$assign (&wsptr->OutputDevDsc, &wsptr->OutputChannel, 0, 0,
                        AGN$M_WRITEONLY);
   if (VMSnok (status))
   {
      sys$dassgn (wsptr->InputChannel);
      wsptr->InputChannel = 0;
      return (status);
   }

   if (VMSnok (status))
   {
      sys$dassgn (wsptr->InputChannel);
      sys$dassgn (wsptr->OutputChannel);
      wsptr->InputChannel = wsptr->OutputChannel = 0;
      return (status);
   }

   /* default data is 8 bit "ASCII" text (requiring implicit UTF-8 encoding) */
   wsptr->SetAscii = 1;

   if (!(wsptr->WatchScript = (wsptr->WatchLog != NULL)))
      wsptr->WatchScript = (WsLibCgiVarNull("WATCH_SCRIPT") != NULL);

   if (wsptr->WatchDogPingSecs)
      wsptr->WatchDogPingTime = CurrentTime + wsptr->WatchDogPingSecs;

   WATCH_WSLIB (wsptr, FI_LI, "OPEN !AZ", SOFTWAREID);

   return (SS$_NORMAL);
}

/*****************************************************************************/
/*
Initiate a close from the application end.
Default status code is 1000 (normal closure).
To suppress any status code delivery specify -1.
WatchDog will shut if close doesn't happen in course.
*/

void WsLibClose
(
struct WsLibStruct *wsptr,
int StatusCode,
char *StatusString
)
{
   static char  DummyBuffer [125];

   int  status,
        FramePayload;
   unsigned char  *aptr, *cptr, *sptr, *zptr;
   struct WsLibFrmStruct  *frmptr;
   struct WsLibMsgStruct  *msgptr;

   /*********/
   /* begin */
   /*********/

   WATCH_WSLIB (wsptr, FI_LI, "CLOSE closed:!UL code:!SL \"!AZ\"",
                wsptr->WebSocketClosed, StatusCode,
                StatusString ? StatusString : "(null)");

   if (wsptr->WebSocketClosed)
   {
      WsLib__Shut (wsptr);
      return;
   }

   wsptr->WebSocketClosed = 1;

   /* allocate a pointer plus a structure (freed by WsLib__OutputFreeAst()) */ 
   aptr = calloc (1, sizeof(struct WsLibStruct*) +
                     sizeof(struct WsLibFrmStruct));
   if (!aptr) WsLibExit (wsptr, FI_LI, vaxc$errno);
   *(struct WsLibStruct**)aptr = wsptr;
   frmptr = (struct WsLibFrmStruct*)(aptr + sizeof(struct WsLibStruct*));

   if (!StatusCode)
      StatusCode = WSLIB_CLOSE_NORMAL;
   else
   if (StatusCode == WSLIB_CLOSE_BANG)
      StatusCode = 0;

   if (!StatusString)
   {
      switch (StatusCode)
      {
         case 0    : break;
         case 1000 : StatusString = "normal closure"; break;
         case 1001 : StatusString = "bye-bye"; break;
         case 1002 : StatusString = "protocol error"; break;
         case 1003 : StatusString = "received data unacceptable"; break;
         case 1004 : StatusString = "RESERVED"; break;
         case 1005 : StatusString = "RESERVED"; break;
         case 1006 : StatusString = "RESERVED"; break;
         case 1007 : StatusString = "received data inconsistency"; break;
         case 1008 : StatusString = "policy violation"; break;
         case 1009 : StatusString = "received message too big"; break;
         case 1010 : StatusString = "expected extention negotiation"; break;
         case 1011 : StatusString = "unexpected condition"; break;
         default   : StatusString = "unknown opcode";
      }
   }

   FramePayload = 0;
   frmptr->FrameHeader[0] = WSLIB_BIT_FIN | WSLIB_OPCODE_CLOSE;
   if (wsptr->RoleClient)
   {
      /* little messier with masking required */
      WsLib__MaskingKey (frmptr);
      frmptr->FrameHeader[2] = frmptr->MaskingKey[0];
      frmptr->FrameHeader[3] = frmptr->MaskingKey[1];
      frmptr->FrameHeader[4] = frmptr->MaskingKey[2];
      frmptr->FrameHeader[5] = frmptr->MaskingKey[3];
      if (StatusCode)
      {
         int  kcnt = 0;
         char  *kptr = (char*)frmptr->MaskingKey;
         frmptr->FrameHeader[6] =
            ((StatusCode & 0xff00) >> 8) ^ kptr[kcnt++&0x3];
         frmptr->FrameHeader[7] = (StatusCode & 0xff) ^ kptr[kcnt++&0x3];
         FramePayload = 2;
         if (StatusString)
         {
            zptr = (sptr = frmptr->FrameHeader + 8) + 123;
            for (cptr = (unsigned char*)StatusString;
                 *cptr && sptr < zptr;
                 *sptr++ = *cptr++ ^ kptr[kcnt++&0x3])
               FramePayload++;
         }
      }
      frmptr->FrameHeader[1] = frmptr->FrameMaskBit | FramePayload;
      FramePayload += 6;
   }
   else
   {
      if (StatusCode)
      {
         frmptr->FrameHeader[2] = (StatusCode & 0xff00) >> 8;
         frmptr->FrameHeader[3] = StatusCode & 0xff;
         FramePayload = 2;
         if (StatusString)
         {
            zptr = (sptr = frmptr->FrameHeader + 4) + 123;
            for (cptr = (unsigned char*)StatusString;
                 *cptr && sptr < zptr;
                 *sptr++ = *cptr++)
               FramePayload++;
         }
      }
      frmptr->FrameHeader[1] = FramePayload;
      FramePayload += 2;
   }

   status = sys$qio (WsLibEfnNoWait, wsptr->OutputChannel,
                     IO$_WRITELBLK | IO$M_READERCHECK,
                     0, WsLib__OutputFreeAst, aptr,
                     frmptr->FrameHeader, FramePayload, 0, 0, 0, 0);
   if (VMSok(status)) wsptr->QueuedOutput++;

   if (StatusCode == WSLIB_CLOSE_NORMAL ||
       StatusCode == WSLIB_CLOSE_BYEBYE ||
       StatusCode == WSLIB_CLOSE_POLICY)
   {
      /************************************/
      /* receive any close response frame */
      /************************************/

      msgptr = calloc (1, sizeof(struct WsLibMsgStruct));
      if (!msgptr) WsLibExit (wsptr, FI_LI, vaxc$errno);
      msgptr->WsLibPtr = wsptr;

      msgptr->DataMax = 4294967295;
      msgptr->DataPtr = DummyBuffer;
      msgptr->DataSize = sizeof(DummyBuffer);
      msgptr->AstFunction = WsLib__DummyClose;
   
      WsLib__ReadFrame (msgptr);
   }
   else
   {
      /*********************/
      /* significant error */
      /*********************/

      /* e.g. protocol error, do not try to continue */
      WsLib__Shut (wsptr);
      return;
   }
}

/*****************************************************************************/
/*
Essentially just a dummy target for the dummy read initiated following an
application WebSocket close, a read that allows any client response close frame
to be received and processed.
*/

static void WsLib__DummyClose (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   WATCH_WSLIB (wsptr, FI_LI, "CLOSE response %X!8XL", wsptr->InputStatus);
}

/*****************************************************************************/
/*
Respond to a close opcode from the client.
*/

static void WsLib__Close (struct WsLibFrmStruct *frmptr)

{
   int  status,
        CloseStatus,
        FramePayload;
   struct WsLibStruct  *wsptr;

   /*********/
   /* begin */
   /*********/

   wsptr = frmptr->WsLibMsgPtr->WsLibPtr;

   if (frmptr->DataCount >= 2)
   {
      CloseStatus = ((unsigned char)frmptr->DataPtr[0] << 8) +
                     (unsigned char)frmptr->DataPtr[1];
      frmptr->DataCount -= 2;
   }
   else
      CloseStatus = 0;

   WATCH_WSLIB (wsptr, FI_LI, "CLOSE code:!UL!AZ!#AZ",
                CloseStatus, frmptr->DataCount ? " " : "",
                frmptr->DataCount, frmptr->DataPtr+2);

   if (frmptr->DataCount)
      WsLib__MsgCallback (wsptr, __LINE__, SS$_SHUT, "CLOSE !UL !#AZ",
                          CloseStatus, frmptr->DataCount, frmptr->DataPtr+2);
   else
      WsLib__MsgCallback (wsptr, __LINE__, SS$_SHUT, "CLOSE 0");

   if (!wsptr->WebSocketClosed)
   {
      /* send the close opcode */
      wsptr->WebSocketClosed = 1;

      WATCH_WSLIB (wsptr, FI_LI, "CLOSE response");

      /* allocate a frame structure (freed by WsLib__CloseFreeAst()) */ 
      frmptr = (struct WsLibFrmStruct*)calloc(1,sizeof(struct WsLibFrmStruct));
      if (!frmptr) WsLibExit (wsptr, FI_LI, vaxc$errno);

      /* close frame header */
      frmptr->FrameHeader[0] = WSLIB_BIT_FIN | WSLIB_OPCODE_CLOSE;
      frmptr->FrameHeader[1] = 0;

      /* send the opcode asynchronously delivering to a specific AST */
      status = sys$qio (WsLibEfnNoWait, wsptr->OutputChannel,
                        IO$_WRITELBLK | IO$M_READERCHECK,
                        0, WsLib__CloseFreeAst, frmptr,
                        frmptr->FrameHeader, 2, 0, 0, 0, 0);
      /* immediately do the WebSocket shutdown, resulting a "clean" close */
   }

   WsLib__Shut (wsptr); 
}

/*****************************************************************************/
/*
Just deallocate the memory containing the close frame from WsLib__Close().
*/

void WsLib__CloseFreeAst (struct WsLibFrmStruct *frmptr)

{
   /*********/
   /* begin */
   /*********/

   free (frmptr);
}

/*****************************************************************************/
/*
Return true if the WebSocket has been closed.
*/

int WsLibIsClosed (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   return (wsptr->WebSocketClosed);
}

/*****************************************************************************/
/*
Shutdown the websocket (this is different to the close handshake).
Cancel any outstanding I/O and deassign channels.
Call any supplied closure AST function.
This function call be called multiple times before full shutdown accomplished.
Returns success status when finally shut, abort if not (fully) shut (yet).
*/

int WsLib__Shut (struct WsLibStruct *wsptr)

{
   int  status;

   /*********/
   /* begin */
   /*********/

   if (wsptr->DestroyAstFunction == &WsLib__Destroy) return (SS$_NORMAL);

   if (!wsptr->WebSocketShut)
   {
      if (wsptr->QueuedInput) sys$cancel (wsptr->InputChannel);

      if (!wsptr->SocketChannel)
      {
         /* let the server know the client is being disconnected */
         status = sys$qio (WsLibEfnNoWait, wsptr->OutputChannel,
                           IO$_WRITEOF | IO$M_NORSWAIT, 0,
                           WsLib__WriteEofAst, wsptr,
                           0, 0, 0, 0, 0, 0);
         if (VMSok(status)) wsptr->QueuedOutput++;
      }

      /* can be shut without having been closed (e.g. network error) */
      wsptr->WebSocketShut = wsptr->WebSocketClosed = 1;
   }

   /* if outstanding I/O */
   if (wsptr->QueuedInput || wsptr->QueuedOutput) return (SS$_ABORT);

   if (wsptr->SocketChannel)
   {
      /* client interface in use */
      sys$dassgn (wsptr->SocketChannel);
      wsptr->InputChannel = wsptr->OutputChannel = wsptr->SocketChannel = 0;
   }
   else
   {
      /* deassign of output channel is handled during destroy */
      sys$dassgn (wsptr->InputChannel);
      wsptr->InputChannel = 0;
   }

   /* first queue any client's destruction code */
   if (wsptr->DestroyAstFunction)
      sys$dclast (wsptr->DestroyAstFunction, wsptr, 0, 0);

   /* then queue the wsLIB structure destruction */
   sys$dclast ((wsptr->DestroyAstFunction = &WsLib__Destroy), wsptr, 0, 0);

   return (SS$_NORMAL);
}

/*****************************************************************************/
/*
Decrement the queued I/O counter and then recall the shut function.
*/

static void WsLib__WriteEofAst (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   if (wsptr->QueuedOutput) wsptr->QueuedOutput--;

   WsLib__Shut (wsptr);
}

/*****************************************************************************/
/*
Send a ping to the client using the (optional) data pointed to by the supplied
string descriptor.  For the responding pong to be notified a
WsLibSetPongCallback() must previously have been set.
*/

int WsLibPingDsc
(
struct WsLibStruct *wsptr,
struct dsc$descriptor_s *DataDsc
)
{
   int  status;

   /*********/
   /* begin */
   /*********/

   if (DataDsc == NULL)
      status = WsLibPing (wsptr, NULL, 0);
   else
   if (DataDsc->dsc$b_class != DSC$K_CLASS_S &&
       DataDsc->dsc$b_dtype != DSC$K_DTYPE_T)
      status = LIB$_INVSTRDES;
   else
      status = WsLib__PingPong (wsptr,
                                DataDsc->dsc$a_pointer,
                                DataDsc->dsc$w_length,
                                WSLIB_OPCODE_PING);
   return (status);
}

/*****************************************************************************/
/*
Wrapper for WsLib__PingPong() to send a ping.
*/

int WsLibPing
(
struct WsLibStruct *wsptr,
char *DataPtr,
int DataCount
)
{
   /*********/
   /* begin */
   /*********/

   return (WsLib__PingPong (wsptr, DataPtr, DataCount, WSLIB_OPCODE_PING));
}

/*****************************************************************************/
/*
Wrapper for WsLib__PingPong() to send an unsolicted pong.
*/

int WsLibPong
(
struct WsLibStruct *wsptr,
char *DataPtr,
int DataCount
)
{
   /*********/
   /* begin */
   /*********/

   return (WsLib__PingPong (wsptr, DataPtr, DataCount, WSLIB_OPCODE_PONG));
}

/*****************************************************************************/
/*
Send a ping/pong to the client using the (optional) data supplied.  For the
responding pong to be notified a SetPongCallback() must have been set.
*/

static int WsLib__PingPong
(
struct WsLibStruct *wsptr,
char *DataPtr,
int DataCount,
int OpCode
)
{
   int  cnt, hcnt, status;
   char  *aptr;
   struct WsLibFrmStruct  *frmptr;

   /*********/
   /* begin */
   /*********/

   if (OpCode == WSLIB_OPCODE_PING)
      WATCH_WSLIB (wsptr, FI_LI, "PING");
   else
      WATCH_WSLIB (wsptr, FI_LI, "PONG");

   if (wsptr->WebSocketClosed)
   {
      WsLib__MsgCallback (wsptr, __LINE__, SS$_SHUT, "can't ping; closed");
      return (SS$_SHUT);
   }

   if (DataCount > 125) DataCount = 125;

   /* allocate a pointer plus a structure (freed by WsLib__OutputFreeAst()) */ 
   aptr = calloc (1, sizeof(struct WsLibStruct*) +
                     sizeof(struct WsLibFrmStruct));
   if (!aptr) WsLibExit (wsptr, FI_LI, vaxc$errno);
   *(struct WsLibStruct**)aptr = wsptr;
   frmptr = (struct WsLibFrmStruct*)(aptr + sizeof(struct WsLibStruct*));

   hcnt = 0;
   frmptr->FrameHeader[hcnt++] = WSLIB_BIT_FIN | OpCode;
   if (wsptr->RoleClient)
   {
      WsLib__MaskingKey (frmptr);
      frmptr->FrameHeader[hcnt++] = frmptr->FrameMaskBit | DataCount;
      frmptr->FrameHeader[hcnt++] = frmptr->MaskingKey[0];
      frmptr->FrameHeader[hcnt++] = frmptr->MaskingKey[1];
      frmptr->FrameHeader[hcnt++] = frmptr->MaskingKey[2];
      frmptr->FrameHeader[hcnt++] = frmptr->MaskingKey[3];
      for (cnt = 0; cnt < DataCount; cnt++)
         frmptr->FrameHeader[hcnt+cnt] = DataPtr[cnt] ^
                                 frmptr->MaskingKey[frmptr->MaskCount++&0x3];
   }
   else
   {
      frmptr->FrameHeader[hcnt++] = DataCount;
      for (cnt = 0; cnt < DataCount; cnt++)
         frmptr->FrameHeader[hcnt+cnt] = DataPtr[cnt];
   }

   status = sys$qio (WsLibEfnNoWait, wsptr->OutputChannel,
                     IO$_WRITELBLK | IO$M_READERCHECK,
                     0, WsLib__OutputFreeAst, aptr,
                     frmptr->FrameHeader, hcnt+cnt, 0, 0, 0, 0);
   if (VMSok(status)) wsptr->QueuedOutput++;

   return (status);
}

/*****************************************************************************/
/*
A ping header has been detected.  Return a pong frame.
*/

static void WsLib__Pong (struct WsLibFrmStruct *frmptr)

{
   int  cnt, hcnt, status,
        DataCount;
   char  *aptr,
         *DataPtr;
   struct WsLibStruct  *wsptr;

   /*********/
   /* begin */
   /*********/

   wsptr = frmptr->WsLibMsgPtr->WsLibPtr;

   WATCH_WSLIB (wsptr, FI_LI, "PONG !UL", frmptr->DataCount);

   if (wsptr->WebSocketClosed)
   {
      WsLib__MsgCallback (wsptr, __LINE__, SS$_SHUT, "can't pong; closed");
      return;
   }

   /* retrieve any pinged data (max is 125 bytes) */
   DataPtr = frmptr->DataPtr;
   if ((DataCount = frmptr->DataCount) > 125) DataCount = 125;

   /* allocate a pointer plus a structure (freed by WsLib__OutputFreeAst()) */ 
   aptr = calloc (1, sizeof(struct WsLibStruct*) +
                     sizeof(struct WsLibFrmStruct));
   if (!aptr) WsLibExit (wsptr, FI_LI, vaxc$errno);
   *(struct WsLibStruct**)aptr = wsptr;
   frmptr = (struct WsLibFrmStruct*)(aptr + sizeof(struct WsLibStruct*));

   hcnt = 0;
   frmptr->FrameHeader[hcnt++] = WSLIB_BIT_FIN | WSLIB_OPCODE_PONG;
   if (wsptr->RoleClient)
   {
      WsLib__MaskingKey (frmptr);
      frmptr->FrameHeader[hcnt++] = frmptr->FrameMaskBit | DataCount;
      frmptr->FrameHeader[hcnt++] = frmptr->MaskingKey[0];
      frmptr->FrameHeader[hcnt++] = frmptr->MaskingKey[1];
      frmptr->FrameHeader[hcnt++] = frmptr->MaskingKey[2];
      frmptr->FrameHeader[hcnt++] = frmptr->MaskingKey[3];
      for (cnt = 0; cnt < DataCount; cnt++)
         frmptr->FrameHeader[hcnt+cnt] = DataPtr[cnt] ^
                                 frmptr->MaskingKey[frmptr->MaskCount++&0x3];
   }
   else
   {
      frmptr->FrameHeader[hcnt++] = DataCount;
      for (cnt = 0; cnt < DataCount; cnt++)
         frmptr->FrameHeader[hcnt+cnt] = DataPtr[cnt];
   }

   status = sys$qio (WsLibEfnNoWait, wsptr->OutputChannel,
                     IO$_WRITELBLK | IO$M_READERCHECK,
                     0, WsLib__OutputFreeAst, aptr,
                     frmptr->FrameHeader, hcnt+cnt, 0, 0, 0, 0);
   if (VMSok(status)) wsptr->QueuedOutput++;

   if (VMSnok(status))
   {
      WsLib__MsgCallback (wsptr, __LINE__, status, "pong");
      return;
   }
}

/*****************************************************************************/
/*
Return true if both input and output channel connected.
*/

int WsLibConnected (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   return (wsptr->InputChannel && wsptr->OutputChannel);
}

/*****************************************************************************/
/*
Read data from the WebSocket client using a descriptor to describe the input
buffer.  If supplied the 'ReadDsc' is populated (pointer and length adjusted)
on a successful read.  If 'DataDsc' is NULL then a dynamic buffer with a
maximum size of 65535 will be allocated as per description in WsLibRead().
*/

int WsLibReadDsc
(
struct WsLibStruct *wsptr,
struct dsc$descriptor_s *DataDsc,
struct dsc$descriptor_s *ReadDsc,
void *AstFunction
)
{
   int  status;
   struct dsc$descriptor_s  ScratchDsc;

   /*********/
   /* begin */
   /*********/

   if (!DataDsc)
   {
      DataDsc = &ScratchDsc;
      DataDsc->dsc$b_class = DSC$K_CLASS_S;
      DataDsc->dsc$b_dtype = DSC$K_DTYPE_T;
      DataDsc->dsc$a_pointer = NULL;
      DataDsc->dsc$w_length = 65535;
   }

   if (DataDsc->dsc$b_class != DSC$K_CLASS_S &&
       DataDsc->dsc$b_dtype != DSC$K_DTYPE_T) return (LIB$_INVSTRDES);

   if (ReadDsc->dsc$b_class != DSC$K_CLASS_S &&
       ReadDsc->dsc$b_dtype != DSC$K_DTYPE_T) return (LIB$_INVSTRDES);

   wsptr->ReadDscPtr = ReadDsc;
   status = WsLibRead (wsptr, DataDsc->dsc$a_pointer,
                       DataDsc->dsc$w_length, AstFunction);
   return (status);
}

/*****************************************************************************/
/*
Read a message from WEBSOCKET_INPUT.  A message can comprise of multiple frames
if the message is fragmented.  If an AST function is supplied then the read is
asynchronous, otherwise blocking.  When the read is complete the status can be
returned by WsLibReadStatus(), the read count by WsLibReadCount(), a pointer to
the data buffer using WsLibReadData(), and a pointer to the read string
descriptor with WsLibReadDataDsc().  A data pointer to NULL can be supplied in
which case data size becomes the maximum allowed frame size (if zero there is
no limit) and an appropriately sized buffer is dynamically allocated (and
deallocated) with each frame read.
*/

int WsLibRead
(
struct WsLibStruct *wsptr,
char *DataPtr,
int DataSize,
void *AstFunction
)
{
   struct WsLibMsgStruct  *msgptr;

   /*********/
   /* begin */
   /*********/

   WATCH_WSLIB (wsptr, FI_LI, "READ size:!UL", DataSize);

   if (wsptr->WebSocketClosed)
   {
      WsLib__MsgCallback (wsptr, __LINE__, SS$_SHUT, "can't read; closed");
      wsptr->InputStatus = SS$_SHUT;
      if (AstFunction) ((void(*)())AstFunction)(wsptr);
      return (SS$_SHUT);
   }

   msgptr = calloc (1, sizeof(struct WsLibMsgStruct));
   if (!msgptr) WsLibExit (wsptr, FI_LI, vaxc$errno);
   msgptr->WsLibPtr = wsptr;

   if (DataPtr)
      wsptr->InputDataMax = 0;
   else
   {
      /* the maxiumum sized message allowed to be buffered */
      if (DataSize)
         wsptr->InputDataMax = msgptr->DataMax = DataSize;
      else
         wsptr->InputDataMax = msgptr->DataMax = 4294967295;
   }

   msgptr->DataPtr = DataPtr;
   msgptr->DataSize = DataSize;
   msgptr->AstFunction = AstFunction;
   
   WsLib__ReadFrame (msgptr);

   return (wsptr->InputStatus);
}

/*****************************************************************************/
/*
Read a frame (can be a fragment).
Read six bytes.  See WsLib__ReadHeader1Ast() for breakdown.
*/

static void WsLib__ReadFrame (struct WsLibMsgStruct *msgptr)

{
   struct WsLibFrmStruct  *frmptr;
   struct WsLibStruct *wsptr;

   /*********/
   /* begin */
   /*********/

   wsptr = msgptr->WsLibPtr;

   WATCH_WSLIB (wsptr, FI_LI, "READ frame");

   /* reset the frame structure (only needed on subsequent reads) */
   frmptr = &msgptr->FrameData;
   if (frmptr->IOsb.iosb$w_status)
      memset (frmptr, 0, sizeof(struct WsLibFrmStruct));
   frmptr->WsLibMsgPtr = msgptr;
   wsptr = frmptr->WsLibMsgPtr->WsLibPtr;

   /* if the client interface is in use then it's TCP/IP */
   if (wsptr->SocketChannel)
      /* not really needed on a TCP connection but seems to do no harm! */
      frmptr->IoRead = IO$_READLBLK | IO$M_WRITERCHECK;
   else
      frmptr->IoRead = IO$_READLBLK | IO$M_STREAM | IO$M_WRITERCHECK;

   if (wsptr->RoleClient)
      frmptr->ReadSize = 2;
   else
      frmptr->ReadSize = 6;

   frmptr->IOsb.iosb$w_bcnt = 0;
   frmptr->IOsb.iosb$w_status = SS$_NORMAL;
   wsptr->QueuedInput++;
   WsLib__ReadHeader1Ast (frmptr);
}

/*****************************************************************************/
/*
First two or six bytes of the frame have been read.

      0                   1                   2                   3
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     +-+-+-+-+-------+-+-------------+-------------------------------+
     |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
     |I|S|S|S|  (4)  |A|     (7)     |             (16/63)           |
     |N|V|V|V|       |S|             |   (if payload len==126/127)   |
     | |1|2|3|       |K|             |                               |
     +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
     |     Extended payload length continued, if payload len == 127  |
     + - - - - - - - - - - - - - - - +-------------------------------+
     |                               |Masking-key, if MASK set to 1  |
     +-------------------------------+-------------------------------+
     | Masking-key (continued)       |          Payload Data         |
     +-------------------------------- - - - - - - - - - - - - - - - +
     :                     Payload Data continued ...                :
     + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
     |                     Payload Data continued ...                |
     +---------------------------------------------------------------+

These can be ...

1) the 9+7 header bits, includes payload length if less than 126 bytes.

2) the 9+7 header bits, includes payload length if less than 126 bytes, plus
any 32 bit masking-key.

3) the 9+7 header bits, plus the word of the 16 bit extended payload length,
plus the first two bytes of any masking-key.

4) the 9+7 header bits, plus the first 4 bytes of the 64 bit extended payload
length.

If the header payload length is less than 126 then the header is complete, if
126 then read another 4 bytes to get the  masking-key, if 127 then read another
10 bytes to complete the extended (64 bit) payload length and to get the
masking-key.

Perform header sanity checking.
*/

static void WsLib__ReadHeader1Ast (struct WsLibFrmStruct *frmptr)

{
   int  status;
   struct WsLibStruct  *wsptr;
   struct WsLibMsgStruct  *msgptr;

   /*********/
   /* begin */
   /*********/

   msgptr = frmptr->WsLibMsgPtr;
   wsptr = msgptr->WsLibPtr;

   if (wsptr->QueuedInput) wsptr->QueuedInput--;

   if (wsptr->WatchDogReadSecs)
      wsptr->WatchDogReadTime = CurrentTime + wsptr->WatchDogReadSecs;
   if (wsptr->WatchDogIdleSecs)
      wsptr->WatchDogIdleTime = CurrentTime + wsptr->WatchDogIdleSecs;

   /* loop until the required number of bytes have been read */
   while (VMSok (frmptr->IOsb.iosb$w_status))
   {
      /* if data is not available mailbox $QIO stream reads return less */
      frmptr->FrameCount += frmptr->IOsb.iosb$w_bcnt;
      frmptr->ReadSize -= frmptr->IOsb.iosb$w_bcnt;
      if (!frmptr->ReadSize) break;

      if (msgptr->AstFunction)
      {
         /* a loop with an intermediate AST delivery! */
         status = sys$qio (WsLibEfnNoWait, wsptr->InputChannel, frmptr->IoRead,
                           &frmptr->IOsb, WsLib__ReadHeader1Ast, frmptr,
                           frmptr->FrameHeader+frmptr->FrameCount,
                           frmptr->ReadSize,
                           0, 0, 0, 0);
         if (VMSok(status)) wsptr->QueuedInput++;
         return;
      }

      sys$qiow (WsLibEfnWait, wsptr->InputChannel, frmptr->IoRead,
                &frmptr->IOsb, 0, 0,
                frmptr->FrameHeader+frmptr->FrameCount, frmptr->ReadSize,
                0, 0, 0, 0);
   }

   if (VMSnok (frmptr->IOsb.iosb$w_status))
   {
      wsptr->QueuedInput++;
      WsLib__ReadDataAst (frmptr);
      return;
   }

   /* check if frame data is masked */
   frmptr->FrameMaskBit = frmptr->FrameHeader[1] & 0x80;

   if (wsptr->RoleClient)
   {
      /* if the server is indicating it's masking frames */
      if (frmptr->FrameMaskBit && frmptr->FrameCount == 2)
      {
         /* need four more octets!! */
         frmptr->ReadSize = 4;
         wsptr->QueuedInput++;
         WsLib__ReadHeader1Ast (frmptr);
         return;
      }
   }
   else
   {
      if (!frmptr->FrameMaskBit)
      {
         /* all client->server frames must be masked */
         WATCH_WSLIB (wsptr, FI_LI, "CLIENT frame not masked 0x!2XL!2XL",
                      frmptr->FrameHeader[0], frmptr->FrameHeader[1]);
         WsLib__MsgCallback (wsptr, __LINE__, SS$_PROTOCOL,
                             "client frame not masked 0x!2XL!2XL",
                             frmptr->FrameHeader[0], frmptr->FrameHeader[1]);
         strcpy (msgptr->CloseMsg, "client frame not masked");
         goto ProtocolError;
      }
   }

   /* process rest of frame header */
   frmptr->FrameFinBit = frmptr->FrameHeader[0] & 0x80;
   frmptr->FrameRsv = frmptr->FrameHeader[0] & 0x70;
   frmptr->FrameOpcode = frmptr->FrameHeader[0] & 0x0f;
   frmptr->FramePayload = frmptr->FrameHeader[1] & 0x7f;

   if (frmptr->FrameRsv)
   {
      /* reserve bits set */
      WATCH_WSLIB (wsptr, FI_LI, "RESERVE bit 0x!2XL", frmptr->FrameRsv);
      WsLib__MsgCallback (wsptr, __LINE__, SS$_PROTOCOL,
                          "reserve bit 0x!2XL", frmptr->FrameRsv); 
      sprintf (msgptr->CloseMsg, "reserve bit 0x%02x", frmptr->FrameRsv);
      goto ProtocolError;
   }

   switch (frmptr->FrameOpcode)
   {
      case WSLIB_OPCODE_CONTIN : break;
      case WSLIB_OPCODE_TEXT   : break;
      case WSLIB_OPCODE_BINARY : break;
      case WSLIB_OPCODE_CLOSE  : break;
      case WSLIB_OPCODE_PING   : break;
      case WSLIB_OPCODE_PONG   : break;
      default :
      {
         /* unknown opcode */
         WATCH_WSLIB (wsptr, FI_LI, "OPCODE unknown 0x!2XL",
                      frmptr->FrameOpcode);
         WsLib__MsgCallback (wsptr, __LINE__, SS$_PROTOCOL,
                             "unknown opcode 0x!2XL", frmptr->FrameOpcode);
         sprintf (msgptr->CloseMsg, "unknown opcode 0x%02x",
                  frmptr->FrameOpcode);
         goto ProtocolError;
      }
   }

   if (frmptr->FrameOpcode & 0x8)
   {
      /* control opcode */
      if (!frmptr->FrameFinBit)
      {
         WATCH_WSLIB (wsptr, FI_LI, "CONTROL frame fragmented");
         WsLib__MsgCallback (wsptr, __LINE__, SS$_PROTOCOL,
                             "control frame fragmented");
         strcpy (msgptr->CloseMsg, "control frame fragmented");
         goto ProtocolError;
      }
      if (frmptr->FramePayload > 125)
      {
         WATCH_WSLIB (wsptr, FI_LI, "CONTROL payload > 125 bytes");
         WsLib__MsgCallback (wsptr, __LINE__, SS$_PROTOCOL,
                             "control payload > 125 bytes");
         strcpy (msgptr->CloseMsg, "control payload > 125 bytes");
         goto ProtocolError;
      }
   }
   else
   if (frmptr->FrameFinBit)
   {
      /* FIN bit set */
      if (frmptr->FrameOpcode)
      {
         if (msgptr->MsgOpcode)
         {
            /* must not have an opcode */
            WATCH_WSLIB (wsptr, FI_LI, "FRAGMENT with opcode");
            WsLib__MsgCallback (wsptr, __LINE__, SS$_PROTOCOL,
                                "fragment with opcode");
            strcpy (msgptr->CloseMsg, "fragment with opcode");
            goto ProtocolError;
         }
         msgptr->MsgOpcode = frmptr->FrameOpcode;
      }
      else
      {
         if (!msgptr->MsgOpcode)
         {
            /* must have an opcode */
            WATCH_WSLIB (wsptr, FI_LI, "FRAME without opcode");
            WsLib__MsgCallback (wsptr, __LINE__, SS$_PROTOCOL,
                                "frame without opcode");
            strcpy (msgptr->CloseMsg, "frame without opcode");
            goto ProtocolError;
         }
      }
   }
   else
   {
      /* FIN bit reset */
      if (msgptr->MsgOpcode)
      {
         if (frmptr->FrameOpcode)
         {
            /* subsequent fragments must not have an opcode */
            WATCH_WSLIB (wsptr, FI_LI, "FRAGMENT with opcode");
            WsLib__MsgCallback (wsptr, __LINE__, SS$_PROTOCOL,
                                "fragment with opcode");
            strcpy (msgptr->CloseMsg, "fragment with opcode");
            goto ProtocolError;
         }
      }
      else
      {
         if (!frmptr->FrameOpcode)
         {
            /* fragments must have an initial opcode */
            WATCH_WSLIB (wsptr, FI_LI, "FRAGMENT without opcode");
            WsLib__MsgCallback (wsptr, __LINE__, SS$_PROTOCOL,
                                "fragment without opcode");
            strcpy (msgptr->CloseMsg, "fragment without opcode");
            goto ProtocolError;
         }
         msgptr->MsgOpcode = frmptr->FrameOpcode;
      }
   }

   if (frmptr->FrameCount == 6)
   {
      /* essentially a longword integer in network byte order */
      frmptr->MaskingKey[0] = frmptr->FrameHeader[2];
      frmptr->MaskingKey[1] = frmptr->FrameHeader[3];
      frmptr->MaskingKey[2] = frmptr->FrameHeader[4];
      frmptr->MaskingKey[3] = frmptr->FrameHeader[5];
   }

   /* prepare to read more */
   frmptr->IOsb.iosb$w_bcnt = 0;
   frmptr->IOsb.iosb$w_status = SS$_NORMAL;
   wsptr->QueuedInput++;

   if (frmptr->FramePayload == 126)
   {
      /* read more of the header  */
      frmptr->ReadSize = 2;
      WsLib__ReadHeader2Ast (frmptr);
      return;
   }

   if (frmptr->FramePayload == 127)
   {
      /* read more of the header  */
      frmptr->ReadSize = 8;
      WsLib__ReadHeader2Ast (frmptr);
      return;
   }

   /* frame length is 125 bytes or less, begin reading data */
   WsLib__ReadDataAst (frmptr);

   return;

ProtocolError:

   frmptr->IOsb.iosb$w_bcnt = 0;
   frmptr->IOsb.iosb$w_status = SS$_PROTOCOL;
   wsptr->QueuedInput++;
   WsLib__ReadDataAst (frmptr);

   return;
}

/*****************************************************************************/
/*
The data $QIOed by WsLib__ReadHeader1Ast() has been read.
For frames less than 126 bytes this function will not be called.
*/

static void WsLib__ReadHeader2Ast (struct WsLibFrmStruct *frmptr)

{
   int  cnt, status;
   struct WsLibStruct  *wsptr;

   /*********/
   /* begin */
   /*********/

   wsptr = frmptr->WsLibMsgPtr->WsLibPtr;

   if (wsptr->QueuedInput) wsptr->QueuedInput--;

   if (wsptr->WatchDogReadSecs)
      wsptr->WatchDogReadTime = CurrentTime + wsptr->WatchDogReadSecs;
   if (wsptr->WatchDogIdleSecs)
      wsptr->WatchDogIdleTime = CurrentTime + wsptr->WatchDogIdleSecs;

   /* loop until the required number of bytes have been read */
   while (VMSok (frmptr->IOsb.iosb$w_status))
   {
      /* if data is not available mailbox $QIO stream reads return less */
      frmptr->FrameCount += frmptr->IOsb.iosb$w_bcnt;
      frmptr->ReadSize -= frmptr->IOsb.iosb$w_bcnt;
      if (!frmptr->ReadSize) break;

      if (frmptr->WsLibMsgPtr->AstFunction)
      {
         /* a loop with an intermediate AST delivery! */
         status = sys$qio (WsLibEfnNoWait, wsptr->InputChannel, frmptr->IoRead,
                           &frmptr->IOsb, WsLib__ReadHeader2Ast, frmptr,
                           frmptr->FrameHeader+frmptr->FrameCount,
                           frmptr->ReadSize,
                           0, 0, 0, 0);
         if (VMSok(status)) wsptr->QueuedInput++;
         return;
      }

      sys$qiow (WsLibEfnWait, wsptr->InputChannel, frmptr->IoRead,
                &frmptr->IOsb, 0, 0,
                frmptr->FrameHeader+frmptr->FrameCount, frmptr->ReadSize,
                0, 0, 0, 0);
   }

   if (VMSnok (frmptr->IOsb.iosb$w_status))
   {
      wsptr->QueuedInput++;
      WsLib__ReadDataAst (frmptr);
      return;
   }

   if (frmptr->FramePayload == 126)
   {
      /* word integer in network byte order */
      frmptr->FramePayload = (frmptr->FrameHeader[2] << 8) +
                             frmptr->FrameHeader[3];
      if (frmptr->FrameCount == 8)
      {
         /* essentially a longword integer in network byte order */
         frmptr->MaskingKey[0] = frmptr->FrameHeader[4];
         frmptr->MaskingKey[1] = frmptr->FrameHeader[5];
         frmptr->MaskingKey[2] = frmptr->FrameHeader[6];
         frmptr->MaskingKey[3] = frmptr->FrameHeader[7];
      }
   }
   else
   if (frmptr->FramePayload == 127)
   {
      /* protocol octaword integer in network byte order */
      if (frmptr->FrameHeader[2] || frmptr->FrameHeader[3] ||
          frmptr->FrameHeader[4] || frmptr->FrameHeader[5])
      {
         /* if >2^32 then something's probably wrong */
         WsLib__MsgCallback (wsptr, __LINE__, SS$_BUGCHECK,
                             "frame length sanity check");
         frmptr->IOsb.iosb$w_bcnt = 0;
         frmptr->IOsb.iosb$w_status = SS$_BUGCHECK;
         wsptr->QueuedInput++;
         WsLib__ReadDataAst (frmptr);
         return;
      }
      /* quadword integer in network byte order (lowest 32 bits anyway) */
      frmptr->FramePayload = (frmptr->FrameHeader[6] << 24) +
                             (frmptr->FrameHeader[7] << 16) +
                             (frmptr->FrameHeader[8] << 8) +
                              frmptr->FrameHeader[9];
      if (frmptr->FrameCount == 14)
      {
         /* essentially a longword integer in network byte order */
         frmptr->MaskingKey[0] = frmptr->FrameHeader[10];
         frmptr->MaskingKey[1] = frmptr->FrameHeader[11];
         frmptr->MaskingKey[2] = frmptr->FrameHeader[12];
         frmptr->MaskingKey[3] = frmptr->FrameHeader[13];
      }
   }
   else
     WsLibExit (wsptr, FI_LI, SS$_BUGCHECK);

   /* the frame length is now known, begin reading data */
   frmptr->IOsb.iosb$w_bcnt = 0;
   frmptr->IOsb.iosb$w_status = SS$_NORMAL;
   wsptr->QueuedInput++;
   WsLib__ReadDataAst (frmptr);
}

/*****************************************************************************/
/*
Called when the length of the frame has been determined.  This repeatedly reads
mailbox-sized chunks of the frame (if necessary), asynchronously or
synchronously as required, until the frame is completely read (or an error
occurs).  The '->AstFunction' or blocking caller should drive the client-end
business logic. 
*/

static void WsLib__ReadDataAst (struct WsLibFrmStruct *frmptr)

{
   int  cnt, status,
        DataCount,
        DataSize;
   char  *DataPtr;
   struct WsLibStruct  *wsptr;
   struct WsLibMsgStruct  *msgptr;

   /*********/
   /* begin */
   /*********/

   msgptr = frmptr->WsLibMsgPtr;
   wsptr = frmptr->WsLibMsgPtr->WsLibPtr;

   if (wsptr->QueuedInput) wsptr->QueuedInput--;

   if (wsptr->WatchDogReadSecs)
      wsptr->WatchDogReadTime = CurrentTime + wsptr->WatchDogReadSecs;
   if (wsptr->WatchDogIdleSecs)
      wsptr->WatchDogIdleTime = CurrentTime + wsptr->WatchDogIdleSecs;

   while (VMSok (frmptr->IOsb.iosb$w_status))
   {
      if (!frmptr->DataPtr)
      {
         /* first call */
         WATCH_WSLIB (wsptr, FI_LI,
"READ header:!UL opcode:!2XL(!AZ) payload:!UL fin:!UL mask:!UL",
                      frmptr->FrameCount,
                      frmptr->FrameOpcode,
                      WsLib__OpCodeName(frmptr->FrameOpcode),
                      frmptr->FramePayload,
                      frmptr->FrameFinBit ? 1 : 0,
                      frmptr->FrameMaskBit ? 1 : 0);

         /* establish buffer */
         if (frmptr->FramePayload <= 125)
         {
            /* use internal frame buffer */
            frmptr->DataPtr = (char*)frmptr->FrameHeader + frmptr->FrameCount;
            frmptr->DataSize = 125;
         }
         else
         {
            /* allocated frame data buffer */
            frmptr->DataSize = frmptr->FramePayload;
            /* ensure that even for zero payload some memory is allocated */
            frmptr->DataPtr = calloc (1, frmptr->DataSize+16);
            if (!frmptr->DataPtr) WsLibExit (wsptr, FI_LI, vaxc$errno);
         }
      }

      if (frmptr->IOsb.iosb$w_bcnt)
      {
         DataPtr = frmptr->DataPtr + frmptr->DataCount;
         if (msgptr->MsgOpcode == WSLIB_OPCODE_TEXT)
         {
            /* will also apply masking key if required */
            if (!WsLib__Utf8Legal (frmptr))
            {
               WATCH_WSLIB (wsptr, FI_LI, "UTF-8 illegal (fast fail)");
               frmptr->IOsb.iosb$w_status = SS$_BADESCAPE;
               strcpy (msgptr->CloseMsg, "UTF-8 illegal");
               /* deliver the status to the application */
               break;
            }
         }
         else
         if (frmptr->FrameMaskBit)
         {
            /* apply masking key */
            for (cnt = 0; cnt < frmptr->IOsb.iosb$w_bcnt; cnt++)
               DataPtr[cnt] ^= frmptr->MaskingKey[frmptr->MaskCount++&0x3];
         }
         frmptr->DataCount += frmptr->IOsb.iosb$w_bcnt;
      }

      WATCH_WSLIB (wsptr, FI_LI, "READ inque:!UL payload:!UL/!UL !AZ",
                   wsptr->QueuedInput,
                   frmptr->DataCount, frmptr->FramePayload,
                   frmptr->DataCount >= frmptr->FramePayload ?
                      "COMPLETE" : "in-progress");

      if (frmptr->DataCount >= frmptr->FramePayload) break;

      DataPtr = frmptr->DataPtr + frmptr->DataCount;
      if (frmptr->FramePayload - frmptr->DataCount <= wsptr->InputMrs)
         DataCount = frmptr->FramePayload - frmptr->DataCount;
      else
         DataCount = wsptr->InputMrs;
      if (frmptr->DataCount + DataCount > frmptr->DataSize)
         DataCount = frmptr->DataSize - frmptr->DataCount;

      if (msgptr->AstFunction)
      {
         /* asynchronous read */
         status = sys$qio (WsLibEfnNoWait, wsptr->InputChannel, frmptr->IoRead,
                           &frmptr->IOsb, WsLib__ReadDataAst, frmptr,
                           DataPtr, DataCount, 0, 0, 0, 0);
         if (VMSok(status)) wsptr->QueuedInput++;
         return;
      }

      /* synchronous read */
      sys$qiow (WsLibEfnWait, wsptr->InputChannel, frmptr->IoRead,
                &frmptr->IOsb, 0, 0,
                DataPtr, DataCount, 0, 0, 0, 0);
   }

   WATCH_WSLIB (wsptr, FI_LI, "READ %X!8XL", frmptr->IOsb.iosb$w_status);

   if (VMSnok (frmptr->IOsb.iosb$w_status) &&
       frmptr->IOsb.iosb$w_status != SS$_LINKDISCON &&
       !wsptr->MsgStringLength)
      WsLib__MsgCallback (wsptr, __LINE__, frmptr->IOsb.iosb$w_status,
                          "frame read");

   /******************/
   /* frame complete */
   /******************/

   if (VMSok (frmptr->IOsb.iosb$w_status))
   {
      if (frmptr->FrameOpcode == WSLIB_OPCODE_PING)
      {
         WsLib__Pong (frmptr);
         /* restart the read of the data */
         WsLib__ReadFrame (msgptr);
         return;
      }

      if (frmptr->FrameOpcode == WSLIB_OPCODE_PONG)
      {
         /* process any (solicited or unsolicited) pong callback */
         if (wsptr->PongCallbackFunction)
            (*wsptr->PongCallbackFunction)(wsptr);
         else
         if (PongCallbackFunction)
            (*PongCallbackFunction)(wsptr);
         else
            WsLib__MsgCallback (wsptr, __LINE__, SS$_NOTMODIFIED,
                                "no pong callback");
         /* restart the read of the data */
         WsLib__ReadFrame (msgptr);
         return;
      }

      if (frmptr->FrameOpcode == WSLIB_OPCODE_CLOSE)
      {
         WsLib__Close (frmptr);
         frmptr->IOsb.iosb$w_status = SS$_SHUT;
         /* deliver closed status to the application */
      }
   }

   /*****************/
   /* build message */
   /*****************/

   msgptr->MsgStatus = frmptr->IOsb.iosb$w_status;

   if (VMSok (msgptr->MsgStatus))
   {
      /* accumulate total message data transfered */
#ifdef __VAX
      {
         unsigned long q[2] = { frmptr->DataCount, 0 };
         lib$addx(&q,&wsptr->InputCount,&wsptr->InputCount,0);
      }
#else
      *(__int64*)wsptr->InputCount = *(__int64*)wsptr->InputCount +
                                      (__int32)frmptr->DataCount;
#endif

      if (!(DataSize = msgptr->DataMax)) DataSize = msgptr->DataSize;
      if (msgptr->DataCount + frmptr->DataCount > (unsigned)DataSize)
      {
         WsLib__MsgCallback (wsptr, __LINE__, SS$_RESULTOVF,
                             "message !UL bytes > buffer !UL bytes",
                             msgptr->DataCount + frmptr->DataCount, DataSize);
         msgptr->MsgStatus = SS$_RESULTOVF;
         msgptr->DataCount = 0;
      }
      else
      if (msgptr->DataMax)
      {
         /* dynamic buffer */
         if (msgptr->DataPtr)
         {
            if (frmptr->DataCount)
            {
               /* append this fragment to previously buffered data */
               msgptr->DataPtr = realloc (msgptr->DataPtr,
                                          msgptr->DataCount +
                                             frmptr->DataCount);
               if (!msgptr->DataPtr) WsLibExit (wsptr, FI_LI, vaxc$errno);
               memcpy (msgptr->DataPtr + msgptr->DataCount,
                       frmptr->DataPtr,
                       frmptr->DataCount);
               msgptr->DataCount += frmptr->DataCount;
            }
         }
         else
         {
            /* ensure that even for zero payload some memory is allocated */
            msgptr->DataPtr = calloc (1, frmptr->DataCount+16);
            if (!msgptr->DataPtr) WsLibExit (wsptr, FI_LI, vaxc$errno);
            memcpy (msgptr->DataPtr, frmptr->DataPtr, frmptr->DataCount);
            msgptr->DataCount = frmptr->DataCount;
         }
      }
      else
      {
         /* supplied buffer */
         memcpy (msgptr->DataPtr + msgptr->DataCount,
                 frmptr->DataPtr,
                 frmptr->DataCount);
         msgptr->DataCount += frmptr->DataCount;
      }

      /* dispose any allocated frame data buffer */
      if (frmptr->FramePayload > 125) free (frmptr->DataPtr);

      if (VMSok (msgptr->MsgStatus))
      {
         /* if not the final fragment */
         if (!frmptr->FrameFinBit)
         {
            /* read the next fragment frame */
            WsLib__ReadFrame (msgptr);
            return;
         }
      }
   }

   if (VMSok (msgptr->MsgStatus))
   {
      if (msgptr->MsgOpcode == WSLIB_OPCODE_TEXT)
      {
         /* ensure any code-point is complete */
         frmptr->IOsb.iosb$w_bcnt = 0;
         if (!WsLib__Utf8Legal (frmptr))
         {
            WATCH_WSLIB (wsptr, FI_LI, "UTF-8 illegal (fast fail)");
            msgptr->MsgStatus = SS$_BADESCAPE;
            strcpy (msgptr->CloseMsg, "UTF-8 illegal");
            /* deliver the status to the application */
         }
      }
   }

   /****************************/
   /* deliver to read function */
   /****************************/

   if (VMSok (msgptr->MsgStatus))
   {
#ifdef __VAX
      /* close enough! */
      wsptr->InputMsgCount[0]++;
#else
      *(__int64*)wsptr->InputMsgCount = *(__int64*)wsptr->InputMsgCount + 1;
#endif

      if (msgptr->MsgOpcode == WSLIB_OPCODE_TEXT)
      {
         /* for text always better if it's null-terminated */
         if (msgptr->DataMax ||
             msgptr->DataCount < msgptr->DataSize)
            msgptr->DataPtr[msgptr->DataCount] = '\0';
         else
            WsLib__MsgCallback (wsptr, __LINE__, SS$_BUFFEROVF,
                                "no space for \\0");

         if (wsptr->SetAscii)
         {
            /* convert from UTF-8 to 8 bit "ASCII" */
            WATCH_WSLIB (wsptr, FI_LI, "UTF-8 decode");
            cnt = WsLibFromUtf8 (msgptr->DataPtr, msgptr->DataCount, 0);
            if (cnt >= 0)
               msgptr->DataCount = cnt;
            else
            {
               /* error in UTF-8 to ASCII conversion */
               WATCH_WSLIB (wsptr, FI_LI, "UTF-8 decode ERROR");
               WsLib__MsgCallback (wsptr, __LINE__, SS$_DATALOST,
                                   "UTF-8 decode error");
               msgptr->MsgStatus = SS$_DATALOST;
               msgptr->DataCount = 0;
            }
         }
      }
   }

   if (!(DataPtr = wsptr->InputDataPtr) && msgptr->DataMax)
   {
      /* the ->MsgDataPtr is only used by WsLibReadGrab() for sanity check */
      wsptr->InputDataPtr = wsptr->MsgDataPtr = msgptr->DataPtr;
   }

   wsptr->InputStatus = msgptr->MsgStatus;
   wsptr->InputOpcode = msgptr->MsgOpcode;
   wsptr->InputDataCount = msgptr->DataCount;
   wsptr->InputDataPtr = wsptr->InputDataDsc.dsc$a_pointer = msgptr->DataPtr;
   if (msgptr->DataCount > 65535)
      wsptr->InputDataDsc.dsc$w_length = 65535;
   else
      wsptr->InputDataDsc.dsc$w_length = msgptr->DataCount;

   if (wsptr->ReadDscPtr)
   {
      wsptr->ReadDscPtr->dsc$a_pointer = msgptr->DataPtr;
      if (msgptr->DataCount > 65535)
         wsptr->ReadDscPtr->dsc$w_length = 65535;
      else
         wsptr->ReadDscPtr->dsc$w_length = msgptr->DataCount;
   }

   if (wsptr->InputStatus == SS$_PROTOCOL)
      WsLibClose (wsptr, WSLIB_CLOSE_PROTOCOL, msgptr->CloseMsg);
   else
   if (wsptr->InputStatus == SS$_BADESCAPE)
      WsLibClose (wsptr, WSLIB_CLOSE_DATA, msgptr->CloseMsg);
   else
   if (VMSnok (wsptr->InputStatus))
      WsLibClose (wsptr, WSLIB_CLOSE_BANG, NULL);

   /* do not molest the buffer pointers if synchronous I/O */
   if (msgptr->AstFunction)
   {
      msgptr->AstFunction (wsptr);
      if (!DataPtr && msgptr->DataMax)
      {
         /* if the dynamic message buffer has not been grabbed then free it */
         if (wsptr->InputDataPtr) free (wsptr->InputDataPtr);
         /* reset the rest */
         wsptr->InputDataPtr = wsptr->MsgDataPtr = NULL;
         wsptr->InputDataCount = wsptr->InputDataMax = 0;
         wsptr->InputDataDsc.dsc$a_pointer = NULL;
         wsptr->InputDataDsc.dsc$w_length = 0;
         wsptr->ReadDscPtr = NULL;
      }
   }

   free (msgptr);

   wsptr->WatchDogReadTime = 0;
   if (wsptr->WatchDogIdleSecs)
      wsptr->WatchDogIdleTime = CurrentTime + wsptr->WatchDogIdleSecs;
   if (wsptr->WatchDogWakeSecs)
      wsptr->WatchDogWakeTime = CurrentTime + wsptr->WatchDogWakeSecs;

   if (wsptr->WebSocketShut) WsLib__Shut (wsptr); 
}

/*****************************************************************************/
/*
When using dynamic message data buffer grab the allocated memory returning a
pointer or NULL.  The data count and other detail can be obtained using
WsLibReadCount(), WsLibReadIsText(), etc.  This should subsequently be freed
using WsLibFree() when no longer required.
*/

char* WsLibReadGrab (struct WsLibStruct *wsptr)

{
   char  *cptr;

   /*********/
   /* begin */
   /*********/

   /* sanity check the call */
   if (wsptr->InputDataPtr &&
       wsptr->InputDataPtr == wsptr->MsgDataPtr)
   {
      cptr = wsptr->InputDataPtr;
      wsptr->InputDataPtr = wsptr->MsgDataPtr = NULL;
      wsptr->InputDataCount = wsptr->InputDataMax = 0;
      wsptr->InputDataDsc.dsc$a_pointer = NULL;
      wsptr->InputDataDsc.dsc$w_length = 0;
      return (cptr);
   }

   WATCH_WSLIB (wsptr, FI_LI, "GRAB sanity check");
   WsLib__MsgCallback (wsptr, __LINE__, SS$_BUGCHECK, "GRAB sanity check (!AZ)",
                       wsptr->InputDataPtr ? "null" : "data");
   return (NULL);
}

/*****************************************************************************/
/*
Return true if the most recent read was BINARY opcode.
*/

int WsLibReadIsBinary (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   return (wsptr->InputOpcode == WSLIB_OPCODE_BINARY);
}

/*****************************************************************************/
/*
Return true if the most recent read was TEXT opcode.
*/

int WsLibReadIsText (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   return (wsptr->InputOpcode == WSLIB_OPCODE_TEXT);
}

/*****************************************************************************/
/*
Return the read status value.
*/

int WsLibReadStatus (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   return (wsptr->InputStatus);
}

/*****************************************************************************/
/*
Return the (most recent) read count value (longword).
*/

int WsLibReadCount (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   return (wsptr->InputDataCount);
}

/*****************************************************************************/
/*
Return a pointer to the (most recent) read buffer.
*/

char* WsLibReadData (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   return (wsptr->InputDataPtr);
}

/*****************************************************************************/
/*
Return a pointer to the (most recent) read data descriptor.
*/

struct dsc$descriptor_s* WsLibReadDataDsc (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   return (&wsptr->InputDataDsc);
}

/*****************************************************************************/
/*
Return a pointer to the total read count value (quadword).
*/

unsigned long* WsLibReadTotal (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   return ((unsigned long*)&wsptr->InputCount);
}

/*****************************************************************************/
/*
Return a pointer to the total messages read value (quadword).
*/

unsigned long* WsLibReadMsgTotal (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   return ((unsigned long*)&wsptr->InputMsgCount);
}

/*****************************************************************************/
/*
Generate a masking key for the supplied IO structure.
*/

static void WsLib__MaskingKey (struct WsLibFrmStruct *frmptr)

{
   static unsigned long  RandomNumber,
                         RandomFiller;

   /*********/
   /* begin */
   /*********/

   /* pseudo-random 32 bytes */
   if (!(RandomNumber & 0xff)) sys$gettim (&RandomNumber);
   RandomNumber = RandomNumber * 69069 + 1;

   frmptr->MaskCount = 0;
   frmptr->FrameMaskBit = 0x80;
   /* network byte order */
   frmptr->MaskingKey[0] = (RandomNumber & 0xff000000) >> 24;
   frmptr->MaskingKey[1] = (RandomNumber & 0x00ff0000) >> 16;
   frmptr->MaskingKey[2] = (RandomNumber & 0x0000ff00) >> 8;
   frmptr->MaskingKey[3] = RandomNumber & 0x000000ff;
}

/*****************************************************************************/
/*
Write the data pointed to by the supplied string descriptor.
*/

int WsLibWriteDsc
(
struct WsLibStruct *wsptr,
struct dsc$descriptor_s *DataDsc,
void *AstFunction
)
{
   int  status;

   /*********/
   /* begin */
   /*********/

   if (DataDsc == NULL)
      status = WsLibWrite (wsptr, NULL, 0, AstFunction);
   else
   if (DataDsc->dsc$b_class != DSC$K_CLASS_S &&
       DataDsc->dsc$b_dtype != DSC$K_DTYPE_T)
      status = LIB$_INVSTRDES;
   else
   status = WsLibWrite (wsptr, DataDsc->dsc$a_pointer,
                        DataDsc->dsc$w_length, AstFunction);
   return (status);
}

/*****************************************************************************/
/*
Queue a write to the client WEBSOCKET_OUTPUT mailbox.  If an AST function is
supplied then the write is asynchronous, otherwise blocking.  If the data
pointer is NULL then send a close-connection frame.  If the AST function is
supplied as (void*)-1 a non-blocking I/O is generated that does not require a
target AST function.
*/

int WsLibWrite
(
struct WsLibStruct *wsptr,
char *DataPtr,
int DataCount,
void *AstFunction
)
{
   int  cnt, hcnt, status,
        Utf8Count;
   unsigned char  *cptr, *czptr, *sptr;
   struct WsLibFrmStruct  *frmptr;
   struct WsLibMsgStruct  *msgptr;

   /*********/
   /* begin */
   /*********/

   WATCH_WSLIB (wsptr, FI_LI, "WRITE count:!UL", DataCount);

   if (wsptr->WebSocketClosed)
   {
      WsLib__MsgCallback (wsptr, __LINE__, SS$_SHUT, "can't write; closed");
      wsptr->OutputStatus = SS$_SHUT;
      if (AstFunction && AstFunction != WSLIB_ASYNCH)
         ((void(*)())AstFunction)(wsptr);
      return (SS$_SHUT);
   }

   msgptr = calloc (1, sizeof(struct WsLibMsgStruct));
   if (!msgptr) WsLibExit (wsptr, FI_LI, vaxc$errno);
   msgptr->WsLibPtr = wsptr;

   /* null or empty writes send an empty message */
   if (!DataPtr)
   {
      DataPtr = "";
      DataCount = 0;
   }

   msgptr->DataPtr = DataPtr;
   msgptr->DataCount = DataCount;
   msgptr->AstFunction = AstFunction;

   if (wsptr->SetAscii)
   {
      /* test if any UTF-8 encoding required */
      Utf8Count = 0;
      czptr = (cptr = (unsigned char*)DataPtr) + DataCount;
      while (cptr < czptr) if (*cptr++ & 0x80) Utf8Count++;

      if (Utf8Count)
      {
         /********************/
         /* convert to UTF-8 */
         /********************/

         WATCH_WSLIB (wsptr, FI_LI, "UTF-8 encode");
         msgptr->Utf8Ptr = calloc (1, DataCount+Utf8Count); 
         if (!msgptr->Utf8Ptr) WsLibExit (wsptr, FI_LI, vaxc$errno);
         czptr = (cptr = (unsigned char*)DataPtr) + DataCount;
         sptr = (unsigned char*)msgptr->Utf8Ptr;
         while (cptr < czptr)
         {
            if (*cptr & 0x80)
            {
               *sptr++ = ((*cptr & 0xc0) >> 6) | 0xc0;
               *sptr++ = (*cptr++ & 0x3f) | 0x80;
            }
            else
               *sptr++ = *cptr++;
         }
         msgptr->DataPtr = msgptr->Utf8Ptr;
         msgptr->DataCount = (char*)sptr - msgptr->Utf8Ptr;
      }
   }

   frmptr = &msgptr->FrameData;
   frmptr->WsLibMsgPtr = msgptr;
   frmptr->IOsb.iosb$w_status = SS$_NORMAL;

   /* either kick-off asynchronous I/O or return after blocking I/O */
   wsptr->QueuedOutput++;
   WsLib__WriteAst (frmptr);

   return (frmptr->IOsb.iosb$w_status);
}

/*****************************************************************************/
/*
Write the message to the WebSocket.  Message may be automatically fragmented.
Can AST back to this function one or more times with asycnchronous I/O.
*/

static void WsLib__WriteAst (struct WsLibFrmStruct *frmptr)

{
   int  cnt, count, hcnt, length, status,
        DataCount;
   char  *pointer,
         *DataPtr;
   struct WsLibStruct  *wsptr;
   struct WsLibMsgStruct  *msgptr;

   /*********/
   /* begin */
   /*********/

   msgptr = frmptr->WsLibMsgPtr;
   wsptr = msgptr->WsLibPtr;

   if (wsptr->QueuedOutput) wsptr->QueuedOutput--;

   if (wsptr->WatchDogIdleSecs)
      wsptr->WatchDogIdleTime = CurrentTime + wsptr->WatchDogIdleSecs;

   while (VMSok (frmptr->IOsb.iosb$w_status))
   {
      if (frmptr->MrsWriteCount)
      {
         /* accumulate total message data transfered */
#ifdef __VAX
         {
            unsigned long q[2] = { frmptr->MrsWriteCount, 0 };
            lib$addx(&q,&wsptr->OutputCount,&wsptr->OutputCount,0);
         }
#else
         *(__int64*)wsptr->OutputCount = *(__int64*)wsptr->OutputCount +
                                          (__int32)frmptr->MrsWriteCount;
#endif
         msgptr->WriteCount += frmptr->MrsWriteCount;
         frmptr->MrsWriteCount = 0;
      }

      WATCH_WSLIB (wsptr, FI_LI, "WRITE outque:!UL payload:!UL/!UL !AZ",
                   wsptr->QueuedOutput,
                   msgptr->WriteCount, msgptr->DataCount,
                   (frmptr->IOsb.iosb$w_bcnt &&
                    msgptr->WriteCount == msgptr->DataCount) ? "COMPLETE" :
                                                             "in-progress");

      /* if the header has been written and the required quantity of data */
      if (frmptr->IOsb.iosb$w_bcnt &&
          msgptr->WriteCount == msgptr->DataCount) break;

      if (wsptr->WebSocketClosed)
      {
         WsLib__MsgCallback (wsptr, __LINE__, SS$_SHUT, "can't write; closed");
         frmptr->IOsb.iosb$w_status = SS$_SHUT;
         break;
      }

      /*********/
      /* frame */
      /*********/

      /* reset the frame structure (only needed on subsequent writes) */
      frmptr = &msgptr->FrameData;
      if (frmptr->IOsb.iosb$w_status)
         memset (frmptr, 0, sizeof(struct WsLibFrmStruct));
      frmptr->WsLibMsgPtr = msgptr;

      /* if being used as a client then mask the data */
      if (wsptr->RoleClient) WsLib__MaskingKey (frmptr);

      DataPtr = msgptr->DataPtr + msgptr->WriteCount;
      if (msgptr->DataCount - msgptr->WriteCount > wsptr->FrameMaxSize)
         DataCount = wsptr->FrameMaxSize;
      else
         DataCount = msgptr->DataCount - msgptr->WriteCount;

      /* only indicate opcode on first write (in case of fragmentation) */
      if (frmptr->IOsb.iosb$w_bcnt)
         frmptr->FrameOpcode = 0;
      else
      if (wsptr->SetAscii || wsptr->SetUtf8)
         frmptr->FrameOpcode = WSLIB_OPCODE_TEXT;
      else
         frmptr->FrameOpcode = WSLIB_OPCODE_BINARY;

      /* if not the last fragment */
      if (msgptr->WriteCount + DataCount < msgptr->DataCount)
         frmptr->FrameFinBit = 0;
      else
         frmptr->FrameFinBit = WSLIB_BIT_FIN;

      WATCH_WSLIB (wsptr, FI_LI,
"WRITE opcode:!2XL(!AZ) fin:!UL mask:!UL data:!UL",
                   frmptr->FrameOpcode,
                   WsLib__OpCodeName(frmptr->FrameOpcode),
                   frmptr->FrameFinBit ? 1 : 0,
                   frmptr->FrameMaskBit ? 1 : 0,
                   DataCount);

      hcnt = 0;
      frmptr->FrameHeader[hcnt++] = frmptr->FrameFinBit | frmptr->FrameOpcode;

      if (DataCount <= 125)
         frmptr->FrameHeader[hcnt++] = frmptr->FrameMaskBit + DataCount;
      else
      if (DataCount <= 65535)
      {
         frmptr->FrameHeader[hcnt++] = frmptr->FrameMaskBit + 126;
         /* network byte order */
         frmptr->FrameHeader[hcnt++] = (DataCount & 0xff00) >> 8;
         frmptr->FrameHeader[hcnt++] = DataCount & 0xff;
      }
      else
      {
         frmptr->FrameHeader[hcnt++] = frmptr->FrameMaskBit + 127;
         /* network byte order */
         frmptr->FrameHeader[hcnt++] = 0;
         frmptr->FrameHeader[hcnt++] = 0;
         frmptr->FrameHeader[hcnt++] = 0;
         frmptr->FrameHeader[hcnt++] = 0;
         frmptr->FrameHeader[hcnt++] = (DataCount & 0xff000000) >> 24;
         frmptr->FrameHeader[hcnt++] = (DataCount & 0xff0000) >> 16;
         frmptr->FrameHeader[hcnt++] = (DataCount & 0xff00) >> 8;
         frmptr->FrameHeader[hcnt++] = DataCount & 0xff;
      }

      if (frmptr->FrameMaskBit)
      {
         frmptr->FrameHeader[hcnt++] = frmptr->MaskingKey[0];
         frmptr->FrameHeader[hcnt++] = frmptr->MaskingKey[1];
         frmptr->FrameHeader[hcnt++] = frmptr->MaskingKey[2];
         frmptr->FrameHeader[hcnt++] = frmptr->MaskingKey[3];

         /* never apply apply the masking key to original data */
         if (!(frmptr->MaskedPtr = msgptr->Utf8Ptr))
         {
            /* allocate a buffer for masked data */
            frmptr->MaskedPtr = calloc (1, DataCount); 
            if (!frmptr->MaskedPtr) WsLibExit (wsptr, FI_LI, vaxc$errno);
         }
         for (cnt = 0; cnt < DataCount; cnt++)
            frmptr->MaskedPtr[cnt] = DataPtr[cnt] ^
                                     frmptr->MaskingKey[cnt&0x03];
         DataPtr = frmptr->MaskedPtr;
      }

      /*********/
      /* write */
      /*********/

      if (DataCount && DataCount <= 125)
      {
         /* for efficiency append the data to the header */
         memcpy (frmptr->FrameHeader+hcnt, DataPtr, DataCount);
         hcnt += DataCount;
         frmptr->MrsWriteCount = DataCount;
         /* indicate that it's all contained in the header */
         DataCount = 0;
      }

      frmptr->MrsDataPtr = DataPtr;
      frmptr->MrsDataCount = DataCount;

      if (msgptr->AstFunction)
      {
         /* asynchronous */
         if (DataCount)
            status = sys$qio (WsLibEfnNoWait, wsptr->OutputChannel,
                              IO$_WRITELBLK | IO$M_READERCHECK,
                              &frmptr->IOsb, WsLib__WriteMrsAst, frmptr, 
                              frmptr->FrameHeader, hcnt, 0, 0, 0, 0);
         else
            status = sys$qio (WsLibEfnNoWait, wsptr->OutputChannel,
                              IO$_WRITELBLK | IO$M_READERCHECK,
                              &frmptr->IOsb, WsLib__WriteAst, frmptr,
                              frmptr->FrameHeader, hcnt, 0, 0, 0, 0);
         if (VMSok(status)) wsptr->QueuedOutput++;
         return;
      }

      /* synchronous */
      sys$qiow (WsLibEfnWait, wsptr->OutputChannel,
                IO$_WRITELBLK | IO$M_READERCHECK,
                &frmptr->IOsb, 0, 0,
                frmptr->FrameHeader, hcnt, 0, 0, 0, 0);
      if (DataCount)
      {
         wsptr->QueuedOutput++;
         WsLib__WriteMrsAst (frmptr);
      }
   }

   WATCH_WSLIB (wsptr, FI_LI, "WRITE %X!8XL", frmptr->IOsb.iosb$w_status);

   /*******/
   /* end */
   /*******/

   if (VMSok (frmptr->IOsb.iosb$w_status))
   {
#ifdef __VAX
      /* close enough! */
      wsptr->OutputMsgCount[0]++;
#else
      *(__int64*)wsptr->OutputMsgCount = *(__int64*)wsptr->OutputMsgCount + 1;
#endif
   }
   else
      WsLibClose (wsptr, WSLIB_CLOSE_BANG, NULL);

   if (msgptr->AstFunction &&
       msgptr->AstFunction != WSLIB_ASYNCH)
   {
      /* status and count are I/O valid only during the AST function */
      status = wsptr->OutputStatus;
      count = wsptr->OutputDataCount;
      pointer = wsptr->OutputDataDsc.dsc$a_pointer;
      length = wsptr->OutputDataDsc.dsc$w_length;

      wsptr->OutputStatus = frmptr->IOsb.iosb$w_status;
      wsptr->OutputDataCount = frmptr->IOsb.iosb$w_bcnt;
      wsptr->OutputDataDsc.dsc$a_pointer = frmptr->DataPtr;
      wsptr->OutputDataDsc.dsc$w_length = wsptr->OutputDataCount;

      (*msgptr->AstFunction)(wsptr);

      wsptr->OutputStatus = status;
      wsptr->OutputDataCount = count;
      wsptr->OutputDataDsc.dsc$a_pointer = pointer;
      wsptr->OutputDataDsc.dsc$w_length = length;
   }

   if (msgptr->Utf8Ptr) free (msgptr->Utf8Ptr);
   free (msgptr);

   if (wsptr->WatchDogIdleSecs)
      wsptr->WatchDogIdleTime = CurrentTime + wsptr->WatchDogIdleSecs;
   if (wsptr->WatchDogWakeSecs)
      wsptr->WatchDogWakeTime = CurrentTime + wsptr->WatchDogWakeSecs;
}

/*****************************************************************************/
/*
Write the frame data in record MRS sized chunks (may only be one).  This
function is only used for payload greater than 125 bytes.  For 125 bytes or
less (including empty frames) it's all handled by WsLib__WriteAst().
*/

static void WsLib__WriteMrsAst (struct WsLibFrmStruct *frmptr)

{
   int  status,
        DataCount;
   char  *DataPtr;
   struct WsLibStruct  *wsptr;

   /*********/
   /* begin */
   /*********/

   wsptr = frmptr->WsLibMsgPtr->WsLibPtr;

   if (wsptr->QueuedOutput) wsptr->QueuedOutput--;

   for (;;)
   {
      if (*(ULONGPTR)frmptr->FrameHeader)
      {
         /* if the header has just been written these are invalid */
         frmptr->MrsWriteCount = frmptr->IOsb.iosb$w_bcnt = 0;
         *(ULONGPTR)frmptr->FrameHeader = 0;
      }

      if (VMSnok (frmptr->IOsb.iosb$w_status)) break;

      frmptr->MrsWriteCount += frmptr->IOsb.iosb$w_bcnt;

      WATCH_WSLIB (wsptr, FI_LI, "WRITE outque:!UL mrs:!UL/!UL !AZ",
                   wsptr->QueuedOutput,
                   frmptr->MrsWriteCount, frmptr->MrsDataCount,
                   frmptr->MrsWriteCount == frmptr->MrsDataCount ?
                      "COMPLETE" : "in-progress");

      if (frmptr->MrsWriteCount == frmptr->MrsDataCount) break;

      DataPtr = frmptr->MrsDataPtr + frmptr->MrsWriteCount;
      if (frmptr->MrsDataCount - frmptr->MrsWriteCount > wsptr->OutputMrs)
         DataCount = wsptr->OutputMrs;
      else
         DataCount = frmptr->MrsDataCount - frmptr->MrsWriteCount;

      if (frmptr->WsLibMsgPtr->AstFunction)
      {
         /* asynchronous data */
         status = sys$qio (WsLibEfnNoWait, wsptr->OutputChannel,
                           IO$_WRITELBLK | IO$M_READERCHECK,
                           &frmptr->IOsb, WsLib__WriteMrsAst, frmptr,
                           DataPtr, DataCount, 0, 0, 0, 0);
         if (VMSok(status)) wsptr->QueuedOutput++;
         return;
      }

      /* synchronous data */
      sys$qiow (WsLibEfnWait, wsptr->OutputChannel,
                IO$_WRITELBLK | IO$M_READERCHECK,
                &frmptr->IOsb, 0, 0,
                DataPtr, DataCount, 0, 0, 0, 0);
   }

   if (frmptr->MaskedPtr)
   {
      free (frmptr->MaskedPtr);
      frmptr->MaskedPtr = NULL;
   }

   if (frmptr->WsLibMsgPtr->AstFunction)
   {
      wsptr->QueuedOutput++;
      WsLib__WriteAst (frmptr);
   }
}

/*****************************************************************************/
/*
Return the write status value.
*/

int WsLibWriteStatus (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   return (wsptr->OutputStatus);
}

/*****************************************************************************/
/*
Return the write count value (longword).
*/

int WsLibWriteCount (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   return (wsptr->OutputDataCount);
}

/*****************************************************************************/
/*
Return a pointer to the (most recent) write data descriptor.
*/

struct dsc$descriptor_s* WsLibWriteDataDsc (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   return (&wsptr->OutputDataDsc);
}

/*****************************************************************************/
/*
Return a pointer to the total write count value (quadword).
*/

unsigned long* WsLibWriteTotal (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   return ((unsigned long*)&wsptr->OutputCount);
}

/*****************************************************************************/
/*
Return a pointer to the total write message value (quadword).
*/

unsigned long* WsLibWriteMsgTotal (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   return ((unsigned long*)&wsptr->OutputMsgCount);
}

/*****************************************************************************/
/*
Set the pointer to the user data associated with the WebSocket structure.
*/

void WsLibSetUserData
(
struct WsLibStruct *wsptr,
void *UserDataPtr
)
{
   /*********/
   /* begin */
   /*********/

   wsptr->UserDataPtr = UserDataPtr;
}

/*****************************************************************************/
/*
Return the pointer to the user data.
*/

void* WsLibGetUserData (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   return (wsptr->UserDataPtr);
}

/*****************************************************************************/
/*
Set/reset the callout response AST.  Returns the previous callout pointer.
*/

void* WsLibSetCallout
(
struct WsLibStruct *wsptr,
void *AstFunction
)
{
   void  *PrevCallout;

   /*********/
   /* begin */
   /*********/

   PrevCallout = wsptr->CalloutAstFunction;
   wsptr->CalloutAstFunction = AstFunction;
   return (PrevCallout);
}

/*****************************************************************************/
/*
Set the maxium frame size before the message is fragmented.
*/

int WsLibSetFrameMax
(
struct WsLibStruct *wsptr,
int FrameMax
)
{
   int  PrevFrameMax;

   /*********/
   /* begin */
   /*********/

   PrevFrameMax = wsptr->FrameMaxSize;
   wsptr->FrameMaxSize = FrameMax;
   return (PrevFrameMax);
}

/*****************************************************************************/
/*
Set wsLIB framing to binary.
*/

int WsLibSetBinary (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   wsptr->SetBinary = 1;
   wsptr->SetAscii = wsptr->SetUtf8 = 0;
   return (1);
}

/*****************************************************************************/
/*
Is wsLIB message content binary?
*/

int WsLibIsSetBinary (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   return (wsptr->SetBinary);
}

/*****************************************************************************/
/*
Set wsLIB framing to Text8, with 8 bit "ASCII" to UTF-8 implicit encoding.
*/

int WsLibSetAscii (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   wsptr->SetAscii = 1;
   wsptr->SetBinary = wsptr->SetUtf8 = 0;
   return (1);
}

/*****************************************************************************/
/*
Is wsLIB message content set to be Text8 (8 bit "ASCII")?
*/

int WsLibIsSetAscii (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   return (wsptr->SetAscii);
}

/*****************************************************************************/
/*
Set wsLIB framing to text (i.e. must already be UTF-8 encoding).
*/

int WsLibSetUtf8 (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   wsptr->SetUtf8 = 1;
   wsptr->SetBinary = wsptr->SetAscii = 0;
   return (1);
}

/*****************************************************************************/
/*
Is wsLIB message content UTF-8?
*/

int WsLibIsSetUtf8 (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   return (wsptr->SetUtf8);
}

/*****************************************************************************/
/*
Set wsLIB framing write data masked.
*/

int WsLibSetRoleClient (struct WsLibStruct *wsptr)

{
   int PrevRoleClient;

   /*********/
   /* begin */
   /*********/

   PrevRoleClient = wsptr->RoleClient;
   wsptr->RoleClient = 1;
   return (PrevRoleClient);
}

/*****************************************************************************/
/*
Set wsLIB framing write data not masked.
*/

int WsLibSetRoleServer (struct WsLibStruct *wsptr)

{
   int PrevRoleClient;

   /*********/
   /* begin */
   /*********/

   PrevRoleClient = wsptr->RoleClient;
   wsptr->RoleClient = 0;
   return (PrevRoleClient);
}

/*****************************************************************************/
/*
Is wsLIB message content set to write masked?
*/

int WsLibIsRoleClient (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   return (wsptr->RoleClient);
}

/*****************************************************************************/
/*
Set number of seconds before the application is considered idle and exited.
*/

void WsLibSetLifeSecs (int LifeSecs)

{
   /*********/
   /* begin */
   /*********/

   if (!(WatchDogLifeSecs = LifeSecs))
     WatchDogLifeSecs = DEFAULT_WATCHDOG_IDLE_SECS;
}

/*****************************************************************************/
/*
Set number of seconds before an unresponded-to WebSocket close is considered
closed and disconnected. If a WebSocket is not specified then set global value.
*/

void WsLibSetCloseSecs
(
struct WsLibStruct *wsptr,
int CloseSecs
)
{
   /*********/
   /* begin */
   /*********/

   if (wsptr)
   {
      /* set WebSocket values */
      if (!(wsptr->WatchDogCloseSecs = CloseSecs))
         wsptr->WatchDogCloseSecs = WatchDogCloseSecs;
      wsptr->WatchDogCloseTime = CurrentTime + wsptr->WatchDogCloseSecs;
   }
   else
   {
      /* set global values */
      if (!(WatchDogCloseSecs = CloseSecs))
        WatchDogCloseSecs = DEFAULT_WATCHDOG_CLOSE_SECS;
   }
}

/*****************************************************************************/
/*
Set number of seconds before a WebSocket is considered idle and closed.
If a WebSocket is not specified then set global value.
*/

void WsLibSetIdleSecs
(
struct WsLibStruct *wsptr,
int IdleSecs
)
{
   /*********/
   /* begin */
   /*********/

   if (wsptr)
   {
      /* set WebSocket values */
      if (!(wsptr->WatchDogIdleSecs = IdleSecs))
         wsptr->WatchDogIdleSecs = WatchDogIdleSecs;
      wsptr->WatchDogIdleTime = CurrentTime + wsptr->WatchDogIdleSecs;
   }
   else
   {
      /* set global values */
      if (!(WatchDogIdleSecs = IdleSecs))
        WatchDogIdleSecs = DEFAULT_WATCHDOG_IDLE_SECS;
   }
}

/*****************************************************************************/
/*
Set number of seconds before a WebSocket is considered ping and closed.
If a WebSocket is not specified then set global value.
*/

void WsLibSetPingSecs
(
struct WsLibStruct *wsptr,
int PingSecs
)
{
   /*********/
   /* begin */
   /*********/

   if (wsptr)
   {
      /* set WebSocket values */
      if (!(wsptr->WatchDogPingSecs = PingSecs))
         wsptr->WatchDogPingSecs = WatchDogPingSecs;
      wsptr->WatchDogPingTime = CurrentTime + wsptr->WatchDogPingSecs;
   }
   else
   {
      /* set global values */
      if (!(WatchDogPingSecs = PingSecs))
        WatchDogPingSecs = DEFAULT_WATCHDOG_PING_SECS;
   }
}

/*****************************************************************************/
/*
Set number of seconds a WebSocket read will wait on the client.
If a WebSocket is not specified then set global value.
*/

void WsLibSetReadSecs
(
struct WsLibStruct *wsptr,
int ReadSecs
)
{
   /*********/
   /* begin */
   /*********/

   if (wsptr)
   {
      /* set WebSocket values */
      if (!(wsptr->WatchDogReadSecs = ReadSecs))
         wsptr->WatchDogReadSecs = WatchDogReadSecs;
   }
   else
   {
      /* set global values */
      if (!(WatchDogReadSecs = ReadSecs))
        WatchDogReadSecs = DEFAULT_WATCHDOG_IDLE_SECS;
   }
}

/*****************************************************************************/
/*
Set/reset the ping (actually pong) callback function.
*/

void* WsLibSetPongCallback
(
struct WsLibStruct *wsptr,
void *AstFunction
)
{
   void  *PrevCallback;

   /*********/
   /* begin */
   /*********/

   if (wsptr)
   {
      PrevCallback = wsptr->PongCallbackFunction;
      wsptr->PongCallbackFunction = AstFunction;
   }
   else
   {
      PrevCallback = PongCallbackFunction;
      PongCallbackFunction = AstFunction;
   }
   return (PrevCallback);
}

/*****************************************************************************/
/*
Set/reset the wake callback function to the specified number of seconds
(defaults to the global setting).  Returns the previous callback pointer.
*/

void* WsLibSetWakeCallback
(
struct WsLibStruct *wsptr,
void *AstFunction,
int WakeSecs
)
{
   void  *PrevCallback;

   /*********/
   /* begin */
   /*********/

   if (wsptr)
   {
      /* set WebSocket values */
      if (!(wsptr->WatchDogWakeSecs = WakeSecs))
         wsptr->WatchDogWakeSecs = WatchDogWakeSecs;
      wsptr->WatchDogWakeTime = CurrentTime + wsptr->WatchDogWakeSecs;

      PrevCallback = wsptr->WakeCallbackFunction;
      wsptr->WakeCallbackFunction = AstFunction;
   }
   else
   {
      /* set global values */
      if (!(WatchDogWakeSecs = WakeSecs))
         WatchDogWakeSecs = DEFAULT_WATCHDOG_WAKE_SECS;
      WatchDogWakeTime = CurrentTime + WatchDogWakeSecs;

      PrevCallback = WakeCallbackFunction;
      WakeCallbackFunction = AstFunction;
   }
   return (PrevCallback);
}

/*****************************************************************************/
/*
Set/reset the error callback function.  Returns the previous callback pointer.
*/

void* WsLibSetMsgCallback
(
struct WsLibStruct *wsptr,
void *AstFunction
)
{
   void  *PrevCallback;

   /*********/
   /* begin */
   /*********/

   PrevCallback = wsptr->MsgCallbackFunction;
   wsptr->MsgCallbackFunction = AstFunction;
   return (PrevCallback);
}

/*****************************************************************************/
/*
Set wsLIB message data.  The 'FormatString' must be a $FAO compiliant
null-terminated string, with following parametersif required.  Activate any set
message callback.
*/

static void WsLib__MsgCallback
(
struct WsLibStruct *wsptr,
int LineNumber,
int VmsStatus,
char *FormatString,
...
)
{
   static $DESCRIPTOR (FormatFaoDsc, "");

   int  argcnt, cnt, status;
   unsigned short  slen = 0;
   unsigned long  *vecptr;
   unsigned long  FaoVector [32];
   char  *cptr, *sptr, *zptr;
   char  FormatBuffer [128];
   va_list  argptr;

   /*********/
   /* begin */
   /*********/

   va_count (argcnt);

   /* bit of a sanity check */
   if (argcnt-4 > 32) WsLibExit (wsptr, FI_LI, SS$_BUGCHECK);
   cnt = 0;
   /* some VERY basic sanity checking */
   for (cptr = FormatString; *cptr; cptr++)
   {
      if (*cptr != '!') continue;
      cptr++;
      if (*cptr == '!') continue;
      cnt++;
      if (*cptr == '#') cnt++;
      if (*(USHORTPTR)cptr == '%T') cnt++; 
      if (*(USHORTPTR)cptr == '%D') cnt++; 
   }
   if (argcnt-4 != cnt) WsLibExit (wsptr, FI_LI, SS$_BUGCHECK);

   zptr = (sptr = FormatBuffer) + sizeof(FormatBuffer)-1;
   for (cptr = "%X!8XL "; *cptr; *sptr++ = *cptr++);
   for (cptr = FormatString; *cptr && sptr < zptr; *sptr++ = *cptr++);
   *sptr = '\0';

   FormatFaoDsc.dsc$a_pointer = FormatBuffer;
   FormatFaoDsc.dsc$w_length = sptr - FormatBuffer;

   vecptr = FaoVector;
   *vecptr++ = VmsStatus;
   va_start (argptr, FormatString);
   for (argcnt -= 4; argcnt; argcnt--)
      *vecptr++ = va_arg (argptr, unsigned long);
   va_end (argptr);

   wsptr->MsgDsc.dsc$b_class = DSC$K_CLASS_S;
   wsptr->MsgDsc.dsc$b_dtype = DSC$K_DTYPE_T;

   for (;;)
   {
      if (wsptr->MsgStringSize)
      {
         wsptr->MsgDsc.dsc$a_pointer = wsptr->MsgStringPtr;
         wsptr->MsgDsc.dsc$w_length = wsptr->MsgStringSize;

         status = sys$faol (&FormatFaoDsc, &slen, &wsptr->MsgDsc, &FaoVector);
         if (VMSnok (status)) WsLibExit (NULL, FI_LI, status);
         if (status != SS$_BUFFEROVF) break;
      }

      if (wsptr->MsgStringSize) free (wsptr->MsgStringPtr);
      wsptr->MsgStringSize += 127;
      wsptr->MsgStringPtr = calloc (1, wsptr->MsgStringSize+1);
      if (!wsptr->MsgStringPtr) WsLibExit (NULL, FI_LI, vaxc$errno);
   }

   wsptr->MsgStringPtr[wsptr->MsgStringLength=slen] = '\0';
   wsptr->MsgDsc.dsc$w_length = slen;

   wsptr->MsgLineNumber = LineNumber;
   sys$gettim (&wsptr->MsgBinTime);

   if (wsptr->MsgCallbackFunction) (*wsptr->MsgCallbackFunction)(wsptr);
}

/****************************************************************************/
/*
Return a pointer to the string descriptor of the latest error string.
*/

struct dsc$descriptor_s* WsLibMsgDsc (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   return (&wsptr->MsgDsc);
}

/****************************************************************************/
/*
Return a pointer to the latest error string.
*/

char* WsLibMsgString (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   return (wsptr->MsgStringPtr);
}

/****************************************************************************/
/*
Return the wsLIB source code line at which the last error occured.
*/

int WsLibMsgLineNumber (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   return (wsptr->MsgLineNumber);
}

/****************************************************************************/
/*
Given a descriptor of UTF-8 convert in-situ to 8 bit ASCII.  The output
descriptor must be provided even if it points to the same storage as the input
descriptor.  Return the length of the converted string or -1 to indicated a
conversion error.  The input string is mangled if an error.
*/

int WsLibFromUtf8Dsc
(
struct dsc$descriptor_s *InDsc,
struct dsc$descriptor_s *OutDsc,
char SubsChar
)
{
   int  len;

   /*********/
   /* begin */
   /*********/

   if (InDsc->dsc$b_class != DSC$K_CLASS_S &&
       InDsc->dsc$b_dtype != DSC$K_DTYPE_T) return (LIB$_INVSTRDES);

   if (OutDsc->dsc$b_class != DSC$K_CLASS_S &&
       OutDsc->dsc$b_dtype != DSC$K_DTYPE_T) return (LIB$_INVSTRDES);

   len = WsLibFromUtf8 (InDsc->dsc$a_pointer, InDsc->dsc$w_length, SubsChar);
   if (len >= 0) OutDsc->dsc$w_length = len;

   return (SS$_NORMAL);
}

/****************************************************************************/
/*
Given a buffer of UTF-8 convert in-situ to 8 bit ASCII.  Ignore non- 8 bit
ASCII characters.  End-of-string is indicated by text-length not a
null-character, however the resultant string is nulled. If supplied the 8 bit
character 'SubsChar' is substituted for any non 8 bit code in the string.  
Return the number of converted characters.  Return -1 if there is an error. 
The input string is mangled if an error.
*/

int WsLibFromUtf8
(
char *UtfPtr,
int UtfCount,
char SubsChar
)
{
   unsigned char  ch;
   unsigned char  *cptr, *sptr, *zptr;

   /*********/
   /* begin */
   /*********/

   if (!UtfPtr) return (-1);

   if (UtfCount == -1) UtfCount = strlen(UtfPtr);

   if (UtfCount < 0) return (-1);

    /* is there a potentially UTF-8 bit pattern here? */
    for (zptr = (cptr = (unsigned char*)UtfPtr) + UtfCount; cptr < zptr; cptr++)
      if ((*cptr & 0xc0) == 0xc0) break;

   /* return if no UTF-8 conversion necessary (i.e. all 7 bit characters) */
   if (cptr >= zptr) return (UtfCount);
   if (*cptr == 0xff) return (cptr - (unsigned char*)UtfPtr);

   sptr = cptr;
   while (cptr < zptr)
   {
      if ((*cptr & 0xf8) == 0xf0)
      {
         /* four byte sequence */
         if (++cptr >= zptr) goto utf8_nbg;
         if ((*cptr & 0xc0) != 0x80) goto utf8_nbg;
         if (++cptr >= zptr) goto utf8_nbg;
         if ((*cptr & 0xc0) != 0x80) goto utf8_nbg;
         if (++cptr >= zptr) goto utf8_nbg;
         if ((*cptr & 0xc0) != 0x80) goto utf8_nbg;
         if (SubsChar) *sptr++ = SubsChar;
      }
      else
      if ((*cptr & 0xf0) == 0xe0)
      {
         /* three byte sequence */
         if (++cptr >= zptr) goto utf8_nbg;
         if ((*cptr & 0xc0) != 0x80) goto utf8_nbg;
         if (++cptr >= zptr) goto utf8_nbg;
         if ((*cptr & 0xc0) != 0x80) goto utf8_nbg;
         if (SubsChar) *sptr++ = SubsChar;
      }
      else
      if ((*cptr & 0xe0) == 0xc0)
      {
         /* two byte sequence */
         if (*cptr & 0x1c)
         {
            /* out-of-range character */
            if (++cptr >= zptr) goto utf8_nbg;
            if ((*cptr & 0xc0) != 0x80) goto utf8_nbg;
            if (++cptr >= zptr) goto utf8_nbg;
            if (SubsChar) *sptr++ = SubsChar;
         }
         else
         {
            /* 8 bit ASCII 128 to 255 */
            ch = (*cptr & 0x03) << 6;
            if (++cptr >= zptr) goto utf8_nbg;
            if ((*cptr & 0xc0) != 0x80) goto utf8_nbg;
            ch |= *cptr & 0x3f;
            *sptr++ = ch;
            cptr++;
         }
      }
      else
      {
         /* 8 bit ASCII 0 to 127 */
         *sptr++ = *cptr++;
      }
   }
   *sptr = '\0';

   return (sptr - (unsigned char*)UtfPtr);

   utf8_nbg:
      return (-1);
}

/****************************************************************************/
/*
Given a descriptor of 8 bit ASCII convert it to UTF-8.  Can be done in-situ.
The output descriptor must be provided even if it points to the same storage as
the input descriptor.  Return the length of the converted string or -1 to
indicated a conversion error.

*/

int WsLibToUtf8Dsc
(
struct dsc$descriptor_s *InDsc,
struct dsc$descriptor_s *OutDsc
)
{
   int  len;

   /*********/
   /* begin */
   /*********/

   if (InDsc->dsc$b_class != DSC$K_CLASS_S &&
       InDsc->dsc$b_dtype != DSC$K_DTYPE_T) return (LIB$_INVSTRDES);

   if (OutDsc->dsc$b_class != DSC$K_CLASS_S &&
       OutDsc->dsc$b_dtype != DSC$K_DTYPE_T) return (LIB$_INVSTRDES);

   len = WsLibToUtf8 (InDsc->dsc$a_pointer, InDsc->dsc$w_length,
                      OutDsc->dsc$a_pointer, OutDsc->dsc$w_length);

   if (len == -1) return (SS$_ABORT);

   return (SS$_NORMAL);
}

/****************************************************************************/
/*
Given a buffer of 8 bit ASCII text convert it to UTF-8.  This can be done
in-situ with the worst-case the buffer space needs to be two times in size
(i.e. every character has the hi bit set requiring a leading UTF-8 byte). 
End-of-string is indicated by text-length not a null-character, however the
resultant string is nulled.  Return the length of the converted string.  Return
-1 if the buffer space will be too small.
*/

int WsLibToUtf8
(
char *InPtr,
int InLength,
char *OutPtr,
int SizeOfOut
)
{
   int  Utf8Count = 0;
   char  *cptr, *czptr, *sptr;

   /*********/
   /* begin */
   /*********/

   if (!InPtr) return (-1);

   if (InLength == -1) InLength = strlen(InPtr);

   for (czptr = (cptr = InPtr) + InLength; cptr < czptr; cptr++)
      if (*cptr & 0x80) Utf8Count++;

   if (!Utf8Count)
   {
      if (!OutPtr) return (InLength);
      if (OutPtr == InPtr) return (InLength);
      /* just copy to output buffer */
      if (InLength >= SizeOfOut - 1) return (-1);
      memcpy (OutPtr, InPtr, InLength);
      OutPtr[InLength] = '\0'; 
      return (InLength);
   }

   if (InLength + Utf8Count >= SizeOfOut - 1) return (-1);

   cptr = (czptr = InPtr) + InLength - 1;
   if (!(sptr = OutPtr)) sptr = InPtr;
   sptr += InLength - 1;
   sptr += Utf8Count + 1;
   *sptr-- = '\0';
   while (cptr >= czptr)
   {
      if (*cptr & 0x80)
      {
         *sptr-- = (*cptr & 0x3f) | 0x80;
         *sptr-- = ((*cptr-- & 0xc0) >> 6) | 0xc0;
      }
      else
         *sptr-- = *cptr--;
   }

   return (InLength + Utf8Count);
}

/****************************************************************************/
/*
Called with a frame pointer after reading UTF-8 data from the client.
The data is parsed as received (i.e. not necessarily a complete message) to
ensure the UTF-8 (received so far) appears legal.  Provides "fast fail" on
illegal UTF-8.  Return true if legal, false if not.

Algorithm and essential code ...

Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
*/

static int WsLib__Utf8Legal (struct WsLibFrmStruct *frmptr)

{
static const unsigned char  utf8d[] =
{
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
  8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
  0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
  0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff
  0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
  1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
  1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
  1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
};

   int  cnt,
        Utf8Count = 0;
   unsigned int  byte, state, type;
   unsigned char  *cptr, *czptr;
   struct WsLibMsgStruct  *msgptr;

   /*********/
   /* begin */
   /*********/

   msgptr = frmptr->WsLibMsgPtr;
   state = msgptr->Utf8State;

   if (!frmptr->IOsb.iosb$w_bcnt)
   {
      /* checking code-point at end of message */
      return (state == 0);
   }

   cptr = (unsigned char*)frmptr->DataPtr + frmptr->DataCount;
   czptr = cptr + frmptr->IOsb.iosb$w_bcnt;
   while (cptr < czptr)
   {
      /* for efficiency, concurrently apply masking key */
      if (frmptr->FrameMaskBit)
         *cptr ^= frmptr->MaskingKey[frmptr->MaskCount++&0x3];
      byte = *cptr++;
      type = utf8d[byte];
      state = utf8d[256+(state*16)+type];
      if (!state) Utf8Count++;
   }
   msgptr->Utf8State = state;
   msgptr->Utf8Count += Utf8Count;

   return (state != 1);
}

/*****************************************************************************/
/*
Return the current wsLIB time in seconds (i.e. C-RTL, Unix time).
*/

unsigned int WsLibTime ()

{
   /*********/
   /* begin */
   /*********/

   return (CurrentTime);
}

/*****************************************************************************/
/*
Called every second to maintain various events in the WebSocket and application
life-cycle.
*/

static void WsLib__WatchDog ()

{
   static unsigned long  OneSecondDelta [2] = { -10000000, -1 };
   static unsigned long  ExitTime;

   int  status,
        StringLength;
   char  StringBuffer [256];
   struct WsLibStruct  *wsptr;

   /*********/
   /* begin */
   /*********/

   sys$gettim (&CurrentBinTime);
   CurrentTime = decc$fix_time (&CurrentBinTime);

   if (0 && WsLibListHead)
      WATCH_WSLIB (WsLibListHead, FI_LI, "WATCHDOG !UL", CurrentTime);

   if (WsLibListHead)
      ExitTime = 0;
   else
   if (!ExitTime)
      ExitTime = CurrentTime + WatchDogLifeSecs;
   else
   if (ExitTime < CurrentTime)
      exit (SS$_NORMAL);

   if (WatchDogWakeTime &&
       WatchDogWakeTime < CurrentTime)
   {
      /* wake globally if requested */
      WatchDogWakeTime = CurrentTime + WatchDogWakeSecs - 1;
      if (WakeCallbackFunction) sys$dclast (WakeCallbackFunction, 0, 0, 0);
   }

   for (wsptr = WsLibListHead; wsptr; wsptr = wsptr->NextPtr)
   {
      /* flush any watch log to disk every second */
      if (wsptr->WatchLog) fsync (fileno(wsptr->WatchLog));

      if (wsptr->WebSocketClosed)
      {
         /* if not yet shut then do so after a little patience */
         if (!wsptr->WatchDogCloseTime)
            if (wsptr->WatchDogCloseSecs)
               wsptr->WatchDogCloseTime = CurrentTime +
                                          wsptr->WatchDogCloseSecs;
            else
               wsptr->WatchDogCloseTime = CurrentTime + WatchDogCloseSecs;
         else
         if (wsptr->WatchDogCloseTime < CurrentTime)
            sys$dclast (WsLib__Shut, wsptr, 0, 0);
      }
      else
      if (wsptr->WatchDogReadTime &&
          wsptr->WatchDogReadTime < CurrentTime)
      {
         /* advise client to close WebSocket */
         WsLibClose (wsptr, WSLIB_CLOSE_POLICY, "read wait exceeded");
      }
      else
      if (wsptr->WatchDogIdleTime &&
          wsptr->WatchDogIdleTime < CurrentTime)
      {
         /* advise client to close WebSocket */
         WsLibClose (wsptr, WSLIB_CLOSE_POLICY, "idle connection");
      }
      else
      if (wsptr->WatchDogPingTime &&
          wsptr->WatchDogPingTime < CurrentTime)
      {
         /* periodic ping (heartbeat) to remote end */
         StringLength = sprintf (StringBuffer, "%u %u",
                                 ++wsptr->WatchDogPingCount, CurrentTime);
         WsLibPing (wsptr, StringBuffer, StringLength);
         if (wsptr->WatchDogPingSecs)
            wsptr->WatchDogPingTime = CurrentTime +
                                      wsptr->WatchDogPingSecs - 1;
         else
            wsptr->WatchDogPingCount = 0;
      }
      else
      if (wsptr->WatchDogWakeTime &&
          wsptr->WatchDogWakeTime < CurrentTime)
      {
         /* wake if requested */
         wsptr->WatchDogWakeTime = CurrentTime + wsptr->WatchDogWakeSecs - 1;
         if (wsptr->WakeCallbackFunction)
            sys$dclast (wsptr->WakeCallbackFunction, wsptr, 0, 0);
      }
   }

   status = sys$setimr (0, &OneSecondDelta, WsLib__WatchDog, 0, 0);
   if (VMSnok(status)) WsLibExit (NULL, FI_LI, status);
}                            

/*****************************************************************************/
/*
Exit from the current image reporting to the server WEBSOCKET_OUTPUT stream (if
a channel assigned) or (last-ditch) to <stdout> the exit code module name and
line number.
*/

void WsLibExit
(
struct WsLibStruct *wsptr,
char *SourceModuleName,
int SourceLineNumber,
int status
)
{
   static unsigned long  OneSecondDelta [2] = { -10000000, -1 };
   static $DESCRIPTOR (FaoDsc, "BYE-BYE [!AZ:!UL] %X!8XL\n\0");

   short  slen;
   char  MsgBuffer [256];
   $DESCRIPTOR (MsgDsc, MsgBuffer);

   /*********/
   /* begin */
   /*********/

   sys$fao (&FaoDsc, &slen, &MsgDsc,
            SourceModuleName, SourceLineNumber, status);

   if (wsptr && wsptr->OutputChannel)
      sys$qiow (WsLibEfnWait, wsptr->OutputChannel,
                IO$_WRITELBLK | IO$M_READERCHECK, 0, 0, 0,
                MsgBuffer, slen-1, 0, 0, 0, 0);
   else
   {
      fputs (MsgBuffer, stdout);
      fflush (stdout);
   }

   sys$schdwk (0, 0, OneSecondDelta, 0);
   sys$hiber ();
   sys$delprc (0, 0, 0);
}

/*****************************************************************************/
/*
Free C-RTL allocated memory (such as that returned by WsLibReadGrab()) in an
application neutral fashion (in case non-C-RTL allocations are used in the
future).
*/

void WsLibFree (char *cptr)

{
   /*********/
   /* begin */
   /*********/

   free (cptr);
}

/*****************************************************************************/
/*
The 'FormatString' must be a $FAO compliant null-terminated string, with
following parameters.
*/

void WsLibCallout
(
struct WsLibStruct *wsptr,
char *FormatString,
...
)
{
   static $DESCRIPTOR (ErrorFaoDsc, "!!WATCH: $FAO %X!8XL");

   int  argcnt, cnt, status;
   unsigned short  slen = 0;
   unsigned long  *vecptr;
   unsigned long  FaoVector [32];
   char  *aptr, *cptr;
   char  CalloutBuffer [1024];
   va_list  argptr;
   $DESCRIPTOR (CalloutBufferDsc, CalloutBuffer);
   $DESCRIPTOR (FormatDsc, "");

   /*********/
   /* begin */
   /*********/

   if (!CgiPlusEscLength && !CgiPlusEotLength) return;

   if (!wsptr->OutputChannel) return;

   va_count (argcnt);

   /* can't call callout while in a callout response delivery */
   if (wsptr->CalloutInProgress) WsLibExit (wsptr, FI_LI, SS$_BUGCHECK);

   /* bit of a sanity check */
   if (argcnt-2 > 32) WsLibExit (wsptr, FI_LI, SS$_BUGCHECK);
   cnt = 0;
   for (cptr = FormatString; *cptr; cptr++)
      if (*cptr == '!' && *(USHORTPTR)cptr != '!!') cnt++;
   if (argcnt-2 != cnt) WsLibExit (wsptr, FI_LI, SS$_BUGCHECK);

   FormatDsc.dsc$a_pointer = FormatString;
   FormatDsc.dsc$w_length = strlen(FormatString);

   vecptr = FaoVector;
   va_start (argptr, FormatString);
   for (argcnt -= 2; argcnt; argcnt--)
      *vecptr++ = va_arg (argptr, unsigned long);
   va_end (argptr);

   status = sys$faol (&FormatDsc, &slen, &CalloutBufferDsc, &FaoVector);
   if (VMSnok(status))
      status = sys$fao (&ErrorFaoDsc, &slen, &CalloutBufferDsc, status);

   /* allocate a pointer plus a buffer (freed by WsLib__OutputFreeAst()) */ 
   aptr = calloc (1, sizeof(struct WsLibStruct*) + slen);
   if (!aptr) WsLibExit (wsptr, FI_LI, vaxc$errno);
   *(struct WsLibStruct**)aptr = wsptr;
   cptr = aptr + sizeof(struct WsLibStruct*);
   memcpy (cptr, CalloutBuffer, slen);

   status = sys$qio (WsLibEfnNoWait, wsptr->OutputChannel,
                     IO$_WRITELBLK | IO$M_READERCHECK,
                     0, WsLib__OutputAst, wsptr,
                     CgiPlusEscPtr, CgiPlusEscLength, 0, 0, 0, 0);
   if (VMSok(status)) wsptr->QueuedOutput++;

   status = sys$qio (WsLibEfnNoWait, wsptr->OutputChannel,
                     IO$_WRITELBLK | IO$M_READERCHECK,
                     0, WsLib__OutputFreeAst, aptr,
                     cptr, slen, 0, 0, 0, 0);
   if (VMSok(status)) wsptr->QueuedOutput++;

   status = sys$qio (WsLibEfnNoWait, wsptr->OutputChannel,
                     IO$_WRITELBLK | IO$M_READERCHECK,
                     0, WsLib__OutputAst, wsptr,
                     CgiPlusEotPtr, CgiPlusEotLength, 0, 0, 0, 0);
   if (VMSok(status)) wsptr->QueuedOutput++;
}

/*****************************************************************************/
/*
Send a script "WATCH:" callout to the server (if [x]script enabled).  The
'SourceModuleName' can be NULL.  The 'FormatString' must be a $FAO compiliant
null-terminated string, with following parameters.  A specific channel is
assigned for WATCH output so that it can be deassigned as late in request
processing as possible.
*/

void WsLibWatchScript
(
struct WsLibStruct *wsptr,
char *SourceModuleName,
int SourceLineNumber,
char *FormatString,
...
)
{
   static $DESCRIPTOR (ErrorFaoDsc, "!!WATCH: $FAO %X!8XL");
   static $DESCRIPTOR (TimeFaoDsc, "!%T\0");
   static $DESCRIPTOR (Watch1FaoDsc, "!!!!WATCH: [!AZ:!4ZL] !AZ");
   static $DESCRIPTOR (Watch2FaoDsc, "!!!!WATCH: !AZ");

   int  argcnt, cnt, status;
   unsigned short  slen = 0;
   unsigned long  *vecptr;
   unsigned long  FaoVector [32];
   char  *aptr, *cptr;
   char  TimeBuffer [32],
         WatchBuffer [1024],
         WatchFao [256];
   va_list  argptr;
   $DESCRIPTOR (FaoDsc, WatchFao);
   $DESCRIPTOR (TimeBufferDsc, TimeBuffer);
   $DESCRIPTOR (WatchBufferDsc, WatchBuffer);

   /*********/
   /* begin */
   /*********/

   if (!wsptr || !wsptr->WatchScript) return;

   if (!wsptr->OutputChannel) return;

   /* can't call callout while in a callout response delivery */
   if (wsptr->CalloutInProgress) WsLibExit (wsptr, FI_LI, SS$_BUGCHECK);

   va_count (argcnt);

   /* bit of a sanity check */
   if (argcnt-4 > 32) WsLibExit (wsptr, FI_LI, SS$_BUGCHECK);
   cnt = 0;
   for (cptr = FormatString; *cptr; cptr++)
   {
      if (*cptr != '!') continue;
      cptr++;
      if (*cptr == '!') continue;
      cnt++;
      if (*cptr == '#') cnt++;
      if (*(USHORTPTR)cptr == '%T') cnt++; 
      if (*(USHORTPTR)cptr == '%D') cnt++; 
   }
   if (argcnt-4 != cnt) WsLibExit (wsptr, FI_LI, SS$_BUGCHECK);

   if (SourceModuleName)
      sys$fao (&Watch1FaoDsc, &slen, &FaoDsc,
               SourceModuleName, SourceLineNumber, FormatString);
   else
      sys$fao (&Watch2FaoDsc, &slen, &FaoDsc, FormatString);
    FaoDsc.dsc$w_length = slen;

   vecptr = FaoVector;
   va_start (argptr, FormatString);
   for (argcnt -= 4; argcnt; argcnt--)
      *vecptr++ = va_arg (argptr, unsigned long);
   va_end (argptr);

   status = sys$faol (&FaoDsc, &slen, &WatchBufferDsc, &FaoVector);
   if (!(status & 1))
      status = sys$fao (&ErrorFaoDsc, &slen, &WatchBufferDsc, status);

   /* allocate a pointer plus a buffer (freed by WsLib__OutputFreeAst()) */ 
   aptr = calloc (1, sizeof(struct WsLibStruct*) + slen);
   if (!aptr) WsLibExit (wsptr, FI_LI, vaxc$errno);
   *(struct WsLibStruct**)aptr = wsptr;
   cptr = aptr + sizeof(struct WsLibStruct*);
   memcpy (cptr, WatchBuffer, slen);

   if (!CgiPlusEscLength && !CgiPlusEotLength)
   {
      fprintf (stdout, "%*.*s\n", slen, slen, WatchBuffer);
      return;
   }

   if (wsptr->WatchLog)
   {
      sys$fao (&TimeFaoDsc, 0, &TimeBufferDsc, 0);
      fprintf (wsptr->WatchLog, "%s %*.*s\n",
               TimeBuffer, slen-8, slen-8, WatchBuffer+8);
   }
   else
   {
      status = sys$qio (WsLibEfnNoWait, wsptr->OutputChannel,
                        IO$_WRITELBLK | IO$M_READERCHECK,
                        0, WsLib__OutputAst, wsptr,
                        CgiPlusEscPtr, CgiPlusEscLength, 0, 0, 0, 0);
      if (VMSok(status))
      {
         wsptr->QueuedOutput++;
         status = sys$qio (WsLibEfnNoWait, wsptr->OutputChannel,
                           IO$_WRITELBLK | IO$M_READERCHECK,
                           0, WsLib__OutputFreeAst, aptr,
                           cptr, slen, 0, 0, 0, 0);
         if (VMSok(status))
         {
            wsptr->QueuedOutput++;
            status = sys$qio (WsLibEfnNoWait, wsptr->OutputChannel,
                              IO$_WRITELBLK | IO$M_READERCHECK,
                              0, WsLib__OutputAst, wsptr,
                              CgiPlusEotPtr, CgiPlusEotLength, 0, 0, 0, 0);
            if (VMSok(status)) wsptr->QueuedOutput++;
         }
      }
   }
}

/*****************************************************************************/
/*
Just decrement the queued output counter.
*/

void WsLib__OutputAst (struct WsLibStruct *wsptr)

{
   /*********/
   /* begin */
   /*********/

   if (wsptr->QueuedOutput) wsptr->QueuedOutput--;
   if (wsptr->WebSocketShut) WsLib__Shut (wsptr);
}

/*****************************************************************************/
/*
The parameter is an address to allocated memory.  The memory contains a pointer
to a WsLib structure and after that opaque data (actually just a write buffer
but what the hey).  Get the pointer to the owning WsLib structure.  Free the
allocated memory.  Decrement the WsLib structure's queued output counter.
*/

void WsLib__OutputFreeAst (char *aptr)

{
   struct WsLibStruct  *wsptr;

   /*********/
   /* begin */
   /*********/

   wsptr = *(struct WsLibStruct**)aptr;
   free (aptr);
   if (wsptr->QueuedOutput) wsptr->QueuedOutput--;
   if (wsptr->WebSocketShut) WsLib__Shut (wsptr);
}

/*****************************************************************************/
/*
Just used for WATCH purposes.
*/

static char* WsLib__OpCodeName (int OpCode)

{
   /*********/
   /* begin */
   /*********/

   switch (OpCode & 0xf)
   {
      case  0 : return ("continue");
      case  1 : return ("text");
      case  2 : return ("binary");
      case  8 : return ("close");
      case  9 : return ("ping");
      case 10 : return ("pong");
      default : return ("unknown");
   }
}

/*****************************************************************************/
/*
If a CGIplus environment then output the end-of-request sentinal.
*/

void WsLibCgiPlusEof ()

{
   /*********/
   /* begin */
   /*********/

   if (!CgiPlusEofLength) return;
   fflush (stdout);
   fputs (CgiPlusEofPtr, stdout);
   fflush (stdout);
}

/*****************************************************************************/
/*
Output the callout end sentinal.
*/

void WsLibCgiPlusEot ()

{
   /*********/
   /* begin */
   /*********/

   if (!CgiPlusEotLength) return;
   fflush (stdout);
   fputs (CgiPlusEotPtr, stdout);
   fflush (stdout);
}

/*****************************************************************************/
/*
Output the callout begin sentinal.
*/

void WsLibCgiPlusEsc ()

{
   /*********/
   /* begin */
   /*********/

   if (!CgiPlusEscLength) return;
   fflush (stdout);
   fputs (CgiPlusEscPtr, stdout);
   fflush (stdout);
}

/*****************************************************************************/
/*
Output the callout begin sentinal.
*/

void WsLibCalloutStart ()

{
   /*********/
   /* begin */
   /*********/

   if (!CgiPlusEscLength) return;
   fflush (stdout);
   fputs (CgiPlusEscPtr, stdout);
   fflush (stdout);
}

/*****************************************************************************/
/*
Return true if it's a CGIplus execution environment.
*/

int WsLibIsCgiPlus ()

{
   static int  InitIs;

   /*********/
   /* begin */
   /*********/

   if (!InitIs)
   {
      InitIs = 1;
      if (CgiPlusEofPtr = getenv("CGIPLUSEOF"))
         CgiPlusEofLength = strlen(CgiPlusEofPtr);
      if (CgiPlusEscPtr = getenv("CGIPLUSESC"))
         CgiPlusEscLength = strlen(CgiPlusEscPtr);
      if (CgiPlusEotPtr = getenv("CGIPLUSEOT"))
         CgiPlusEotLength = strlen(CgiPlusEotPtr);
   }
   return (CgiPlusEofLength);
}

/*****************************************************************************/
/*
String descriptor equivalent of WsLibCgiVar() with the same requirements and
constraints.  Set 'ValueDsc' to the value of the CGI variable (or empty if it
doesn't exist).  Return the length of the value of -1 if the CGI variable name
does not exist.
*/

int WsLibCgiVarDsc
(
struct dsc$descriptor_s *NameDsc,
struct dsc$descriptor_s *ValueDsc
)
{
   char  *cptr, *czptr, *sptr, *zptr;
   char  VarName [256];

   /*********/
   /* begin */
   /*********/

   if (NameDsc->dsc$b_class != DSC$K_CLASS_S &&
       ValueDsc->dsc$b_dtype != DSC$K_DTYPE_T) return (LIB$_INVSTRDES);

   if (ValueDsc->dsc$b_class != DSC$K_CLASS_S &&
       ValueDsc->dsc$b_dtype != DSC$K_DTYPE_T) return (LIB$_INVSTRDES);

   zptr = (sptr = VarName) + sizeof(VarName)-1;
   czptr = (cptr = NameDsc->dsc$a_pointer) + NameDsc->dsc$w_length;
   while (cptr < czptr && sptr < zptr) *sptr++ = *cptr++;
   *sptr = '\0';

   if (cptr = WsLibCgiVarNull (VarName))
   {
      for (sptr = cptr; *sptr; sptr++);
      ValueDsc->dsc$w_length = sptr - cptr;
      ValueDsc->dsc$a_pointer = cptr;
      return (SS$_NORMAL);
   }
   else
      return (SS$_ITEMNOTFOUND);
}

/*****************************************************************************/
/*
Return empty string rather than NULL if the CGI variable does not exist.
*/

char* WsLibCgiVar (char* VarName)
{
   char  *cptr;

   /*********/
   /* begin */
   /*********/

   if (cptr = WsLibCgiVarNull (VarName))
      return (cptr);
   else
      return ("");
}

/*****************************************************************************/
/*
Return the value of a CGI variable regardless of whether it is used in a
standard CGI environment or a WASD CGIplus environment.  Automatically switches
WASD V7.2 into 'struct' mode for significantly improved performance.

Call with 'VarName' empty ("") to synchronize CGIplus requests.  This waits for
a CGIplus variable stream, checks if it's in 'record' or 'struct' mode, reads
the stream appropriately before returning ready to provide variables.

DO NOT modify the character string returned by this function.  Copy to other
storage if this is necessary.  The behaviour is indeterminate if the returned
values are modified in any way!

All CGIplus variables may be returned by making successive calls using a
'VarName' of "*" (often useful when debugging).  NULL is returned when
variables exhausted.
*/
/*****************************************************************************/

char* WsLibCgiVarNull (char *VarName)
{
#define SOUS sizeof(unsigned short)

   static int  CalloutDone,
               InitPrefix,
               StructBufferSize,
               StructLength,
               WwwPrefix;
   static char  *CgiPlusVarRecordPtr,
                *NextVarNamePtr,
                *StructBufferPtr;
   static FILE  *CgiPlusIn;

   int  Length;
   char  WwwVarName [256];
   char  *bptr, *cptr, *sptr;

   /*********/
   /* begin */
   /*********/

   if (!StructBufferSize)
   {
      StructBufferSize = 4096;
      StructBufferPtr = calloc (1, StructBufferSize);
      if (!StructBufferPtr) WsLibExit (NULL, FI_LI, vaxc$errno);
   }

   if (VarName == NULL || !VarName[0])
   {
      /* initialize */
      StructLength = WwwPrefix = 0;
      NextVarNamePtr = StructBufferPtr;
      if (VarName == NULL) return (NULL);
   }

   if (!InitPrefix)
   {
      InitPrefix = 1;
      WwwPrefix = (getenv ("WWW_SERVER_SOFTWARE") != NULL);
      /* ensure initialisation */
      WsLibIsCgiPlus ();
   }

   if (VarName[0])
   {
      /***************************/
      /* return a variable value */
      /***************************/

      if (*(ULONGPTR)VarName == 'WWW_' && !WwwPrefix)
         VarName += 4;
      else
      if (*(ULONGPTR)VarName != 'WWW_' && WwwPrefix)
      {
         strcpy (WwwVarName, "WWW_");
         strcpy (WwwVarName+4, VarName);
         VarName = WwwVarName;
      }

      if (CgiPlusEofPtr == NULL)
      {
         /* standard CGI environment */
         if ((cptr = getenv (VarName)) == NULL) cptr = NULL;
         return (cptr);
      }

      /* hmmm, not initialized */
      if (!StructLength) return (NULL);

      if (VarName[0] == '*')
      {
         /* return each CGIplus variable in successive calls */
         if (!(Length = *(USHORTPTR)NextVarNamePtr))
         {
            NextVarNamePtr = StructBufferPtr;
            return (NULL);
         }
         sptr = (NextVarNamePtr += SOUS);
         NextVarNamePtr += Length;
         return (sptr);
      }

      /* return a pointer to this CGIplus variable's value */
      for (bptr = StructBufferPtr;
           Length = *(USHORTPTR)bptr;
           bptr += Length)
      {
         sptr = (bptr += SOUS);
         for (cptr = VarName; *cptr && *sptr && *sptr != '='; cptr++, sptr++)
            if (toupper(*cptr) != toupper(*sptr)) break;
         /* if found return a pointer to the value */
         if (!*cptr && *sptr == '=') return (sptr+1);
      }
      /* not found */
      return (NULL);
   }

   /*****************************/
   /* get the CGIplus variables */
   /*****************************/

   /* cannot "sync" in a non-CGIplus environment */
   if (!VarName[0] && CgiPlusEofPtr == NULL) return (NULL);

   WwwPrefix = 0;

   /* the CGIPLUSIN stream can be left open */
   if (CgiPlusIn == NULL)
      if ((CgiPlusIn = fopen (getenv("CGIPLUSIN"), "r")) == NULL)
         WsLibExit (NULL, FI_LI, vaxc$errno);

   /* get the starting record (the essentially discardable one) */
   for (;;)
   {
      cptr = fgets (StructBufferPtr, StructBufferSize, CgiPlusIn);
      if (cptr == NULL) WsLibExit (NULL, FI_LI, vaxc$errno);
      /* if the starting sentinal is detected then break */
      if (*(USHORTPTR)cptr == '!\0' ||
          *(USHORTPTR)cptr == '!\n' ||
          (*(USHORTPTR)cptr == '!!' && isdigit(*(cptr+2)))) break;
   }

   /* detect the CGIplus "force" record-mode environment variable (once) */
   if (CgiPlusVarRecordPtr == NULL)
      if ((CgiPlusVarRecordPtr = getenv ("CGIPLUS_VAR_RECORD")) == NULL)
         CgiPlusVarRecordPtr = "";

   if (*(USHORTPTR)cptr == '!!' && !CgiPlusVarRecordPtr[0])
   {
      /********************/
      /* CGIplus 'struct' */
      /********************/

      /* get the size of the binary structure */
      StructLength = atoi(cptr+2);
      if (StructLength <= 0) WsLibExit (NULL, FI_LI, SS$_BUGCHECK);

      if (StructLength > StructBufferSize)
      {
         while (StructLength > StructBufferSize) StructBufferSize *= 2;
         free (StructBufferPtr);
         StructBufferPtr = calloc (1, StructBufferSize);
         if (!StructBufferPtr) WsLibExit (NULL, FI_LI, vaxc$errno);
         NextVarNamePtr = StructBufferPtr;
      }

      if (!fread (StructBufferPtr, 1, StructLength, CgiPlusIn))
         WsLibExit (NULL, FI_LI, vaxc$errno);
   }
   else
   {
      /*********************/
      /* CGIplus 'records' */
      /*********************/

      /* reconstructs the original 'struct'ure from the records */
      sptr = (bptr = StructBufferPtr) + StructBufferSize;
      while (fgets (bptr+SOUS, sptr-(bptr+SOUS), CgiPlusIn) != NULL)
      {
         /* first empty record (line) terminates variables */
         if (bptr[SOUS] == '\n') break;
         /* note the location of the length word */
         cptr = bptr;
         for (bptr += SOUS; *bptr && *bptr != '\n'; bptr++);
         if (*bptr != '\n') WsLibExit (NULL, FI_LI, SS$_BUGCHECK);
         *bptr++ = '\0';
         if (bptr >= sptr) WsLibExit (NULL, FI_LI, SS$_BUGCHECK);
         /* update the length word */
         *(USHORTPTR)cptr = bptr - (cptr + SOUS);
      }
      if (bptr >= sptr) WsLibExit (NULL, FI_LI, SS$_BUGCHECK);
      /* terminate with a zero-length entry */
      *(USHORTPTR)bptr = 0;
      StructLength = (bptr + SOUS) - StructBufferPtr;
   }

   if (!CalloutDone && !CgiPlusVarRecordPtr[0])
   {
      /* provide the CGI callout to set CGIplus into 'struct' mode */
      fflush (stdout);
      fputs (CgiPlusEscPtr, stdout);
      fflush (stdout);
      /* the leading '!' indicates we're not going to read the response */
      fputs ("!CGIPLUS: struct", stdout);
      fflush (stdout);
      fputs (CgiPlusEotPtr, stdout);
      fflush (stdout);
      /* don't need to do this again (the '!!' tells us what mode) */
      CalloutDone = 1;
   }

   sptr = StructBufferPtr + SOUS;
   if (*(ULONGPTR)sptr == 'WWW_') WwwPrefix = 1;

   return (NULL);

#  undef SOUS
}

/*****************************************************************************/

