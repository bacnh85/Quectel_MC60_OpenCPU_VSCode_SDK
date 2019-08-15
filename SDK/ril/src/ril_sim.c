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
 *   ril_network.c 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The module implements sim card related APIs.
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
#include "ril_sim.h"
#include "ql_stdlib.h"
#include "ql_trace.h"
#include "ql_error.h"
#include "ql_system.h"
#include "ql_trace.h"

#ifdef __OCPU_RIL_SUPPORT__

s32 RIL_SIM_GetSimStateByName(char* simStat, u32 len)
{
    s32 ss = SIM_STAT_UNSPECIFIED;
    if (Ql_strncmp(simStat, "READY", len) == 0)
    {
        ss = SIM_STAT_READY;
    }
    else if (Ql_strncmp(simStat, "NOT INSERTED", len) == 0)
    {
        ss = SIM_STAT_NOT_INSERTED;
    }
    else if (Ql_strncmp(simStat, "SIM PIN", len) == 0)
    {
        ss = SIM_STAT_PIN_REQ;
    }
    else if (Ql_strncmp(simStat, "SIM PUK", len) == 0)
    {
        ss = SIM_STAT_PUK_REQ;
    }
    else if (Ql_strncmp(simStat, "PH-SIM PIN", len) == 0)
    {
        ss = SIM_STAT_PH_PIN_REQ;
    }
    else if (Ql_strncmp(simStat, "PH-SIM PUK", len) == 0)
    {
        ss = SIM_STAT_PH_PUK_REQ;
    }
    else if (Ql_strncmp(simStat, "SIM PIN2", len) == 0)
    {
        ss = SIM_STAT_PIN2_REQ;
    }
    else if (Ql_strncmp(simStat, "SIM PUK2", len) == 0)
    {
        ss = SIM_STAT_PUK2_REQ;
    }
    else if (Ql_strncmp(simStat, "SIM BUSY", len) == 0)
    {
        ss = SIM_STAT_BUSY;
    }
    else if (Ql_strncmp(simStat, "NOT READY", len) == 0)
    {
        ss = SIM_STAT_NOT_READY;
    }
    return ss;
}
static s32 RIL_SIM_GetSimStateByErrCode(s32 errCode)
{
    s32 ss;
    switch (errCode)
    {
    case 10:
        ss = SIM_STAT_NOT_INSERTED;
        break;
    case 11:
        ss = SIM_STAT_PIN_REQ;
        break;
    case 12:
        ss = SIM_STAT_PUK_REQ;
        break;
    case 13:
    case 15:
    case 16:
        ss = SIM_STAT_UNSPECIFIED;
        break;
    case 14:
        ss = SIM_STAT_BUSY;
        break;
    case 17:
        ss = SIM_STAT_PIN2_REQ;
        break;
    case 18:
        ss = SIM_STAT_PUK2_REQ;
        break;
    default:
        ss = SIM_STAT_UNSPECIFIED;
        break;
    }
    return ss;
}


/******************************************************************************
* Function:     ATResponse_CPIN_Handler
*  
* Description:
*               This function is used to deal with the response of the AT+CPIN command.
*
* Parameters:    
*                line:  
*                    [in]The address of the string.
*                len:   
*                    [in]The length of the string.
*                userdata: 
*                    [out]Used to transfer the customer's parameter.
*                       
* Return:  
*               RIL_ATRSP_SUCCESS, indicates AT executed successfully..
*               RIL_ATRSP_FAILED, indicates AT executed failed. 
*               RIL_ATRSP_CONTINUE,indicates continue to wait for the response
*               of the AT command.
* Notes:
*               1.Can't send any new AT commands in this function.
*               2.RIL handle the AT response line by line, so this function may 
*                 be called multiple times.
******************************************************************************/
static s32 ATResponse_CPIN_Handler(char* line, u32 len, void* userdata)
{
    s32 *result = (s32 *)userdata;
    char *head = Ql_RIL_FindString(line, len, "+CPIN:"); //continue wait
    if(head)
    {
        char str[100] = {0};
        char *p = NULL;
        char *q = NULL; 
        p = head + Ql_strlen("+CPIN: ");
        q = Ql_strstr(p,"\r\n");
        if (p)
        {
            Ql_memcpy(str, p, q - p);
        }
        
        *result = RIL_SIM_GetSimStateByName(str,Ql_strlen(str));
        return  RIL_ATRSP_SUCCESS;
    }

   head = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
   if(head)
   {  
       return  RIL_ATRSP_SUCCESS;
   }

    head = Ql_RIL_FindLine(line, len, "ERROR");// find <CR><LF>ERROR<CR><LF>, <CR>ERROR<CR>£¬<LF>ERROR<LF>
    if(head)
    {  
        *result = SIM_STAT_UNSPECIFIED;
        return  RIL_ATRSP_FAILED;
    } 

    head = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if(head)
    {
        s32 err = 0;
        Ql_sscanf(head,"%*[^ ]%d,%[^\r\n]",&err);
        *result = RIL_SIM_GetSimStateByErrCode(err);
        return  RIL_ATRSP_FAILED;
    }

    return RIL_ATRSP_CONTINUE; //continue wait
}

/******************************************************************************
* Function:     RIL_SIM_GetSimState
*  
* Description:
*                This function gets the state of SIM card. 
*
* Parameters:    
*               stat:
*                   [out]SIM card State.
* Return:  
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*                                      and then call Ql_RIL_Initialize to initialize RIL.
******************************************************************************/
s32  RIL_SIM_GetSimState(s32 *stat)
{
    s32 retRes = -1;
    s32 nStat = 0;
    char strAT[] = "AT+CPIN?\0";

    retRes = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATResponse_CPIN_Handler, &nStat, 0);
    if(RIL_AT_SUCCESS == retRes)
    {
       *stat = nStat; 
    }
    return retRes;
}

static s32 ATRsp_IMSI_Handler(char* line, u32 len, void* param)
{
    char* pHead = NULL;
    pHead = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
    if (pHead)
    {  
        return RIL_ATRSP_SUCCESS;
    }

    pHead = Ql_RIL_FindLine(line, len, "ERROR");// find <CR><LF>ERROR<CR><LF>, <CR>ERROR<CR>£¬<LF>ERROR<LF>
    if (pHead)
    {  
        return RIL_ATRSP_FAILED;
    } 

    pHead = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if (pHead)
    {
        return RIL_ATRSP_FAILED;
    }
    Ql_memcpy((char*)param, line, len - 2); // <imsi number>\r\n
    return RIL_ATRSP_CONTINUE; //continue wait
}
s32 RIL_SIM_GetIMSI(char* imsi)
{
    char strAT[] = "AT+CIMI\0";
    if (NULL == imsi)
    {
        return RIL_AT_INVALID_PARAM;
    }
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_IMSI_Handler,(void*)imsi, 0);
}

static s32 ATRsp_CCID_Handler(char* line, u32 len, void* param)
{
    char* pHead = Ql_RIL_FindString(line, len, "+CCID:");
    if (pHead)
    {
        Ql_sscanf(pHead,"%*[^: \"]: \"%[^\"\r\n]", (char*)param);
        return  RIL_ATRSP_CONTINUE; // wait for OK
    }

    pHead = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
    if (pHead)
    {  
        return RIL_ATRSP_SUCCESS;
    }

    pHead = Ql_RIL_FindLine(line, len, "ERROR");// find <CR><LF>ERROR<CR><LF>, <CR>ERROR<CR>£¬<LF>ERROR<LF>
    if (pHead)
    {  
        return RIL_ATRSP_FAILED;
    } 

    pHead = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if (pHead)
    {
        return RIL_ATRSP_FAILED;
    }

    return RIL_ATRSP_CONTINUE; //continue wait
}
s32 RIL_SIM_GetCCID(char* ccid)
{
    char strAT[] = "AT+CCID\0";
    if (NULL == ccid)
    {
        return RIL_AT_INVALID_PARAM;
    }
    return Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT), ATRsp_CCID_Handler,(void*)ccid, 0);
}

#endif  //__OCPU_RIL_SUPPORT__

