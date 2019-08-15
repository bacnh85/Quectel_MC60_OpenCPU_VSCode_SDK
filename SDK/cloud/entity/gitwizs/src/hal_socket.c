#include "custom_feature_def.h"
#ifdef __OCPU_SMART_CLOUD_SUPPORT__
#ifdef __GITWIZS_SOLUTION__

#include "gagent.h"
#include "hal_socket.h"
#include "adapter.h"
#include "http.h"
#include "hal_timer.h"
#include "cloud.h"
#include "cloud_http.h"
#include "mqttxpg.h"




#define APN_NAME        "UNINET\0"
#define APN_USERID      ""
#define APN_PASSWD      ""



extern s32 g_cloud_task_id;
//bool g_http_context = FALSE;
bool g_socket_data_ready = FALSE;
static u8* m_ipaddress = NULL;  //only save the number of server ip, remove the comma
static ST_GprsConfig  m_gprsCfg;









static void HTTP_RcvData(u8* ptrData, u32 dataLen, void* reserved);
static void Callback_GetIpByName(u8 contexId, u8 requestId, s32 errCode,  u32 ipAddrCnt, u32* ipAddr);
static void Callback_GPRS_Actived(u8 contexId, s32 errCode, void* customParam);
static void CallBack_GPRS_Deactived(u8 contextId, s32 errCode, void* customParam );
static void callback_socket_connect(s32 socketId, s32 errCode, void* customParam );
static void callback_socket_close(s32 socketId, s32 errCode, void* customParam );
static void callback_socket_accept(s32 listenSocketId, s32 errCode, void* customParam );
static void callback_socket_read(s32 socketId, s32 errCode, void* customParam );
static void callback_socket_write(s32 socketId, s32 errCode, void* customParam );


static ST_PDPContxt_Callback     callback_gprs_func = 
{
    Callback_GPRS_Actived,
    CallBack_GPRS_Deactived
};

static ST_SOC_Callback      callback_soc_func=
{
    callback_socket_connect,
    callback_socket_close,
    callback_socket_accept,
    callback_socket_read,    
    callback_socket_write
};


static void callback_socket_connect(s32 socketId, s32 errCode, void* customParam )
{
    if (errCode == SOC_SUCCESS)
    {
        APP_DEBUG("<--Callback: socket connect successfully.-->\r\n");          
        Adapter_SendMsg(g_cloud_task_id,MSG_ID_MQTT_CONFIG,NULL,NULL);
        Adapter_SendMsg(g_cloud_task_id,MSG_ID_MQTT_READY,NULL,NULL);
    }else
    {
        APP_DEBUG("<--Callback: socket connect failure,(socketId=%d),errCode=%d-->\r\n",socketId,errCode);
        Ql_SOC_Close(socketId);
    }
    
}
static void callback_socket_close(s32 socketId, s32 errCode, void* customParam )
{
    if (errCode == SOC_SUCCESS)
    {
        Ql_Debug_Trace("<--CallBack: close socket successfully.-->\r\n"); 
    }else if(errCode == SOC_BEARER_FAIL)
    {   
        Ql_Debug_Trace("<--CallBack: close socket failure,(socketId=%d,error_cause=%d)-->\r\n",socketId,errCode); 
    }else
    {
        Ql_Debug_Trace("<--CallBack: close socket failure,(socketId=%d,error_cause=%d)-->\r\n",socketId,errCode); 
    }
    PGC->rtinfo.waninfo.m2m_socketid = -1;
}
static void callback_socket_accept(s32 listenSocketId, s32 errCode, void* customParam )
{
    Ql_Debug_Trace("callback_socket_accept\r\n");
}
static void callback_socket_read(s32 socketId, s32 errCode, void* customParam )
{  
    s32 ret;
    u32 packetLen=0;
    
    APP_DEBUG("Data from M2M!!!\r\n");
    ret  = Mqtt_RecvPkt(gw_ctx->mqttctx);
    if(ret < 0)
    {
        MQTTclose_socket();
        return -1;
    }


    
   // 
}
static void callback_socket_write(s32 socketId, s32 errCode, void* customParam )
{
    Ql_Debug_Trace("callback_socket_write\r\n");
}



static void HTTP_RcvData(u8* ptrData, u32 dataLen, void* reserved)
{
    APP_DEBUG("<-- data = %s-->\r\n",ptrData);

    resetPacket(PGC->rtinfo.Cloud_Rxbuf);

    Adapter_Memcpy(PGC->rtinfo.Cloud_Rxbuf->phead,ptrData,dataLen);

    Adapter_SendMsg(g_cloud_task_id,MSG_ID_CLOUD_CONFIG,(void *)PGC,NULL);
 
}


static void Callback_GPRS_Actived(u8 contexId, s32 errCode, void* customParam)
{
    s32 ret;
    
    if(errCode == SOC_SUCCESS)
    {
        APP_DEBUG("<--CallBack: active GPRS successfully.-->\r\n");
                
        ret = Ql_SOC_Register(callback_soc_func, NULL);
        if (SOC_SUCCESS == ret)
        {
            APP_DEBUG("<--Register socket callback function successfully.-->\r\n");
             PGC->rtinfo.waninfo.m2m_socketid = Ql_SOC_Create(MQTT_CONTEXT, SOC_TYPE_TCP);
            if (PGC->rtinfo.waninfo.m2m_socketid >= 0)
            {
                APP_DEBUG("<--Create socket id successfully,socketid=%d.-->\r\n",PGC->rtinfo.waninfo.m2m_socketid);
                Adapter_TimerInit(HAL_HTTP_CONNECT_TIMEOUT_TIMER,PGC);
                Adapter_SendMsg(g_cloud_task_id,MSG_ID_CLOUD_CONFIG,NULL,NULL);
            }else
            {
                APP_DEBUG("<--Create socket id failure,error=%d.-->\r\n",PGC->rtinfo.waninfo.m2m_socketid);
            }
        }else if (SOC_ALREADY == ret)
        {
            APP_DEBUG("<--Socket callback function has already been registered,ret=%d.-->\r\n",ret);
        }else
        {
            APP_DEBUG("<--Register Socket callback function failure,ret=%d.-->\r\n",ret);
        }
        
    }else
    {
        APP_DEBUG("<--CallBack: active GPRS failed,errCode=%d-->\r\n",errCode);
    }      
}


static void CallBack_GPRS_Deactived(u8 contextId, s32 errCode, void* customParam )
{
    if (errCode == SOC_SUCCESS)
    {
        APP_DEBUG("<--CallBack: deactived GPRS successfully.-->\r\n"); 
    }else
    {
        APP_DEBUG("<--CallBack: deactived GPRS failure,(contexid=%d,error_cause=%d)-->\r\n",contextId,errCode); 
    }
}




static void Callback_GetIpByName(u8 contexId, u8 requestId, s32 errCode,  u32 ipAddrCnt, u32* ipAddr)
{
    u8 i=0;
    u8* ipSegment = (u8*)ipAddr;
    s32 ret;
    
    APP_DEBUG("<-- %s:contexid=%d, requestId=%d,error=%d,num_entry=%d -->\r\n", __func__, contexId, requestId,errCode,ipAddrCnt);
    if (errCode == SOC_SUCCESS)
    {
        APP_DEBUG("<--CallBack: get ip by name successfully.-->\r\n");
        for(i=0;i<ipAddrCnt;i++)
        {
            ipSegment = (u8*)(ipAddr + i);
            APP_DEBUG("<--Entry=%d, ip=%d.%d.%d.%d-->\r\n",i,ipSegment[0],ipSegment[1],ipSegment[2],ipSegment[3]);
        }

        // Fetch the first ip address as the valid IP
        Ql_memcpy(m_ipaddress, ipAddr, 4);
        ret = Ql_SOC_Connect(PGC->rtinfo.waninfo.m2m_socketid,(u32) m_ipaddress, PGC->minfo.m2m_Port);
        if(ret == SOC_SUCCESS)
        {
            APP_DEBUG("<--The socket is already connected.-->\r\n");            
        }else if(ret == SOC_WOULDBLOCK)
        {
              APP_DEBUG("<--Waiting for the result of socket connection,ret=%d.-->\r\n",ret);
              //waiting CallBack_getipbyname
              
        }else //error
        {
            APP_DEBUG("<--Socket Connect failure,ret=%d.-->\r\n",ret);
            APP_DEBUG("<-- Close socket.-->\r\n");
            Ql_SOC_Close(PGC->rtinfo.waninfo.m2m_socketid);
        }      
        
    }
}




/****************************************************************
FunctionName    :   Hal_Login_MQTT_TCP_Init
Description     :   Init login to mqtt SERVER
return          :   void
****************************************************************/
void Hal_Login_MQTT_TCP_Init(void)
{
 
    Adapter_TimerInit(HAL_MQTT_CONNECT_TIMEOUT_TIMER,PGC);
    Mqtt_InitContext(gw_ctx->mqttctx,1<<10);
    gw_ctx->mqttctx->read_func_arg = (void *)(&(gw_ctx->mqttfd));
    gw_ctx->mqttctx->read_func = Adapter_SOC_Recv;
    gw_ctx->mqttctx->writev_func_arg = (void *)(&(gw_ctx->mqttfd));
    gw_ctx->mqttctx->writev_func = Adapter_SOC_Send;
    gw_ctx->mqttctx->handle_conn_ack = Mqttxpg_HandleConnAck;
    gw_ctx->mqttctx->handle_conn_ack_arg =(void *)gw_ctx;
    gw_ctx->mqttctx->handle_ping_resp = Mqttxpg_HandlePingResp;
    gw_ctx->mqttctx->handle_ping_resp_arg =(void *)gw_ctx;
    gw_ctx->mqttctx->handle_publish = Mqttxpg_HandlePublish;
    gw_ctx->mqttctx->handle_publish_arg = (void *)gw_ctx;
    gw_ctx->mqttctx->handle_pub_ack = Mqttxpg_HandlePubAck;
    gw_ctx->mqttctx->handle_pub_ack_arg = (void *)gw_ctx;
    gw_ctx->mqttctx->handle_pub_rec = Mqttxpg_HandlePubRec;
    gw_ctx->mqttctx->handle_pub_rec_arg = (void *)gw_ctx;
    gw_ctx->mqttctx->handle_pub_rel = Mqttxpg_HandlePubRel;
    gw_ctx->mqttctx->handle_pub_rel_arg = (void *)gw_ctx;
    gw_ctx->mqttctx->handle_pub_comp = Mqttxpg_HandlePubComp;
    gw_ctx->mqttctx->handle_pub_comp_arg = (void *)gw_ctx;
    gw_ctx->mqttctx->handle_sub_ack = Mqttxpg_HandleSubAck;
    gw_ctx->mqttctx->handle_sub_ack_arg = (void *)gw_ctx;
    gw_ctx->mqttctx->handle_unsub_ack = Mqttxpg_HandleUnsubAck;
    gw_ctx->mqttctx->handle_unsub_ack_arg = (void *)gw_ctx;
    gw_ctx->mqttctx->handle_cmd = Mqttxpg_HandleCmd;
    gw_ctx->mqttctx->handle_cmd_arg = (void *)gw_ctx;

    gw_ctx->cmdid[0] = '\0';
    MqttBuffer_Init(gw_ctx->mqttbuf);  
}




/****************************************************************
FunctionName    :   Hal_Login_Gservice_HTTP_Init
Description     :   Init login to gservice
return          :   void
****************************************************************/
void Hal_Login_Gservice_HTTP_Init(void)
{
 
    s32 ret;

    ret = RIL_NW_SetGPRSContext(MQTT_CONTEXT);
    APP_DEBUG("<-- Set GPRS PDP context, ret=%d -->\r\n", ret);
    
    ret = Ql_GPRS_Register(MQTT_CONTEXT, &callback_gprs_func, NULL);
    if (GPRS_PDP_SUCCESS == ret)
    {
        APP_DEBUG("<--Register GPRS callback function successfully.-->\r\n");
    }else if (GPRS_PDP_ALREADY == ret)
    {
        APP_DEBUG("<--GPRS callback function has already been registered,ret=%d.-->\r\n",ret);
    }else
    {
        APP_DEBUG("<--Register GPRS callback function failure,ret=%d.-->\r\n",ret);
    }
//how to solve the apn issue
    Ql_strcpy(m_gprsCfg.apnName,   APN_NAME);
    Ql_strcpy(m_gprsCfg.apnUserId, APN_USERID);
    Ql_strcpy(m_gprsCfg.apnPasswd, APN_PASSWD);
    m_gprsCfg.authtype = 0;

    ret = Ql_GPRS_Config(MQTT_CONTEXT, &m_gprsCfg);
    if (GPRS_PDP_SUCCESS == ret)
    {
        APP_DEBUG("<--configure GPRS param successfully.-->\r\n");
    }else
    {
        APP_DEBUG("<--configure GPRS param failure,ret=%d.-->\r\n",ret);
    }

    ret = Ql_GPRS_Activate(MQTT_CONTEXT);
    if (ret == GPRS_PDP_SUCCESS)
    {
        APP_DEBUG("<--Activate GPRS successfully.-->\r\n");
    }else if (ret == GPRS_PDP_WOULDBLOCK)
    {
         APP_DEBUG("<--Waiting for the result of GPRS activated.,ret=%d.-->\r\n",ret);
        //waiting Callback_GPRS_Actived
    }else if (ret == GPRS_PDP_ALREADY)
    {
        APP_DEBUG("<--GPRS has already been activated,ret=%d.-->\r\n",ret);
    }else//error
    {
        APP_DEBUG("<--Activate GPRS failure,ret=%d.-->\r\n",ret);
    }

}


/****************************************************************
FunctionName    :   Hal_Close_PDPContext
Description     :   close dedicated pdp context
pdp_type        :   which pdp context to deactive
return          :   void
****************************************************************/
void Hal_Close_PDPContext(u8 pdp_type)
{   
       // Set PDP context
       RIL_NW_SetGPRSContext(pdp_type);
       RIL_NW_ClosePDPContext();
       //if(g_http_context)
       {
          // g_http_context = FALSE;
       }
    
}


/****************************************************************
FunctionName    :   Hal_HTTP_RequestTo_Post
Description     :   post msg to http server
strPostMsg      :   the pointer to msg
len             :   msg len
return          :   result code
****************************************************************/
s32 Hal_HTTP_RequestTo_Post(char* strPostMsg, u16 len)
{
      s32 ret;
      ret=Ql_RIL_SendATCmd("AT+QHTTPCFG=\"REQUESTHEADER\",1",Ql_strlen("AT+QHTTPCFG=\"REQUESTHEADER\",1"),NULL, NULL, 0);
      APP_DEBUG("<-- Set http config, ret=%d -->\r\n", ret);
       // Set HTTP server address (URL)
      ret = http_set_server_url(HTTP_SERVER, Adapter_Strlen(HTTP_SERVER));
      APP_DEBUG("<-- Set http server URL, ret=%d -->\r\n", ret);
      ret = http_request_post(strPostMsg, Adapter_Strlen((char*)strPostMsg));
      APP_DEBUG("<-- Send post-request, postMsg=%s, ret=%d -->\r\n", (char*)strPostMsg, ret);

      if(RET_SUCCESS == ret)
      {
        ret = http_read_response(120, HTTP_RcvData);
        APP_DEBUG("<-- Read http response data, ret=%d-->\r\n", ret);
      }
      return ret;
}


/****************************************************************
FunctionName    :   Hal_HTTP_RequestTo_Get
Description     :   get item from http server
timeout         :   timeout
return          :   result code
****************************************************************/
s32 Hal_HTTP_RequestTo_Get(u8* buf,u32 timeout)
{
      s32 ret;
      ret=Ql_RIL_SendATCmd("AT+QHTTPCFG=\"REQUESTHEADER\",0",Ql_strlen("AT+QHTTPCFG=\"REQUESTHEADER\",1"),NULL, NULL, 0);
      APP_DEBUG("<-- Set http config, ret=%d -->\r\n", ret);
      http_set_server_url(buf, Adapter_Strlen(buf));
      ret = http_request_get(timeout);
      APP_DEBUG("<-- Send get-requestret=%d -->\r\n",ret);

      if(RET_SUCCESS == ret)
      {
        ret = http_read_response(120, HTTP_RcvData);
        APP_DEBUG("<-- Read http response data, ret=%d-->\r\n", ret);
      }
      return ret;
}


/****************************************************************
FunctionName    :   HAL_SOC_GetIPByHostName
Description     :   convert hostname to ip addr
hostname        :   hostname
ip              :   the converted ip
return          :   result code
****************************************************************/
s32 HAL_SOC_GetIPByHostName(u8* hostname,u8 *ip)
{
    s32 ret;

    
    m_ipaddress = ip ;
              
    ret = Ql_IpHelper_GetIPByHostName(MQTT_CONTEXT, 0, hostname, Callback_GetIpByName);
    if(ret == SOC_SUCCESS)
    {
        APP_DEBUG("<--Get ip by hostname successfully.-->\r\n");

    }
    else if(ret == SOC_WOULDBLOCK)
    {
        APP_DEBUG("<--Waiting for the result of Getting ip by hostname,ret=%d.-->\r\n",ret);
        //waiting CallBack_getipbyname
    }
    else
    {
        APP_DEBUG("<--Get ip by hostname failure:ret=%d-->\r\n",ret);
 
    }
}
















#endif
#endif

