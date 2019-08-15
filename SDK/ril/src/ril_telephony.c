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
 *   ril_sms.c 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The module implements telephony related APIs.
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
#include "ril_telephony.h "
#include "ql_stdlib.h"

#if defined(__OCPU_RIL_SUPPORT__) 

ST_ComingCallInfo  g_comingCall;

#if defined(__OCPU_RIL_CALL_SUPPORT__)
static s32 Telephony_Dial_AT_handler(char* line, u32 len, void* userdata)
{
    char *head = Ql_RIL_FindLine(line, len, "NO DIALTONE"); 
    if(head)
    {
        (*(s32* )userdata) = CALL_STATE_NO_DIALTONE;
        return  RIL_ATRSP_SUCCESS;
    }
    
    head = Ql_RIL_FindLine(line, len, "BUSY"); 
    if(head)
    {
        (*(s32* )userdata) = CALL_STATE_BUSY;     
        return  RIL_ATRSP_SUCCESS;
    }

    head = Ql_RIL_FindLine(line, len, "NO CARRIER"); 
    if(head)
    {
        (*(s32* )userdata) = CALL_STATE_NO_CARRIER;          
        return  RIL_ATRSP_SUCCESS;
    }

    head = Ql_RIL_FindLine(line, len, "OK");
    if(head)
    {  
        (*(s32* )userdata) = CALL_STATE_OK;
        return  RIL_ATRSP_SUCCESS;
    }

    head = Ql_RIL_FindLine(line, len, "ERROR");
    if(head)
    {  
        (*(s32* )userdata) = CALL_STATE_ERROR;
        return  RIL_ATRSP_FAILED;
    } 

    head = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if(head)
    {
        (*(s32* )userdata) = CALL_STATE_ERROR;
        return  RIL_ATRSP_FAILED;
    }

    return RIL_ATRSP_CONTINUE; //continue wait
}

s32 RIL_Telephony_Dial(u8 type, char* phoneNumber, s32 *result)
{
    char strAT[40];
    if (NULL == phoneNumber)
    {
        return -1;
    }
    Ql_memset(strAT, 0x0, sizeof(strAT));
    Ql_sprintf(strAT, "ATD%s;", phoneNumber);
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), Telephony_Dial_AT_handler, (void* )result, 0);
}

static s32 Telephony_Answer_AT_handler(char* line, u32 len, void* userdata)
{
    char *head;
    head = Ql_RIL_FindLine(line, len, "OK"); 
    if(head)
    {
        (*(s32* )userdata) = CALL_STATE_OK;
        return  RIL_ATRSP_SUCCESS;
    }

    head = Ql_RIL_FindLine(line, len, "NO CARRIER"); 
    if(head)
    {
        (*(s32* )userdata) = CALL_STATE_NO_CARRIER;
        return  RIL_ATRSP_SUCCESS;
    }

    head = Ql_RIL_FindLine(line, len, "ERROR");
    if(head)
    {
        (*(s32* )userdata) = CALL_STATE_ERROR;
        return  RIL_ATRSP_FAILED;
    }

    head = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if(head)
    {
        (*(s32* )userdata) = CALL_STATE_ERROR;
        return  RIL_ATRSP_FAILED;
    }

    return RIL_ATRSP_CONTINUE; //continue wait
}

s32 RIL_Telephony_Answer(s32 *result)
{
    return Ql_RIL_SendATCmd("ATA", 3, Telephony_Answer_AT_handler, result, 0);
}

s32 RIL_Telephony_Hangup(void)
{
    return Ql_RIL_SendATCmd("ATH", 3, NULL, NULL, 0);
}
#endif
#endif
