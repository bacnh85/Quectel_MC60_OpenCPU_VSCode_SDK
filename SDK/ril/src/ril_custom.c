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
 *   ril_custom.c 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The module has been designed for customer to develop new API functions over RIL.
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
#include "custom_feature_def.h"
#include "ril.h"
#include "ril_util.h"
#include "ql_stdlib.h"


#if defined(__OCPU_RIL_SUPPORT__)

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
CB_RIL_RcvDataFrmCore cb_rcvCoreData = NULL;
void Ql_RIL_RcvDataFrmCore(u8* ptrData, u32 dataLen, void* reserved)
{
    if (cb_rcvCoreData != NULL)
    {
        cb_rcvCoreData(ptrData, dataLen, reserved);
    }
}

//
// Customer may add new API functions definition here.
//
//
//
//
//
#endif

