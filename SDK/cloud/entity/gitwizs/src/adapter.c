#include "custom_feature_def.h"
#ifdef __OCPU_SMART_CLOUD_SUPPORT__
#ifdef __GITWIZS_SOLUTION__

/**

Adapter Layer for different platforms

**/

#include "adapter.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "gagent.h"
#include "mqttxpg.h"





#define  PATH_ROOT    		((u8 *)"myroot")  //in UFS
#define  GAGENT_CONFIG_FILE "gagent_config.txt"


extern bool get_ril_init_status(void);


/****************************************************************
Function    :   Adapter_Module_Init
Description :   module init,waiting for ril layer init and network ok
pgc         :   global staruc pointer.
return      :   void
****************************************************************/
void Adapter_Module_Init( pgcontext pgc )
{
    while(!get_ril_init_status())
    {
        APP_DEBUG("the ril status is not init ,should wait..\r\n");
        Adapter_Sleep(1000);
    }
    
    APP_DEBUG("the ril status is ok ,now check the network..\r\n");
    
    while(TRUE)
    {
       Adapter_Get_GPRS_Status(&(pgc->rtinfo.gprs_status));
       APP_DEBUG("gprs status = %d\r\n",pgc->rtinfo.gprs_status);
       if(pgc->rtinfo.gprs_status == NW_STAT_REGISTERED || pgc->rtinfo.gprs_status == NW_STAT_REGISTERED_ROAMING)
       {
          break;
       }
       Adapter_Sleep(1000);
    }

    Adapter_Get_Signal_Quality(&(pgc->rtinfo.devSignalStatus.devSignalrssi),&(pgc->rtinfo.devSignalStatus.devSignalber));

    APP_DEBUG("gprs status = %d,rssi = %d ,ber = %d\r\n",pgc->rtinfo.gprs_status,pgc->rtinfo.devSignalStatus.devSignalrssi,pgc->rtinfo.devSignalStatus.devSignalber);
    
    APP_DEBUG("network is ok now\r\n");
}




/****************************************************************
Function    :   Adapter_Memcpy
Description :   memcpy func
dest        :   pointer to dest
src         :   pointer to src
size        :   copy size
return      :   the pointer to dest
****************************************************************/
void * Adapter_Memcpy(void* dest, const void* src, u32 size)
{
    return Ql_memcpy(dest,src,size);
}



/****************************************************************
Function    :   Adapter_Mem_Alloc
Description :   malloc func 
size         :  malloc size
return      :   the pointer to malloc area
****************************************************************/
void * Adapter_Mem_Alloc(u32 size)
{
    return Ql_MEM_Alloc(size);
}

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
void Adapter_Mem_Free(void *ptr)
{
    Ql_MEM_Free(ptr);
}



/****************************************************************
Function    :   Adapter_Sleep
Description :   sleep func
msec        :  time to sleep
return      :   void
****************************************************************/
void Adapter_Sleep(u32 msec)
{
    Ql_Sleep(msec);
}


/****************************************************************
Function    :   Adapter_Memset
Description :   memset func
dest        :   the pointer to memset
value       :   the value to init
size        :   the size of memset area
return      :   the pointer to memset area
****************************************************************/
void * Adapter_Memset(void* dest, u8 value, u32 size)
{
    return Ql_memset(dest,value,size);
}

/****************************************************************
Function    :   Adapter_StrStr
Description :   match string func
s1          :   the pointer to s1
s2          :   the pointer to s2
return      :   the pointer to match area
****************************************************************/
char * Adapter_StrStr(const char* s1,const char* s2)
{
    return Ql_strstr(s1,s2);
}

/****************************************************************
Function    :   Adapter_Atoi
Description :   convert ascii to interger
s           :   the pointer to s
return      :   the converted result
****************************************************************/
s32 Adapter_Atoi(const char *s)
{
    return Ql_atoi(s);
}



/****************************************************************
Function    :   Adapter_Strlen
Description :   strlen func
str         :   the pointer to string
return      :   the length of string
****************************************************************/
u32 Adapter_Strlen(const char* str)
{
    return Ql_strlen(str);
}

/****************************************************************
Function    :   Adapter_Strcmp
Description :   strcmp func
s1          :   the pointer to s1
s2          :   the pointer to s2
return      :   the cmp result
****************************************************************/
s32 Adapter_Strcmp(const char*s1, const char*s2)
{
    return Ql_strcmp(s1,s2);
}

/****************************************************************
Function    :   Adapter_Strncmp
Description :   strncmp func
s1          :   the pointer to s1
s2          :   the pointer to s2
size        :   the size to compare
return      :   the cmp result
****************************************************************/
s32 Adapter_Strncmp(const char*s1, const char*s2,u32 size)
{
    return Ql_strncmp(s1,s2,size);
}



/****************************************************************
Function    :   Adapter_Strncpy
Description :   strcpy func
dest        :   the pointer to dest
src         :   the pointer to src
size        :   copy size
return      :   the result
****************************************************************/
char * Adapter_Strncpy(char* dest, const char* src,u32 size)
{
    return Ql_strncpy(dest,src,size);
 }




/****************************************************************
Function    :   Adapter_DevGetConfigData
Description :   get config data from flash
pConfig     :   struc pointer to store the config data
return      :   result code
****************************************************************/
u32 Adapter_DevGetConfigData( gconfig *pConfig )
{
    s32 handle = -1;
    s32 ret;
    u8 filePath[100] = {0};

    ret = Ql_FS_CheckDir(PATH_ROOT);
 
    if(ret != QL_RET_OK)
    {
		ret  = Ql_FS_CreateDir(PATH_ROOT);
		if(ret != QL_RET_OK)
		{
			APP_DEBUG("\r\n<-- failed!! Create Dir(%s) fail-->", PATH_ROOT);
			return ADAPTER_ERROR_CREATE_DIR;
		}      
    }

    Ql_sprintf(filePath,"%s\\%s\0",PATH_ROOT,GAGENT_CONFIG_FILE);

    handle = Ql_FS_Open(filePath,QL_FS_READ_WRITE |QL_FS_CREATE );

    if(handle < QL_RET_OK )
    {
        APP_DEBUG("\r\n<--!! Ql_FS_Open failed handle =%d -->\r\n",handle);
        return ADAPTER_ERROR_OPEN_FILE;
    }
    ret = Ql_FS_Read(handle, pConfig, sizeof(gconfig),NULL);

    if(ret != QL_RET_OK)
	{
        APP_DEBUG("\r\n<-- Ql_FS_Read() failed ret=%d\r\n", ret);
		return ADAPTER_ERROR_READ_FILE;
	}  
    //close file
    Ql_FS_Close(handle); 


    return 0;
}

/****************************************************************
Function    :   Adapter_DevSaveConfigData
Description :   save changed config data to flash
pConfig     :   struc pointer to save data
return      :   result code
****************************************************************/
u32 Adapter_DevSaveConfigData( gconfig *pConfig )
{
    s32 handle = -1;
    s32 ret;
    u8 filePath[100] = {0};
    
    Ql_sprintf(filePath,"%s\\%s\0",PATH_ROOT,GAGENT_CONFIG_FILE);

    handle = Ql_FS_Open(filePath,QL_FS_READ_WRITE );
    
    APP_DEBUG("\r\n<--!! Ql_FS_Open  handle =%d -->\r\n",handle);

    ret = Ql_FS_Write(handle, pConfig, sizeof(gconfig),NULL);
    
    APP_DEBUG("\r\n<-- Ql_FS_Write() ret=%d\r\n", ret);
    //close file
    Ql_FS_Close(handle); 
    return 0;
}


/****************************************************************
Function    :   Adapter_DevGetIMEI
Description :   get module IMEI
imei        :   pointer to store the imei
return      :   result code
****************************************************************/
s32  Adapter_DevGetIMEI( u8* imei )
{
    return RIL_GetIMEI(imei);     
}


/****************************************************************
Function    :   Adapter_LocalDataIOInit
Description :   init local uart for communication with MCU
pgc         :   global staruc pointer.
return      :   void
****************************************************************/
void Adapter_LocalDataIOInit( pgcontext pgc )
{
    Hal_Uart_Init(pgc);
    Hal_UartBuffer_Init(pgc);
    Hal_UartReg_RcvHook(Hal_Rcv_Data_Handle);
    Hal_UartReg_SendHook(Hal_Send_Data_Handle);
}


/****************************************************************
Function    :   Adapter_TimerInit
Description :   init a timer
timid       :   the timer id to init
pgc         :   global staruc pointer.
return      :   void
****************************************************************/
void Adapter_TimerInit(u32 timerid,pgcontext pgc)
{
    HAL_Timer_Init(timerid,pgc);
}


/****************************************************************
Function    :   Adapter_TimerStart
Description :   start a timer
timid       :   the timer id to init
interval    :   timer interval
autoRepeat  :   repeat or not
return      :   result code
****************************************************************/
void Adapter_TimerStart(u32 timerid,u32 interval,bool autoRepeat)
{
    HAL_Timer_Start(timerid,interval,autoRepeat);
}


/****************************************************************
Function    :   Adapter_TimerStop
Description :   stop a timer
timid       :   the timer id to init
return      :   result code
****************************************************************/
void Adapter_TimerStop(u32 timerid)
{
    HAL_Timer_Stop(timerid);
}


/****************************************************************
Function    :   Adapter_CreatEvent
Description :   creat a event
evtName     :   the event name
return      :   the event id
****************************************************************/
u32 Adapter_CreatEvent(char* evtName)
{
    return Ql_OS_CreateEvent(evtName);
}


/****************************************************************
Function    :   Adapter_SetEvent
Description :   release a event
evtId       :   the event id
evtFlag     :   the event flag
return      :   result code
****************************************************************/
void Adapter_SetEvent(u32 evtId, u32 evtFlag)
{
    Ql_OS_SetEvent(evtId, evtFlag);
}


/****************************************************************
Function    :   Adapter_WaitEvent
Description :   wait a event
evtId       :   the event id
evtFlag     :   the event flag
return      :   result code
****************************************************************/
void Adapter_WaitEvent(u32 evtId, u32 evtFlag)
{
    Ql_OS_WaitEvent(evtId, evtFlag);
}


/****************************************************************
Function    :   Adapter_GetMsg
Description :   get a msg from msg queue
msg         :   pointer to store the msg
return      :   always return ok
****************************************************************/
s32 Adapter_GetMsg(ST_MSG *msg)
{
    return Ql_OS_GetMessage(msg);
}


/****************************************************************
Function    :   Adapter_SendMsg
Description :   send a msg to queue
destTaskId  :   dest taskid      
msgId       :   msgid
param1      :   custom param  
param2      :   custom param
return      :   void
****************************************************************/
void Adapter_SendMsg(s32 destTaskId, u32 msgId, u32 param1, u32 param2)
{
    Ql_OS_SendMessage(destTaskId,msgId,param1,param2);
}


/****************************************************************
FunctionName    :   GAgent_DevReset
Description     :   reset device                   
return          :   void
****************************************************************/
void Adapter_DevReset(void)
{
    Ql_Reset(0);
}


/****************************************************************
FunctionName    :   Adapter_Get_GPRS_Status
Description     :   get current GPRS status   
state           :   pointer to store the state
return          :   the return code
****************************************************************/
s32 Adapter_Get_GPRS_Status(s32 *state)
{
    if(NULL == state)
    {
        return ADAPTER_ERROR_INVALID_PARAM;
    }
    return RIL_NW_GetGPRSState(state);
}


/****************************************************************
FunctionName    :   Adapter_Get_Signal_Quality
Description     :   get current signal quality 
rssi            :   Signal quality level, 0~31 or 99. 99 indicates module
*                   doesn't register to GSM network.
ber             :   The bit error code of signal.
return          :   the return code
****************************************************************/
s32 Adapter_Get_Signal_Quality(u32* rssi, u32* ber)
{
    if(rssi == NULL || ber == NULL)
    {
        return ADAPTER_ERROR_INVALID_PARAM;
    }
    return RIL_NW_GetSignalQuality(rssi,ber);
}


/****************************************************************
FunctionName    :   Adapter_Login_Gservice_Init
Description     :   init login to gservice
return          :   void
****************************************************************/
void Adapter_Login_Gservice_Init(void)
{
    Hal_Login_Gservice_HTTP_Init();
}


/****************************************************************
FunctionName    :   Adapter_Close_PDPContext
Description     :   close dedicated pdp context
pdp_type        :   which pdp context to deactive
return          :   void
****************************************************************/
void Adapter_Close_PDPContext(u8 pdp_type)
{
    Hal_Close_PDPContext(pdp_type);
}


/****************************************************************
FunctionName    :   Adapter_HTTP_Post
Description     :   post msg to http server
strPostMsg      :   the pointer to msg
len             :   msg len
return          :   result code
****************************************************************/
s32 Adapter_HTTP_Post(char* strPostMsg, u16 len)
{
    return Hal_HTTP_RequestTo_Post(strPostMsg,len);
}

/****************************************************************
FunctionName    :   Adapter_HTTP_Get
Description     :   get item from http server
timeout         :   timeout
return          :   result code
****************************************************************/
s32 Adapter_HTTP_Get(u8* buf,u32 timeout)
{
    return Hal_HTTP_RequestTo_Get(buf,timeout);
}

/****************************************************************
FunctionName    :   Adapter_GetIPByHostName
Description     :   convert hostname to ip addr
hostname        :   hostname
ip              :   the converted ip
return          :   result code
****************************************************************/
s32 Adapter_GetIPByHostName(u8* hostname,u8 *ip)
{
    return HAL_SOC_GetIPByHostName(hostname,ip);
}


/****************************************************************
FunctionName    :   Adapter_Login_MQTT_Init
Description     :   init login to mqtt
return          :   void
****************************************************************/
void Adapter_Login_MQTT_Init(void)
{
    Hal_Login_MQTT_TCP_Init();
}




/****************************************************************
FunctionName    :   Adapter_SOC_Send
Description     :   soc send func
socketId        :   the id of socket
pData           :   pointer to the data area
dataLen         :   the length of data to send
return          :   the real send count or error
****************************************************************/
s32 Adapter_SOC_Send(void *arg, const struct iovec *iov, int iovcnt)
{
    s32 bytes = 0;
    s32 i = 0;
    s32 len = 0;
    for(i = 0; i < iovcnt ;i++)
    {
      
      Ql_memcpy(PGC->rtinfo.Cloud_Txbuf->phead+len,iov[i].iov_base,iov[i].iov_len);
      len += iov[i].iov_len; 
    }



    bytes = Ql_SOC_Send(gw_ctx->mqttfd,PGC->rtinfo.Cloud_Txbuf->phead,len);
    
    resetPacket( PGC->rtinfo.Cloud_Txbuf );
    return bytes;
}


/****************************************************************
FunctionName    :   Adapter_SOC_Recv
Description     :   soc recv func
socketId        :   the id of socket
pData           :   pointer to the data area
dataLen         :   the length of data to send
return          :   the real recv count or error
****************************************************************/
s32 Adapter_SOC_Recv(void *arg, u8* pData, s32 dataLen)
{
    return Ql_SOC_Recv((*(int*)arg),pData,dataLen);
}

/****************************************************************
FunctionName    :   Adapter_SOC_Close
Description     :   soc close func
socketId        :   the id of socket
return          :   the result code
****************************************************************/
s32 Adapter_SOC_Close(s32 socketId)
{
    return Ql_SOC_Close(socketId);
}


/****************************************************************
FunctionName    :   Adapter_UART_Read
Description     :   read data from uart
fd              :   the uart no
data            :   recieve buffer
readlen         :   expected length to read
return          :   the actual read count
****************************************************************/
s32 Adapter_UART_Read(s32 fd,u8 *data,u32 readlen)
{
    return Ql_UART_Read(fd,data,readlen);
}


/****************************************************************
FunctionName    :   Adapter_UART_Send
Description     :   send data to uart
fd              :   the uart no
data            :   send buffer
writelen        :   expected length to send
return          :   the actual send count
****************************************************************/
s32 Adapter_UART_Send(s32 fd,u8 *data,u32 writelen)
{
    return Ql_UART_Write(fd,data,writelen);
}




















#endif
#endif

