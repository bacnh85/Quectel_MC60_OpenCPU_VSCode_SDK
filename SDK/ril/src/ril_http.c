/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Quectel Co., Ltd. 2015
*
*****************************************************************************/
/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ril_ftp.c 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The module implements HTTP related APIs.
 *
 * Author:
 * -------
 *   Created by: Stanley.YONG  24Jun2015
 * -------
 *
 *============================================================================
 *             HISTORY
 *----------------------------------------------------------------------------
 * 
 ****************************************************************************/
#include "custom_feature_def.h"
#include "ql_type.h"
#include "ql_stdlib.h"
#include "ql_trace.h"
#include "ql_error.h"
#include "ql_common.h"
#include "ql_system.h"
#include "ql_memory.h"
#include "ql_uart.h"
#include "ril.h"
#include "ril_util.h"
#include "ril_network.h"
#include "ril_http.h"

#ifdef __OCPU_RIL_SUPPORT__

#define RIL_HTTP_DEBUG_ENABLE 0
#if RIL_HTTP_DEBUG_ENABLE > 0
#define RIL_HTTP_DEBUG_PORT  UART_PORT2
static char DBG_Buffer[100];
#define RIL_HTTP_DEBUG(BUF,...)  QL_TRACE_LOG(RIL_HTTP_DEBUG_PORT,BUF,100,__VA_ARGS__)
#else
#define RIL_HTTP_DEBUG(BUF,...) 
#endif

static Enum_HTTP_Ation m_httpAction = HTTP_ACTION_IDLE;
static s32 ATRsp_QHTTP_Handler(char* line, u32 len, void* param);

char* http_url_addr = NULL;
u16   http_url_addr_len = 0;
s32 RIL_HTTP_SetServerURL(char* strURL, u16 len)
{
    s32  retRes;
    s32  errCode = RIL_AT_FAILED;
    char strAT[30];

    if (!strURL)
    {
        return RIL_AT_INVALID_PARAM;
    }
    http_url_addr = strURL;
    http_url_addr_len = len;
    m_httpAction = HTTP_ACTION_SETRUL;
    Ql_sprintf(strAT, "AT+QHTTPURL=%d,%d\0", len, 120);
    retRes = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QHTTP_Handler, &errCode, 0);
    if (retRes != RIL_AT_SUCCESS)
    {
        if (RIL_AT_FAILED == errCode)
        {
            return retRes;
        } else {
            return errCode;
        }
    }
    return retRes; 
}

s32 RIL_HTTP_RequestToGet(u32 timeout)
{
    s32  retRes;
    s32  errCode = RIL_AT_FAILED;
    char strAT[30];

    m_httpAction = HTTP_ACTION_GET_REQ;
    Ql_sprintf(strAT, "AT+QHTTPGET=%d\0", timeout);
    retRes = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QHTTP_Handler, &errCode, 0);
    if (retRes != RIL_AT_SUCCESS)
    {
        if (RIL_AT_FAILED == errCode)
        {
            return retRes;
        } else {
            return errCode;
        }
    }
    return retRes; 
}

char* http_post_msg = NULL;
u16   http_post_msg_len = 0;
s32 RIL_HTTP_RequestToPost(char* strPostMsg, u16 len)
{
    s32  retRes;
    s32  errCode = RIL_AT_FAILED;
    char strAT[30];

    if (!strPostMsg)
    {
        return RIL_AT_INVALID_PARAM;
    }
    http_post_msg = strPostMsg;
    http_post_msg_len = len;
    m_httpAction = HTTP_ACTION_POST_REQ;
    Ql_sprintf(strAT, "AT+QHTTPPOST=%d,120,120\0", len);
    retRes = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QHTTP_Handler, &errCode, 0);
    if (retRes != RIL_AT_SUCCESS)
    {
        if (RIL_AT_FAILED == errCode)
        {
            return retRes;
        } else {
            return errCode;
        }
    }
    return retRes; 
}

s32 RIL_HTTP_ReadResponse(u32 timeout, CB_RIL_RcvDataFrmCore cb_rcvData)
{
    s32  retRes;
    s32  errCode = RIL_AT_FAILED;
    char strAT[30];
    extern CB_RIL_RcvDataFrmCore cb_rcvCoreData;

    if (!cb_rcvData)
    {
        return RIL_AT_INVALID_PARAM;
    }
    cb_rcvCoreData = cb_rcvData;
    m_httpAction = HTTP_ACTION_READ_RSP;
    Ql_sprintf(strAT, "AT+QHTTPREAD=%d\0", timeout);
    retRes = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QHTTP_Handler, &errCode, 0);
    if (retRes != RIL_AT_SUCCESS)
    {
        if (RIL_AT_FAILED == errCode)
        {
            return retRes;
        } else {
            return errCode;
        }
    }
    return retRes; 
}

CB_HTTP_DwnldFile callback_http_dwnld = NULL;
s32 RIL_HTTP_DownloadFile(char* filePath, u32 size, CB_HTTP_DwnldFile cb)
{
    s32  retRes;
    char* strAT = NULL;

    callback_http_dwnld = cb;
    m_httpAction = HTTP_ACTION_DOWNLOAD_FILE;
    strAT = (char*)Ql_MEM_Alloc(Ql_strlen(filePath) + 20);
    Ql_sprintf(strAT, "AT+QHTTPDL=\"%s\",%d\0", filePath, size);
    retRes = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL, NULL, 0);
    Ql_MEM_Free(strAT);
    return retRes; 
}

static s32 ATRsp_QHTTP_Handler(char* line, u32 len, void* param)
{
    char* pHead = NULL;
    RIL_HTTP_DEBUG(DBG_Buffer, "RCV:%s, len:%d, m_httpAction:%d\r\n", line, len, m_httpAction);
    pHead = Ql_RIL_FindLine(line, len, "CONNECT");
    if (pHead)
    {
        if (HTTP_ACTION_SETRUL == m_httpAction)
        {
            Ql_RIL_WriteDataToCore((u8*)http_url_addr, http_url_addr_len);
        }
        else if (HTTP_ACTION_POST_REQ == m_httpAction)
        {
            RIL_HTTP_DEBUG(DBG_Buffer, "Post msg:len=%d, msg:%s\r\n", http_post_msg_len, http_post_msg);
            Ql_RIL_WriteDataToCore((u8*)http_post_msg, http_post_msg_len);
        }
        else if (HTTP_ACTION_READ_RSP == m_httpAction)
        {
            RIL_HTTP_DEBUG(DBG_Buffer, "<Enter data mode>\r\n");
        }
        return RIL_ATRSP_CONTINUE;  // wait for OK
    }
    pHead = Ql_RIL_FindLine(line, len, "OK");
    if (pHead)
    {
        return RIL_ATRSP_SUCCESS;
    }
    pHead = Ql_RIL_FindLine(line, len, "ERROR");
    if (pHead)
    {
        if (param != NULL)
        {
            *((s32*)param) = RIL_AT_FAILED;
        }
        return RIL_ATRSP_FAILED;
    }
    pHead = Ql_RIL_FindString(line, len, "+CME ERROR:");
    if (pHead)
    {
        if (param != NULL)
        {
            Ql_sscanf(line, "%*[^: ]: %d[^\r\n]", (s32*)param);
        }
        return RIL_ATRSP_FAILED;
    }
    return RIL_ATRSP_CONTINUE;  // Just wait for the specified results above
}

#endif  //__OCPU_RIL_SUPPORT__

