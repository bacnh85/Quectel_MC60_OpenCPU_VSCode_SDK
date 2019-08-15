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
 *   The module implements network related APIs.
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
#include "ril_network.h"
#include "ril.h"
#include "ril_util.h"
#include "ql_stdlib.h"
#include "ql_trace.h"
#include "ql_error.h"
#include "ql_system.h"
#include "ql_trace.h"

#ifdef __OCPU_RIL_SUPPORT__

/******************************************************************************
* Function:     ATResponse_CREG_Handler
*  
* Description:
*               This function is used to deal with the response of the AT+CREG command.
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
static s32 ATResponse_CREG_Handler(char* line, u32 len, void* userdata)
{
    char *head = Ql_RIL_FindString(line, len, "+CREG:"); //continue wait
    if(head)
    {
        s32 n = 0;
        s32 *state = (s32 *)userdata;
        Ql_sscanf(head,"%*[^ ]%d,%d,%[^\r\n]",&n,state);
        return  RIL_ATRSP_CONTINUE;
    }

   head = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
   if(head)
   {
       return  RIL_ATRSP_SUCCESS;
   }

    head = Ql_RIL_FindLine(line, len, "ERROR");// find <CR><LF>ERROR<CR><LF>, <CR>ERROR<CR>£¬<LF>ERROR<LF>
    if(head)
    {  
        return  RIL_ATRSP_FAILED;
    } 

    head = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if(head)
    {
        return  RIL_ATRSP_FAILED;
    }

    return RIL_ATRSP_CONTINUE; //continue wait
}

/******************************************************************************
* Function:     RIL_NW_GetGSMState
*  
* Description:
*               This function gets the GSM network register state. 
*
* Parameters:    
*               stat:
*                   [out]GPRS State.
* Return:
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*                                      and then call Ql_RIL_Initialize to initialize RIL.
******************************************************************************/
s32  RIL_NW_GetGSMState(s32 *stat)
{
    s32 retRes = -1;
    s32 nStat = 0;
    char strAT[] = "AT+CREG?\0";

    retRes = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATResponse_CREG_Handler, &nStat, 0);
    if(RIL_AT_SUCCESS == retRes)
    {
       *stat = nStat; 
    }
    return retRes;
}

/******************************************************************************
* Function:     ATResponse_CGREG_Handler
*  
* Description:
*               This function is used to deal with the response of the AT+CGREG command.
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
static s32 ATResponse_CGREG_Handler(char* line, u32 len, void* userdata)
{
    char *head = Ql_RIL_FindString(line, len, "+CGREG:"); //continue wait
    if(head)
    {
        s32 n = 0;
        s32 *state = (s32 *)userdata;
        Ql_sscanf(head,"%*[^ ]%d,%d,%[^\r\n]",&n,state);
        return  RIL_ATRSP_CONTINUE;
    }

   head = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
   if(head)
   {  
       return  RIL_ATRSP_SUCCESS;
   }

    head = Ql_RIL_FindLine(line, len, "ERROR");// find <CR><LF>ERROR<CR><LF>, <CR>ERROR<CR>£¬<LF>ERROR<LF>
    if(head)
    {  
        return  RIL_ATRSP_FAILED;
    } 

    head = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if(head)
    {
        return  RIL_ATRSP_FAILED;
    }

    return RIL_ATRSP_CONTINUE; //continue wait
}

/******************************************************************************
* Function:     RIL_NW_GetGPRSState
*  
* Description:
*               This function gets the GPRS network register state. 
*
* Parameters:    
*               stat:
*                   [out]GPRS State.
* Return:  
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*                                      and then call Ql_RIL_Initialize to initialize RIL.
******************************************************************************/
s32  RIL_NW_GetGPRSState(s32 *stat)
{
    s32 retRes = -1;
    s32 nStat = 0;
    char strAT[] = "AT+CGREG?\0";

    retRes = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATResponse_CGREG_Handler, &nStat, 0);
    if(RIL_AT_SUCCESS == retRes)
    {
       *stat = nStat; 
    }
    return retRes;
}


/******************************************************************************
* Function:     ATResponse_CSQ_Handler
*  
* Description:
*               This function is used to deal with the response of the AT+CSQ command.
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
static s32 ATResponse_CSQ_Handler(char* line, u32 len, void* userdata)
{
    ST_CSQ_Reponse *CSQ_Reponse = (ST_CSQ_Reponse*)userdata;

    char *head = Ql_RIL_FindString(line, len, "+CSQ:"); //continue wait
    if(head)
    {
        Ql_sscanf(head,"%*[^ ]%d,%d,%[^\r\n]",&CSQ_Reponse->rssi,&CSQ_Reponse->ber);
        return  RIL_ATRSP_CONTINUE;
    }

    head = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
    if(head)
    {  
        return  RIL_ATRSP_SUCCESS;
    }

    head = Ql_RIL_FindLine(line, len, "ERROR");// find <CR><LF>ERROR<CR><LF>, <CR>ERROR<CR>£¬<LF>ERROR<LF>
    if(head)
    {  
        return  RIL_ATRSP_FAILED;
    } 

    head = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if(head)
    {
        return  RIL_ATRSP_FAILED;
    }

    return RIL_ATRSP_CONTINUE; //continue wait
}

/******************************************************************************
* Function:     RIL_NW_GetSignalQuality
*  
* Description:
*               This function gets the signal quality level and bit error rate. 
*
* Parameters:    
*               rssi:
*                   [out] Signal quality level, 0~31 or 99. 99 indicates module
*                         doesn't register to GSM network.
*                       
*               ber:
*                   [out] The bit error code of signal.
* Return:  
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*                                      and then call Ql_RIL_Initialize to initialize RIL. 
******************************************************************************/
s32  RIL_NW_GetSignalQuality(u32* rssi, u32* ber)
{
    s32 retRes = 0;
    char strAT[] = "AT+CSQ\0";
    ST_CSQ_Reponse pCSQ_Reponse;
    Ql_memset(&pCSQ_Reponse,0, sizeof(pCSQ_Reponse));
    retRes = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT), ATResponse_CSQ_Handler,(void*)&pCSQ_Reponse,0);
    if(RIL_AT_SUCCESS == retRes)
    {
       *rssi = pCSQ_Reponse.rssi; 
       *ber = pCSQ_Reponse.ber;
    }
 
    return retRes;
}

s32  RIL_NW_SetGPRSContext(u8 foregroundContext)
{
    s32 retRes = 0;
    char strAT[20] ;

    Ql_memset(strAT,0x00, sizeof(strAT));
    Ql_sprintf(strAT,"AT+QIFGCNT=%d",foregroundContext);
    retRes = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT), NULL,NULL,0);
    return retRes;
}

s32  RIL_NW_SetAPN(u8 mode, char* apn, char* userName, char* pw)
{
    s32 retRes = 0;
    char strAT[200] ;

    Ql_memset(strAT,0x00, sizeof(strAT));
    if((NULL != apn) && (NULL != userName) && (NULL != pw))
    {
        Ql_sprintf(strAT,"AT+QICSGP=%d,\"%s\"",mode,apn,userName,pw);
    }
    else if((NULL != apn) && (NULL != userName))
    {
        Ql_sprintf(strAT,"AT+QICSGP=%d,\"%s\"",mode,apn,userName);
    }
    else if(NULL != apn)
    {
        Ql_sprintf(strAT,"AT+QICSGP=%d,\"%s\"",mode,apn);
    }
    else
    {
        Ql_sprintf(strAT,"AT+QICSGP=%d",mode);
    }
        
    retRes = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT), NULL,NULL,0);
    return retRes;

}

s32 RIL_NW_GetIpStatusByName(char* ipStsStr, u32 len)
{
    s32 ipSts = IP_INITIAL;
    if (Ql_strncmp(ipStsStr, "IP INITIAL", len) == 0)
    {
        ipSts = IP_INITIAL;
    }
    else if (Ql_strncmp(ipStsStr, "IP START", len) == 0)
    {
        ipSts = IP_START;
    }
    else if (Ql_strncmp(ipStsStr, "IP CONFIG", len) == 0)
    {
        ipSts = IP_CONFIG;
    }
    else if (Ql_strncmp(ipStsStr, "IP IND", len) == 0)
    {
        ipSts = IP_IND;
    }
    else if (Ql_strncmp(ipStsStr, "IP GPRSACT", len) == 0)
    {
        ipSts = IP_GPRSACT;
    }
    else if (Ql_strncmp(ipStsStr, "IP STATUS", len) == 0)
    {
        ipSts = IP_STATUS;
    }
    else if (Ql_strncmp(ipStsStr, "TCP CONNECTING", len) == 0)
    {
        ipSts = TCP_PROCESSING;
    }
    else if (Ql_strncmp(ipStsStr, "UDP CONNECTING", len) == 0)
    {
        ipSts = UDP_PROCESSING;
    }
    else if (Ql_strncmp(ipStsStr, "IP CLOSE", len) == 0)
    {
        ipSts = IP_CLOSE;
    }
    else if (Ql_strncmp(ipStsStr, "CONNECT OK", len) == 0)
    {
        ipSts = CONNECT_OK;
    }
    else if (Ql_strncmp(ipStsStr, "PDP DEACT", len) == 0)
    {
        ipSts = GPRS_CONTEXT_DEACT;
    }
    return ipSts;
}
static s32 ATResponse_IPStatus_Handler(char* line, u32 len, void* userdata)
{
    s32 *result = (s32 *)userdata;
    char *head = Ql_RIL_FindString(line, len, "\r\nSTATE:"); //continue wait
    if(head)
    {
        char str[30] = {0};
        char *p = NULL;
        char *q = NULL; 
        p = head + Ql_strlen("\r\nSTATE:");
        q = Ql_strstr(p,"\r\n");
        if (p)
        {
            Ql_memcpy(str, p, q - p);
        }
        
        *result = RIL_NW_GetIpStatusByName(str, Ql_strlen(str));
        return  RIL_ATRSP_SUCCESS;
    }

    head = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
    if(head)
    {  
       return  RIL_ATRSP_CONTINUE;
    }

    head = Ql_RIL_FindLine(line, len, "ERROR");// find <CR><LF>ERROR<CR><LF>, <CR>ERROR<CR>£¬<LF>ERROR<LF>
    if(head)
    {  
        *result = IP_INITIAL;
        return  RIL_ATRSP_FAILED;
    } 

    return RIL_ATRSP_FAILED; //not supported
}

s32  RIL_NW_GetIPStatus(void)
{
    s32 retRes;
    s32 ip_status = IP_INITIAL;
    retRes = Ql_RIL_SendATCmd("AT+QISTAT\0", 10, ATResponse_IPStatus_Handler, &ip_status, 0);
    if (RIL_AT_SUCCESS == retRes)
    {
        return ip_status;
    }else{
        return retRes;
    }
}
//
// This function activates pdp context for AT command mode.
s32  RIL_NW_OpenPDPContext(void)
{
    s32 retRes;
    char strAT[20];
    u32  strATLen;
    
    strATLen = Ql_sprintf(strAT, "AT+QIREGAPP\0");
    retRes = Ql_RIL_SendATCmd(strAT, strATLen ,NULL, NULL, 0);
    if (RIL_AT_SUCCESS != retRes)
    {
        return retRes;
    }
    
    Ql_Sleep(100);  // Wait for the QISTAT state changing to "IP start"

    // AT+QIACT
    Ql_memset(strAT, 0, sizeof(strAT));
    strATLen = Ql_sprintf(strAT, "AT+QIACT\0");
    retRes = Ql_RIL_SendATCmd(strAT, strATLen, NULL, NULL, 0);
    return retRes;
}

static s32 ATRsp_QIDEACT_Hdlr(char* line, u32 len, void* userData)
{
    if (Ql_RIL_FindLine(line, len, "DEACT OK"))
    {
        return  RIL_ATRSP_SUCCESS;
    } 
    else if (Ql_RIL_FindLine(line, len, "ERROR"))
    {
        return  RIL_ATRSP_FAILED;
    }
    return RIL_ATRSP_CONTINUE; //continue wait
}

s32 RIL_NW_ClosePDPContext(void)
{
    char strAT[20] = "AT+QIDEACT\0";
    return Ql_RIL_SendATCmd("AT+QIDEACT\n", Ql_strlen(strAT), ATRsp_QIDEACT_Hdlr, NULL, 0);
}

static s32 ATRsp_COPS_Handler(char* line, u32 len, void* param)
{
    char* pStr = (char *)param;
    char* pHead = Ql_RIL_FindString(line, len, "+COPS:"); //continue wait
    if (pHead)
    {
        char str[100] = {0};
        char *p = NULL;
        char *q = NULL; 
        p = pHead + Ql_strlen("+COPS: ");
        q = Ql_strstr(p, "\"");
        if (p)
        {// the response is like: +COPS: 0,0,"CHINA MOBILE"
            p = q + 1;
            q = Ql_strstr(p, "\"");
            if (q != NULL)
            {
                Ql_memcpy(pStr, p, q - p);
                pStr[q - p] = '\0';
            }
        }
        else
        {//  the response is like +COPS: 0
            *pStr = '\0';
        }
        return  RIL_ATRSP_SUCCESS;
    }

   pHead = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
   if (pHead)
   {  
       return  RIL_ATRSP_SUCCESS;
   }

    pHead = Ql_RIL_FindLine(line, len, "ERROR");// find <CR><LF>ERROR<CR><LF>, <CR>ERROR<CR>£¬<LF>ERROR<LF>
    if (pHead)
    {  
        return  RIL_ATRSP_FAILED;
    } 

    pHead = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if (pHead)
    {
        return  RIL_ATRSP_FAILED;
    }

    return RIL_ATRSP_CONTINUE; //continue wait
}

s32 RIL_NW_GetOperator(char* operator)
{
    char strAT[] = "AT+COPS?\0";
    if (NULL == operator)
    {
        return RIL_AT_INVALID_PARAM;
    }
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_COPS_Handler,(void*)operator, 0);
}

#endif  //__OCPU_RIL_SUPPORT__

