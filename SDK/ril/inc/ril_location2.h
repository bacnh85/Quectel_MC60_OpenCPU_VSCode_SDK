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
 *   ril_location2.h 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The module implements QLBS related APIs.
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
#ifndef __RIL_LOCATION2_H__
#define __RIL_LOCATION2_H__

#include "custom_feature_def.h"
#include "ql_type.h"
#ifdef __OCPU_RIL_QLBS_SUPPORT__

#define MAX_TOKEN_LENGTH           16+1
#define MAX_SERVER_NAME_LENGTH     128
#define TIME_STRING_LENGTH         20


typedef enum
{
   ASYNCH_MODE = 0,       //>Selection of positioning interface, the default interface is use to QUECTEL server
   TIMEOUT,
   SERVER_ADDRESS,        //>The maximum waiting time for data from server
   TOKEN,                 //>The user-defined address and port for QuecLocator.
   TIMEUPDATE,            //>the value of identification required for QUECTEL positioning interface
   WITHTIME               //>the value of identification required for Gaode positioning interface,
}Enum_Qlbs_Cfg_Flag;

typedef enum
{
   SYNC_MODE = 0,                 //> Synchronous mode 
   ASYNC_MODE 	                  //> Asynchronous mode
   
}Enum_Qlbs_Asynch_Mode;

typedef enum
{
   DO_NOT_UPDATE_TIME2RTC = 0,    //> Do not update the time to RTC
   UPDATE_TIME2RTC 	              //> Update the time to RTC
   
}Enum_Qlbs_Update_Mode;

typedef enum
{
   DO_NOT_OUTPUT_TIME = 0,        //> Do not output the time
   OUTPUT_TIME 	                  //> Output the time
   
}Enum_Qlbs_Time_Mode;

typedef struct
{
    Enum_Qlbs_Asynch_Mode asynch_mode;
    s32 timeout;
    u8 server_name[MAX_SERVER_NAME_LENGTH];
	u8 token_value[MAX_TOKEN_LENGTH];
    Enum_Qlbs_Update_Mode update_mode;
    Enum_Qlbs_Time_Mode time_mode;
} ST_Qlbs_Cfg_Para;

typedef struct{
    float longitude;
    float latitude;
    u8 time[TIME_STRING_LENGTH];
}ST_Lbs_LocInfo;

typedef void(*CB_LocInfo)(s32 result,ST_Lbs_LocInfo* loc_info);

/*****************************************************************
* Function:     RIL_QLBS_Cfg
* 
* Description:
*               This function is used to wifi positioning configure.
*
* Parameters:	
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
s32 RIL_QLBS_Cfg(Enum_Qlbs_Cfg_Flag cfg_flag,ST_Qlbs_Cfg_Para*  cfg_para);


/*****************************************************************
* Function:     RIL_QLBS_Loc
* 
* Description:
*               This function is used to obtain position.
*
* Parameters:	
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
s32 RIL_QLBS_Loc(CB_LocInfo cb_qlbsloc);
#endif 
#endif
