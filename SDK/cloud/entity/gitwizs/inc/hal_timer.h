#ifndef __TIMER_H__
#define __TIMER_H__


#include "gagent_typedef.h"


#define HAL_TIMER_START    (0x108)
#define HAL_MCU_ACK_TIMER   (HAL_TIMER_START+1)
#define HAL_HTTP_CONNECT_TIMEOUT_TIMER    (HAL_TIMER_START+2)
#define HAL_MQTT_CONNECT_TIMEOUT_TIMER     (HAL_TIMER_START+3)
#define HAL_MCU_HEATBEAT_TIMER     (HAL_TIMER_START+4)
#define HAL_CLOUD_HEATBEAT_TIMER   (HAL_TIMER_START+5)




/****************************************************************
Function    :   HAL_Timer_Init
Description :   init a timer
timid       :   the timer id to init
pgc         :   global staruc pointer.
return      :   result code
****************************************************************/
void HAL_Timer_Init(s32 timerid,pgcontext pgc);

/****************************************************************
Function    :   HAL_Timer_Start
Description :   start a timer
timid       :   the timer id to init
interval    :   timer interval
autoRepeat  :   repeat or not
return      :   result code
****************************************************************/
void HAL_Timer_Start(u32 timerid,u32 interval,bool autoRepeat);



/****************************************************************
Function    :   HAL_Timer_Stop
Description :   stop a timer
timid       :   the timer id to stop
return      :   result code
****************************************************************/
void HAL_Timer_Stop(u32 timerid);





#endif
