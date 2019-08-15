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
 *   ril.h 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The file intends for OpenCPU RIL. Declare the APIs for RIL initialization,
 *   send AT command and register URC callback.
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
#ifndef __RIL_H__
#define __RIL_H__
#include "ql_type.h"

/******************************************************************************
* Module URC definition
******************************************************************************/
typedef enum {
    /****************************************/
    /* System URC definition begin          */
    /****************************************/
    URC_SYS_BEGIN = 0,
    URC_SYS_INIT_STATE_IND,     // Indication for module initialization state during boot stage, see also "Enum_SysInitState".
    URC_SIM_CARD_STATE_IND,     // Indication for SIM card state (state change), see also "Enum_SIMState".
    URC_GSM_NW_STATE_IND,       // Indication for GSM  network state (state change), see also "Enum_NetworkState".
    URC_GPRS_NW_STATE_IND,      // Indication for GPRS network state (state change), see also "Enum_NetworkState".
    URC_CFUN_STATE_IND,         // Indication for CFUN state, see also "Enum_CfunState".
    URC_COMING_CALL_IND,        // Indication for coming call.
    URC_CALL_STATE_IND,         // Indication for call state (state change), see also "Enum_CallState".
    URC_NEW_SMS_IND,            // Indication for new short message.
    URC_MODULE_VOLTAGE_IND,     // Indication for abnormal voltage of module supply power.
	URC_ALARM_RING_IND,		    // Indication for clock alarm
    URC_SYS_END = 100,
    /*****************************************/
    /* System URC definition end             */
    /*****************************************/
    URC_END
}Enum_URCType;

/******************************************************************************
* Define URC struct
******************************************************************************/
typedef struct {
    u32   urcType;
    void* urcResult;
}ST_URC, *PST_URC;

/******************************************************************************
* Define URC Handle struct
******************************************************************************/
#define  RIL_MAX_URC_PREFIX_LEN    50

typedef struct {
    char  keyword[RIL_MAX_URC_PREFIX_LEN];
    void  (* handler)(const char* strURC, void* reserved);
}ST_URC_HDLENTRY;

/******************************************************************************
* Define the return value of AT callback function.
* RIL_ATRSP_FAILED,  means that the AT command executed failed. 
* RIL_ATRSP_SUCCESS, means that the AT command executed successfully.
* RIL_ATRSP_CONTINUE,means that continue to wait for the response of AT command.
* if return RIL_ATRSP_CONTINUE,the "Ql_RIL_SendATCmd" function will be blocked, 
* until you return to RIL_ATRSP_FAILED or RIL_ATRSP_SUCCESS.
******************************************************************************/
typedef enum {
    RIL_ATRSP_FAILED   =  -1,
    RIL_ATRSP_SUCCESS  =   0,
    RIL_ATRSP_CONTINUE =   1,
}Enum_ATRspError;

/******************************************************************************
* Define the return value of Ql_RIL_SendATCmd.
* RIL_AT_SUCCESS,send AT successfully.
* RIL_AT_FAILED, send AT failed.
* RIL_AT_TIMEOUT,send AT timeout.
* RIL_AT_BUSY,   sending AT.
* RIL_AT_INVALID_PARAM, invalid input parameter.
* RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
* and then call Ql_RIL_Initialize to initialize RIL.
******************************************************************************/
typedef enum {
    RIL_AT_SUCCESS           =  0,
    RIL_AT_FAILED            = -1,
    RIL_AT_TIMEOUT           = -2,
    RIL_AT_BUSY              = -3,
    RIL_AT_INVALID_PARAM     = -4,
    RIL_AT_UNINITIALIZED     = -5,
}Enum_ATSndError;


/******************************************************************************
* Function:     Ql_RIL_Initialize
*  
* Description:
*               This function initializes RIl-related functions.
*               Set the initial AT commands, please refer to "m_InitCmds".
* Parameters:    
*               None.
* Return:  
*               None.
******************************************************************************/
extern void Ql_RIL_Initialize(void);


/******************************************************************************
* Function:     Ql_RIL_SendATCmd
*  
* Description:
*                This function implements sending AT command with the result  
*                being returned synchronously.The response of the AT command 
*                will be reported to the callback function,you can get the results
*                you want in it.
*
* Parameters:    
*                atCmd:     
*                     [in]AT command string.
*                atCmdLen:  
*                     [in]The length of AT command string.
*                atRsp_callBack: 
*                     [in]Callback function for handle the response of the AT command.
*                userData:  
*                     [out]Used to transfer the customer's parameter.
*                timeOut:   
*                     [in]Timeout for the AT command, unit in ms. If set it to 0,
*                         RIL uses the default timeout time (3min).
*
* Return:  
*                RIL_AT_SUCCESS,succeed in executing AT command, and the response is OK.
*                RIL_AT_FAILED, fail to execute AT command, or the response is ERROR.
*                               you may call Ql_RIL_AT_GetErrCode() to get the 
*                               specific error code.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*                                      and then call Ql_RIL_Initialize to initialize RIL.
******************************************************************************/
typedef s32 (*Callback_ATResponse)(char* line, u32 len, void* userData);
extern s32 Ql_RIL_SendATCmd(char*  atCmd, u32 atCmdLen, Callback_ATResponse atRsp_callBack, void* userData, u32 timeOut);


/******************************************************************************
* Function:     Ql_RIL_AT_GetErrCode
*  
* Description:
*                This function retrieves the specific error code after executing AT failed.
*
* Parameters:    
*                None.
*
* Return:  
*                0, succeed in executing AT
*                -1, fails to execute AT
*                other value indicates the specific error code when executing AT failed.
*                you can usually find the meaning of error code in ATC document.
******************************************************************************/
extern s32 Ql_RIL_AT_GetErrCode(void);


/*****************************************************************
* Function:     Ql_RIL_WriteDataToCore 
* 
* Description:
*               This function is used to send data to the core system.   
* Parameters:
*               [in]data:
*                       Pointer to data to be send
*               [in]writeLen:
*                       The length of the data to be written
*
* Return:        
*               If >= 0, indicates success, and the return value 
*                        is the length of actual write data length
*               If <  0, indicates failure to write      
*
*****************************************************************/
extern s32 Ql_RIL_WriteDataToCore (u8* data, u32 writeLen);

/*****************************************************************
* Function:     Ql_RIL_RcvDataFrmCore 
* 
* Description:
*               This function is used to receive data from the core 
*               system when programing some AT commands that need to
*               response with much data, such as "AT+QHTTPREAD". 
*
*               This function is implemented in ril_custom.c. Developer 
*               don't need to call this function. Under mode, this function
*               will be invoken when data coming automatically.
*
*               The CB_RIL_RcvDataFrmCore is defined for ustomer to define
*               the callback function for each AT command.
* Parameters:
*               [in]ptrData:
*                       Pointer to the data to be received.
*
*               [in]dataLen:
*                       The length to be received.
*
*               [in]reserved:
*                       Not used.
*
* Return:        
*               None.     
*
*****************************************************************/
typedef void (*CB_RIL_RcvDataFrmCore)(u8* ptrData, u32 dataLen, void* reserved);
void Ql_RIL_RcvDataFrmCore(u8* ptrData, u32 dataLen, void* reserved);


#define NUM_ELEMS(x) (sizeof(x)/sizeof(x[0]))
#define FREE_MEM(x) {if (x) {void* ptr = (void*)(x); Ql_MEM_Free(ptr); ptr = NULL;}}

#endif  //__RIL_H__

