/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Quectel Co., Ltd. 2017
*
*****************************************************************************/
/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ql_gnss.h 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *  GNSS APIs defines.
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
 

#ifndef __QL_GNSS_H__
#define __QL_GNSS_H__
#include "ql_common.h"
#include "ql_system.h"

#define MAX_NMEA_LEN		255

typedef enum {
    NO_NMEA_EN  = 0x00000000,
	RMC_EN      = 0x00000001,
	VTG_EN      = 0x00000002,
	GGA_EN      = 0x00000004,
	GSA_EN      = 0x00000008,
	GSV_EN      = 0x00000010,
	GLL_EN      = 0x00000020,
	EPE_EN      = 0x00000040,
	ECEF_EN     = 0x00000080,
	PZ90_EN     = 0x00000100,
	VEL_EN      = 0x00000200,
	GEO_EN      = 0x00000400,
	JAM_EN      = 0x00000800,
	RLM_EN      = 0x00001000,
	
	ALL_NMEA_EN = 0xFFFFFFFF
}Enum_NMEAFlag;

typedef struct 
{ 
    double lat;     // Latitude
    double lon;     // Longitude
}st_pos;

/*****************************************************************
* Function:     Ql_GNSS_PowerOn 
* 
* Description:
*               This function is used to power on the GNSS and register the callback function for the
*               specified NMEA sentences. 
*               NMEA callback function used for receiving the NMEA sentences from CORE by parameters
*               of nmea_buff and len.
*               NOTE: 
*                   The stack size of task calling this function shouldn't less than 2kB.
*                   This function should be called in pairs with Ql_GNSS_PowerDown.
*
* Parameters:
*               [in]nmea_type:   
*                       NMEA types be enabled to output which can multiselect by OR the bit mask. 
*                       For example, RMC_EN | GGA_EN will enable RMC and GGA sentences to output.
*               [in]callback_nmea:     
*                       The pointer of the NMEA call back function.
*               [in]customizePara: 
*                       Customize parameter, if not use just set to NULL.    
*
* Return:        
*               QL_RET_OK indicates success;
*               Ql_RET_ERR_MEM_FULL indicates malloc memory failed.
*               QL_RET_ERR_GNSS_OPERATION_FAILED indicates operation failure when power on or power down.
*
*****************************************************************/
typedef void (*CallBack_NMEA_Hdlr)(u8 *nmea_buff, u16 len, void *customizePara);
s32 Ql_GNSS_PowerOn(Enum_NMEAFlag nmea_type, CallBack_NMEA_Hdlr callback_nmea,void * customizePara);

/*****************************************************************
* Function:     Ql_GNSS_PowerDown 
* 
* Description:
*               This function is used to power down the GNSS.
*               NOTE: 
*                   This function should be called in pairs with Ql_GNSS_PowerOn.
* Parameters:
*               None.
* Return:        
*               QL_RET_OK indicates success;
*               QL_RET_ERR_GNSS_OPERATION_FAILED indicates operation failure when power on or power down.
*
*****************************************************************/
s32 Ql_GNSS_PowerDown(void);

#endif  // End-of __QL_GNSS_H__

