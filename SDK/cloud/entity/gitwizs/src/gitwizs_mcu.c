#include "custom_feature_def.h"
#ifdef __OCPU_SMART_CLOUD_SUPPORT__
#ifdef __GITWIZS_SOLUTION__

#include "gagent.h"
#include "adapter.h"
#include "hal_uart.h"
#include "hal_timer.h"

s32 g_mcu_task_id;

//mcu task entry for mcu --- module interactive
void gagent_mcu_task(s32 taskId)
{
    ST_MSG msg;
    s32 ret;

    APP_DEBUG("Gagent mcu task starts running\r\n");
    
    g_mcu_task_id = taskId;
    
    GAgent_Init( PPGC );

    while (TRUE)
    {
        Adapter_GetMsg(&msg);
        switch(msg.message)
        {
            case MSG_ID_MCU_SEND_DATA:
                //Adapter_Memcpy(PGC->rtinfo.Txbuf,PGC->rtinfo.Cloud_Rxbuf,sizeof(packet));
                
                Adapter_TimerStart(HAL_MCU_ACK_TIMER,MCU_ACK_TIME_MS,FALSE);
                //GAgent_LocalDataWriteP0( PGC,PGC->rtinfo.local.uart_fd, (ppacket)msg.param1,MCU_CTRL_CMD );   
                //HAL_Local_SendData(s32 fd,u8 * pData,u32 bufferMaxLen)
                break;
            case MSG_ID_PING_REQUEST:
                GAgent_LocalDataWriteP0(PGC, PGC->rtinfo.local.uart_fd, PGC->rtinfo.Txbuf, MODULE_PING2MCU);
                break;
            default:
                break;
        }
    }
}




#endif
#endif
