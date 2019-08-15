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
 *   ril_gps.h 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The module declares GPS related APIs.
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
#ifndef __RIL_GPS_H__
#define __RIL_GPS_H__

#include "ql_type.h"

/*****************************************************************
* Function:     RIL_GPS_Open
* 
* Description:
*               Power on/off GPS.
*
* Parameters:   op:[in]
*                      1: Power on GPS.
*                      0: Power off GPS.
* Return:        
*               QL_RET_OK indicates this function successes.
*               QL_RET_ERR_PARAM indicates parameter error.
*****************************************************************/
s32 RIL_GPS_Open(u8 op);

/******************************************************************************
* Function:     RIL_GPS_SetRefLoc
*  
* Description:
*               This function sets the reference location for QuecFastFixOnline. 
*
* Parameters:    
*               <lat>:
*                   [in]double, latitude
*               <lon>
*                   [in]double, longitude
* Return:
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*                                      and then call Ql_RIL_Initialize to initialize RIL.
******************************************************************************/
s32 RIL_GPS_SetRefLoc(double lat, double lon);

/******************************************************************************
* Function:     RIL_GPS_GetPowerState
*  
* Description:
*               This function gets the power state of GNSS. 
*
* Parameters:    
*               <stat>:
*                   [out]pointer of s32, address of s32 variable
* Return:
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*                                      and then call Ql_RIL_Initialize to initialize RIL.
******************************************************************************/
s32  RIL_GPS_GetPowerState(s32 *stat);

/*****************************************************************
* Function:     RIL_GPS_Read
* 
* Description:
*               Query the navigation information.
*
* Parameters:   item  :  [in] Pointer to the query item
*               rdBuff:  [out] Pointer to the information buffer
* Return:        
*               QL_RET_OK indicates this function successes.
*               QL_RET_ERR_PARAM indicates parameter error.
*****************************************************************/
s32 RIL_GPS_Read(u8 *item, u8 *rdBuff);

/*****************************************************************
* Function:     RIL_GPS_CMD_Send
* 
* Description:
*               This function is used to send NMEA to GPS module.
*
* Parameters:
*                <cmdType>:
*                       [IN] always 0 currently.
*                <cmdStr>:
*                       [IN] this string is an NMEA sentence.
*                <cb_GPSCMD_hdl>:
*                       [IN] callback function for QGPSCMD URC handle.
*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*                                      and then call Ql_RIL_Initialize to initialize RIL.               
*****************************************************************/
typedef void (* CB_GPSCMD)(char *strURC);
s32 RIL_GPS_CMD_Send(u8 cmdType, u8 *cmdStr, CB_GPSCMD cb_GPSCMD_hdl);

/*****************************************************************
* Function:     RIL_GPS_EPO_Enable
* 
* Description:
*               This function is used to enable/disable EPO download.
*
* Parameters:
*                <status>: [IN]
*							0 - disable.
*							1 - enable.
*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*                                      and then call Ql_RIL_Initialize to initialize RIL.               
*****************************************************************/
s32 RIL_GPS_EPO_Enable(u8 status);

/*****************************************************************
* Function:     RIL_GPS_EPO_Aid
* 
* Description:
*               This function is used to inject EPO data to GPS module.
*
* Parameters:	NONE
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*                                      and then call Ql_RIL_Initialize to initialize RIL.               
*****************************************************************/
s32 RIL_GPS_EPO_Aid(void);

/*****************************************************************
* Function:     RIL_GPS_Read_TimeSync_Status
* 
* Description:
*               This function is used to read time synchronization status.
*
* Parameters:	
*				<status> : [OUT] point to the result if readed successfully.
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*                                      and then call Ql_RIL_Initialize to initialize RIL.               
*****************************************************************/
s32 RIL_GPS_Read_TimeSync_Status(u8 *status);

/*****************************************************************
* Function:     RIL_GPS_EPO_Config_APN
* 
* Description:
*               This function is used to config the APN of EPO context.
*
* Parameters:	
*				<apnName> :  [IN] point to the string which indicates the access point name.
*				<apnUserId>: [IN] point to the string which indicates the user name.
*				<apnPasswd>: [IN] point to the string which indicates the password.
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*                                      and then call Ql_RIL_Initialize to initialize RIL.               
*****************************************************************/
s32 RIL_GPS_EPO_Config_APN(u8 *apnName, u8 *apnUserId, u8 *apnPasswd);

#endif	//__RIL_GPS_H__

