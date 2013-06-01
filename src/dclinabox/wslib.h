/*****************************************************************************/
/*
                                  wslib.h
*/
/*****************************************************************************/

#ifndef WSLIB_H_LOADED
#define WSLIB_H_LOADED 1

#include <in.h>
#include <socket.h>

/* mainly to allow easy use of the __unaligned directive */
#define ULONGPTR __unaligned unsigned long*
#define USHORTPTR __unaligned unsigned short*
#define INT64PTR __unaligned __int64*

/* constants that can be passed to WsLibClose() as the status code */
#define WSLIB_CLOSE_NORMAL    1000
#define WSLIB_CLOSE_BYEBYE    1001
#define WSLIB_CLOSE_PROTOCOL  1002
#define WSLIB_CLOSE_UNACCEPT  1003
#define WSLIB_CLOSE_RESERVE1  1004
#define WSLIB_CLOSE_RESERVE2  1005
#define WSLIB_CLOSE_RESERVE3  1006
#define WSLIB_CLOSE_DATA      1007
#define WSLIB_CLOSE_POLICY    1008
#define WSLIB_CLOSE_TOOBIG    1009
#define WSLIB_CLOSE_EXTENSION 1010
#define WSLIB_CLOSE_UNEXPECT  1011
#define WSLIB_CLOSE_BANG        -1

#define WSLIB_ASYNCH ((void*)-1)

#define WSLIB_BIT_FIN  0x80
#define WSLIB_BIT_RSV1 0x40
#define WSLIB_BIT_RSV2 0x20
#define WSLIB_BIT_RSV3 0x10

#define WSLIB_OPCODE_CONTIN 0x0
#define WSLIB_OPCODE_TEXT   0x1
#define WSLIB_OPCODE_BINARY 0x2
#define WSLIB_OPCODE_CLOSE  0x8
#define WSLIB_OPCODE_PING   0x9
#define WSLIB_OPCODE_PONG   0xA

#define WSLIB_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WSLIB_GUID_LEN 36

#define WSLIB_WEBSOCKET_VERSION "13, 8"

/********************/
/* TCP/IP CONSTANTS */
/********************/

/* from TCPIP$INETDEF.H */
#define INET_PROTYP$C_STREAM 1
#define INETACP$C_TRANS 2
#define INETACP_FUNC$C_GETHOSTBYNAME 1
#define INETACP_FUNC$C_GETHOSTBYADDR 2
#define TCPIP$C_TCP 6
#define TCPIP$C_AF_INET 2
#define TCPIP$C_DSC_ALL 2
#define TCPIP$C_FULL_DUPLEX_CLOSE 8192
#define TCPIP$C_MSG_PEEK 2
#define TCPIP$C_REUSEADDR 4
#define TCPIP$C_SOCK_NAME 4
#define TCPIP$C_SOCKOPT 1
#define TCPIP$C_TCPOPT 6
#define TCPIP$C_TCP_NODELAY 1
#define TCPIP$C_TCP_NODELACK 9

/*******************/
/* data structures */
/*******************/

struct WsLibItemList2
{
   unsigned short  buf_len;
   unsigned short  item;
   void  *buf_addr;
};

static struct WsLibIOsb {
   unsigned short  iosb$w_status;
   unsigned short  iosb$w_bcnt;
   unsigned int    iosb$l_reserved;
};

/* frame data structure */

struct WsLibFrmStruct
{
   char  *DataPtr,
         *MaskedPtr,
         *MrsDataPtr;

   int  DataCount,
        DataSize,
        FrameCount,
        FrameFinBit,
        FrameMaskBit,
        FrameOpcode,
        FramePayload,
        FrameRsv,
        IoRead,
        MaskCount,
        MrsDataCount,
        MrsWriteCount,
        ReadSize,
        WriteCount;

   /* sufficient extra space to accomodate <=125 byte transmitted data */
   unsigned char  FrameHeader [2+4+125],
                  MaskingKey [4];

   struct WsLibIOsb  IOsb;
   struct WsLibMsgStruct  *WsLibMsgPtr;
};

/* message data structure */

struct WsLibMsgStruct
{
   char  *DataPtr,
         *Utf8Ptr;

   int  DataCount,
        DataMax,
        DataSize,
        MsgOpcode,
        MsgStatus,
        Utf8Count,
        WriteCount;

   unsigned int  Utf8State;

   /* small string describing any specifics of the close */
   char  CloseMsg [32];

   struct WsLibFrmStruct  FrameData;

   void  (*AstFunction)();
   struct WsLibStruct  *WsLibPtr;
};

/* WebSocket data structure */

struct WsLibStruct
{
   unsigned long  CalloutInProgress,
                  ClientAcceptSize,
                  ClientHeaderSize,
                  ClientKeySize,
                  ClientServerPort,
                  ClientServerSize,
                  ClientUriSize,
                  FrameMaxSize,
                  InBufferCount,
                  InBufferSize,
                  InputDataCount,
                  InputDataMax,
                  InputDataSize,
                  InputFinBit,
                  InputMrs,
                  InputOpcode,
                  InputStatus,
                  MsgLineNumber,
                  MsgStringLength,
                  MsgStringSize,
                  Opcode,
                  OutBufferSize,
                  OutputDataCount,
                  OutputMrs,
                  OutputStatus,
                  QueuedInput,
                  QueuedOutput,
                  SetBinary,
                  SetAscii,
                  SetUtf8,
                  WatchScript,
                  WatchDogCloseTime,
                  WatchDogCloseSecs,
                  WatchDogIdleSecs,
                  WatchDogIdleTime,
                  WatchDogPingCount,
                  WatchDogPingSecs,
                  WatchDogPingTime,
                  WatchDogReadSecs,
                  WatchDogReadTime,
                  WatchDogWakeSecs,
                  WatchDogWakeTime,
                  WebSocketClosed,
                  WebSocketShut,
                  WebSocketVersion,
                  RoleClient;

   unsigned long  InputCount [2],
                  InputMsgCount [2],
                  MsgBinTime [2],
                  OutputCount [2],
                  OutputMsgCount [2];

   unsigned short  InputChannel,
                   OutputChannel,
                   SocketChannel;

   char  InputDevName [64],
         OutputDevName [64];

   char  *ClientAcceptPtr,
         *ClientHeaderPtr,
         *ClientKeyPtr,
         *ClientServerPtr,
         *ClientUriPtr,
         *InBufferPtr,
         *InputDataPtr,
         *InFramePtr,
         *MsgStringPtr,
         *MsgDataPtr,
         *OutBufferPtr,
         *OutputDataPtr,
         *ServerAcceptPtr,
         *ServerConnectionPtr,
         *ServerSoftwarePtr,
         *ServerUpgradePtr;

   FILE  *WatchLog;

   void  (*CalloutAstFunction)(),
         (*ConnectAstFunction)(),
         (*DestroyAstFunction)(),
         (*MsgCallbackFunction)(),
         (*PongCallbackFunction)(),
         (*WakeCallbackFunction)();

   struct dsc$descriptor_s  CalloutDataDsc,
                            MsgDsc,
                            InputDataDsc,
                            InputDevDsc,
                            OutputDataDsc,
                            OutputDevDsc;

   struct dsc$descriptor_s  *ReadDscPtr;

   struct sockaddr_in  SocketName;
   int  SocketNameItem [2];

   struct WsLibIOsb  InputIOsb,
                     OutputIOsb,
                     SocketIOsb;

   void  *UserDataPtr;

   struct WsLibStruct  *NextPtr;
};

/***********************/
/* function prototypes */
/***********************/

struct WsLibStruct* WsLibCreate (void*, void*);
void* WsLibDestroy (struct WsLibStruct*);
struct WsLibStruct* WsLibNext (struct WsLibStruct**);

void WsLibSetUserData (struct WsLibStruct*, void*);
void* WsLibGetUserData (struct WsLibStruct*);

int WsLibSetBinary (struct WsLibStruct*);
int WsLibIsSetBinary (struct WsLibStruct*);
int WsLibSetFrameMax (struct WsLibStruct*, int);
int WsLibSetAscii (struct WsLibStruct*);
int WsLibIsSetAscii (struct WsLibStruct*);
int WsLibSetUtf8 (struct WsLibStruct*);
int WsLibIsSetUtf8 (struct WsLibStruct*);
int WsLibSetRoleClient (struct WsLibStruct*);
int WsLibSetRoleServer (struct WsLibStruct*);
int WsLibIsRoleClient (struct WsLibStruct*);

int WsLibUnFrame (struct WsLibStruct*);
int WsLibFromUtf8 (char*, int, char);
int WsLibFromUtf8Dsc (struct dsc$descriptor_s*, struct dsc$descriptor_s*, char);
int WsLibToUtf8 (char*, int, char*, int);
int WsLibToUtf8Dsc (struct dsc$descriptor_s*, struct dsc$descriptor_s*);

int WsLibOpen (struct WsLibStruct*);
int WsLibConnected (struct WsLibStruct*);
int WsLibPing (struct WsLibStruct*, char*, int);
int WsLibPingDsc (struct WsLibStruct*, struct dsc$descriptor_s*);
int WsLibPong (struct WsLibStruct*, char*, int);
void WsLibClose (struct WsLibStruct*, int, char*);
int WsLibIsClosed (struct WsLibStruct*);
static void WsLib__Close (struct WsLibFrmStruct*);
static void WsLib__CloseFreeAst (struct WsLibFrmStruct*);
static void WsLib__Destroy (struct WsLibStruct*);
static void WsLib__DummyClose (struct WsLibStruct*);
static int WsLib__PingPong (struct WsLibStruct*, char*, int, int);
int WsLib__Shut (struct WsLibStruct*);

int WsLibRead (struct WsLibStruct*, char*, int, void*);
int WsLibReadDsc (struct WsLibStruct*, struct dsc$descriptor_s*,
                  struct dsc$descriptor_s*, void*);
char* WsLibReadData (struct WsLibStruct*);
int WsLibReadCount (struct WsLibStruct*);
struct dsc$descriptor_s* WsLibReadDataDsc (struct WsLibStruct*);
int WsLibReadFree (struct WsLibStruct*);
char* WsLibReadGrab (struct WsLibStruct*);
int WsLibReadIsBinary (struct WsLibStruct*);
int WsLibReadIsText (struct WsLibStruct*);
int WsLibReadStatus (struct WsLibStruct*);
unsigned long* WsLibReadTotal (struct WsLibStruct*);
unsigned long* WsLibReadMsgTotal (struct WsLibStruct*);

int WsLibWrite (struct WsLibStruct*, char*, int, void*);
int WsLibWriteDsc (struct WsLibStruct*, struct dsc$descriptor_s*, void*);
void WsLibWriteClose (struct WsLibStruct*, void*);
unsigned long* WsLibWriteTotal (struct WsLibStruct*);
unsigned long* WsLibWriteMsgTotal (struct WsLibStruct*);
int WsLibWriteStatus (struct WsLibStruct*);

void* WsLibSetCallout (struct WsLibStruct*, void*);
void* WsLibSetMsgCallback (struct WsLibStruct*, void*);
void* WsLibSetPingCallback (struct WsLibStruct*, void*);
void* WsLibSetPongCallback (struct WsLibStruct*, void*);
void* WsLibSetWakeCallback (struct WsLibStruct*, void*, int);
void WsLibSetCloseSecs (struct WsLibStruct*, int);
void WsLibSetIdleSecs (struct WsLibStruct*, int);
void WsLibSetLifeSecs (int);
void WsLibSetPingSecs (struct WsLibStruct*, int);
void WsLibSetReadSecs (struct WsLibStruct*, int);

char* WsLibCgiVar (char*);
char* WsLibCgiVarNull (char*);
void WsLibCgiPlusEof ();
void WsLibCgiPlusEot ();
void WsLibCgiPlusEsc ();
int WsLibIsCgiPlus ();
void WsLibCallout (struct WsLibStruct*, char*, ...);
void WsLibWatchScript (struct WsLibStruct*, char*, int, char*, ...);
char* WsLibVersion ();

void WsLibInit();
unsigned int WsLibTime();
void WsLibExit (struct WsLibStruct*, char*, int, int);
void WsLibFree (char*);
struct dsc$descriptor_s* WsLibMsgDsc (struct WsLibStruct*);
char* WsLibMsgString (struct WsLibStruct*);
int WsLibMsgLineNumber (struct WsLibStruct*);
void WsLibResetMsg (struct WsLibStruct *wsptr);

static void WsLib__MaskingKey (struct WsLibFrmStruct*);
static void WsLib__MsgCallback (struct WsLibStruct*, int, int, char*, ...);
static void WsLib__OutputAst (struct WsLibStruct*);
static void WsLib__OutputFreeAst (char*);
static void WsLib__Pong (struct WsLibFrmStruct*);
static void WsLib__ReadFrame (struct WsLibMsgStruct*);
static void WsLib__ReadHeader1Ast (struct WsLibFrmStruct*);
static void WsLib__ReadHeader2Ast (struct WsLibFrmStruct*);
static void WsLib__ReadDataAst (struct WsLibFrmStruct*);
static int WsLib__Utf8Legal (struct WsLibFrmStruct*);
static void WsLib__WatchDog ();
static void WsLib__WriteAst (struct WsLibFrmStruct*);
static void WsLib__WriteEofAst (struct WsLibStruct*);
static void WsLib__WriteMrsAst (struct WsLibFrmStruct*);
static char* WsLib__OpCodeName (int);

/* for the WSLIBCL.C module */
int WsLibClBreakNow (struct WsLibStruct*);
int WsLibClConnect (struct WsLibStruct*, char*, int, char*, void*);
int WsLibClSocketStatus (struct WsLibStruct*);
int WsLibClSetSocketMrs (struct WsLibStruct*, int);
static void WsLibCl__ConnAst (struct WsLibStruct*);
static void WsLibCl__ConnRequAst (struct WsLibStruct*);
static void WsLibCl__ConnResp1Ast (struct WsLibStruct*);
static void WsLibCl__ConnResp2Ast (struct WsLibStruct*);

#endif /* WSLIB_H_LOADED */

/*****************************************************************************/

