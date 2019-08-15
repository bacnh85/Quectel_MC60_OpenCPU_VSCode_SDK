#include "custom_feature_def.h"
#ifdef __OCPU_SMART_CLOUD_SUPPORT__
#ifdef __GITWIZS_SOLUTION__

#include "cloud.h"
#include "adapter.h"
#include "gagent.h"
#include "hal_socket.h"
#include "hal_timer.h"
#include "http.h"
#include "utils.h"
#include "mqttxpg.h"
//#include "mqttlib.h"

//extern bool g_http_context;
extern s32 g_cloud_task_id;
extern bool g_socket_data_ready;
extern s32 g_main_task_id;
extern int g_http_connect_count;
extern int g_mqtt_connect_count;
extern int g_cloud_hearbeat_count;









static u8 Cloud_SetDeviceID( pgcontext pgc,s8 *p_szDeviceID );
static u32 Cloud_Get_Gserver_Time( u32 *clock, u8 *Http_recevieBuf, s32 respondCode );




/****************************************************************
*
*   function    :   gagent do cloud config.
*   cloudstatus :   gagent cloud status.
*   return      :   0 successful other fail.
****************************************************************/
u32 Cloud_ConfigDataHandle( pgcontext pgc /*int32 cloudstatus*/ )
{
    s32 cloudstatus = 0;
    s8 *pDeviceID=NULL;
    s32 http_fd;
    s32 ret;
    s32 respondCode=0;
    pgcontext pGlobalVar=NULL;
    pgconfig pConfigData=NULL;
    u8 *pCloudConfiRxbuf = NULL;


    pConfigData = &(pgc->gc);
    pGlobalVar = pgc;
    pCloudConfiRxbuf = pgc->rtinfo.Cloud_Rxbuf->phead;
    cloudstatus = pgc->rtinfo.waninfo.CloudStatus;

    Adapter_Get_GPRS_Status(&(pgc->rtinfo.gprs_status));

    if(pgc->rtinfo.gprs_status != NW_STAT_REGISTERED && pgc->rtinfo.gprs_status != NW_STAT_REGISTERED_ROAMING)
    {  
        
        APP_DEBUG("network error,pls check\r\n");
        return CLOUD_ERROR_NETWORK;
    }

    if(CLOUD_CONFIG_OK == cloudstatus)
    {
        
        APP_DEBUG("cloud config has already done\r\n");
        return CLOUD_ERROR_CONFIG_ALREADY;
    }
    pDeviceID = pgc->gc.DID;

    
    APP_DEBUG("pDeviceID = %s \r\n",pDeviceID );

    if(CLOUD_INIT == cloudstatus)
    {
        if(Adapter_Strlen(pDeviceID) == (DID_LEN - 2))/*had did*/
        {
            APP_DEBUG("Had did !!!!\r\n go to Provision\r\n" );

            ret = Cloud_ReqProvision( pgc );
             if(RET_SUCCESS != ret)/* can't got the did */
             {
                pgc->rtinfo.waninfo.CloudStatus = CLOUD_INIT;
                pgc->rtinfo.waninfo.ReConnectHttpTime += GAGENT_HTTP_TIMEOUT;
                goto label;
             }
             pgc->rtinfo.waninfo.CloudStatus = CLOUD_RES_PROVISION;
             pgc->rtinfo.waninfo.ReConnectHttpTime = 0;
             g_http_connect_count = 0;
             Adapter_TimerStop(HAL_HTTP_CONNECT_TIMEOUT_TIMER);
        }
        else
        {
            APP_DEBUG("Need to get did!!!\r\n" );
            Cloud_SetDeviceID( pgc,NULL );/*clean did*/
            ret = Cloud_ReqRegister( pgc );

            if(ret != RET_SUCCESS)
            {
                pgc->rtinfo.waninfo.CloudStatus = CLOUD_INIT;  
                pgc->rtinfo.waninfo.ReConnectHttpTime += GAGENT_HTTP_TIMEOUT;
                goto label;

            }
           pgc->rtinfo.waninfo.CloudStatus = CLOUD_RES_GET_DID;
           pgc->rtinfo.waninfo.ReConnectHttpTime = 0;
           g_http_connect_count = 0;
           Adapter_TimerStop(HAL_HTTP_CONNECT_TIMEOUT_TIMER);
        }
        
        return 0;
    }
    switch( cloudstatus )
    {
        case CLOUD_RES_GET_DID:
            
             ret = Cloud_ResRegister( pCloudConfiRxbuf,pDeviceID,201 ); 
             
             if(RET_SUCCESS != ret)/* can't got the did */
             {
                pgc->rtinfo.waninfo.CloudStatus = CLOUD_INIT;
                pgc->rtinfo.waninfo.ReConnectHttpTime += GAGENT_HTTP_TIMEOUT;
                goto label;
             }
             else
             {
                 Cloud_SetDeviceID( pgc,pDeviceID );
                 //Adapter_DevGetConfigData( &(pgc->gc) );
                 APP_DEBUG( "Register got did :%s len=%d\r\n",pgc->gc.DID,Adapter_Strlen(pgc->gc.DID) );
              
                 ret = Cloud_ReqProvision( pgc );
                 if(ret != RET_SUCCESS)
                 {
                    pgc->rtinfo.waninfo.CloudStatus = CLOUD_INIT;
                    pgc->rtinfo.waninfo.ReConnectHttpTime += GAGENT_HTTP_TIMEOUT;
                    goto label;

                 }
                 pgc->rtinfo.waninfo.CloudStatus = CLOUD_RES_PROVISION;
                 pgc->rtinfo.waninfo.ReConnectHttpTime = 0;
                  g_http_connect_count = 0;
                 Adapter_TimerStop(HAL_HTTP_CONNECT_TIMEOUT_TIMER);

             }
             break;
         case CLOUD_RES_PROVISION:
             ret = Cloud_ResProvision( pGlobalVar->minfo.m2m_SERVER , &pGlobalVar->minfo.m2m_Port,pCloudConfiRxbuf,200);
             //Cloud_Get_Gserver_Time( &pGlobalVar->rtinfo.clock, pCloudConfiRxbuf,200 );
             if( RET_SUCCESS != ret )
             {
                 pgc->rtinfo.waninfo.CloudStatus = CLOUD_INIT;
                 pgc->rtinfo.waninfo.ReConnectHttpTime += GAGENT_HTTP_TIMEOUT;
                 goto label;
             }
             else
             {
                pgc->rtinfo.waninfo.ReConnectHttpTime = 0;
                pgc->rtinfo.waninfo.CloudStatus = CLOUD_CONFIG_OK;
                g_http_connect_count = 0;
                
                Adapter_TimerStop(HAL_HTTP_CONNECT_TIMEOUT_TIMER);
                //login to m2m.
                pgc->rtinfo.waninfo.mqttstatus = MQTT_STATUS_START;
                APP_DEBUG("Provision OK!\r\n");
                APP_DEBUG("M2M host:%s port:%d\r\n",pGlobalVar->minfo.m2m_SERVER,pGlobalVar->minfo.m2m_Port);
                APP_DEBUG("http config is ok  and login M2M !\r\n");
                
                Adapter_GetIPByHostName(PGC->minfo.m2m_SERVER,PGC->gc.m2m_ip);
                Adapter_DevSaveConfigData(pConfigData);
              }
            break;
        default :
            break;
    }
    return 0;
label:
    
     Adapter_TimerStart(HAL_HTTP_CONNECT_TIMEOUT_TIMER,pgc->rtinfo.waninfo.ReConnectHttpTime,FALSE);
}


void Cloud_SetClientAttrs(pgcontext pgc, u8 *clientid, u16 cmd, s32 sn)
{
    if(NULL != clientid)
    {
        Adapter_Strncpy(pgc->rtinfo.waninfo.srcAttrs.phoneClientId, (s8 *)clientid,Adapter_Strlen((s8 *)clientid));
    }
    pgc->rtinfo.waninfo.srcAttrs.cmd = cmd;
    pgc->rtinfo.waninfo.srcAttrs.sn = sn;
}




/****************************************************************
        FunctionName        :   Cloud_M2MDataHandle.
        Description         :   Receive cloud business data .
        xpg                 :   global context.
        return              :   >0 have business data,and need to 
                                   handle.
                                other,no business data.
****************************************************************/
s32 Cloud_M2MDataHandle(  pgcontext pgc)
{
    pgcontext pGlobalVar=NULL;
    pgconfig pConfigData=NULL;
    u16 mqttstatus;
    s8 *username=NULL;
    s8 *password=NULL;
    u32 ret;
    u8* pMqttBuf=NULL;
    unsigned int dataLen;

    pConfigData = &(pgc->gc);
    pGlobalVar = pgc;
    mqttstatus = pGlobalVar->rtinfo.waninfo.mqttstatus;
    username = pConfigData->DID;
    pMqttBuf = pgc->rtinfo.Cloud_Rxbuf->phead ;
    
    APP_DEBUG("<--lebron:ip=%d.%d.%d.%d-->\r\n",pConfigData->m2m_ip[0],pConfigData->m2m_ip[1],pConfigData->m2m_ip[2],pConfigData->m2m_ip[3]);

    
    Adapter_Get_GPRS_Status(&(pgc->rtinfo.gprs_status));
    
    if(pgc->rtinfo.gprs_status != NW_STAT_REGISTERED && pgc->rtinfo.gprs_status != NW_STAT_REGISTERED_ROAMING)
    {  
       
       APP_DEBUG("network error,pls check\r\n");
       return CLOUD_ERROR_NETWORK;
    }

    if( MQTT_STATUS_START==mqttstatus )
    {
        APP_DEBUG("Req to connect m2m !\r\n");
        APP_DEBUG("username: %s password: %s\r\n",username,pConfigData->modulepasscode);

        ret = Cloud_ReqConnect( pgc,username,pConfigData->modulepasscode );

        if(ret != RET_SUCCESS)
        {
            pgc->rtinfo.waninfo.mqttstatus = MQTT_STATUS_START;
            pgc->rtinfo.waninfo.ReConnectMqttTime += GAGENT_CLOUDREADD_TIME;
            goto label;
           
        }
        else
        {
           pgc->rtinfo.waninfo.mqttstatus = MQTT_STATUS_RES_LOGIN;
           pgc->rtinfo.waninfo.ReConnectMqttTime = 0;
           g_mqtt_connect_count = 0;
           Adapter_TimerStop(HAL_MQTT_CONNECT_TIMEOUT_TIMER); 
        }
        APP_DEBUG(" MQTT_STATUS_START \r\n");       
        return 0;
    }
    if( MQTT_STATUS_RES_LOGIN==mqttstatus )
    {
        if(pgc->rtinfo.m2m_result != 0)
        {
            pgc->rtinfo.waninfo.mqttstatus = MQTT_STATUS_START;
            pgc->rtinfo.waninfo.ReConnectMqttTime += GAGENT_CLOUDREADD_TIME;
            goto label;
        }
        else
        {
             APP_DEBUG("GAgent do req connect m2m OK !\r\n");
             APP_DEBUG("Go to MQTT_STATUS_REQ_LOGINTOPIC1. \r\n");
             Cloud_ReqSubTopic( pgc,MQTT_STATUS_REQ_LOGINTOPIC1 );
             pGlobalVar->rtinfo.waninfo.mqttstatus = MQTT_STATUS_RES_LOGINTOPIC1;
             pgc->rtinfo.waninfo.ReConnectMqttTime = 0;
             g_mqtt_connect_count = 0;
             Adapter_TimerStop(HAL_MQTT_CONNECT_TIMEOUT_TIMER); 
        }
    }
    if( MQTT_STATUS_RES_LOGINTOPIC1==mqttstatus )
    {
        if(pgc->rtinfo.m2m_result != 0)
        {
            pgc->rtinfo.waninfo.mqttstatus = MQTT_STATUS_START;
            pgc->rtinfo.waninfo.ReConnectMqttTime += GAGENT_CLOUDREADD_TIME;
            goto label;
        }
        else
        {
             Cloud_ReqSubTopic( pgc,MQTT_STATUS_REQ_LOGINTOPIC2 );
             pGlobalVar->rtinfo.waninfo.mqttstatus = MQTT_STATUS_RES_LOGINTOPIC2;
             pgc->rtinfo.waninfo.ReConnectMqttTime = 0;
             g_mqtt_connect_count = 0 ;
             Adapter_TimerStop(HAL_MQTT_CONNECT_TIMEOUT_TIMER); 
        }
    }
    if( MQTT_STATUS_RES_LOGINTOPIC2==mqttstatus )
    {
        if(pgc->rtinfo.m2m_result != 0)
        {
            pgc->rtinfo.waninfo.mqttstatus = MQTT_STATUS_START;
            pgc->rtinfo.waninfo.ReConnectMqttTime += GAGENT_CLOUDREADD_TIME;
            goto label;
        }
        else
        {
             Cloud_ReqSubTopic( pgc,MQTT_STATUS_REQ_LOGINTOPIC3 );
             pGlobalVar->rtinfo.waninfo.mqttstatus = MQTT_STATUS_RES_LOGINTOPIC3;
             pgc->rtinfo.waninfo.ReConnectMqttTime = 0;
             g_mqtt_connect_count = 0 ;
             Adapter_TimerStop(HAL_MQTT_CONNECT_TIMEOUT_TIMER); 
        }
    }
    if( MQTT_STATUS_RES_LOGINTOPIC3==mqttstatus )
    {
        if(pgc->rtinfo.m2m_result != 0)
        {
            pgc->rtinfo.waninfo.mqttstatus = MQTT_STATUS_START;
            pgc->rtinfo.waninfo.ReConnectMqttTime += GAGENT_CLOUDREADD_TIME;
            goto label;
        }
        else
        {
             pGlobalVar->rtinfo.waninfo.mqttstatus = MQTT_STATUS_RUNNING;
             pgc->rtinfo.waninfo.ReConnectMqttTime = 0;
             g_mqtt_connect_count = 0;
             Adapter_TimerStop(HAL_MQTT_CONNECT_TIMEOUT_TIMER); 
             Adapter_SendMsg(g_main_task_id,MSG_ID_HEARTBEAT_READY,NULL,NULL);
             
             APP_DEBUG("mqtt server is running now !\r\n");
        }
    }
#if 1

    if(MQTT_STATUS_RUNNING==mqttstatus)
    {

        dataLen = Mqtt_DispatchPublishPacket( pgc,pMqttBuf,pgc->rtinfo.m2m_recvlen );
        
        Ql_Debug_Trace("%s %d\r\n",__FUNCTION__,__LINE__);
        
        Ql_Debug_Trace("dataLen = %d\r\n",dataLen);
        if( dataLen>0 )
        {
            {
              pgc->rtinfo.Cloud_Rxbuf->type = SetPacketType( pgc->rtinfo.Cloud_Rxbuf->type,CLOUD_DATA_IN,1 );
              ParsePacket(  pgc->rtinfo.Cloud_Rxbuf );
              APP_DEBUG("%s %d type : %04X len :%d\r\n",__FUNCTION__,__LINE__,pgc->rtinfo.Cloud_Rxbuf->type,dataLen );
              dealPacket(pgc,pgc->rtinfo.Cloud_Rxbuf);
            }
        }
        
        Ql_Debug_Trace("%s %d\r\n",__FUNCTION__,__LINE__);

    }
#endif

    
        

    return 0;
  label:
    
     Adapter_TimerStart(HAL_MQTT_CONNECT_TIMEOUT_TIMER,pgc->rtinfo.waninfo.ReConnectMqttTime,FALSE);
}

u32 Cloud_ReqSubTopic( pgcontext pgc,u16 mqttstatus )
{
  u32 ret=0;
  ret = Mqtt_DoSubTopic( pgc,mqttstatus);
  return ret;
}


/****************************************************************
        FunctionName    :   GAgent_Cloud_SendData
        Description     :   send buf data to M2M server.
        return          :   0-ok 
                            other fail.
****************************************************************/
u32 Cloud_SendData( pgcontext pgc,ppacket pbuf,s32 buflen )
{
    s8 ret = 0;
    u16 cmd;

    stCloudAttrs_t *client = &pgc->rtinfo.stChannelAttrs.cloudClient;
    cmd = client->cmd;
    
    if( isPacketTypeSet( pbuf->type,CLOUD_DATA_OUT ) == 1)
    {

        ret = MQTT_SenData( pgc, pgc->gc.DID, pbuf,buflen );

        if(0x0093 == cmd || 0x0094 == cmd)
        {
            client->cmd = 0x0091;
            MQTT_SenData( pgc, pgc->gc.DID, pbuf,buflen );
        }

        APP_DEBUG("Send date to cloud :len =%d ,ret =%d\r\n",buflen,ret );
        
        pbuf->type = SetPacketType( pbuf->type,CLOUD_DATA_OUT,0 );
    }
    return ret;
}


void Cloud_ClearClientAttrs(pgcontext pgc, stCloudAttrs_t *client)
{
    Adapter_Memset((char *)client, 0, sizeof(stCloudAttrs_t));    
}

/****************************************************************
        Function        :   Cloud_ResSubTopic
        Description     :   check sub topic respond.
        buf             :   data form mqtt.
        msgsubId        :   sub topic messages id
        return          :   0 sub topic ok.
                            other fail.
****************************************************************/
u32 Cloud_ResSubTopic( const u8* buf,u16 msgsubId )
{
    u16 recmsgId=0;
     if(NULL == buf)
        return RET_FAILED;
    recmsgId = mqtt_parse_msg_id( buf );
    if( recmsgId!=msgsubId )
        return RET_FAILED;
    else 
        return RET_SUCCESS;
}


/****************************************************************
        Function    :   Cloud_ReqConnect
        Description :   send req m2m connect packet.
        username    :   username.
        password    :   username.
        return      :   0: send req connect packet ok
                        other req connect fail.
****************************************************************/
u32 Cloud_ReqConnect( pgcontext pgc,const u8 *username,const u8 *password )
{
    s8 ret = 0;
    s32 socket = 0;
    pgcontext pGlobalVar=NULL;
    pgconfig pConfigData=NULL;
    s32 nameLen=0,passwordLen=0;

    pGlobalVar=pgc;
    pConfigData = &(pgc->gc);

    nameLen = Adapter_Strlen( username );
    passwordLen = Adapter_Strlen( password );
    
    if( nameLen<=0 || nameLen>22 ) /* invalid name */
    {
        APP_DEBUG(" can't req to connect to m2m invalid name length !\r\n");
        return -1;
    }
    if( passwordLen<=0 || passwordLen>16 )/* invalid password */
    {
        APP_DEBUG(" can't req to connect to m2m invalid password length !\r\n");
        return -1;
    }
    APP_DEBUG("Connect to server domain:%s port:%d\r\n",pGlobalVar->minfo.m2m_SERVER,pGlobalVar->minfo.m2m_Port );

    if( pGlobalVar->rtinfo.waninfo.m2m_socketid<0 )
    {
        APP_DEBUG("m2m socket :%d\r\n",socket);
        return RET_FAILED;
    }    
    ret = Mqtt_Login2Server( pGlobalVar->rtinfo.waninfo.m2m_socketid,(const u8*)username,(const u8*)password );
    return ret;
}



/****************************************************************
        Function    :   Cloud_ReqRegister
        description :   sent register data to cloud
        Input       :   NULL;
        return      :   0-send register data ok.
                        other fail.
****************************************************************/
u32 Cloud_ReqRegister( pgcontext pgc )
{
    u32 socket = 0;
    s8 ret = 0;

    pgcontext pGlobalVar=NULL;
    pgconfig pConfigData=NULL;

    pGlobalVar=pgc;
    pConfigData = &(pgc->gc);
   
    ret = Http_POST(HTTP_SERVER,(char *)pGlobalVar->gc.modulepasscode,(char *)pGlobalVar->minfo.imei,
                        (char *)pGlobalVar->mcu.product_key );
    
    if( RET_SUCCESS!=ret )
    {
        return RET_FAILED;
    }
    else
    {
        return RET_SUCCESS;
    }
    
}


/* 
    will get the device id.
*/
s8 Cloud_ResRegister( u8 *cloudConfiRxbuf,s8 *pDID,s32 respondCode )
{
    s32 ret=0;
    
    if( 201 != respondCode)
    {
        return RET_FAILED;
    }
    ret = Http_Response_DID( cloudConfiRxbuf,pDID );
    if( RET_SUCCESS==ret )
    {
        return RET_SUCCESS;
    }
    else 
        return RET_FAILED;
}


/****************************************************************
*       FunctionName    :   Cloud_ReqProvision
*       Description     :   send provision req to host.
*       host            :   GServer host,like "api.gizwits.com"
*       return          :   0 success other error.
****************************************************************/
u32 Cloud_ReqProvision( pgcontext pgc )
{
    s8 ret = 0;
    pgcontext pGlobalVar=NULL;
    pgconfig pConfigData=NULL;
    
    pGlobalVar=pgc;
    pConfigData = &(pgc->gc);
    
    ret = Http_GET( HTTP_SERVER,pConfigData->DID);
    return ret;
}


u32 Cloud_ReqDisable( pgcontext pgc )
{
    s32 ret = 0;
    s32 socket = 0;
    pgcontext pGlobalVar=NULL;
    pgconfig pConfigData=NULL;
    
    pGlobalVar=pgc;
    pConfigData = &(pgc->gc);

    ret = Http_Delete(HTTP_SERVER,pConfigData->old_did,pConfigData->old_modulepasscode );
    return ret;
}




/****************************************************************
*       FunctionName    :   Cloud_ResProvision.
*       Description     :   data form server after provision.
*       szm2mhost       :   m2m server like: "m2m.gizwits.com"
*       port            :   m2m port .
*       respondCode     :   http respond code.
*       return          :   0 success other fail.
****************************************************************/
u32 Cloud_ResProvision( s8 *szdomain,s32 *port,u8 *cloudConfiRxbuf,s32 respondCode )
{
    s32 ret = 0;
    if( 200 != respondCode )
    {
        return RET_FAILED;
    }
    ret = Http_getdomain_port( cloudConfiRxbuf,szdomain,port );
    return ret;
}


static u32 Cloud_Get_Gserver_Time( u32 *clock, u8 *Http_recevieBuf, s32 respondCode )
{
    s8 *p_start = NULL;
    s8 *p_end =NULL;   
    s8 stime[20]={0};
    u32 time;
    
    if( 200 != respondCode )
    {        
        return RET_FAILED;   
    }   
    p_start = Adapter_StrStr((char *)Http_recevieBuf, "server_ts=");
    if( p_start==NULL ) 
        return RET_FAILED;   
    p_start = p_start+Adapter_Strlen("server_ts=");
    p_end = Adapter_StrStr( p_start,"&" ); 
    if( p_end == NULL )
    {
        p_end = Adapter_StrStr( p_start,"\r" ); 
    }    
    Adapter_Memcpy(stime,p_start,( p_end-p_start));
    time = Adapter_Atoi(stime);
    *clock = time;
    return RET_SUCCESS;
}







static u8 Cloud_SetDeviceID( pgcontext pgc,s8 *p_szDeviceID )
{
    if( p_szDeviceID != NULL )
    {
        Adapter_Strncpy( pgc->gc.DID,p_szDeviceID,Adapter_Strlen(p_szDeviceID));
    }
    else
    {
        Adapter_Memset( pgc->gc.DID,0,DID_LEN );
    }
    Adapter_DevSaveConfigData( &(pgc->gc) );
    return 0;
}




/****************************************************************
        Function    :   Cloud_ResConnect
        Description :   handle packet form mqtt req connect 
        buf         :   data form mqtt.
        return      :   0: req connect ok
                        other req connect fail.
****************************************************************/
u32 Cloud_ResConnect( u8* buf )
{
    if(NULL == buf)
        return RET_FAILED;

    if( buf[3] == 0x00 )
    {
        if( (buf[0]!=0) && (buf[1] !=0) )
        {
        return RET_SUCCESS;
        }
        else
        {
            APP_DEBUG("%s %s %d\r\n",__FILE__,__FUNCTION__,__LINE__ );
            APP_DEBUG("MQTT Connect res  fail ret =%d!!\r\n",buf[3] );
            return RET_FAILED;
        }
    }
    APP_DEBUG("res connect fail with %d \r\n",buf[3] );
        return buf[3];
}









#endif
#endif
