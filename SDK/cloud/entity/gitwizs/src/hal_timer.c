#include "custom_feature_def.h"
#ifdef __OCPU_SMART_CLOUD_SUPPORT__
#ifdef __GITWIZS_SOLUTION__

#include "hal_timer.h"
#include "gagent.h"
#include "adapter.h"
#include "mqttxpg.h"
//#include "mqttlib.h"
#include "cloud.h"


int g_mcu_ack_count = 0;
int g_http_connect_count = 0;
int g_mqtt_connect_count = 0;
int g_mcu_hearbeat_count = 0;
int g_cloud_hearbeat_count = 0;



extern s32 g_main_task_id;
extern s32 g_cloud_task_id;
extern s32 g_mcu_task_id;





//MCU ACK TIMER handler
static void Timer_MCU_Ack_Handler(u32 timerId, void* param);

//HTTP connect timeout handler
static void Timer_HTTP_Connect_Handler(u32 timerId, void* param);

//MQTT connect timeout handler
static void Timer_MQTT_Connect_Handler(u32 timerId, void* param);

//MCU heartbeat timer handler
static void Timer_MCU_HeartBeat_Handler(u32 timerId, void* param);

//CLOUD heartbeat timer handler
static void Timer_Cloud_HeartBeat_Handler(u32 timerId, void* param);









/****************************************************************
Function    :   HAL_Timer_Init
Description :   init a timer
timid       :   the timer id to init
pgc         :   global staruc pointer.
return      :   result code
****************************************************************/
void HAL_Timer_Init(s32 timerid,pgcontext pgc)
{
    s32 ret;
     //register  a timer
    if(HAL_MCU_ACK_TIMER == timerid)
    {
       ret = Ql_Timer_Register(HAL_MCU_ACK_TIMER, Timer_MCU_Ack_Handler, (void *)pgc);
    }
    if(HAL_HTTP_CONNECT_TIMEOUT_TIMER == timerid)
    {
        ret = Ql_Timer_Register(HAL_HTTP_CONNECT_TIMEOUT_TIMER, Timer_HTTP_Connect_Handler, (void *)pgc);
    }
    if(HAL_MQTT_CONNECT_TIMEOUT_TIMER == timerid)
    {        
        ret = Ql_Timer_Register(HAL_MQTT_CONNECT_TIMEOUT_TIMER, Timer_MQTT_Connect_Handler, (void *)pgc);
    }
    if(HAL_MCU_HEATBEAT_TIMER == timerid)
    {        
        ret = Ql_Timer_Register(HAL_MCU_HEATBEAT_TIMER, Timer_MCU_HeartBeat_Handler, (void *)pgc);
    }
    if(HAL_CLOUD_HEATBEAT_TIMER == timerid)
    {        
        ret = Ql_Timer_Register(HAL_CLOUD_HEATBEAT_TIMER, Timer_Cloud_HeartBeat_Handler, (void *)pgc);
    }
    if(ret <0)
    {
        APP_DEBUG("\r\n<--failed!!, Ql_Timer_Register: timer(%d) fail ,ret = %d -->\r\n",timerid,ret);
    }
}



/****************************************************************
Function    :   HAL_Timer_Start
Description :   start a timer
timid       :   the timer id to init
interval    :   timer interval
autoRepeat  :   repeat or not
return      :   result code
****************************************************************/
void HAL_Timer_Start(u32 timerid,u32 interval,bool autoRepeat)
{
    s32 ret;

    ret = Ql_Timer_Start(timerid,interval,autoRepeat);
    
    if(ret < 0)
    {
        APP_DEBUG("\r\n<--failed!! stack timer Ql_Timer_Start ret=%d,timer id=%d-->\r\n",ret,timerid);        
    }
}


/****************************************************************
Function    :   HAL_Timer_Stop
Description :   stop a timer
timid       :   the timer id to stop
return      :   result code
****************************************************************/
void HAL_Timer_Stop(u32 timerid)
{
     s32 ret;

    ret = Ql_Timer_Stop(timerid);
   
    if(ret < 0)
    {
        APP_DEBUG("\r\n<--failed!! stack timer Ql_Timer_Stop ret=%d,timer id=%d-->\r\n",ret,timerid);        
    }
}




static void Timer_MCU_Ack_Handler(u32 timerId, void* param)
{
    APP_DEBUG("mcu ack timeout\r\n");
    g_mcu_ack_count++;
    pgcontext pgc = (pgcontext)param;

    if(g_mcu_ack_count >= 3)
    {
        APP_DEBUG("3 times send data to local io no response\r\n");
        HAL_Timer_Stop(HAL_MCU_ACK_TIMER);
        if((pgc->rtinfo.Cloud_Rxbuf->type&(LOCAL_DATA_OUT)) == LOCAL_DATA_OUT )
        {
            
            Adapter_SendMsg(g_cloud_task_id,MSG_ID_MCU_ACK_TIMEOUT,NULL,NULL);
        }
        else
        {
             Adapter_SendMsg(g_mcu_task_id,MSG_ID_MCU_ACK_TIMEOUT,NULL,NULL);
        }
        g_mcu_ack_count = 0;
        
              
    }
    else
    {
        HAL_Local_SendData(pgc->rtinfo.local.uart_fd,pgc->rtinfo.Txbuf->phead,pgc->mcu.TxbufInfo.local_send_len);
        HAL_Timer_Start(HAL_MCU_ACK_TIMER,MCU_ACK_TIME_MS,FALSE);
    }
}


static void Timer_HTTP_Connect_Handler(u32 timerId, void* param)
{
    g_http_connect_count ++;

    if(g_http_connect_count >= 5)
    {
        
        APP_DEBUG("5 times http operation attempts failed\r\n");
        g_http_connect_count = 0 ;
        Adapter_DevReset();
    }
    else
    {
        
        HAL_Timer_Stop(HAL_HTTP_CONNECT_TIMEOUT_TIMER);
        Adapter_SendMsg(g_cloud_task_id,MSG_ID_CLOUD_CONFIG,NULL,NULL);
    }
}


static void Timer_MQTT_Connect_Handler(u32 timerId, void* param)
{
    g_mqtt_connect_count ++;

    if(g_mqtt_connect_count >= 5)
    {
        
        APP_DEBUG("5 times connection attempts failed\r\n");
        g_mqtt_connect_count = 0 ;
        Adapter_DevReset();
    }
    else
    {
        
        HAL_Timer_Stop(HAL_MQTT_CONNECT_TIMEOUT_TIMER);
        Adapter_SendMsg(g_cloud_task_id,MSG_ID_MQTT_READY,NULL,NULL);
    }
}

static void Timer_MCU_HeartBeat_Handler(u32 timerId, void* param)
{
    g_mcu_hearbeat_count ++;

    if(g_mcu_hearbeat_count >= 3)
    {
        APP_DEBUG("module ping to mcu failed\r\n");
        g_mcu_hearbeat_count = 0 ;
        Adapter_DevReset();
    }
    else
    {
        APP_DEBUG("Local ping...\r\n");
        Adapter_SendMsg(g_mcu_task_id,MSG_ID_PING_REQUEST,NULL,NULL);
    }
}


static void Timer_Cloud_HeartBeat_Handler(u32 timerId, void* param)
{
    g_cloud_hearbeat_count++;

    if(g_cloud_hearbeat_count >= 2)
    {
        APP_DEBUG("module ping to cloud failed,connect to m2m again\r\n");
        g_cloud_hearbeat_count = 0 ;
        
        PGC->rtinfo.waninfo.mqttstatus = MQTT_STATUS_START;
        
        Adapter_SendMsg(g_cloud_task_id,MSG_ID_MQTT_READY,NULL,NULL);
        
    }
    else
    {
        //MQTT_HeartbeatTime();
        
        Adapter_SendMsg(g_cloud_task_id,MSG_ID_PING_REQUEST,NULL,NULL);
        APP_DEBUG("GAgent Cloud Ping ...\r\n" );
    }
    
}



#endif
#endif

