/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Quectel Co., Ltd. 2014
*
*****************************************************************************/
/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ril_alarm.h 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The module declares RTC alarm related APIs.
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
#ifndef __RIL_ALARM_H__
#define __RIL_ALARM_H__
#include "ql_time.h"

/*****************************************************************
* Function:     RIL_Alarm_Create
* 
* Description:
*               Set and start the alarm with the specified date and time.
*
* Parameters:
*               dateTime:  [in] Pointer to the ST_Time
*               mode:[in]
*                               0: start alarm only one time.
*                               1: repeat alarm every day.
*                               2: repeat alarm every week.
*                               3: repeat alarm every month.
* Return:        
*               QL_RET_OK indicates this function successes.
*               QL_RET_ERR_PARAM indicates parameter error.
*               QL_RET_ERR_INVALID_TIMER indicates invalid timer.
*****************************************************************/
s32 RIL_Alarm_Create(ST_Time* dateTime, u8 mode);

/*****************************************************************
* Function:     RIL_Alarm_Query
* 
* Description:
*               Query the current setting of the clock alarm.
*
* Parameters:
*               dateTime:  [out] Pointer to the ST_Time
* Return:        
*               QL_RET_OK indicates this function successes.
*               QL_RET_ERR_PARAM indicates parameter error.
*               QL_RET_ERR_INVALID_TIMER indicates invalid timer.
*****************************************************************/
s32 RIL_Alarm_Query(ST_Time* dateTime);

/*****************************************************************
* Function:     RIL_Alarm_Remove
* 
* Description:
*               Remove the alarm.
*
* Parameters:
*               dateTime:  [out] Pointer to the ST_Time
*
* Return:        
*               QL_RET_OK indicates this function successes.
*               QL_RET_ERR_PARAM indicates this function fail.
*****************************************************************/
s32 RIL_Alarm_Remove(ST_Time* dateTime);

#endif	//__RIL_ALARM_H__
