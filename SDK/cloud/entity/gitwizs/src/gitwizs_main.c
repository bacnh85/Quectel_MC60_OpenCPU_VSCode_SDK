#include "custom_feature_def.h"
#ifdef __OCPU_SMART_CLOUD_SUPPORT__
#ifdef __GITWIZS_SOLUTION__

#include "adapter.h"
#include "gagent.h"
#include "hal_timer.h"

s32 g_main_task_id;
extern int g_mcu_hearbeat_count;
extern int g_cloud_hearbeat_count;



//main task entry for heartbeat maintenance
void gagent_main_task(s32 taskId)
{ 
    ST_MSG msg;

    g_main_task_id = taskId;
    

    APP_DEBUG("Gagent main task starts running\r\n");

     // Start message loop of this task
    while (TRUE)
    {
        Adapter_GetMsg(&msg);
        switch(msg.message)
        {
            case MSG_ID_HEARTBEAT_READY:
                Adapter_TimerInit(HAL_MCU_HEATBEAT_TIMER,PGC);
                Adapter_TimerInit(HAL_CLOUD_HEATBEAT_TIMER,PGC);
                Adapter_TimerStart(HAL_MCU_HEATBEAT_TIMER,MCU_HEARTBEAT,TRUE);
                Adapter_TimerStart(HAL_CLOUD_HEATBEAT_TIMER,CLOUD_HEARTBEAT,TRUE);
                break;
            case MSG_ID_MCU_PING_RSP_OK:
                g_mcu_hearbeat_count = 0;
                APP_DEBUG("\r\n module ping to mcu is ok\r\n");
                break;
            case MSG_ID_CLOUD_PING_RSP_OK:
                APP_DEBUG("cloud pinrsp is ok\r\n");
                g_cloud_hearbeat_count = 0;
               break;
            default:
                break;
        }
    }
}


#endif
#endif

