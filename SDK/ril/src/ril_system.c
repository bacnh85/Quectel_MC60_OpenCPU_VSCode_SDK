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
 *   ril_system.c 
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
#include "ril.h"
#include "ril_util.h"
#include "ril_system.h"
#include "ql_stdlib.h"
#include "ql_trace.h"
#include "ql_error.h"
#include "ql_system.h"

#ifdef __OCPU_RIL_SUPPORT__ 
static s32 SYS_ATResponse_Hanlder(char* line, u32 len, void* userdata)
{
    s32* pSysInitStatus = (s32* )userdata; 
    char *head = Ql_RIL_FindString(line, len, "+QINISTAT:"); //continue wait
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
            *pSysInitStatus = Ql_atoi(strTmp);
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

    return RIL_ATRSP_CONTINUE; //continue wait
}


s32 RIL_QuerySysInitStatus( s32* SysInitStatus)
{
    return Ql_RIL_SendATCmd("AT+QINISTAT", 11, SYS_ATResponse_Hanlder, (void *)SysInitStatus, 0); 
}

static s32 Power_ATResponse_Hanlder(char* line, u32 len, void* userdata)
{
    ST_SysPower *PowerSupply;

    PowerSupply = (ST_SysPower *)userdata;
    char *head = Ql_RIL_FindString(line, len, "+CBC:"); //continue wait
    if(head)
    {
        char strTmp[10];
        char *p1,*p2;
        p1 = Ql_strstr(head, ":");
        p2 = Ql_strstr(p1 + 1, ",");
        if (p1 && p2)
        {
            p1 = p2;
            p2 = Ql_strstr(p1 + 1, ",");
            if (p1 && p2)
            {
                Ql_memset(strTmp, 0x0, sizeof(strTmp));
                Ql_memcpy(strTmp, p1 + 1, p2 - p1 - 1);
                PowerSupply->capacity = Ql_atoi(strTmp);
                p1 = p2;
                p2 = Ql_strstr(p1 + 1, "\r\n");
                if (p1 && p2)
                {
                    Ql_memset(strTmp, 0x0, sizeof(strTmp));
                    Ql_memcpy(strTmp, p1 + 1, p2 - p1 - 1);
                    PowerSupply->voltage = Ql_atoi(strTmp);
                }
            } 
        }             
        //    Ql_sscanf(head,"%*[^ ]%d,%d,%[^\r\n]",&PowerSupply->capacity,&PowerSupply->voltage);
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


/*****************************************************************
* Function:     RIL_GetPowerSupply 
* 
* Description:
*               This function queries the battery balance, and the battery voltage.
*
* Parameters:
*               capacity:      
*                   [out] battery balance, a percent, ranges from 1 to 100.
*
*               voltage:       
*                   [out] battery voltage, unit in mV
* Return:        
*               QL_RET_OK, indicates this function successes.
*		   -1, fail.
*****************************************************************/
s32 RIL_GetPowerSupply(u32* capacity, u32* voltage)
{
    s32 ret;
    ST_SysPower PowerSupply;

    ret = Ql_RIL_SendATCmd("AT+CBC", 6, Power_ATResponse_Hanlder, (void *)&PowerSupply, 0);
    if (RIL_AT_SUCCESS == ret)
    {
        *capacity = PowerSupply.capacity;
        *voltage  = PowerSupply.voltage;
    }
    return ret;
}

s32 Ql_SecureData_Store(u8 index , u8* pData, u32 len)
{
    s32 ret,i,AtHeadlen, hexstrlen;
    char* pstrBuff = NULL;
    char* ATstrhead = NULL;
    
    if(index <= 0 || index > 13 )
    {
        return QL_RET_ERR_PARAM;
    }
    if(((index <= 8) && (len > 50)) || ((index > 8) && (index <= 12) && (len > 100)) ||((index > 12) && (len > 500)))
    {
        return QL_RET_ERR_PARAM;
    }
    
    hexstrlen = 2*len;
    ATstrhead = (char*)Ql_MEM_Alloc(hexstrlen+40);
    if(NULL == ATstrhead)
        return QL_RET_ERR_GET_MEM;

    Ql_memset(ATstrhead, 0x00, hexstrlen+40);
    pstrBuff = ATstrhead;
    Ql_sprintf(pstrBuff,"AT+QUSERDAT=1,%d,\"",index);
    AtHeadlen =Ql_strlen(pstrBuff);
    pstrBuff = ATstrhead+AtHeadlen;
    for(i=0; i<len;i++)
    {
        Ql_sprintf(pstrBuff, "%02X", *pData++);
        pstrBuff = pstrBuff+2;
    }
    *pstrBuff = '"';
    //Ql_Debug_Trace("<--%s-->\r\n",ATstrhead);
    ret = Ql_RIL_SendATCmd(ATstrhead, Ql_strlen(ATstrhead), NULL, NULL, 0);
    Ql_MEM_Free(ATstrhead);
    ATstrhead = NULL;
    return ret;
}


static bool hexstring_to_value(u8 *str, u8*val)
{
    u16 i = 0, j = 0;
    u8 temp = 0;

    if((str == NULL) && (val == NULL))
        return FALSE;
    
    Ql_StrToUpper((char*)str);
    while (str[i] != END_OF_STR)
    {
        if (IS_NUMBER(str[i]))
        {
            temp = (temp << 4) + (str[i] - CHAR_0);
        }
        else if ((str[i] >= CHAR_A) && (str[i] <= CHAR_F))
        {
            temp = (temp << 4) + ((str[i] - CHAR_A) + 10);
        }
        else
        {
            return FALSE;
        }
        if(i%2)
        {            
            *(val+j)= temp;
            j++;
            temp = 0; 
        }
        i++;
        
    }

    return TRUE;
}

u32 SecureDataStringLen;
bool IsDataReadSuccess = FALSE;
typedef struct {
    u8* pHexBuf;
    u32 realLen;
}ST_CustomParam;
static s32 SecureData_Read_Callback(char* line, u32 len, void* userdata)
{
   char *head = Ql_RIL_FindString(line, len, "+QUSERDAT: 3,"); //continue wait
    if(head)
    {
        u32 cplen;
        char* p1 = NULL;
        char* p2 = NULL;
        p1 = Ql_strstr(line, "\"");
        p2 = Ql_strstr(p1+1, "\"");
        if ((p1 != NULL)&&(p2!= NULL))
        {
            ST_CustomParam* pCus = (ST_CustomParam*)userdata;
            cplen = (SecureDataStringLen < (p2-p1-1))? (SecureDataStringLen):(p2-p1-1) ;
            pCus->realLen = cplen / 2;
            Ql_memcpy(pCus->pHexBuf, p1+1,cplen);
          //  Ql_Debug_Trace("Read backup hex:%s",userdata);
            IsDataReadSuccess = TRUE;
        }
        else
        {
            IsDataReadSuccess = FALSE;
        }
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
s32 Ql_SecureData_Read(u8 index, u8* pBuffer, u32 len)
{
    s32 ret;
    char ATstr[20];
    ST_CustomParam ptrCus;
    
    if(index <= 0 || index > 13 )
    {
        return QL_RET_ERR_PARAM;
    }
    if(((index <= 8) && (len > 50)) || ((index > 8) && (index <= 12) && (len > 100)) ||((index > 12) && (len > 500)))
    {
        return QL_RET_ERR_PARAM;
    }

    SecureDataStringLen = 2*len;
    Ql_memset(&ptrCus, 0x0, sizeof(ST_CustomParam));
    ptrCus.pHexBuf = Ql_MEM_Alloc(SecureDataStringLen+1); // 2*len   the buffer stroe the data read form 
    if(NULL == ptrCus.pHexBuf)
        return QL_RET_ERR_GET_MEM;

    Ql_memset(ATstr, 0x00, sizeof(ATstr));
    Ql_sprintf(ATstr,"AT+QUSERDAT=3,%d",index); // read index 
    Ql_memset(ptrCus.pHexBuf, 0x00, SecureDataStringLen+1);
    ret = Ql_RIL_SendATCmd(ATstr, Ql_strlen(ATstr), SecureData_Read_Callback, &ptrCus, 0);
    if(ret <0)
        return ret;
    if(!IsDataReadSuccess)
    {
        Ql_MEM_Free(ptrCus.pHexBuf);
        ptrCus.pHexBuf = NULL;
        return Ql_RET_ERR_UNKOWN; 
    }
    hexstring_to_value(ptrCus.pHexBuf,pBuffer);
    Ql_MEM_Free(ptrCus.pHexBuf);
    ptrCus.pHexBuf = NULL;
    
   return ptrCus.realLen;
}

static s32 ATRsp_IMEI_Handler(char* line, u32 len, void* param)
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

s32 RIL_GetIMEI(char* imei)
{
    char strAT[] = "AT+GSN\0";
    if (NULL == imei)
    {
        return RIL_AT_INVALID_PARAM;
    }
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_IMEI_Handler,(void*)imei, 0);
}

#endif  //__OCPU_RIL_SUPPORT__

