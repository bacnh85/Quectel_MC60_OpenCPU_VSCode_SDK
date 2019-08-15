#ifndef __ADAPTER_H__
#define __ADAPTER_H__

#include "gagent_typedef.h"
#include "ql_stdlib.h"
#include "ql_uart.h"
#include "ql_trace.h"
#include "ql_type.h"
#include "ql_system.h"
#include "ql_fs.h"
#include "ql_memory.h"
#include "ql_error.h"
#include "ril_system.h"
#include "ril_network.h"
#include "ql_socket.h"
#include "ql_timer.h"
#include "ql_gprs.h"
#include "ril.h"
#include "mqttlib_ext.h"









#define DEBUG_ENABLE 1
#if DEBUG_ENABLE > 0
#define DEBUG_PORT  UART_PORT2
#define DBG_BUF_LEN   512
static char DBG_BUFFER[DBG_BUF_LEN];
#define APP_DEBUG(FORMAT,...) {\
    Ql_memset(DBG_BUFFER, 0, DBG_BUF_LEN);\
    Ql_sprintf(DBG_BUFFER,FORMAT,##__VA_ARGS__); \
    if (UART_PORT2 == (DEBUG_PORT)) \
    {\
        Ql_Debug_Trace(DBG_BUFFER);\
    } else {\
        Ql_UART_Write((Enum_SerialPort)(DEBUG_PORT), (u8*)(DBG_BUFFER), Ql_strlen((const char *)(DBG_BUFFER)));\
    }\
}
#else
#define APP_DEBUG(FORMAT,...) 
#endif


typedef enum
{
    ADAPTER_ERROR_CREATE_DIR = -100,
    ADAPTER_ERROR_OPEN_FILE = -101,
    ADAPTER_ERROR_READ_FILE = -102,
    ADAPTER_ERROR_INVALID_PARAM = -103,

}Enum_Adapter_ErrorCode;

/****************************************************************
Function    :   Adapter_Module_Init
Description :   module init,waiting for ril layer init and network ok
pgc         :   global staruc pointer.
return      :   void
****************************************************************/
void Adapter_Module_Init( pgcontext pgc );



/****************************************************************
Function    :   Adapter_Memcpy
Description :   memcpy func
dest        :   pointer to dest
src         :   pointer to src
size        :   copy size
return      :   the pointer to dest
****************************************************************/
void * Adapter_Memcpy(void* dest, const void* src, u32 size);

/****************************************************************
Function    :   Adapter_Mem_Alloc
Description :   malloc func 
size         :  malloc size
return      :   the pointer to malloc area
****************************************************************/
void * Adapter_Mem_Alloc(u32 size);

/*****************************************************************
* Function:     Adapter_Mem_Free 
* 
* Description:
*               Free memory 
* Parameters:
*               Ptr: 
*                  [in]Previously allocated memory block to be freed. 
* Return:        
*               none
*               
*****************************************************************/
void Adapter_Mem_Free(void *ptr);

/****************************************************************
Function    :   Adapter_Sleep
Description :   sleep func
msec        :  time to sleep
return      :   void
****************************************************************/
void  Adapter_Sleep(u32 msec);

/****************************************************************
Function    :   Adapter_Memset
Description :   memset func
dest        :   the pointer to memset
value       :   the value to init
size        :   the size of memset area
return      :   the pointer to memset area
****************************************************************/
void *Adapter_Memset(void* dest, u8 value, u32 size);

/****************************************************************
Function    :   Adapter_StrStr
Description :   match string func
s1          :   the pointer to s1
s2          :   the pointer to s2
return      :   the pointer to match area
****************************************************************/
char * Adapter_StrStr(const char* s1,const char* s2);

/****************************************************************
Function    :   Adapter_Atoi
Description :   convert ascii to interger
s           :   the pointer to s
return      :   the converted result
****************************************************************/
s32 Adapter_Atoi(const char *s);

/****************************************************************
Function    :   Adapter_Strlen
Description :   strlen func
str         :   the pointer to string
return      :   the length of string
****************************************************************/
u32 Adapter_Strlen(const char* str);

/****************************************************************
Function    :   Adapter_Strcmp
Description :   strcmp func
s1          :   the pointer to s1
s2          :   the pointer to s2
return      :   the cmp result
****************************************************************/
s32 Adapter_Strcmp(const char*s1, const char*s2);

/****************************************************************
Function    :   Adapter_Strncmp
Description :   strncmp func
s1          :   the pointer to s1
s2          :   the pointer to s2
size        :   the size to compare
return      :   the cmp result
****************************************************************/
s32 Adapter_Strncmp(const char*s1, const char*s2,u32 size);



/****************************************************************
Function    :   Adapter_Strncpy
Description :   strcpy func
dest        :   the pointer to dest
src         :   the pointer to src
size        :   copy size
return      :   the result
****************************************************************/
char * Adapter_Strncpy(char* dest, const char* src,u32 size);



/****************************************************************
Function    :   Adapter_DevGetIMEI
Description :   get module IMEI
imei        :   pointer to store the imei
return      :   result code
****************************************************************/
s32  Adapter_DevGetIMEI( u8* imei );


/****************************************************************
Function    :   Adapter_DevGetConfigData
Description :   get config data from flash
pConfig     :   struc pointer to store the config data
return      :   result code
****************************************************************/
u32 Adapter_DevGetConfigData( gconfig *pConfig );

/****************************************************************
Function    :   Adapter_DevSaveConfigData
Description :   save changed config data to flash
pConfig     :   struc pointer to save data
return      :   result code
****************************************************************/
u32 Adapter_DevSaveConfigData( gconfig *pConfig );

/****************************************************************
Function    :   Adapter_LocalDataIOInit
Description :   init local uart for communication with MCU
pgc         :   global staruc pointer.
return      :   void
****************************************************************/
void Adapter_LocalDataIOInit( pgcontext pgc );

/****************************************************************
Function    :   Adapter_TimerInit
Description :   init a timer
timid       :   the timer id to init
pgc         :   global staruc pointer.
return      :   void
****************************************************************/
void Adapter_TimerInit(u32 timerid,pgcontext pgc);


/****************************************************************
Function    :   Adapter_TimerStart
Description :   start a timer
timid       :   the timer id to init
interval    :   timer interval
autoRepeat  :   repeat or not
return      :   result code
****************************************************************/
void Adapter_TimerStart(u32 timerid,u32 interval,bool autoRepeat);


/****************************************************************
Function    :   Adapter_TimerStop
Description :   stop a timer
timid       :   the timer id to init
return      :   result code
****************************************************************/
void Adapter_TimerStop(u32 timerid);



/****************************************************************
Function    :   Adapter_CreatEvent
Description :   creat a event
evtName     :   the event name
return      :   the event id
****************************************************************/
u32 Adapter_CreatEvent(char* evtName);



/****************************************************************
Function    :   Adapter_SetEvent
Description :   release a event
evtId       :   the event id
evtFlag     :   the event flag
return      :   result code
****************************************************************/
void Adapter_SetEvent(u32 evtId, u32 evtFlag);

/****************************************************************
Function    :   Adapter_WaitEvent
Description :   wait a event
evtId       :   the event id
evtFlag     :   the event flag
return      :   result code
****************************************************************/
void Adapter_WaitEvent(u32 evtId, u32 evtFlag);

/****************************************************************
Function    :   Adapter_GetMsg
Description :   get a msg from msg queue
msg         :   pointer to store the msg
return      :   always return ok
****************************************************************/
s32 Adapter_GetMsg(ST_MSG *msg);


/****************************************************************
Function    :   Adapter_SendMsg
Description :   send a msg to queue
destTaskId  :   dest taskid      
msgId       :   msgid
param1      :   custom param  
param2      :   custom param
return      :   void
****************************************************************/
void Adapter_SendMsg(s32 destTaskId, u32 msgId, u32 param1, u32 param2);


/****************************************************************
FunctionName    :   GAgent_DevReset
Description     :   reset device                   
return          :   void
****************************************************************/
void Adapter_DevReset(void);


/****************************************************************
FunctionName    :   Adapter_Get_GPRS_Status
Description     :   get current GPRS status  
state           :   pointer to store the state
return          :   the return code
****************************************************************/
s32 Adapter_Get_GPRS_Status(s32 *state);

/****************************************************************
FunctionName    :   Adapter_Get_Signal_Quality
Description     :   get current signal quality 
rssi            :   Signal quality level, 0~31 or 99. 99 indicates module
*                   doesn't register to GSM network.
ber             :   The bit error code of signal.
return          :   the return code
****************************************************************/
s32 Adapter_Get_Signal_Quality(u32* rssi, u32* ber);

/****************************************************************
FunctionName    :   Adapter_Login_Gservice_Init
Description     :   init login to gservice
return          :   void
****************************************************************/
void Adapter_Login_Gservice_Init(void);

/****************************************************************
FunctionName    :   Adapter_Close_PDPContext
Description     :   close dedicated pdp context
pdp_type        :   which pdp context to deactive
return          :   void
****************************************************************/
void Adapter_Close_PDPContext(u8 pdp_type);

/****************************************************************
FunctionName    :   Adapter_HTTP_Post
Description     :   post msg to http server
strPostMsg      :   the pointer to msg
len             :   msg len
return          :   result code
****************************************************************/
s32 Adapter_HTTP_Post(char* strPostMsg, u16 len);

/****************************************************************
FunctionName    :   Adapter_HTTP_Get
Description     :   get item from http server
timeout         :   timeout
return          :   result code
****************************************************************/
s32 Adapter_HTTP_Get(u8* buf,u32 timeout);



/****************************************************************
FunctionName    :   Adapter_GetIPByHostName
Description     :   convert hostname to ip addr
hostname        :   hostname
ip              :   the converted ip
return          :   result code
****************************************************************/
s32 Adapter_GetIPByHostName(u8* hostname,u8 *ip);

/****************************************************************
FunctionName    :   Adapter_Login_MQTT_Init
Description     :   init login to mqtt
return          :   void
****************************************************************/
void Adapter_Login_MQTT_Init(void);

/****************************************************************
FunctionName    :   Adapter_SOC_Send
Description     :   soc send func
socketId        :   the id of socket
pData           :   pointer to the data area
dataLen         :   the length of data to send
return          :   the real send count or error
****************************************************************/
s32 Adapter_SOC_Send(void *arg, const struct iovec *iov, int iovcnt);



/****************************************************************
FunctionName    :   Adapter_SOC_Recv
Description     :   soc recv func
socketId        :   the id of socket
pData           :   pointer to the data area
dataLen         :   the length of data to send
return          :   the real recv count or error
****************************************************************/
s32 Adapter_SOC_Recv(void *arg, u8* pData, s32 dataLen);

/****************************************************************
FunctionName    :   Adapter_SOC_Close
Description     :   soc close func
socketId        :   the id of socket
return          :   the result code
****************************************************************/
s32 Adapter_SOC_Close(s32 socketId);

/****************************************************************
FunctionName    :   Adapter_UART_Read
Description     :   read data from uart
fd              :   the uart no
data            :   recieve buffer
readlen         :   expected length to read
return          :   the actual read count
****************************************************************/
s32 Adapter_UART_Read(s32 fd,u8 *data,u32 readlen);

/****************************************************************
FunctionName    :   Adapter_UART_Send
Description     :   send data to uart
fd              :   the uart no
data            :   send buffer
writelen        :   expected length to send
return          :   the actual send count
****************************************************************/
s32 Adapter_UART_Send(s32 fd,u8 *data,u32 writelen);




#endif
