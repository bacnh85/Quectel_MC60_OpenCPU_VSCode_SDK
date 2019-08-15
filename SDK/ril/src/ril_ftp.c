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
 *   ril_ftp.c 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The module defines the information, and APIs related to RIL.
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
#include "ql_type.h"
#include "ql_stdlib.h"
#include "ql_trace.h"
#include "ql_error.h"
#include "ql_common.h"
#include "ql_system.h"
#include "ql_uart.h"
#include "ril.h"
#include "ril_util.h"
#include "ril_ftp.h"
#include "ril_network.h"

#ifdef __OCPU_RIL_SUPPORT__ 

#define RIL_FTP_DEBUG_ENABLE 0
#if RIL_FTP_DEBUG_ENABLE > 0
#define RIL_FTP_DEBUG_PORT  UART_PORT1
static char DBG_Buffer[100];
#define RIL_FTP_DEBUG(BUF,...)  QL_TRACE_LOG(RIL_FTP_DEBUG_PORT,BUF,100,__VA_ARGS__)
#else
#define RIL_FTP_DEBUG(BUF,...) 
#endif


extern CallBack_Ftp_Upload FtpPut_IND_CB;
extern CallBack_Ftp_Download FtpGet_IND_CB;
static s32 ATResponse_FTP_handler_Common(char* line, u32 len, void* userdata)
{
    ST_AT_ftpParam *ftpParam = (ST_AT_ftpParam *)userdata;
    char *head = Ql_RIL_FindString(line, len, ftpParam->prefix); //continue wait
    if(head)
    {
            char strTmp[10];
            char* p1 = NULL;
            char* p2 = NULL;
            Ql_memset(strTmp, 0x0, sizeof(strTmp));
            p1 = Ql_strstr(head, ":");
            p2 = Ql_strstr(p1 + 1, "\r\n");
            if (p1 && p2)
            {
                //Ql_memcpy(strTmp, p1 + 2, p2 - p1 - 2);
                Ql_memcpy(strTmp, p1 + 1, p2 - p1 - 1);
                ftpParam->data= Ql_atoi(strTmp);
            }
        return  RIL_ATRSP_SUCCESS;
    }
    head = Ql_RIL_FindLine(line, len, "OK");
    if(head)
    {  
        return  RIL_ATRSP_CONTINUE;  
    }
    head = Ql_RIL_FindLine(line, len, "ERROR");
    if(head)
    {  
        return  RIL_ATRSP_FAILED;
    }
    head = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if(head)
    {
        return  RIL_ATRSP_FAILED;
    }
    head = Ql_RIL_FindString(line, len, "+CMS ERROR:");//fail
    if(head)
    {
        return  RIL_ATRSP_FAILED;
    }
    return RIL_ATRSP_CONTINUE; //continue wait
}

s32 RIL_FTP_QFTPOPEN(u8* hostName, u32 port,u8* userName,u8* password, bool mode) 
{
    s32 ret = RIL_AT_SUCCESS;
    ST_AT_ftpParam ftpParam;
    char strAT[200];

    if(NULL != userName)
    {
        Ql_memset(strAT, 0, sizeof(strAT));
        Ql_sprintf(strAT, "AT+QFTPUSER=\"%s\"\n", userName);
        ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),NULL,NULL,0);
        RIL_FTP_DEBUG(DBG_Buffer,"<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
        if (RIL_AT_SUCCESS != ret)
        {
            return ret;
        }
    }

    if(NULL != password)
    {
        Ql_memset(strAT, 0, sizeof(strAT));
        Ql_sprintf(strAT, "AT+QFTPPASS=\"%s\"\n", password);
        ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),NULL,NULL,0);
        RIL_FTP_DEBUG(DBG_Buffer,"<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
        if (RIL_AT_SUCCESS != ret)
        {
            return ret;
        }
    }

    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QFTPCFG=1,%d\n", mode);
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),NULL,NULL,0);
    RIL_FTP_DEBUG(DBG_Buffer,"<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if (RIL_AT_SUCCESS != ret)
    {
        return ret;
    }
	
	Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QFTPCFG=6,1\n");
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),NULL,NULL,0);
    RIL_FTP_DEBUG(DBG_Buffer,"<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if (RIL_AT_SUCCESS != ret)
    {
        return ret;
    }

#if 1   //AT+QIREGAPP is not necessary in RDA platform, just verify QISTAT
    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QIREGAPP\n");
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),NULL,NULL,0);
    RIL_FTP_DEBUG(DBG_Buffer,"<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if (RIL_AT_SUCCESS != ret)
    {
        return ret;
    }
#else
{
    u8 attempts = 0;
    // 3 attempts for query QISTAT
    do
    {
        ret = RIL_NW_GetIPStatus();
        RIL_FTP_DEBUG(DBG_Buffer, "<-- IP status: %d -->\r\n", ret);
        if (IP_INITIAL == ret)
        {
            break;
        }
        attempts++;
        Ql_Sleep(500);
    } while (attempts < 3);
}
#endif
	Ql_Sleep(100);
    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QIACT\n", mode);
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),NULL,NULL,0);
    RIL_FTP_DEBUG(DBG_Buffer,"<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if (RIL_AT_SUCCESS != ret)
    {
        return ret;
    }

    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QFTPOPEN=\"%s\",\"%d\"\n", hostName, port);
    ftpParam.prefix="+QFTPOPEN:";
    ftpParam.data = 255;
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_FTP_handler_Common,(void* )&ftpParam,0);
    RIL_FTP_DEBUG(DBG_Buffer,"<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if(RIL_AT_SUCCESS != ret)
    {
        RIL_FTP_DEBUG(DBG_Buffer,"\r\n<-- send AT command failure -->\r\n");
        return ret;
    }
    else if(0 != ftpParam.data) // ftp open failed!!!
    {
        RIL_FTP_DEBUG(DBG_Buffer,"\r\n<-- FTP OPEN SET failure, =-%d -->\r\n", ftpParam.data);
        return QL_RET_ERR_RIL_FTP_OPENFAIL;
    }
    
    return ret;
    
}


s32 RIL_FTP_QFTPCLOSE(void) 
{
    s32 ret = RIL_AT_SUCCESS;
    ST_AT_ftpParam ftpParam;
    char strAT[200];
    
    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QFTPCLOSE\n");
    ftpParam.prefix = "+QFTPCLOSE:";
    ftpParam.data = 255;
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_FTP_handler_Common,(void* )&ftpParam,0);
    RIL_FTP_DEBUG(DBG_Buffer,"<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if (RIL_AT_SUCCESS != ret)
    {
        return ret;
    }
    else if(0 != ftpParam.data) 
    {
        RIL_FTP_DEBUG(DBG_Buffer,"\r\n<-- FTP CLOSE failure, =-%d -->\r\n", ftpParam.data);
        return QL_RET_ERR_RIL_FTP_CLOSEFAIL;
    }
    return ret;
}

s32 RIL_FTP_QFTPPUT(u8* fileName, u32 fileSize, u32 timeOut, CallBack_Ftp_Upload ftpPut_CB)
{
    s32 ret = RIL_AT_SUCCESS;
    char strAT[200];
    
    Ql_memset(strAT, 0, sizeof(strAT));
    if(0 == timeOut)
    {
        Ql_sprintf(strAT, "AT+QFTPPUT=\"%s\",%d\n",fileName,fileSize);// time out use the default value
    }
    else
    {
        Ql_sprintf(strAT, "AT+QFTPPUT=\"%s\",%d,%d\n",fileName,fileSize,timeOut);
    }
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),NULL,NULL,0);
    RIL_FTP_DEBUG(DBG_Buffer,"<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if(RIL_ATRSP_SUCCESS == ret)
    {
        FtpPut_IND_CB = ftpPut_CB;
    }
    return ret;
}

s32 RIL_FTP_QFTPGET(u8* fileName, u32 fileSize,CallBack_Ftp_Download ftpGet_CB)
{
    s32 ret = RIL_AT_SUCCESS;
    char strAT[200];
    
    Ql_memset(strAT, 0, sizeof(strAT));
    if(0 == fileSize)
    {
        Ql_sprintf(strAT, "AT+QFTPGET=\"%s\"\n",fileName);
    }
    else
    {
        Ql_sprintf(strAT, "AT+QFTPGET=\"%s\",%d\n",fileName,fileSize);
    }
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),NULL,NULL,0);
    RIL_FTP_DEBUG(DBG_Buffer,"<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if(RIL_ATRSP_SUCCESS == ret)
    {
        FtpGet_IND_CB = ftpGet_CB;
    }
    return ret;
}

s32 RIL_FTP_QFTPPATH(u8* pathName) 
{
    s32 ret = RIL_AT_SUCCESS;
    ST_AT_ftpParam ftpParam;
    char strAT[200];
    
    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QFTPPATH=\"%s\"\n",pathName);
    ftpParam.prefix = "+QFTPPATH:";
    ftpParam.data = 255;
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_FTP_handler_Common,(void *)&ftpParam,0);
    RIL_FTP_DEBUG(DBG_Buffer,"<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if(RIL_AT_SUCCESS != ret)
    {
        RIL_FTP_DEBUG(DBG_Buffer,"\r\n<-- send FTP PATH command failure -->\r\n");
        return ret;
    }
    else if(0 != ftpParam.data) // ftp open failed!!!
    {
        RIL_FTP_DEBUG(DBG_Buffer,"\r\n<-- FTP PATH failure, =-%d -->\r\n", ftpParam.data);
        return QL_RET_ERR_RIL_FTP_SETPATHFAIL;
    }
    return ret;
}

s32 RIL_FTP_QFTPCFG(u8 type, u8* value)
{
    s32 ret = RIL_AT_SUCCESS;
    ST_AT_ftpParam ftpParam;
    char strAT[200];
    
    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QFTPCFG=%d,\"/%s/\"\n",type,value);
    ftpParam.prefix = "+QFTPCFG:";
    ftpParam.data = 255;
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_FTP_handler_Common,(void *)&ftpParam,0);
    RIL_FTP_DEBUG(DBG_Buffer,"<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if(RIL_AT_SUCCESS != ret)
    {
        RIL_FTP_DEBUG(DBG_Buffer,"\r\n<-- send QFTPCFG command failure -->\r\n");
        return ret;
    }
    else if(0 != ftpParam.data) // ftp CFG failed!!!
    {
        RIL_FTP_DEBUG(DBG_Buffer,"\r\n<-- FTP QFTPCFG failure, =-%d -->\r\n", ftpParam.data);
        return QL_RET_ERR_RIL_FTP_SETCFGFAIL;
    }
    return ret;
}

static s32 ATResponse_QFTPSTAT_handler(char* line, u32 len, void* userdata)
{
    char *head = Ql_RIL_FindString(line, len, "+QFTPSTAT:"); //continue wait
    if(head)
    {
        if(NULL != Ql_strstr(line,"+QFTPSTAT:IDLE"))
        {
            *(s32*) userdata = FTP_STATUS_IDLE;
        }
         else if(NULL != Ql_strstr(line,"+QFTPSTAT:OPENING"))
        {
            *(s32*) userdata = FTP_STATUS_OPENING;
        }
         else if(NULL != Ql_strstr(line,"+QFTPSTAT:OPENED"))
        {
            *(s32*) userdata = FTP_STATUS_OPENED;
        }
         else if(NULL != Ql_strstr(line,"+QFTPSTAT:WORKING"))
        {
            *(s32*) userdata = FTP_STATUS_WORKING;
        }
         else if(NULL != Ql_strstr(line,"+QFTPSTAT:TRANSFER"))
        {
            *(s32*) userdata = FTP_STATUS_TRANSFER;
        }
         else if(NULL != Ql_strstr(line,"+QFTPSTAT:CLOSING"))
        {
            *(s32*) userdata = FTP_STATUS_CLOSING;
        }
         else if(NULL != Ql_strstr(line,"+QFTPSTAT:CLOSED"))
        {
            *(s32*) userdata = FTP_STATUS_CLOSED;
        }
         
        return RIL_ATRSP_CONTINUE;
    }
    head = Ql_RIL_FindLine(line, len, "OK");
    if(head)
    {  
        return  RIL_ATRSP_SUCCESS;  
    }
    head = Ql_RIL_FindLine(line, len, "ERROR");
    if(head)
    {  
        return  RIL_ATRSP_FAILED;
    }
    head = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if(head)
    {
        return  RIL_ATRSP_FAILED;
    }
    head = Ql_RIL_FindString(line, len, "+CMS ERROR:");//fail
    if(head)
    {
        return  RIL_ATRSP_FAILED;
    }
    return RIL_ATRSP_CONTINUE; //continue wait
}

 s32 RIL_FTP_QFTPSTAT(s32* state) 
 {
    s32 ret = RIL_AT_SUCCESS;
    char strAT[200];
    
    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QFTPSTAT\n");
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_QFTPSTAT_handler,(void *)&state,0);
    RIL_FTP_DEBUG(DBG_Buffer,"<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if(RIL_AT_SUCCESS != ret)
    {
        RIL_FTP_DEBUG(DBG_Buffer,"\r\n<-- send QFTPSTAT command failure =%d -->\r\n",ret);
        *state = -1; // if the AT command set fail ,the state is Invalied
        return ret;
    } 
    return ret;
}

static s32 ATResponse_QFTPLEN_handler(char* line, u32 len, void* userdata)
{
    char *head = Ql_RIL_FindString(line, len, "+QFTPLEN:"); //continue wait
    if(head)
    {
            char strTmp[10];
            char* p1 = NULL;
            char* p2 = NULL;
            Ql_memset(strTmp, 0x0, sizeof(strTmp));
            p1 = Ql_strstr(head, ":");
            p2 = Ql_strstr(p1 + 1, "\r\n");
            if (p1 && p2)
            {
                Ql_memcpy(strTmp, p1 + 2, p2 - p1 - 2);
                *(s32* )userdata = Ql_atoi(strTmp);
            }
        return  RIL_ATRSP_CONTINUE;
    }
    head = Ql_RIL_FindLine(line, len, "OK");
    if(head)
    {  
        return  RIL_ATRSP_SUCCESS;  
    }
    head = Ql_RIL_FindLine(line, len, "ERROR");
    if(head)
    {  
        return  RIL_ATRSP_FAILED;
    }
    head = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if(head)
    {
        return  RIL_ATRSP_FAILED;
    }
    head = Ql_RIL_FindString(line, len, "+CMS ERROR:");//fail
    if(head)
    {
        return  RIL_ATRSP_FAILED;
    }
    return RIL_ATRSP_CONTINUE; //continue wait
}
s32 RIL_FTP_QFTPLEN(s32* len)
{
    s32 ret = RIL_AT_SUCCESS;
    char strAT[200];
    
    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QFTPLEN\n");
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_QFTPLEN_handler,(void *)&len,0);
    RIL_FTP_DEBUG(DBG_Buffer,"<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if(RIL_AT_SUCCESS != ret)
    {
        RIL_FTP_DEBUG(DBG_Buffer,"\r\n<-- send QFTPLEN command failure =%d -->\r\n",ret);
        *len = -1; // if the AT command set fail ,the state is Invalied
        return ret;
    } 
    return ret;
}

 s32 RIL_FTP_QFTPRENAME(u8* sourcName, u8* targetName) 
 {
    s32 ret = RIL_AT_SUCCESS;
    ST_AT_ftpParam ftpParam;
    char strAT[200];
    
    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QFTPRENAME=\"%s\",\"%s\"\n",sourcName,targetName);
    ftpParam.prefix = "+QFTPRENAME:";
    ftpParam.data = 255;
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_FTP_handler_Common,(void *)&ftpParam,0);
    RIL_FTP_DEBUG(DBG_Buffer,"<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if(RIL_AT_SUCCESS != ret)
    {
        RIL_FTP_DEBUG(DBG_Buffer,"\r\n<-- send QFTPRENAME command failure -->\r\n");
        return ret;
    }
    else if(0 != ftpParam.data ) // ftp CFG failed!!!
    {
        RIL_FTP_DEBUG(DBG_Buffer,"\r\n<-- FTP QFTPCFG failure, =-%d -->\r\n", ftpParam.data );
        return QL_RET_ERR_RIL_FTP_RENAMEFAIL;
    }
    return ret;
}

s32 RIL_FTP_QFTPSIZE(u8* fileName, u32* fileSize)
{
    s32 ret = RIL_AT_SUCCESS;
    ST_AT_ftpParam ftpParam;
    char strAT[200];
    
    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QFTPSIZE=\"%s\"\n",fileName);
    ftpParam.prefix = "+QFTPSIZE:";
    ftpParam.data = -1;
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_FTP_handler_Common,(void *)&ftpParam,0);
    RIL_FTP_DEBUG(DBG_Buffer,"<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if(RIL_AT_SUCCESS != ret)
    {
        RIL_FTP_DEBUG(DBG_Buffer,"\r\n<-- send QFTPSIZE command failure -->\r\n");
        fileSize = 0;
        return ret;
    }
    else if(0 > ftpParam.data)
    {
        RIL_FTP_DEBUG(DBG_Buffer,"\r\n<-- +QFTPSIZE failure %d-->\r\n", ftpParam.data);
        fileSize = 0;
        return QL_RET_ERR_RIL_FTP_SIZEFAIL;
    }
    *fileSize = ftpParam.data;
    
    return ret;
}

s32 RIL_FTP_QFTPDELETE(u8* fileName) 
{
    s32 ret = RIL_AT_SUCCESS;
    ST_AT_ftpParam ftpParam;
    char strAT[200];
    
    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QFTPDELETE=\"%s\"\n",fileName);
    ftpParam.prefix = "+QFTPDELETE:";
    ftpParam.data = 255;
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_FTP_handler_Common,(void *)&ftpParam,0);
    RIL_FTP_DEBUG(DBG_Buffer,"<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if(RIL_AT_SUCCESS != ret)
    {
        RIL_FTP_DEBUG(DBG_Buffer,"\r\n<-- send QFTPDELETE command failure -->\r\n");
        return ret;
    }
    else if(0 != ftpParam.data) // ftp QFTPDELETE failed!!!
    {
        RIL_FTP_DEBUG(DBG_Buffer,"\r\n<-- FTP +QFTPDELETE failure, =-%d -->\r\n", ftpParam.data);
        return QL_RET_ERR_RIL_FTP_DELETEFAIL;
    }
    return ret;
}

s32 RIL_FTP_QFTPMKDIR(u8* pathName)
{
    s32 ret = RIL_AT_SUCCESS;
    ST_AT_ftpParam ftpParam;
    char strAT[200];
    
    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QFTPMKDIR=\"%s\"\n",pathName);
    ftpParam.prefix = "+QFTPMKDIR:";
    ftpParam.data = 255;
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_FTP_handler_Common,(void *)&ftpParam,0);
    RIL_FTP_DEBUG(DBG_Buffer,"<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if(RIL_AT_SUCCESS != ret)
    {
        RIL_FTP_DEBUG(DBG_Buffer,"\r\n<-- send QFTPMKDIR command failure -->\r\n");
        return ret;
    }
    else if(0 != ftpParam.data) // ftp QFTPMKDIR failed!!!
    {
        RIL_FTP_DEBUG(DBG_Buffer,"\r\n<-- FTP +QFTPMKDIR failure, =-%d -->\r\n", ftpParam.data);
        return QL_RET_ERR_RIL_FTP_MKDIRFAIL;
    }
    return ret;
}

 s32 RIL_FTP_QFTPRMDIR(u8* pathName)
 {
    s32 ret = RIL_AT_SUCCESS;
    ST_AT_ftpParam ftpParam;
    char strAT[200];
    
    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QFTPMKDIR=\"%s\"\n",pathName);
    ftpParam.prefix = "+QFTPMKDIR:";
    ftpParam.data = 255;
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_FTP_handler_Common,(void *)&ftpParam,0);
    RIL_FTP_DEBUG(DBG_Buffer,"<-- Send AT:%s, ret = %d -->\r\n",strAT, ret);
    if(RIL_AT_SUCCESS != ret)
    {
        RIL_FTP_DEBUG(DBG_Buffer,"\r\n<-- send QFTPRMDIR command failure -->\r\n");
        return ret;
    }
    else if(0 != ftpParam.data) // 
    {
        RIL_FTP_DEBUG(DBG_Buffer,"\r\n<-- FTP +QFTPRMDIR failure, =-%d -->\r\n", ftpParam.data);
        return QL_RET_ERR_RIL_FTP_MKDIRFAIL;
    }
    return ret;
}

static s32 Callback_QIDEACT(char* line, u32 len, void* userData)
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

s32 RIL_FTP_QIDEACT(void)
{
    return Ql_RIL_SendATCmd("AT+QIDEACT\n", 11, Callback_QIDEACT, NULL, 0);
}

#endif


