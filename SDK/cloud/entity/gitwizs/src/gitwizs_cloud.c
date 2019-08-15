#include "custom_feature_def.h"
#ifdef __OCPU_SMART_CLOUD_SUPPORT__
#ifdef __GITWIZS_SOLUTION__

#include "cloud.h"
#include "gagent.h"
#include "hal_socket.h"
#include "adapter.h"
#include "mqttxpg.h"
#include "utils.h"

s32 g_cloud_task_id;

//cloud task entry for cloud interactive
void gagent_cloud_task(s32 taskId)
{
    ST_MSG msg;

    g_cloud_task_id = taskId;
    
    APP_DEBUG("cloud task is running\r\n");
    
    while (TRUE)
    {
        Adapter_GetMsg(&msg);
        switch(msg.message)
        {
            case MSG_ID_GPRS_OK_HINT:
                //login to http server
                APP_DEBUG("begin to logging http server\r\n");
                Adapter_Login_Gservice_Init();
                break;
            case MSG_ID_CLOUD_CONFIG:
                Cloud_ConfigDataHandle(PGC);
                break;
            case MSG_ID_MQTT_CONFIG:
                //login to mqtt server
                APP_DEBUG("begin to logging mqtt server\r\n");
                Adapter_Login_MQTT_Init();
                break;
            case MSG_ID_MQTT_READY:
                Cloud_M2MDataHandle(PGC);
                break;
            case MSG_ID_PING_REQUEST:
                MQTT_HeartbeatTime();
                break;
            case MSG_ID_CLOUD_SEND_DATA:
                if(PGC->rtinfo.waninfo.mqttstatus == MQTT_STATUS_RUNNING)
                {
                   // Adapter_Memcpy(PGC->rtinfo.Cloud_Txbuf,PGC->rtinfo.Rxbuf,sizeof(packet));
                    Cloud_SendData(PGC,(ppacket)msg.param1,(((ppacket)msg.param1)->pend)-(((ppacket)msg.param1)->ppayload) );
                    APP_DEBUG("ReSetpacket Type : CLOUD_DATA_OUT \r\n");
                    PGC->rtinfo.Cloud_Txbuf->type = SetPacketType(PGC->rtinfo.Cloud_Txbuf->type, CLOUD_DATA_OUT, 0);
                }
                else
                {
                    APP_DEBUG("MQTT is not ready,so pls check....\r\n");
                }
                break;
                
            default:
                break;
        }
    }
}




#endif
#endif
