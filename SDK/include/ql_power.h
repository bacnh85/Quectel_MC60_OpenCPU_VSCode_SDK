/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Quectel Co., Ltd. 2013
*
*****************************************************************************/
/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ql_power.h 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   Power APIs defines.
 *
 * Author:
 * -------
 * -------
 *
 *============================================================================
 *             HISTORY
 *----------------------------------------------------------------------------
 * 
 ****************************************************************************/
 

#ifndef __POWER_MGMT_H__
#define __POWER_MGMT_H__
#include "ql_type.h"

typedef enum {
    KEY_UP,
    KEY_DOWN
}Enum_KeyState;

typedef enum {
    POWER_ON,
    POWER_OFF
}Enum_PowerKeyOpType;

typedef enum {
    PWRKEYPWRON = 0,
    CHRPWRON	= 1,
    RTCPWRON = 2,
    CHRPWROFF = 3,
    WDTRESET = 4, /*NORMAL*/
    ABNRESET = 5,  /*ABNORMAL RESET*/
    USBPWRON = 6,  
    USBPWRON_WDT = 7,
    PRECHRPWRON = 8,
    HWSYSRST = 9,
    UNKNOWN_PWRON = 0xF9	
}Enum_PowerOnReason;

typedef enum{
    VBATT_UNDER_WRN = 0,
    VBATT_UNDER_PDN,
    VBATT_OVER_WRN,
    VBATT_OVER_PDN
}Enum_VoltageIndType;

/*****************************************************************
* Function:     Ql_PowerDown
*
* Description:
*               This function powers down the module. Before powering  
*               down, this function unregisters the GSM/GPRS network.
*               So there's some delay for powering down.
* Parameters:
*               pwrDwnType:
*                   Must be 1 = Normally power off
* Return:
*               None
*****************************************************************/
void Ql_PowerDown(u8 pwrDwnType);

/*****************************************************************
* Function:     Ql_LockPower
*
* Description:  This function locks the power supply for module. And
*               it's used when only power key is controlled by application.
* Parameters:
*               None
* Return:
*               None
*****************************************************************/
void Ql_LockPower(void);

/*****************************************************************
* Function:     Ql_PwrKey_Register 
* 
* Description:
*               Register the callback for PWRKEY indication.
*               the callback function will be triggered when the power
*               KEY pressed down or release.
*
* Parameters:
*               callback_pwrKey:
*                   callback function for PWRKEY indication.
*               param1:
*                   Power on or power off, one value of Enum_PowerKeyOpType.
*               param2:
*                   Pressing state of power key. 
*                   Key up or key down, one value of Enum_KeyState.
* Return:        
*               QL_RET_OK indicates this function successes.
*               QL_RET_ERR_INVALID_PARAMETER, indicates the wrong input parameter.
*****************************************************************/
typedef void (*Callback_PowerKey_Ind)(s32 param1, s32 param2);
s32 Ql_PwrKey_Register(Callback_PowerKey_Ind callback_pwrKey);

/*****************************************************************
* Function:     Ql_GetPowerOnReason
*
* Description:
*               This function gets a reason for power on.
*
* Parameters:
*               None
*
* Return:
*               The reason of the Power on, one value of Enum_PowerOnReason.
*****************************************************************/
s32 Ql_GetPowerOnReason(void);


/*****************************************************************
* Function:     Ql_SleepEnable 
* 
* Description:
*               Set the module into sleep mode at once
*
* Return:        
*               QL_RET_OK indicates this function successes.
*		   Ql_RET_NOT_SUPPORT this function not support.	
*****************************************************************/
s32 Ql_SleepEnable(void);

/*****************************************************************
* Function:     Ql_SleepDisable 
* 
* Description:
*               Exit  the sleep mode 
*
* Return:        
*               QL_RET_OK indicates this function successes.
*		   Ql_RET_NOT_SUPPORT this function not support.	
*****************************************************************/
 s32 Ql_SleepDisable(void);

#endif
