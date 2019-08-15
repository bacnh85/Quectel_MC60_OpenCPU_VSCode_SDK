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
 *   The module implements SMS related APIs.
 *
 * Author:
 * -------
 * -------
 *  Designed by     :   Vicent GAO
 *  Coded    by     :   Vicent GAO
 *  Tested   by     :   Vicent GAO
 *============================================================================
 *             HISTORY
 *----------------------------------------------------------------------------
 * 2013/11/19    Vicent GAO      This file is created by ROTVG00006-P01
 * 2015/06/02    Vicent GAO      Add support for read/send con-sms by ROTVG00006-P05
 * 2015/06/04    Vicent GAO      Remove useless codes by ROTVG00006-P07
 ****************************************************************************/
#include "custom_feature_def.h"

#if (defined(__OCPU_RIL_SUPPORT__) && defined(__OCPU_RIL_SMS_SUPPORT__))

#include "ril.h"
#include "ril_sms.h"
#include "lib_ril_sms.h"
#include "ril_util.h"
#include "ql_stdlib.h"
#include "ql_trace.h"
#include "ql_uart.h"

/***********************************************************************
 * MACRO CONSTANT DEFINITIONS
************************************************************************/
#define DBG_SWITCH  (FALSE)
#define DBG_PORT    (UART_PORT2)  //NOTE: Only DBG_SWITCH set to TRUE,this macro can be VALID.
#define DBG_BUF_MAX_LEN   (512)

#define SMS_CMD_MAX_LEN   (30)

#define CPMS_KEY_STR   "+CPMS: "   //Warning: Please NOT-CHANGE this value!!
#define CMGR_KEY_STR   "+CMGR: "   //Warning: Please NOT-CHANGE this value!!
#define CMGS_KEY_STR   "+CMGS: "   //Warning: Please NOT-CHANGE this value!!

#define STR_CMGS_HINT  "\r\n>"
#define STR_CR_LF      "\r\n"
#define STR_COMMA      ","

#define CHARSET_GSM_INSTEAD_0X1B_CHAR_FLAG   (0x80)
#define CHARSET_GSM_INSTEAD_0X1A_CHAR_FLAG   (0x81)

/***********************************************************************
 * ENUM TYPE DEFINITIONS
************************************************************************/
typedef enum
{
    HDLR_TYPE_CPMS_READ_CMD = 0,
    HDLR_TYPE_CPMS_SET_CMD = 1,
    HDLR_TYPE_CMGR_PDU_CMD = 2,
    HDLR_TYPE_CMGS_PDU_CMD = 3,
    
    //==> Warning: Please add new Handler Type upper this line.
    HDLR_TYPE_INVALID = 0xFFFFFFFF
} Enum_HdlrType;

/***********************************************************************
 * STRUCT TYPE DEFINITIONS
************************************************************************/
typedef struct
{
    u32 uHdlrType;
    void  *pUserData;
} ST_SMS_HdlrUserData;

typedef struct 
 {
    u8   storage;
    u32  used;
    u32  total;
}ST_SMSStorage;

/***********************************************************************
 * OTHER TYPE DEFINITIONS
************************************************************************/

/***********************************************************************
 * GLOBAL DATA DEFINITIONS
************************************************************************/

/***********************************************************************
 * FUNCTION DECLARATIONS --> Adapter layer functions
 * NOTE: If you get this RIL SMS to another platform, you MAY need to recode these functions
************************************************************************/
static u32 ADP_SMS_ConvIdxToCoreIdx(u8 uStoType,u32 uRILIdx, u32 uRILMaxIdx);

/***********************************************************************
 * FUNCTION DECLARATIONS --> Internal functions
 * NOTE: These functions are ONLY used in this file.
************************************************************************/
static bool IsValidConParam(ST_RIL_SMS_Con *pCon);
static bool ConvStringToPhoneNumber(char *pString,u32 uStrLen,LIB_SMS_PhoneNumberStruct *pNumber);
static bool ConvPhoneNumberToString(LIB_SMS_PhoneNumberStruct *pNumber,char *pString,u32 uStrLen);
static bool ConvTimeStampToString(LIB_SMS_TimeStampStruct *pTimeStamp,char *pString,u32 uStrLen);
static bool ConvDeliverSMSParamToTextInfo(LIB_SMS_DeliverPDUParamStruct *pSMSParam,ST_RIL_SMS_DeliverParam *pTextInfo);
static bool ConvSubmitSMSParamToTextInfo(LIB_SMS_SubmitPDUParamStruct *pSMSParam,ST_RIL_SMS_SubmitParam *pTextInfo);
static bool ConvSMSParamToTextInfo(u8 uCharSet,LIB_SMS_PDUParamStruct *pSMSParam,ST_RIL_SMS_TextInfo* pTextInfo);

static bool GetStorageType(char *pStr,u8 *pType);

static char* HdlrGetStorageInfo(char *pLine,u32 uLen,ST_SMSStorage *pInfo);
static char* HdlrSetStorage(char *pLine,u32 uLen,ST_SMSStorage *pInfo);
static char* HdlrReadPDUMsg(char *pLine,u32 uLen,ST_RIL_SMS_PDUInfo *pPDU);
static char* HdlrSendPDUMsg(char *pLine,u32 uLen,ST_RIL_SMS_SendPDUInfo *pInfo);

static s32 SMS_CMD_GeneralHandler(char* pLine, u32 uLen, void* pUserData);

static s32 CmdGetStorageInfo(u8* pCurrMem, u32* pUsed,u32* pTotal);
static s32 CmdSetStorageInfo(u8 uStoType,u32* pUsed,u32* pTotal);
static s32 CmdReadPDUMsg(u32 uIndex, ST_RIL_SMS_PDUInfo* pPDU);
static s32 CmdSendPDUMsg(char* pPDUStr,u32 uPDUStrLen,u32 *pMsgRef);

/***********************************************************************
 * MACRO FUNCTION DEFINITIONS --> ADPATER LAYER USE
 * NOTE: If you get this RIL SMS to another platform, you MAY need to recode these macro functions
************************************************************************/
#define ADP_IS_SUPPORT_STORAGE_TYPE(StorageType)   \
(   \
    (   \
            (RIL_SMS_STORAGE_TYPE_SM == (StorageType))  \
    ) ? TRUE : FALSE   \
)

/***********************************************************************
 * MACRO FUNCTION DEFINITIONS --> INTERNAL USE
 * NOTE: These functions are ONLY used in this file.
************************************************************************/

#if DBG_SWITCH == (TRUE)

static char sg_aDbgBuf[DBG_BUF_MAX_LEN];

#define DBG_TRACE(Buffer,...) \
    do  \
    {   \
        Ql_memset((Buffer), 0, sizeof(Buffer)); \
        Ql_snprintf((char *)(Buffer),DBG_BUF_MAX_LEN,__VA_ARGS__); \
            \
        if(UART_PORT2 == DBG_PORT) \
        {   \
            Ql_Debug_Trace((char*)(Buffer));  \
            Ql_Debug_Trace((char*)(STR_CR_LF));  \
        }   \
        else    \
        {   \
            Ql_UART_Write(DBG_PORT,(u8*)(Buffer),Ql_strlen((char*)(Buffer)));  \
            Ql_UART_Write(DBG_PORT,(u8*)(STR_CR_LF),Ql_strlen((char*)(STR_CR_LF)));  \
        }   \
    } while(0)

#define DBG_NO_CRLF_TRACE(Buffer,...) \
    do  \
    {   \
        Ql_memset((Buffer), 0, sizeof(Buffer)); \
        Ql_snprintf((char *)(Buffer),sizeof(Buffer),__VA_ARGS__); \
            \
        if (UART_DEBUG_PORT == DBG_PORT) \
        {\
            Ql_Debug_Trace("%s", (char*)Buffer);\
        } else {\
            Ql_UART_Write(DBG_PORT,(u8*)(Buffer),Ql_strlen((char*)(Buffer)));  \
        }\
    } while(0)

#define DBG_BIN_TRACE(Data,DataLen) \
    do  \
    {   \
        Ql_UART_Write(DBG_PORT,(u8*)(Data),(DataLen));  \
        Ql_UART_Write(DBG_PORT,(u8*)"\r\n",2);  \
    } while(0)

#define DBG_BIN_NO_CRLF_TRACE(Data,DataLen) \
    do  \
    {   \
        Ql_UART_Write(DBG_PORT,(u8*)(Data),(DataLen));  \
    } while(0)
        
#else //#if DBG_SWITCH == (TRUE)

#define DBG_TRACE(Buffer,...) 
#define DBG_NO_CRLF_TRACE(Buffer,...)
#define DBG_BIN_TRACE(Data,DataLen)
#define DBG_BIN_NO_CRLF_TRACE(Data,DataLen) 

#endif  //#if DBG_SWITCH == (TRUE)

#define IS_VALID_PDU_INFO(PDUInfo)  \
(   \
    (   \
            ((((ST_RIL_SMS_PDUInfo*)(PDUInfo))->status) <= RIL_SMS_STATUS_TYPE_STO_SENT)   \
        &&  ((((ST_RIL_SMS_PDUInfo*)(PDUInfo))->length) <= sizeof(((ST_RIL_SMS_PDUInfo*)(PDUInfo))->data)) \
    ) ? TRUE : FALSE   \
)

#define CONV_STRING_TO_INTEGER(pStr,uLen,uVal) \
    do  \
    {   \
        char aBufTmpZ[40] = {0,};   \
        \
        Ql_memcpy(aBufTmpZ,pStr,uLen);   \
        uVal = Ql_atoi(aBufTmpZ);    \
    } while(0)

#define SMS_SET_INVALID_PDU_INFO(PDUInfo)   \
    do  \
    {   \
        Ql_memset(((ST_RIL_SMS_PDUInfo*)(PDUInfo)),0x00,sizeof(ST_RIL_SMS_PDUInfo));    \
        \
        (((ST_RIL_SMS_PDUInfo*)(PDUInfo))->status) = RIL_SMS_STATUS_TYPE_INVALID;   \
    } while(0)
  
#define SMS_SET_INVALID_TEXT_INFO(TextInfo) \
    do  \
    {   \
        Ql_memset(((ST_RIL_SMS_TextInfo*)(TextInfo)),0x00,sizeof(ST_RIL_SMS_TextInfo)); \
        \
        (((ST_RIL_SMS_TextInfo*)(TextInfo))->status) = RIL_SMS_STATUS_TYPE_INVALID; \
    } while(0)

#define SMS_SET_PDU_MODE(ErrCode,DbgFunName)   \
    do  \
    {   \
        (ErrCode) = Ql_RIL_SendATCmd("AT+CMGF=0",Ql_strlen("AT+CMGF=0"),NULL,NULL,0);   \
        if(RIL_ATRSP_SUCCESS != (ErrCode))  \
        {   \
            DBG_TRACE(sg_aDbgBuf,"Enter " DbgFunName ",FAIL! AT+CMGF=0 execute error! Code:%d\r\n",(ErrCode)); \
            return (ErrCode);    \
        }   \
    } while(0)

#define SMS_HDLR_CHECK_ERROR(pLine,uLen,DbgFunName) \
    do  \
    {   \
        if(NULL != Ql_RIL_FindLine((pLine),(uLen),"OK"))    \
        {   \
            DBG_TRACE(sg_aDbgBuf,"Enter " DbgFunName ",SUCCESS. Find string: \"OK\"\r\n"); \
            return RIL_ATRSP_SUCCESS;   \
        }   \
            \
        if(NULL != Ql_RIL_FindLine((pLine), (uLen), "ERROR"))   \
        {   \
            DBG_TRACE(sg_aDbgBuf,"Enter " DbgFunName ",SUCCESS. Find string: \"ERROR\"\r\n"); \
            return RIL_ATRSP_FAILED;    \
        }   \
            \
        if(NULL != Ql_RIL_FindString((pLine), (uLen), "+CME ERROR:")) \
        {   \
            DBG_TRACE(sg_aDbgBuf,"Enter " DbgFunName ",SUCCESS. Find string: \"+CME ERROR:\"\r\n"); \
            return RIL_ATRSP_FAILED;    \
        }   \
            \
        if(NULL != Ql_RIL_FindString((pLine), (uLen), "+CMS ERROR:"))   \
        {   \
            DBG_TRACE(sg_aDbgBuf,"Enter " DbgFunName ",SUCCESS. Find string: \"+CMS ERROR:\"\r\n"); \
            return RIL_ATRSP_FAILED;    \
        }   \
    } while(0);

#define SMS_GET_STORAGE_NAME(StorageType,StorageName)  \
    do  \
    {   \
        if(RIL_SMS_STORAGE_TYPE_MT == (StorageType))    \
        {   \
            Ql_strcpy((StorageName),"MT");  \
        }   \
        else if(RIL_SMS_STORAGE_TYPE_ME == (StorageType))    \
        {   \
            Ql_strcpy((StorageName),"ME");  \
        }   \
        else    \
        {   \
            Ql_strcpy((StorageName),"SM");  \
        }   \
    } while(0)

/***********************************************************************
 * FUNCTION DEFINITIONS --> Adapter layer functions
 * NOTE: If you get this RIL SMS to another platform, you MAY need to recode these functions
************************************************************************/

/******************************************************************************
* Function:     ADP_SMS_ConvIdxToCoreIdx
*  
* Description:
*               Covert RIL SMS index to core SMS index
*
* Parameters:    
*               <uStoType>:
*                   [In] SMS storage type,same as: 'Enum_RIL_SMS_SMSStorage'
*               <uRILIdx>:
*                   [In] The given RIL SMS index
*               <uRILMaxIdx>
*                   [In] The maximum valid RIL SMS index
*
* Return:  
*               0:  This function works FAIL!
*               OTHER VALUE: This function works SUCCESS. And this value is the core SMS index.
*
* NOTE:
*               1. This is an Adapter function.
******************************************************************************/
static u32 ADP_SMS_ConvIdxToCoreIdx(u8 uStoType,u32 uRILIdx, u32 uRILMaxIdx)
{
    u32 uCoreIdx = 0;

    if(FALSE == ADP_IS_SUPPORT_STORAGE_TYPE(uStoType))
    {
        DBG_TRACE(sg_aDbgBuf,"Enter ADP_SMS_ConvIdxToCoreIdx,FAIL! ADP_IS_SUPPORT_STORAGE_TYPE FAIL! uStoType:%u",uStoType);
        return 0;
    }
    
    if((uRILIdx < 1) || (uRILIdx > uRILMaxIdx))
    {
        DBG_TRACE(sg_aDbgBuf,"Enter ADP_SMS_ConvIdxToCoreIdx,FAIL! uRILIdx:%u,uRILMaxIdx:%u\r\n",uRILIdx,uRILMaxIdx);
        return 0;
    }
    
    uCoreIdx = uRILIdx;
    
    DBG_TRACE(sg_aDbgBuf,"Enter ADP_SMS_ConvIdxToCoreIdx,SUCCESS. uRILIdx:%u,uRILMaxIdx:%u,uCoreIdx:%u\r\n",uRILIdx,uRILMaxIdx,uCoreIdx);
    
    return uCoreIdx;
}

/***********************************************************************
 * FUNCTION DEFINITIONS --> Internal functions
 * NOTE: These functions are ONLY used in this file.
************************************************************************/

/******************************************************************************
* Function:     IsValidConParam
*  
* Description:
*               Check 'pCon' is valid or not
*
* Parameters:    
*               <pCon>:
*                   [In] 'ST_RIL_SMS_Con' data
*                          + msgType  Con-SMS type,valid value list: LIB_SMS_UD_TYPE_CON_6_BYTE,LIB_SMS_UD_TYPE_CON_7_BYTE
*                          + msgRef   Con-SMS reference value
*                          + msgSeg   Con-SMS current segment
*                          + msgTot   Con-SMS total segment
*
* Return:  
*               TRUE:  This function works SUCCESS.
*               FALSE: This function works FAIL!
*
* NOTE:
*               1. This function ONLY used in AT handler function.
******************************************************************************/
static bool IsValidConParam(ST_RIL_SMS_Con *pCon)
{
    if(NULL == pCon)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter IsValidConParam,FAIL! Parameter is NULL.");
        return FALSE;
    }
    
    if(    ((pCon->msgType) !=  LIB_SMS_UD_TYPE_CON_6_BYTE)
        && ((pCon->msgType) !=  LIB_SMS_UD_TYPE_CON_7_BYTE)
      )
    {
        DBG_TRACE(sg_aDbgBuf,"Enter IsValidConParam,FAIL! msgType:%d is INVALID.",pCon->msgType);
        return FALSE;
    }
    
    if(    ((pCon->msgSeg) < 1)
        || ((pCon->msgSeg) > (pCon->msgTot))
      )
    {
        DBG_TRACE(sg_aDbgBuf,"Enter IsValidConParam,FAIL! msgSeg:%d or msgTot: %d INVALID.",pCon->msgSeg,pCon->msgTot);
        return FALSE;
    }
    
    return TRUE;
}

/******************************************************************************
* Function:     ConvStringToPhoneNumber
*  
* Description:
*               Get storage type refer to given string.
*
* Parameters:    
*               <pString>:
*                   [In] The pointer of number string
*               <uStrLen>
*                   [In] The length of number string
*               <pNumber>:
*                   [In] The pointer of 'LIB_SMS_PhoneNumberStruct' data
*
* Return:  
*               TRUE:  This function works SUCCESS.
*               FALSE: This function works FAIL!
*
* NOTE:
*               1. This function ONLY used in AT handler function.
******************************************************************************/
static bool ConvStringToPhoneNumber(char *pString,u32 uStrLen,LIB_SMS_PhoneNumberStruct *pNumber)
{
    u8 uType = 0;
    char *pTmp = NULL;
    u32 i = 0;

    if((NULL == pString) || (NULL == pNumber))
    {
        DBG_TRACE(sg_aDbgBuf,"Enter ConvStringToPhoneNumber,FAIL! Parameter is NULL.");
        return FALSE;
    }

    if(0 == uStrLen)  //Not given number string
    {
        Ql_memset(pNumber,0x00,sizeof(LIB_SMS_PhoneNumberStruct));
        (pNumber->uType) = LIB_SMS_PHONE_NUMBER_TYPE_UNKNOWN;
    
        DBG_TRACE(sg_aDbgBuf,"Enter ConvStringToPhoneNumber,SUCCESS. NOTE: uStrLen is 0.");
        return TRUE;
    }

    //Initialize
    pTmp = pString;

    if(LIB_SMS_CHAR_PLUS == (*pTmp))
    {
        uType = LIB_SMS_PHONE_NUMBER_TYPE_INTERNATIONAL;
        pTmp += 1;
    }
    else
    {
        uType = LIB_SMS_PHONE_NUMBER_TYPE_NATIONAL;
    }

    if((uStrLen - (pTmp - pString)) > LIB_SMS_PHONE_NUMBER_MAX_LEN)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter ConvStringToPhoneNumber,FAIL! Phone number is too long. LIB_SMS_PHONE_NUMBER_MAX_LEN:%d,Now:%d",LIB_SMS_PHONE_NUMBER_MAX_LEN,(uStrLen - (pTmp - pString)));
        return FALSE;
    }

    //Check the given number's validity
    for(i = 0; i < (uStrLen - (pTmp - pString)); i++)
    {
        if(FALSE == LIB_SMS_IS_VALID_ASCII_NUMBER_CHAR(pTmp[i]))
        {
            DBG_TRACE(sg_aDbgBuf,"Enter ConvStringToPhoneNumber,FAIL! LIB_SMS_IS_VALID_ASCII_NUMBER_CHAR FAIL! Char[%d]:0x%x",i,pTmp[i]);
            return FALSE;
        }
    }

	pNumber->uType = uType;
    (pNumber->uLen) = (uStrLen - (pTmp - pString));
    Ql_memcpy((pNumber->aNumber), pTmp, (pNumber->uLen));

    DBG_TRACE(sg_aDbgBuf,"Enter ConvStringToPhoneNumber,SUCCESS. uLen:%d,uType:%d",(pNumber->uLen),uType);

    return TRUE;
}

/******************************************************************************
* Function:     ConvPhoneNumberToString
*  
* Description:
*               Convert 'LIB_SMS_PhoneNumberStruct' data to string.
*
* Parameters:    
*               <pNumber>:
*                   [In] The pointer of phone number
*               <pString>
*                   [In] The pointer of string
*               <uStrLen>
*                   [In] The maximum length of string
*
* Return:  
*               TRUE:    This function works SUCCESS.
*               FALSE:   This function works FAIL!
******************************************************************************/
static bool ConvPhoneNumberToString(LIB_SMS_PhoneNumberStruct *pNumber,char *pString,u32 uStrLen)
{
    u32 uLimitLen = 0;
    char *pTmp = NULL;

    if((NULL == pNumber) || (NULL == pString) || (0 == uStrLen))
    {
        DBG_TRACE(sg_aDbgBuf,"Enter ConvPhoneNumberToString,FAIL! Parameter is NULL.");
        return FALSE;
    }

    if(0 == (pNumber->uLen))
    {
        DBG_TRACE(sg_aDbgBuf,"Enter ConvPhoneNumberToString,SUCCESS. NOTE: Number length is 0.");
    
        Ql_memset(pString,0x00,uStrLen);
        return TRUE;
    }
    
    //Check <uStrLen> is VALID or not
    if(LIB_SMS_PHONE_NUMBER_TYPE_INTERNATIONAL == (pNumber->uType))
    {
        uLimitLen = ((pNumber->uLen) + 1 + 1);  //It will add '+' at the first position,'\0' at the end position.
    }
    else if(LIB_SMS_PHONE_NUMBER_TYPE_NATIONAL == (pNumber->uType))
    {
        uLimitLen = ((pNumber->uLen) + 1);  //It will add '\0' at the end position.
    }
    else if(LIB_SMS_PHONE_NUMBER_TYPE_UNKNOWN == (pNumber->uType))
    {
        uLimitLen = ((pNumber->uLen) + 1);  //It will add '\0' at the end position.
    }
    //<2015/03/23-ROTVG00006-P04-Vicent.Gao,<[SMS] Segment 4==>Fix issues of RIL SMS LIB.>
    else if(LIB_SMS_PHONE_NUMBER_TYPE_ALPHANUMERIC == (pNumber->uType))
    {
        uLimitLen = ((pNumber->uLen) + 1);  //It will add '\0' at the end position.
    }
    //>2015/03/23-ROTVG00006-P04-Vicent.Gao    
    else
    {
        DBG_TRACE(sg_aDbgBuf,"Enter ConvPhoneNumberToString,FAIL! Number type is INVALID. uType:%u",(pNumber->uType));
        return FALSE;
    }
    
    if(uStrLen < uLimitLen)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter ConvPhoneNumberToString,FAIL! uStrLen is less than uLimitLen. uStrLen:%u,uLimitLen:%u",uStrLen,uLimitLen);
        return FALSE;
    }

    //Initialize
    Ql_memset(pString,0x00,uStrLen);
    pTmp = pString;
    
    if((LIB_SMS_PHONE_NUMBER_TYPE_INTERNATIONAL == (pNumber->uType)))
    {
        pTmp[0] = LIB_SMS_CHAR_PLUS;
        pTmp += 1;
    }
    
    Ql_memcpy(pTmp,(pNumber->aNumber),(pNumber->uLen));
    
    DBG_TRACE(sg_aDbgBuf,"Enter ConvPhoneNumberToString,SUCCESS. pString: %s",pString);
    
    return TRUE;
}

/******************************************************************************
* Function:     ConvTimeStampToString
*  
* Description:
*               Convert 'LIB_SMS_TimeStampStruct' data to string.
*
* Parameters:    
*               <pTimeStamp>:
*                   [In] The pointer of 'LIB_SMS_TimeStampStruct' data
*               <pString>
*                   [In] The pointer of string
*               <uStrLen>
*                   [In] The maximum length of string
*
* Return:  
*               TRUE:    This function works SUCCESS.
*               FALSE:   This function works FAIL!
******************************************************************************/
static bool ConvTimeStampToString(LIB_SMS_TimeStampStruct *pTimeStamp,char *pString,u32 uStrLen)
{
    s32 iLen = 0;
    s32 iTimeZone = 0;

    if((NULL == pTimeStamp) || (NULL == pString))
    {
        DBG_TRACE(sg_aDbgBuf,"Enter ConvTimeStampToString,FAIL! Parameter is NULL.");
        return FALSE;
    }
    
    if(uStrLen < RIL_SMS_TIME_STAMP_STR_MAX_LEN)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter ConvTimeStampToString,FAIL! uStrLen is too less. uStrLen:%u,RIL_SMS_TIME_STAMP_STR_MAX_LEN:%u",uStrLen,RIL_SMS_TIME_STAMP_STR_MAX_LEN);
        return FALSE;
    }
    
    iLen = Ql_sprintf(pString,"%02u/%02u/%02u,%02u:%02u:%02u",
        (pTimeStamp->uYear),
        (pTimeStamp->uMonth),
        (pTimeStamp->uDay),
        (pTimeStamp->uHour),
        (pTimeStamp->uMinute),
        (pTimeStamp->uSecond)
    );
    
    iTimeZone = (pTimeStamp->iTimeZone);
    if(iTimeZone < 0)
    {
        Ql_sprintf((pString+iLen),"%02d",iTimeZone);
    }
    else
    {
        Ql_sprintf((pString+iLen),"+%02d",iTimeZone);
    }
    
    DBG_TRACE(sg_aDbgBuf,"Enter ConvTimeStampToString,SUCCESS. pString:%s",pString);
    
    return TRUE;
}

/******************************************************************************
* Function:     ConvDeliverSMSParamToTextInfo
*  
* Description:
*               Convert 'LIB_SMS_DeliverPDUParamStruct' data to 'ST_RIL_SMS_DeliverParam' data.
*
* Parameters:    
*               <pSMSParam>:
*                   [In] The pointer of SMS parameter
*               <pTextInfo>
*                   [In] The pointer of TEXT SMS info
*
* Return:  
*               TRUE:    This function works SUCCESS.
*               FALSE:   This function works FAIL!
******************************************************************************/
static bool ConvDeliverSMSParamToTextInfo(LIB_SMS_DeliverPDUParamStruct *pSMSParam,ST_RIL_SMS_DeliverParam *pTextInfo)
{
    bool bResult = FALSE;

    if((NULL == pSMSParam) || (NULL == pTextInfo))
    {
        DBG_TRACE(sg_aDbgBuf,"Enter ConvDeliverSMSParamToTextInfo,FAIL! Parameter is NULL.");
        return FALSE;
    }
    
    LIB_SMS_GET_ALPHA_IN_PDU_DCS((pSMSParam->uDCS),(pTextInfo->alpha));
    
    bResult = ConvPhoneNumberToString(&(pSMSParam->sOA),(pTextInfo->oa),sizeof(pTextInfo->oa));
    if(FALSE == bResult)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter ConvDeliverSMSParamToTextInfo,FAIL! ConvPhoneNumberToString FAIL!");
        return FALSE;
    }
    
    bResult = ConvTimeStampToString(&(pSMSParam->sSCTS),(pTextInfo->scts),sizeof(pTextInfo->scts));
    if(FALSE == bResult)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter ConvDeliverSMSParamToTextInfo,FAIL! ConvTimeStampToString FAIL!");
        return FALSE;
    }    
    
    DBG_TRACE(sg_aDbgBuf,"Enter ConvDeliverSMSParamToTextInfo,SUCCESS. alpha:%u,oa:%s,scts:%s",(pTextInfo->alpha),(pTextInfo->oa),(pTextInfo->scts));
    
    return TRUE;
}

/******************************************************************************
* Function:     ConvSubmitSMSParamToTextInfo
*  
* Description:
*               Convert 'LIB_SMS_SubmitPDUParamStruct' data to 'ST_RIL_SMS_SubmitParam' data.
*
* Parameters:    
*               <pSMSParam>:
*                   [In] The pointer of SMS parameter
*               <pTextInfo>
*                   [In] The pointer of TEXT SMS info
*
* Return:  
*               TRUE:    This function works SUCCESS.
*               FALSE:   This function works FAIL!
******************************************************************************/
static bool ConvSubmitSMSParamToTextInfo(LIB_SMS_SubmitPDUParamStruct *pSMSParam,ST_RIL_SMS_SubmitParam *pTextInfo)
{
    bool bResult = FALSE;

    if((NULL == pSMSParam) || (NULL == pTextInfo))
    {
        DBG_TRACE(sg_aDbgBuf,"Enter ConvSubmitSMSParamToTextInfo,FAIL! Parameter is NULL.");
        return FALSE;
    }
    
    LIB_SMS_GET_ALPHA_IN_PDU_DCS((pSMSParam->uDCS),(pTextInfo->alpha));

    bResult = ConvPhoneNumberToString(&(pSMSParam->sDA),(pTextInfo->da),sizeof(pTextInfo->da));
    if(FALSE == bResult)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter ConvSubmitSMSParamToTextInfo,FAIL! ConvPhoneNumberToString FAIL!");
        return FALSE;
    }

    DBG_TRACE(sg_aDbgBuf,"Enter ConvSubmitSMSParamToTextInfo,SUCCESS. alpha:%u,da:%s",(pTextInfo->alpha),(pTextInfo->da));
    
    return TRUE;
}

/******************************************************************************
* Function:     ConvSMSParamToTextInfo
*  
* Description:
*               Convert 'LIB_SMS_PDUParamStruct' data to 'ST_RIL_SMS_TextInfo' data
*
* Parameters:    
*               <uCharSet>
*                   [In] CharSet,It's value is in 'LIB_SMS_CharSetEnum'
*               <pSMSParam>:
*                   [In] The pointer of SMS parameters
*               <pTextInfo>
*                   [In] The pointer of TEXT SMS info
*
* Return:  
*               RIL_ATRSP_SUCCESS:  This function works SUCCESS.
*               OTHER VALUES:       This function works FAIL!
******************************************************************************/
static bool ConvSMSParamToTextInfo(u8 uCharSet,LIB_SMS_PDUParamStruct *pSMSParam,ST_RIL_SMS_TextInfo* pTextInfo)
{
    bool bResult = FALSE;
    ST_RIL_SMS_DeliverParam *pDeliverTextInfo = NULL;
    ST_RIL_SMS_SubmitParam *pSubmitTextInfo = NULL;

    if((NULL == pSMSParam) || (NULL == pTextInfo))
    {
        DBG_TRACE(sg_aDbgBuf,"Enter ConvSMSParamToTextInfo,FAIL! Parameter is NULL.");
        return FALSE;
    }

    bResult = ConvPhoneNumberToString(&(pSMSParam->sSCA),(pTextInfo->sca),sizeof(pTextInfo->sca));
    if(FALSE == bResult)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter ConvSMSParamToTextInfo,FAIL! ConvPhoneNumberToString FAIL!");
        return FALSE;
    }

    (pTextInfo->type) = LIB_SMS_GET_MSG_TYPE_IN_PDU_FO(pSMSParam->uFO);
    if(LIB_SMS_PDU_TYPE_DELIVER == (pTextInfo->type))
    {
        pDeliverTextInfo = &((pTextInfo->param).deliverParam);

        if((pSMSParam->uFO) & 0x40) //Concatenate SMS
        {
            pDeliverTextInfo->conPres = TRUE;
            pDeliverTextInfo->con.msgType = pSMSParam->sParam.sDeliverParam.sConSMSParam.uMsgType;
            pDeliverTextInfo->con.msgRef = pSMSParam->sParam.sDeliverParam.sConSMSParam.uMsgRef;
            pDeliverTextInfo->con.msgSeg = pSMSParam->sParam.sDeliverParam.sConSMSParam.uMsgSeg;
            pDeliverTextInfo->con.msgTot= pSMSParam->sParam.sDeliverParam.sConSMSParam.uMsgTot;
        }
    
        bResult = ConvDeliverSMSParamToTextInfo(&((pSMSParam->sParam).sDeliverParam),pDeliverTextInfo);
        if(FALSE == bResult)
        {
            DBG_TRACE(sg_aDbgBuf,"Enter ConvSMSParamToTextInfo,FAIL! ConvDeliverSMSParamToTextInfo FAIL!");
            return FALSE;
        }

        (pDeliverTextInfo->length) = sizeof(pDeliverTextInfo->data);

        bResult = LIB_SMS_ConvAlphaToCharSet(
            ((pSMSParam->sParam).sDeliverParam.uDCS),
            ((pSMSParam->sParam).sDeliverParam.sUserData.aUserData),
            ((pSMSParam->sParam).sDeliverParam.sUserData.uLen),
            uCharSet,
            (pDeliverTextInfo->data),
            (u16*)&(pDeliverTextInfo->length)
        );
        
        if(FALSE == bResult)
        {
            DBG_TRACE(sg_aDbgBuf,"Enter ConvSMSParamToTextInfo,FAIL! LIB_SMS_ConvAlphaToCharSet FAIL!");
            return FALSE;
        }
    }
    else if(LIB_SMS_PDU_TYPE_SUBMIT == (pTextInfo->type))
    {
        pSubmitTextInfo = &((pTextInfo->param).submitParam);

        if((pSMSParam->uFO) & 0x40) //Concatenate SMS
        {
            pSubmitTextInfo->conPres = TRUE;
            pSubmitTextInfo->con.msgType = pSMSParam->sParam.sSubmitParam.sConSMSParam.uMsgType;
            pSubmitTextInfo->con.msgRef = pSMSParam->sParam.sSubmitParam.sConSMSParam.uMsgRef;
            pSubmitTextInfo->con.msgSeg = pSMSParam->sParam.sSubmitParam.sConSMSParam.uMsgSeg;
            pSubmitTextInfo->con.msgTot= pSMSParam->sParam.sSubmitParam.sConSMSParam.uMsgTot;
        }

        bResult = ConvSubmitSMSParamToTextInfo(&((pSMSParam->sParam).sSubmitParam),pSubmitTextInfo);
        if(FALSE == bResult)
        {
            DBG_TRACE(sg_aDbgBuf,"Enter ConvSMSParamToTextInfo,FAIL! ConvSubmitSMSParamToTextInfo FAIL!");
            return FALSE;
        }

        (pSubmitTextInfo->length) = sizeof(pSubmitTextInfo->data);

        bResult = LIB_SMS_ConvAlphaToCharSet(
            ((pSMSParam->sParam).sSubmitParam.uDCS),
            ((pSMSParam->sParam).sSubmitParam.sUserData.aUserData),
            ((pSMSParam->sParam).sSubmitParam.sUserData.uLen),
            uCharSet,
            (pSubmitTextInfo->data),
            (u16*)&(pSubmitTextInfo->length)
        );
        
        if(FALSE == bResult)
        {
            DBG_TRACE(sg_aDbgBuf,"Enter ConvSMSParamToTextInfo,FAIL! LIB_SMS_ConvAlphaToCharSet FAIL!");
            return FALSE;
        }
    }
    else
    {
        DBG_TRACE(sg_aDbgBuf,"Enter ConvSMSParamToTextInfo,FAIL! Msg type is INVALID. type:%d",(pTextInfo->type));
        return FALSE;
    }

    DBG_TRACE(sg_aDbgBuf,"Enter ConvSMSParamToTextInfo,SUCCESS. bResult:%d",bResult);

    return bResult;
}

/******************************************************************************
* Function:     GetStorageType
*  
* Description:
*               Get storage type refer to given string.
*
* Parameters:    
*               <pStr>:
*                   [In] The pointer of a string
*               <pType>:
*                   [In] The type of SMS storage,same as 'Enum_RIL_SMS_SMSStorage'
*
* Return:  
*               TRUE:  This function works SUCCESS.
*               FALSE: This function works FAIL!
*
* NOTE:
*               1. This function ONLY used in AT handler function.
******************************************************************************/
static bool GetStorageType(char *pStr,u8 *pType)
{
    #define SMS_TYPE_STR_LEN   (4)   //As: "SM"  "ME"

    char aBuf[SMS_TYPE_STR_LEN+1] = {0,};

    if((NULL == pStr) || (NULL == pType))
    {
        DBG_TRACE(sg_aDbgBuf,"Enter GetStorageType FAIL! Parameter is NULL.");
        return FALSE;
    }

    Ql_strncpy(aBuf,pStr,SMS_TYPE_STR_LEN);
    
    if(0 == Ql_strcmp(aBuf,"\"SM\""))
    {
        (*pType) = RIL_SMS_STORAGE_TYPE_SM;
    }
    else if(0 == Ql_strcmp(aBuf,"\"ME\""))
    {
        (*pType) = RIL_SMS_STORAGE_TYPE_ME;
    }
    else if(0 == Ql_strcmp(aBuf,"\"MT\""))
    {
        (*pType) = RIL_SMS_STORAGE_TYPE_MT;
    }
    else
    {
        DBG_TRACE(sg_aDbgBuf,"Enter GetStorageType FAIL! Storage type is INVALID. aBuf:%s",aBuf);
        return FALSE;
    }
    
    DBG_TRACE(sg_aDbgBuf,"Enter GetStorageType SUCCESS. aBuf:%s,*pType:%d",aBuf,(*pType));
    
    return TRUE;
}

/******************************************************************************
* Function:     HdlrGetStorageInfo
*  
* Description:
*               Get storage info refer to given string.
*
* Parameters:    
*               <pLine>:
*                   [In] The pointer of a string
*               <uLen>
*                   [In] The length of a string
*               <pInfo>:
*                   [In] The pointer of SMS storage info data
*
* Return:  
*               NULL:  This function works FAIL!
*               OTHER VALUE: This function works SUCCESS
*
* NOTE:
*               1. This function ONLY used in AT handler function.
******************************************************************************/
static char* HdlrGetStorageInfo(char *pLine,u32 uLen,ST_SMSStorage *pInfo)
{
    char *pHead = NULL;
    char *pTail = NULL;

    if((NULL == pLine) || (0 == uLen) || (NULL == pInfo))
    {
        DBG_TRACE(sg_aDbgBuf,"Enter HdlrGetStorageInfo FAIL! Parameter is NULL.");
        return NULL;
    }
    
    pHead = Ql_RIL_FindString(pLine,uLen,CPMS_KEY_STR);
    if(NULL == pHead)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter HdlrGetStorageInfo FAIL! NOT find \"" CPMS_KEY_STR "\"");
        return NULL;
    }
    
    //Get <mem1> of +CPMS
    pHead += Ql_strlen(CPMS_KEY_STR);
    if(FALSE == GetStorageType(pHead,&(pInfo->storage)))
    {
        DBG_TRACE(sg_aDbgBuf,"Enter HdlrGetStorageInfo FAIL! GetStorageType FAIL! string:%c%c%c%c",pHead[0],pHead[1],pHead[2],pHead[3]);
        return NULL;
    }
    
    //Get <used1> of +CPMS
    pHead = Ql_strstr(pHead, STR_COMMA);
    if(NULL == pHead)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter HdlrGetStorageInfo FAIL! Ql_strstr FAIL! NOT find comma.");
        return NULL;
    }
    
    pTail = Ql_strstr((pHead+1), STR_COMMA);
    if(NULL == pTail)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter HdlrGetStorageInfo FAIL! Ql_strstr FAIL! NOT find comma.");
        return NULL;
    }
    
    CONV_STRING_TO_INTEGER((pHead+1),(pTail-(pHead+1)),(pInfo->used));
    
    //Get <total1> of +CPMS
    pHead = pTail;
    pTail = Ql_strstr((pHead+1), STR_COMMA);
    if(NULL == pTail)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter HdlrGetStorageInfo FAIL! Ql_strstr FAIL! NOT find comma.");
        return NULL;
    }    
    
    CONV_STRING_TO_INTEGER((pHead+1),(pTail-(pHead+1)),(pInfo->total));

    DBG_TRACE(sg_aDbgBuf,"Enter HdlrGetStorageInfo SUCCESS. storage:%u,used:%u,total:%u",(pInfo->storage),(pInfo->used),(pInfo->total));
    
    return pLine;
}

/******************************************************************************
* Function:     HdlrSetStorage
*  
* Description:
*               Set storage and get useful parameters.
*
* Parameters:    
*               <pLine>:
*                   [In] The pointer of a string
*               <uLen>
*                   [In] The length of a string
*               <pInfo>:
*                   [In] The pointer of SMS storage info data
*
* Return:  
*               NULL:  This function works FAIL!
*               OTHER VALUE: This function works SUCCESS
*
* NOTE:
*               1. This function ONLY used in AT handler function.
******************************************************************************/
static char* HdlrSetStorage(char *pLine,u32 uLen,ST_SMSStorage *pInfo)
{
    char *pHead = NULL;
    char *pTail = NULL;

    if((NULL == pLine) || (0 == uLen) || (NULL == pInfo))
    {
        DBG_TRACE(sg_aDbgBuf,"Enter HdlrSetStorage FAIL! Parameter is NULL.");
        return NULL;
    }
    
    pHead = Ql_RIL_FindString(pLine,uLen,CPMS_KEY_STR);
    if(NULL == pHead)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter HdlrSetStorage FAIL! NOT find \"" CPMS_KEY_STR "\"");
        return NULL;
    }

    //Get <mem1> of +CPMS
    pHead += Ql_strlen(CPMS_KEY_STR);
    pTail = Ql_strstr(pHead, STR_COMMA);
    if(NULL == pTail)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter HdlrSetStorage FAIL! Ql_strstr FAIL! NOT find comma.");
        return NULL;
    }

    CONV_STRING_TO_INTEGER(pHead,(pTail-pHead),(pInfo->used));

    pHead = pTail;
    pTail = Ql_strstr((pHead+1), STR_COMMA);
    if(NULL == pTail)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter HdlrSetStorage FAIL! Ql_strstr FAIL! NOT find comma.");
        return NULL;
    }

    CONV_STRING_TO_INTEGER((pHead+1),(pTail-(pHead+1)),(pInfo->total));

    DBG_TRACE(sg_aDbgBuf,"Enter HdlrSetStorage SUCCESS. used:%u,total:%u",(pInfo->used),(pInfo->total));
    
    return pLine;
}

/******************************************************************************
* Function:     HdlrReadPDUMsg
*  
* Description:
*               Handler of read PDU message.
*
* Parameters:    
*               <pLine>:
*                   [In] The pointer of a string
*               <uLen>
*                   [In] The length of a string
*               <pPDU>:
*                   [In] The pointer of 'ST_RIL_SMS_PDUInfo' data
*
* Return:  
*               NULL:  This function works FAIL!
*               OTHER VALUE: This function works SUCCESS
*
* NOTE:
*               1. This function ONLY used in AT handler function.
******************************************************************************/
static char* HdlrReadPDUMsg(char *pLine,u32 uLen,ST_RIL_SMS_PDUInfo *pPDU)
{
    char *pHead = NULL;
    char *pTail = NULL;
    u32 uDataLen = 0;
    
    static bool s_bSMSReadContentFlag = FALSE; //FALSE: NOT read SMS content. TRUE: Read SMS content.
    
    if((NULL == pLine) || (0 == uLen) || (NULL == pPDU))
    {
        DBG_TRACE(sg_aDbgBuf,"Enter HdlrReadPDUMsg,FAIL! Parameter is NULL.");
        return NULL;
    }
    
    //Read SMS PDU content
    if(TRUE == s_bSMSReadContentFlag)
    {
        //Initialize
        Ql_memset((pPDU->data),0x00,sizeof(pPDU->data));
    
        uDataLen = (((uLen - 2) < (LIB_SMS_PDU_BUF_MAX_LEN * 2)) ? (uLen - 2) : (LIB_SMS_PDU_BUF_MAX_LEN * 2));
        Ql_memcpy((pPDU->data), pLine, uDataLen);

        (pPDU->length) = uDataLen;  //Get the count of characters in PDU string.
        
        s_bSMSReadContentFlag = FALSE;
        
        DBG_TRACE(sg_aDbgBuf,"Enter HdlrReadPDUMsg,SUCCESS. status:%u,length:%u,s_bSMSReadContentFlag:%d",(pPDU->status),(pPDU->length),s_bSMSReadContentFlag);        
        
        return pLine;
    }
    
    //Read SMS PDU header info
    pHead = Ql_RIL_FindString(pLine,uLen,CMGR_KEY_STR);
    if(NULL == pHead)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter HdlrReadPDUMsg FAIL! NOT find \"" CMGR_KEY_STR "\"");
        return NULL;
    }

    s_bSMSReadContentFlag = TRUE;  //Set flag
    
    //Get <stat> of +CMGR
    pHead += Ql_strlen(CMGR_KEY_STR);
    pTail = Ql_strstr(pHead, STR_COMMA);
    if(NULL == pTail)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter HdlrReadPDUMsg FAIL! Ql_strstr FAIL! NOT find comma.");
        return NULL;
    }
    
    CONV_STRING_TO_INTEGER(pHead,(pTail-pHead),(pPDU->status));
    
    DBG_TRACE(sg_aDbgBuf,"Enter HdlrReadPDUMsg SUCCESS. status:%u,s_bSMSReadContentFlag:%d",(pPDU->status),s_bSMSReadContentFlag);
    
    return pLine;
}

/******************************************************************************
* Function:     HdlrSendPDUMsg
*  
* Description:
*               Handler of read PDU message.
*
* Parameters:    
*               <pLine>:
*                   [In] The pointer of a string
*               <uLen>
*                   [In] The length of a string
*               <pPDU>:
*                   [In] The pointer of 'ST_RIL_SMS_SendPDUInfo' data
*
* Return:  
*               NULL:  This function works FAIL!
*               OTHER VALUE: This function works SUCCESS
*
* NOTE:
*               1. This function ONLY used in AT handler function.
******************************************************************************/
static char* HdlrSendPDUMsg(char *pLine,u32 uLen,ST_RIL_SMS_SendPDUInfo *pInfo)
{
    char *pHead = NULL;
    char *pTail = NULL;
    u8 uCtrlZ = 0x1A;

    if((NULL == pLine) || (0 == uLen) || (NULL == pInfo))
    {
        DBG_TRACE(sg_aDbgBuf,"Enter HdlrSendPDUMsg FAIL! Parameter is NULL.");
        return NULL;
    }
    
    pHead = Ql_RIL_FindString(pLine, uLen, STR_CMGS_HINT);
    if(NULL != pHead)
    {
        Ql_RIL_WriteDataToCore((u8*)(pInfo->pduString),(pInfo->pduLen));
        Ql_RIL_WriteDataToCore(&uCtrlZ,1);
        
        return pLine;
    }
    
    pHead = Ql_RIL_FindString(pLine, uLen, CMGS_KEY_STR);
    if(NULL == pHead)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter HdlrSendPDUMsg FAIL! NOT find \"" CMGS_KEY_STR "\"");
        return NULL;
    }
    
    pHead += Ql_strlen(CMGR_KEY_STR);
    pTail = Ql_strstr(pHead, STR_CR_LF);
    if(NULL == pTail)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter HdlrSendPDUMsg FAIL! NOT find \"" CMGS_KEY_STR "\"");
        return NULL;
    }
    
    CONV_STRING_TO_INTEGER(pHead,(pTail-pHead),(pInfo->mr));
    
    DBG_TRACE(sg_aDbgBuf,"Enter HdlrSendPDUMsg SUCCESS. mr:%u",(pInfo->mr));
    
    return pLine;
}

/******************************************************************************
* Function:     SMS_CMD_GeneralHandler
*  
* Description:
*               The general handler for RIL SMS AT Command
*
* Parameters:    
*               <pLine>:
*                   [In] The pointer of a line string
*               <uLen>:
*                   [In] The length of a line string
*               <pUserData>
*                   [In] The pointer of 'ST_SMS_HdlrUserData' data
*
* Return:  
*               RIL_ATRSP_CONTINUE: Need to wait later AT response
*               RIL_ATRSP_SUCCESS:  AT command run SUCCESS.
*               RIL_ATRSP_FAILED:   AT command run FAIL!
*               OTHER VALUES:       This function works FAIL!
******************************************************************************/
static s32 SMS_CMD_GeneralHandler(char* pLine, u32 uLen, void* pUserData)
{
    u32 uType = 0;
    ST_SMS_HdlrUserData *pParam = (ST_SMS_HdlrUserData*)pUserData;

    if((NULL == pLine) || (0 == uLen) || (NULL == pUserData))
    {
        DBG_TRACE(sg_aDbgBuf,"Enter SMS_CMD_GeneralHandler,FAIL! Parameter is NULL.");
        return RIL_AT_INVALID_PARAM;
    }

    uType = (pParam->uHdlrType);

    switch(uType)
    {
        case HDLR_TYPE_CPMS_READ_CMD:
        {
            ST_SMSStorage *pInfo = (ST_SMSStorage*)(pParam->pUserData);
            
            if(NULL != HdlrGetStorageInfo(pLine, uLen, pInfo))
            {
                DBG_TRACE(sg_aDbgBuf,"Enter SMS_CMD_GeneralHandler,SUCCESS. uType:HDLR_TYPE_CPMS_READ_CMD,storage:%u,used:%u,total:%u",(pInfo->storage),(pInfo->used),(pInfo->total));
                return RIL_ATRSP_CONTINUE;
            }
        }
        break;

        case HDLR_TYPE_CPMS_SET_CMD:
        {
            ST_SMSStorage *pInfo = (ST_SMSStorage*)(pParam->pUserData);
            
            if(NULL != HdlrSetStorage(pLine, uLen, pInfo))
            {
                DBG_TRACE(sg_aDbgBuf,"Enter SMS_CMD_GeneralHandler,SUCCESS. uType:HDLR_TYPE_CPMS_SET_CMD,used:%u,total:%u",(pInfo->used),(pInfo->total));
                return RIL_ATRSP_CONTINUE;
            }
        }
        break;

        case HDLR_TYPE_CMGR_PDU_CMD:
        {
            ST_RIL_SMS_PDUInfo *pInfo = (ST_RIL_SMS_PDUInfo*)(pParam->pUserData);

            if(NULL != HdlrReadPDUMsg(pLine, uLen, pInfo))
            {
                DBG_TRACE(sg_aDbgBuf,"Enter SMS_CMD_GeneralHandler,SUCCESS. uType:HDLR_TYPE_CMGR_PDU_CMD,status:%u,length:%u",(pInfo->status),(pInfo->length));
                return RIL_ATRSP_CONTINUE;
            }
        }
        break;

        case HDLR_TYPE_CMGS_PDU_CMD:
        {
            ST_RIL_SMS_SendPDUInfo *pInfo = (ST_RIL_SMS_SendPDUInfo*)(pParam->pUserData);

            if(NULL != HdlrSendPDUMsg(pLine, uLen, pInfo))
            {
                DBG_TRACE(sg_aDbgBuf,"Enter SMS_CMD_GeneralHandler,SUCCESS. uType:HDLR_TYPE_CMGS_PDU_CMD,length:%u",(pInfo->pduLen));
                return RIL_ATRSP_CONTINUE;
            }
        }
        break;
    
        default:
        {
            DBG_TRACE(sg_aDbgBuf,"Enter SMS_CMD_GeneralHandler,SUCCESS. Warning: NOT support type! uType:%u",uType);
        }
        break;
    }

    SMS_HDLR_CHECK_ERROR(pLine,uLen,"SMS_CMD_GeneralHandler");

    DBG_TRACE(sg_aDbgBuf,"Enter SMS_CMD_GeneralHandler,SUCCESS. uType:%u",uType);

    return RIL_ATRSP_CONTINUE; //continue wait
}

/******************************************************************************
* Function:     CmdGetStorageInfo
*  
* Description:
*               Get SMS storage info
*
* Parameters:    
*               <pCurrMem>:
*                   [In] The pointer of current SMS storage type,same as: 'Enum_RIL_SMS_SMSStorage'
*               <pUsed>:
*                   [In] The pointer of used count in SMS storage
*               <pTotal>
*                   [In] The pointer of total count in SMS storage
*
* Return:  
*               RIL_ATRSP_CONTINUE: Need to wait later AT response
*               RIL_ATRSP_SUCCESS:  AT command run SUCCESS.
*               RIL_ATRSP_FAILED:   AT command run FAIL!
*               OTHER VALUES:       This function works FAIL!
*
* NOTE:
*               1. This function ONLY used in send AT command.
*               2. If you DONOT want to get certain parameter,please set it to NULL.
******************************************************************************/
static s32 CmdGetStorageInfo(u8* pCurrMem, u32* pUsed,u32* pTotal)
{
    s32 iResult = 0;
    char aCmd[SMS_CMD_MAX_LEN] = {0,};
    ST_SMSStorage  sInfo;
    ST_SMS_HdlrUserData sUserData;

    //Initialize
    Ql_memset(&sInfo,0x00,sizeof(sInfo));
    Ql_memset(&sUserData,0x00,sizeof(sUserData));

    Ql_strcpy(aCmd,"AT+CPMS?");

    (sUserData.uHdlrType) = HDLR_TYPE_CPMS_READ_CMD;
    (sUserData.pUserData) = (void*)(&sInfo);

    iResult = Ql_RIL_SendATCmd(aCmd, Ql_strlen(aCmd), SMS_CMD_GeneralHandler,&sUserData, 0);
    if(RIL_ATRSP_SUCCESS != iResult)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter CmdSetStorageInfo,FAIL! AT+CPMS? execute error! Code:%d\r\n",iResult);
        return iResult;
    }

    LIB_SMS_SET_POINTER_VAL(pCurrMem,(sInfo.storage));
    LIB_SMS_SET_POINTER_VAL(pUsed,(sInfo.used));
    LIB_SMS_SET_POINTER_VAL(pTotal,(sInfo.total));

    DBG_TRACE(sg_aDbgBuf,"Enter CmdGetStorageInfo,SUCCESS. storage:%d,used:%d,total:%d",(sInfo.storage),(sInfo.used),(sInfo.total));

    return RIL_ATRSP_SUCCESS;        
}

/******************************************************************************
* Function:     CmdSetStorageInfo
*  
* Description:
*               Set SMS storage type and get SMS storage info.
*
* Parameters:    
*               <uStoType>:
*                   [In] SMS storage type,same as: 'Enum_RIL_SMS_SMSStorage'
*               <pUsed>:
*                   [In] The pointer of used count in SMS storage
*               <pTotal>
*                   [In] The pointer of total count in SMS storage
*
* Return:  
*               RIL_ATRSP_CONTINUE: Need to wait later AT response
*               RIL_ATRSP_SUCCESS:  AT command run SUCCESS.
*               RIL_ATRSP_FAILED:   AT command run FAIL!
*               OTHER VALUES:       This function works FAIL!
*
* NOTE:
*               1. This function ONLY used in send AT command.
******************************************************************************/
static s32 CmdSetStorageInfo(u8 uStoType,u32* pUsed,u32* pTotal)
{
    char aCmd[SMS_CMD_MAX_LEN] = {0,};
    char aStoName[4] = {0};  //SMS storage name
    s32 iLen = 0;
    s32 iResult = 0;

    ST_SMSStorage  sInfo;
    ST_SMS_HdlrUserData sUserData;    

    //Initialize
    Ql_memset(&sInfo,0x00,sizeof(sInfo));
    Ql_memset(&sUserData,0x00,sizeof(sUserData));

    //Check SMS storage type is valid or not
    if(FALSE == ADP_IS_SUPPORT_STORAGE_TYPE(uStoType))
    {
        DBG_TRACE(sg_aDbgBuf,"Enter CmdSetStorageInfo,FAIL! ADP_IS_SUPPORT_STORAGE_TYPE FAIL!");
        return RIL_AT_INVALID_PARAM;
    }

    //Get SMS storage's name refer to <uStoType>
    SMS_GET_STORAGE_NAME(uStoType,aStoName);
    
    iLen = Ql_sprintf(aCmd, "AT+CPMS=\"%s\",\"%s\",\"%s\"", aStoName, aStoName, aStoName);

    (sUserData.uHdlrType) = HDLR_TYPE_CPMS_SET_CMD;
    (sUserData.pUserData) = (void*)(&sInfo);

    iResult = Ql_RIL_SendATCmd(aCmd, iLen, SMS_CMD_GeneralHandler,&sUserData, 0);
    if(RIL_ATRSP_SUCCESS != iResult)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter CmdSetStorageInfo,FAIL! AT+CPMS? execute error! iResult:%d\r\n",iResult);
        return iResult;
    }

    LIB_SMS_SET_POINTER_VAL(pUsed,(sInfo.used));
    LIB_SMS_SET_POINTER_VAL(pTotal,(sInfo.total));

    DBG_TRACE(sg_aDbgBuf,"Enter CmdGetStorageInfo,SUCCESS. uStoType:%d,used:%d,total:%d",uStoType,(sInfo.used),(sInfo.total));
    
    return RIL_ATRSP_SUCCESS;
}

/******************************************************************************
* Function:     CmdReadPDUMsg
*  
* Description:
*               Read a PDU SMS.
*
* Parameters:    
*               <uIndex>:
*                   [In] The SMS index in current SMS storage
*               <pPDU>:
*                   [In] The pointer of ST_PDU_SMS data
*
* Return:  
*               RIL_ATRSP_CONTINUE: Need to wait later AT response
*               RIL_ATRSP_SUCCESS:  AT command run SUCCESS.
*               RIL_ATRSP_FAILED:   AT command run FAIL!
*               OTHER VALUES:       This function works FAIL!
*
* NOTE:
*               1. This function ONLY used in send AT command.
******************************************************************************/
static s32 CmdReadPDUMsg(u32 uIndex, ST_RIL_SMS_PDUInfo* pPDU)
{
    char aCmd[SMS_CMD_MAX_LEN] = {0,};
    s32 iResult = 0;
    u8 uStoType = 0;
    u32 uStoTot = 0;
    s32 iLen = 0;
    u32 uCoreIdx = 0;

    ST_SMS_HdlrUserData sUserData;

    if(NULL == pPDU)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter CmdReadPDUMsg,FAIL! Parameter is NULL.");
        return RIL_AT_INVALID_PARAM;
    }

    //Initialize
    Ql_memset(&sUserData,0x00,sizeof(sUserData));

    //Check parameter <uIndex>
    iResult = CmdGetStorageInfo(&uStoType,NULL,&uStoTot);
    if(RIL_ATRSP_SUCCESS != iResult)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter CmdReadPDUMsg,FAIL! iResult:%d",iResult);
        return iResult;
    }

    if((uIndex < 1) || (uIndex > uStoTot))
    {
        DBG_TRACE(sg_aDbgBuf,"Enter CmdReadPDUMsg,FAIL! uIndex is INVALID. uIndex:%d",uIndex);
        return RIL_AT_INVALID_PARAM;
    }

    uCoreIdx = ADP_SMS_ConvIdxToCoreIdx(uStoType,uIndex,uStoTot);
    if(0 == uCoreIdx)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter CmdReadPDUMsg,FAIL! ADP_SMS_ConvIdxToCoreIdx FAIL! uStoType:%u,index:%u,uStoTot:%u", uStoType,uIndex,uStoTot);
        return RIL_AT_INVALID_PARAM;
    }

    //Set to PDU mode
    SMS_SET_PDU_MODE(iResult,"CmdReadPDUMsg");

    (sUserData.uHdlrType) = HDLR_TYPE_CMGR_PDU_CMD;
    (sUserData.pUserData) = pPDU;

    iLen = Ql_sprintf(aCmd, "AT+CMGR=%u", uCoreIdx);
    iResult = Ql_RIL_SendATCmd(aCmd, iLen, SMS_CMD_GeneralHandler, &sUserData, 0);
    if(iResult != RIL_ATRSP_SUCCESS)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter CmdReadPDUMsg,FAIL! CMD: %s execute FAIL!",aCmd);
        return iResult;
    }

    DBG_TRACE(sg_aDbgBuf,"Enter CmdReadPDUMsg,SUCCESS. CMD: %s,uStoType:%u,uStoTot:%u,index:%u,length:%u", aCmd,uStoType,uStoTot,uIndex,(pPDU->length));

    return iResult;
}

/******************************************************************************
* Function:     CmdSendPDUMsg
*  
* Description:
*               This function is used to send PDU message
*
* Parameters:    
*               <pPDUStr>:
*                   [In] The pointer of PDU string
*               <uPDUStrLen>:
*                   [In] The length of PDU string
*               <pMsgRef>
*                   [Out] The pointer of message reference number
*
* Return:  
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*                                      and then call Ql_RIL_Initialize to initialize RIL.
* Note:
*                1. If you DONOT want to get <pMsgRef> value,please set it to NULL.
******************************************************************************/
static s32 CmdSendPDUMsg(char* pPDUStr,u32 uPDUStrLen,u32 *pMsgRef)
{
    char aCmd[SMS_CMD_MAX_LEN] = {0,};
    u32 uTPDULen = 0;
    u32 uSCALen = 0;
    bool bResult = FALSE;
    s32 iResult = 0;
    s32 iLen = 0;
    ST_RIL_SMS_SendPDUInfo sInfo;
    ST_SMS_HdlrUserData sUserData;

    //Initialize
    Ql_memset(&sInfo,0x00,sizeof(sInfo));
    Ql_memset(&sUserData,0x00,sizeof(sUserData));
    
    if((NULL == pPDUStr) || (0 == uPDUStrLen))
    {
        DBG_TRACE(sg_aDbgBuf,"Enter CmdSendPDUMsg,FAIL! Parameter is NULL.");
        return RIL_AT_INVALID_PARAM;
    }

    bResult = LIB_SMS_IsValidHexStr(pPDUStr,uPDUStrLen);
    if(FALSE == bResult)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter CmdSendPDUMsg,FAIL! LIB_SMS_IsValidHexStr FAIL!");
        return RIL_AT_INVALID_PARAM;
    }

    if((uPDUStrLen % 2) != 0)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter CmdSendPDUMsg,FAIL! (uPDUStrLen % 2) != 0");
        return RIL_AT_INVALID_PARAM;
    }
    
    CONV_STRING_TO_INTEGER(pPDUStr,2,uSCALen);
    uTPDULen = ((uPDUStrLen / 2) - (uSCALen + 1));
    
    //Set to PDU mode
    SMS_SET_PDU_MODE(iResult,"RIL_SMS_ReadSMS_PDU");
    
    (sInfo.pduString) = pPDUStr;
    (sInfo.pduLen) = uPDUStrLen;
    (sUserData.uHdlrType) = HDLR_TYPE_CMGS_PDU_CMD;
    (sUserData.pUserData) = &sInfo;    
    
    iLen = Ql_sprintf(aCmd, "AT+CMGS=%d", uTPDULen);
    iResult = Ql_RIL_SendATCmd(aCmd, iLen, SMS_CMD_GeneralHandler,&sUserData, 0);    
    if(iResult != RIL_ATRSP_SUCCESS)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter CmdSendPDUMsg,FAIL! CMD: %s execute FAIL!",aCmd);
        return iResult;
    }

    LIB_SMS_SET_POINTER_VAL(pMsgRef,(sInfo.mr));

    DBG_TRACE(sg_aDbgBuf,"Enter CmdSendPDUMsg,SUCCESS. mr:%u",(sInfo.mr));
    
    return RIL_ATRSP_SUCCESS;
}

/******************************************************************************
* Function:     RIL_SMS_GetStorage
*  
* Description:
*               Get SMS storage info
*
* Parameters:    
*               <pCurrMem>:
*                   [In] The pointer of current SMS storage type,same as: 'Enum_RIL_SMS_SMSStorage'
*               <pUsed>:
*                   [In] The pointer of used count in SMS storage
*               <pTotal>
*                   [In] The pointer of total count in SMS storage
*
* Return:  
*               RIL_ATRSP_CONTINUE: Need to wait later AT response
*               RIL_ATRSP_SUCCESS:  AT command run SUCCESS.
*               RIL_ATRSP_FAILED:   AT command run FAIL!
*               OTHER VALUES:       This function works FAIL!
* NOTE:
*               1. If you DONOT want to get <pUsed>,pTotal value,please set it to NULL.
******************************************************************************/
s32 RIL_SMS_GetStorage(u8* pCurrMem, u32* pUsed,u32* pTotal)
{
    s32 iResult = 0;

    iResult = CmdGetStorageInfo((u8*)pCurrMem,pUsed,pTotal);

    DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_GetStorage,SUCCESS. iResult:%d",iResult);

    return iResult;
}

/******************************************************************************
* Function:     RIL_SMS_SetStorage
*  
* Description:
*               Set SMS storage
*
* Parameters:    
*               <storage>:
*                   [In] SMS storage,same as 'Enum_RIL_SMS_SMSStorage'
*
* Return:  
*               RIL_ATRSP_SUCCESS:  This function works SUCCESS.
*               RIL_ATRSP_CONTINUE: Need to wait later AT response
*               RIL_ATRSP_SUCCESS:  AT command run SUCCESS.
*               RIL_ATRSP_FAILED:   AT command run FAIL!
*               OTHER VALUES:       This function works FAIL!
*
* NOTE:
*               1. If you DONOT want to get <pUsed>,pTotal value,please set it to NULL.
******************************************************************************/
s32 RIL_SMS_SetStorage(Enum_RIL_SMS_StorageType eStorage,u32* pUsed,u32* pTotal)
{
    s32 iResult = 0;

    iResult = CmdSetStorageInfo(eStorage,pUsed,pTotal);

    DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SetStorage,SUCCESS. iResult:%d",iResult);
    
    return iResult;
}

/******************************************************************************
* Function:     RIL_SMS_ReadSMS_PDU
*  
* Description:
*               Read a PDU SMS
*
* Parameters:    
*               <index>:
*                   [In] The SMS index in current SMS storage
*               <pduInfo>:
*                   [In] The pointer of 'ST_RIL_SMS_PDUInfo' data
*
* Return:  
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*                                      and then call Ql_RIL_Initialize to initialize RIL.
******************************************************************************/
s32 RIL_SMS_ReadSMS_PDU(u32 uIndex, ST_RIL_SMS_PDUInfo* pPDUInfo)
{
    s32 iResult = 0;
    bool bResult = FALSE;

    if(NULL == pPDUInfo)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_ReadSMS_PDU,FAIL! Parameter is NULL.");
        return RIL_AT_INVALID_PARAM;
    }

    //Initialize
    Ql_memset(pPDUInfo,0x00,sizeof(ST_RIL_SMS_PDUInfo));

    iResult = CmdReadPDUMsg(uIndex,pPDUInfo);
    if(iResult != RIL_ATRSP_SUCCESS)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_ReadSMS_PDU,FAIL! CmdReadPDUMsg FAIL!");
        return iResult;
    }

    //The message of <uIndex> position is NOT exist.
    if(0 == (pPDUInfo->length))
    {
        SMS_SET_INVALID_PDU_INFO(pPDUInfo);
    
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_ReadSMS_PDU,SUCCESS. NOTE:(pPDUInfo->length) is 0.");
        return RIL_ATRSP_SUCCESS;
    }

    //Check the <pPDUInfo>
    if(FALSE == IS_VALID_PDU_INFO(pPDUInfo))
    {
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_ReadSMS_PDU,FAIL! IS_VALID_PDU_INFO FAIL!");
        return RIL_AT_FAILED;
    }

    LIB_SMS_CHECK_PDU_STR((pPDUInfo->data),(pPDUInfo->length),bResult);
    if(FALSE == bResult)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_ReadSMS_PDU,WARNING! LIB_SMS_CHECK_PDU_STR FAIL!");
    }

    DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_ReadSMS_PDU,SUCCESS. status:%u,length:%u",(pPDUInfo->status),(pPDUInfo->length));

    return RIL_ATRSP_SUCCESS;
}

/******************************************************************************
* Function:     RIL_SMS_ReadSMS_Text
*  
* Description:
*               Read a TEXT SMS
*
* Parameters:    
*               <uIndex>:
*                   [In] The SMS index in current SMS storage
*               <eCharset>:
*                   [In] Character set enum value
*               <pTextInfo>
*                   [In] The pointer of TEXT SMS info
*
* Return:  
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*                                      and then call Ql_RIL_Initialize to initialize RIL.
******************************************************************************/
s32 RIL_SMS_ReadSMS_Text(u32 uIndex, LIB_SMS_CharSetEnum eCharset,ST_RIL_SMS_TextInfo* pTextInfo)
{
    ST_RIL_SMS_PDUInfo *pPDUInfo = NULL;
    LIB_SMS_PDUParamStruct *pSMSParam = NULL;
    s32 iResult = 0;
    bool bResult = FALSE;

    if(NULL == pTextInfo)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_ReadSMS_Text,FAIL! Parameter is NULL.");
        return RIL_AT_INVALID_PARAM;
    }

    //Initialize
    Ql_memset(pTextInfo,0x00,sizeof(ST_RIL_SMS_TextInfo));

    if(FALSE == LIB_SMS_IS_SUPPORT_CHARSET(eCharset))
    {
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_ReadSMS_Text,FAIL! LIB_SMS_IS_SUPPORT_CHARSET FAIL! eCharset:%d",eCharset);
        return RIL_AT_INVALID_PARAM;
    }

    pPDUInfo = (ST_RIL_SMS_PDUInfo*)Ql_MEM_Alloc(sizeof(ST_RIL_SMS_PDUInfo));
    if(NULL == pPDUInfo)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_ReadSMS_Text,FAIL! Ql_MEM_Alloc FAIL! size:%u",sizeof(ST_RIL_SMS_PDUInfo));
        return RIL_AT_FAILED;
    }

    pSMSParam = (LIB_SMS_PDUParamStruct*)Ql_MEM_Alloc(sizeof(LIB_SMS_PDUParamStruct));
    if(NULL == pSMSParam)
    {
        Ql_MEM_Free(pPDUInfo);
    
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_ReadSMS_Text,FAIL! Ql_MEM_Alloc FAIL! size:%u",sizeof(LIB_SMS_PDUParamStruct));
        return RIL_AT_FAILED;
    }

    //Initialize
    Ql_memset(pPDUInfo,0x00,sizeof(ST_RIL_SMS_PDUInfo));
    Ql_memset(pSMSParam,0x00,sizeof(LIB_SMS_PDUParamStruct));

    iResult = CmdReadPDUMsg(uIndex,pPDUInfo);
    if(iResult != RIL_ATRSP_SUCCESS)
    {    
        Ql_MEM_Free(pPDUInfo);
        Ql_MEM_Free(pSMSParam);
    
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_ReadSMS_Text,FAIL! CmdReadPDUMsg FAIL!");
        return iResult;
    }

    //The message of <uIndex> position is NOT exist.
    if(0 == (pPDUInfo->length))
    {
        SMS_SET_INVALID_TEXT_INFO(pTextInfo);
    
        Ql_MEM_Free(pPDUInfo);
        Ql_MEM_Free(pSMSParam);
    
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_ReadSMS_PDU,SUCCESS. NOTE: (pPDUInfo->length) is 0.");
        return RIL_ATRSP_SUCCESS;
    }

    //Check the <pPDUInfo>
    if(FALSE == IS_VALID_PDU_INFO(pPDUInfo))
    {
        Ql_MEM_Free(pPDUInfo);
        Ql_MEM_Free(pSMSParam);
    
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_ReadSMS_Text,FAIL! IS_VALID_PDU_INFO FAIL!");
        return RIL_AT_FAILED;
    }

    bResult = LIB_SMS_DecodePDUStr((pPDUInfo->data),(pPDUInfo->length),pSMSParam);
    if(FALSE == bResult)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_ReadSMS_Text,FAIL! LIB_SMS_DecodePDUStr FAIL! PDU length:%u",(pPDUInfo->length));
    
        Ql_MEM_Free(pPDUInfo);
        Ql_MEM_Free(pSMSParam);
    
        return RIL_AT_FAILED;
    }

    (pTextInfo->status) = (pPDUInfo->status);

    bResult = ConvSMSParamToTextInfo(eCharset,pSMSParam,pTextInfo);
    if(FALSE == bResult)
    {
        Ql_MEM_Free(pPDUInfo);
        Ql_MEM_Free(pSMSParam);
    
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_ReadSMS_Text,FAIL! ConvSMSParamToTextInfo FAIL!");
        return RIL_AT_FAILED;
    }

    Ql_MEM_Free(pPDUInfo);
    Ql_MEM_Free(pSMSParam);

    DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_ReadSMS_Text,SUCCESS. status:%u",(pTextInfo->status));

    return RIL_AT_SUCCESS;
}

/******************************************************************************
* Function:     RIL_SMS_SendSMS_PDU
*  
* Description:
*               This function is used to send PDU message
*
* Parameters:    
*               <pPDUStr>:
*                   [In] The pointer of PDU string
*               <uPDUStrLen>:
*                   [In] The length of PDU string
*               <pMsgRef>
*                   [Out] The pointer of message reference number
*
* Return:  
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*                                      and then call Ql_RIL_Initialize to initialize RIL.
* Note:
*                1. If you DONOT want to get <pMsgRef> value,please set it to NULL.
******************************************************************************/
s32 RIL_SMS_SendSMS_PDU(char* pPDUStr,u32 uPDUStrLen,u32 *pMsgRef)
{
    bool bResult = FALSE;
    s32 iResult = 0;
    
    if((NULL == pPDUStr) || (0 == uPDUStrLen))
    {
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SendSMS_PDU,FAIL! Parameter is NULL.");
        return RIL_AT_INVALID_PARAM;
    }

    LIB_SMS_CHECK_SUBMIT_PDU_STR_FOR_SEND(pPDUStr,uPDUStrLen,bResult);
    if(FALSE == bResult)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SendSMS_PDU,FAIL! LIB_SMS_CHECK_SUBMIT_PDU_STR_FOR_SEND FAIL!");
        return RIL_AT_INVALID_PARAM;
    }

    iResult = CmdSendPDUMsg(pPDUStr,uPDUStrLen,pMsgRef);

    DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SendSMS_PDU,SUCCESS. uPDUStrLen:%u,iResult:%d",uPDUStrLen,iResult);

    return iResult;
}

/******************************************************************************
* Function:     RIL_SMS_SendSMS_Text
*  
* Description:
*               This function is used to send TEXT message
*
* Parameters:    
*               <pNumber>:
*                   [In] The pointer of phone number
*               <uNumberLen>:
*                   [In] The length of phone number
*               <eCharset>
*                   [In] CharSet,it's value is same as 'LIB_SMS_CharSetEnum'
*               <pMsg>
*                   [In] The pointer of message content
*               <uMsgLen>
*                   [In] The length of message content
*               <pMsgRef>
*                   [Out] The pointer of message reference number
*
* Return:  
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*                                      and then call Ql_RIL_Initialize to initialize RIL.
* Note:
*                1. If you DONOT want to get <pMsgRef> value,please set it to NULL.
******************************************************************************/
s32 RIL_SMS_SendSMS_Text(char* pNumber, u8 uNumberLen, LIB_SMS_CharSetEnum eCharset, u8* pMsg, u32 uMsgLen,u32 *pMsgRef)
{
    LIB_SMS_PDUParamStruct *pParam = NULL;
    LIB_SMS_SubmitPDUParamStruct *pSubmitParam = NULL;
    LIB_SMS_PDUInfoStruct *pInfo = NULL;
    bool bResult = FALSE;
    char *pPDUStr = NULL;
    u32 uPDUStrLen = 0;
    s32 iResult = 0;

    if((NULL == pNumber) || (0 == uNumberLen) || (NULL == pMsg))
    {
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SendSMS_Text,FAIL! Parameter is NULL.");
        return RIL_AT_INVALID_PARAM;
    }

    if(FALSE == LIB_SMS_IS_SUPPORT_CHARSET(eCharset))
    {
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SendSMS_Text,FAIL! LIB_SMS_IS_SUPPORT_CHARSET FAIL! eCharset:%u",eCharset);
        return RIL_AT_INVALID_PARAM;
    }

    pParam = (LIB_SMS_PDUParamStruct*)Ql_MEM_Alloc(sizeof(LIB_SMS_PDUParamStruct));
    if(NULL == pParam)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SendSMS_Text,FAIL! Ql_MEM_Alloc FAIL! size:%u",sizeof(LIB_SMS_PDUParamStruct));
        return RIL_AT_FAILED;
    }

    pInfo = (LIB_SMS_PDUInfoStruct*)Ql_MEM_Alloc(sizeof(LIB_SMS_PDUInfoStruct));
    if(NULL == pInfo)
    {
        Ql_MEM_Free(pParam);
    
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SendSMS_Text,FAIL! Ql_MEM_Alloc FAIL! size:%u",sizeof(LIB_SMS_PDUInfoStruct));
        return RIL_AT_FAILED;
    }

    //Initialize
    Ql_memset(pParam,0x00,sizeof(LIB_SMS_PDUParamStruct));
    Ql_memset(pInfo,0x00,sizeof(LIB_SMS_PDUInfoStruct));
    pSubmitParam = &((pParam->sParam).sSubmitParam);

    //Set <FO>
    (pParam->uFO) = LIB_SMS_DEFAULT_FO_IN_SUBMIT_PDU;

    //Set <DA>
    bResult = ConvStringToPhoneNumber(pNumber,uNumberLen,&(pSubmitParam->sDA));
    if(FALSE == bResult)
    {
        Ql_MEM_Free(pParam);
        Ql_MEM_Free(pInfo);

        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SendSMS_Text,FAIL! ConvStringToPhoneNumber FAIL!");
        return RIL_AT_FAILED;
    }

    //Set <PID>
    (pSubmitParam->uPID) = LIB_SMS_PDU_DEFAULT_PID;
    //Set <DCS>
    LIB_SMS_SET_DEFAULT_DCS_IN_SUBMIT_PDU(eCharset,(pSubmitParam->uDCS));
    //Set <VP>
    ((pSubmitParam->sVP).uRelative) = LIB_SMS_SUBMIT_PDU_DEFAULT_VP_RELATIVE;

    //Set <UD>
    ((pSubmitParam->sUserData).uLen) = sizeof((pSubmitParam->sUserData).aUserData);
    bResult = LIB_SMS_ConvCharSetToAlpha(
            eCharset,
            pMsg,
            uMsgLen,
            (pSubmitParam->uDCS),
            ((pSubmitParam->sUserData).aUserData),
            &((pSubmitParam->sUserData).uLen)
        );
    
    if(FALSE == bResult)
    {
        Ql_MEM_Free(pParam);
        Ql_MEM_Free(pInfo);

        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SendSMS_Text,FAIL! LIB_SMS_ConvCharSetToAlpha FAIL!");
        return RIL_AT_FAILED;
    }

    bResult = LIB_SMS_EncodeSubmitPDU(pParam,pInfo);
    if(FALSE == bResult)
    {
        Ql_MEM_Free(pParam);
        Ql_MEM_Free(pInfo);

        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SendSMS_Text,FAIL! LIB_SMS_EncodeSubmitPDU FAIL!");
        return RIL_AT_FAILED;
    }

    Ql_MEM_Free(pParam);  //Not need <pParam>,free it directly
    pParam = NULL;

    pPDUStr = (char*)Ql_MEM_Alloc((pInfo->uLen) * 2);
    if(NULL == pPDUStr)
    {
        Ql_MEM_Free(pInfo);
        
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SendSMS_Text,FAIL! Ql_MEM_Alloc FAIL! size:%d",((pInfo->uLen) * 2));
        return RIL_AT_FAILED;
    }

    uPDUStrLen = ((pInfo->uLen) * 2);
    bResult = LIB_SMS_ConvHexOctToHexStr((pInfo->aPDUOct),(pInfo->uLen),pPDUStr,(u16*)&uPDUStrLen);    
    if(NULL == pPDUStr)
    {
        Ql_MEM_Free(pInfo);
        Ql_MEM_Free(pPDUStr);
        
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SendSMS_Text,FAIL! Ql_MEM_Alloc FAIL! size:%d",((pInfo->uLen) * 2));
        return RIL_AT_FAILED;
    }

    //Now send SUBMIT-PDU message
    LIB_SMS_CHECK_SUBMIT_PDU_STR_FOR_SEND(pPDUStr,uPDUStrLen,bResult);
    if(FALSE == bResult)
    {
        Ql_MEM_Free(pInfo);
        Ql_MEM_Free(pPDUStr);
    
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SendSMS_PDU,FAIL! LIB_SMS_CHECK_SUBMIT_PDU_STR_FOR_SEND FAIL!");
        return RIL_AT_INVALID_PARAM;
    }

    iResult = CmdSendPDUMsg(pPDUStr,uPDUStrLen,pMsgRef);

    DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SendSMS_PDU,SUCCESS. iResult:%d",iResult);

    Ql_MEM_Free(pInfo);
    Ql_MEM_Free(pPDUStr);

    return iResult;
}

/******************************************************************************
* Function:     RIL_SMS_SendSMS_Text_Ext
*  
* Description:
*               This function is used to send TEXT message witch external info
*
* Parameters:    
*               <pNumber>:
*                   [In] The pointer of phone number
*               <uNumberLen>:
*                   [In] The length of phone number
*               <eCharset>
*                   [In] CharSet,it's value is same as 'LIB_SMS_CharSetEnum'
*               <pMsg>
*                   [In] The pointer of message content
*               <uMsgLen>
*                   [In] The length of message content
*               <pMsgRef>
*                   [Out] The pointer of message reference number
*               <pExt>
*                   [In]  The pointer of 'ST_RIL_SMS_SendExt' data
*                         + conPres  con-SMS present or not
*                         + con      'ST_RIL_SMS_Con' data
*
* Return:  
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*                                      and then call Ql_RIL_Initialize to initialize RIL.
* Note:
*                1. If you DONOT want to get <pMsgRef> value,please set it to NULL.
*                2. If you want to send normal SMS, you can set <pExt> to NULL.
******************************************************************************/
s32 RIL_SMS_SendSMS_Text_Ext(char* pNumber, u8 uNumberLen, LIB_SMS_CharSetEnum eCharset, u8* pMsg, u32 uMsgLen,u32 *pMsgRef,ST_RIL_SMS_SendExt *pExt)
{
    LIB_SMS_PDUParamStruct *pParam = NULL;
    LIB_SMS_SubmitPDUParamStruct *pSubmitParam = NULL;
    LIB_SMS_PDUInfoStruct *pInfo = NULL;
    bool bResult = FALSE;
    char *pPDUStr = NULL;
    u32 uPDUStrLen = 0;
    s32 iResult = 0;

    if((NULL == pNumber) || (0 == uNumberLen) || (NULL == pMsg))
    {
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SendSMS_Text_Ext,FAIL! Parameter is NULL.");
        return RIL_AT_INVALID_PARAM;
    }

    if(FALSE == LIB_SMS_IS_SUPPORT_CHARSET(eCharset))
    {
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SendSMS_Text_Ext,FAIL! LIB_SMS_IS_SUPPORT_CHARSET FAIL! eCharset:%u",eCharset);
        return RIL_AT_INVALID_PARAM;
    }

    if((pExt != NULL) && (TRUE == pExt->conPres))
    {
        if(    ((pExt->con.msgType) !=  LIB_SMS_UD_TYPE_CON_6_BYTE)
            && ((pExt->con.msgType) !=  LIB_SMS_UD_TYPE_CON_7_BYTE)
          )
        {
            DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SendSMS_Text_Ext,WARNING! msgType:%d is INVALID,use default!",pExt->con.msgType);
            (pExt->con.msgType) = LIB_SMS_UD_TYPE_CON_DEFAULT;
        }

        if(FALSE == IsValidConParam(&(pExt->con)))
        {
            DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SendSMS_Text_Ext,FAIL! IsValidConParam FAIL!");
            return RIL_AT_INVALID_PARAM;
        }
    }

    pParam = (LIB_SMS_PDUParamStruct*)Ql_MEM_Alloc(sizeof(LIB_SMS_PDUParamStruct));
    if(NULL == pParam)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SendSMS_Text_Ext,FAIL! Ql_MEM_Alloc FAIL! size:%u",sizeof(LIB_SMS_PDUParamStruct));
        return RIL_AT_FAILED;
    }

    pInfo = (LIB_SMS_PDUInfoStruct*)Ql_MEM_Alloc(sizeof(LIB_SMS_PDUInfoStruct));
    if(NULL == pInfo)
    {
        Ql_MEM_Free(pParam);
    
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SendSMS_Text_Ext,FAIL! Ql_MEM_Alloc FAIL! size:%u",sizeof(LIB_SMS_PDUInfoStruct));
        return RIL_AT_FAILED;
    }

    //Initialize
    Ql_memset(pParam,0x00,sizeof(LIB_SMS_PDUParamStruct));
    Ql_memset(pInfo,0x00,sizeof(LIB_SMS_PDUInfoStruct));
    pSubmitParam = &((pParam->sParam).sSubmitParam);

    //Set <FO>
    (pParam->uFO) = LIB_SMS_DEFAULT_FO_IN_SUBMIT_PDU;
    if((pExt != NULL) && (TRUE == pExt->conPres))
    {
        (pParam->uFO) |= (LIB_SMS_PDU_FO_UDHI_BIT_HAS_UDH << 6);
        pSubmitParam->sConSMSParam.uMsgType = pExt->con.msgType;
        pSubmitParam->sConSMSParam.uMsgRef = pExt->con.msgRef;
        pSubmitParam->sConSMSParam.uMsgSeg= pExt->con.msgSeg;
        pSubmitParam->sConSMSParam.uMsgTot= pExt->con.msgTot;
    }

    //Set <DA>
    bResult = ConvStringToPhoneNumber(pNumber,uNumberLen,&(pSubmitParam->sDA));
    if(FALSE == bResult)
    {
        Ql_MEM_Free(pParam);
        Ql_MEM_Free(pInfo);

        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SendSMS_Text,FAIL! ConvStringToPhoneNumber FAIL!");
        return RIL_AT_FAILED;
    }

    //Set <PID>
    (pSubmitParam->uPID) = LIB_SMS_PDU_DEFAULT_PID;
    //Set <DCS>
    LIB_SMS_SET_DEFAULT_DCS_IN_SUBMIT_PDU(eCharset,(pSubmitParam->uDCS));
    //Set <VP>
    ((pSubmitParam->sVP).uRelative) = LIB_SMS_SUBMIT_PDU_DEFAULT_VP_RELATIVE;

    //Set <UD>
    ((pSubmitParam->sUserData).uLen) = sizeof((pSubmitParam->sUserData).aUserData);
    bResult = LIB_SMS_ConvCharSetToAlpha(
            eCharset,
            pMsg,
            uMsgLen,
            (pSubmitParam->uDCS),
            ((pSubmitParam->sUserData).aUserData),
            &((pSubmitParam->sUserData).uLen)
        );
    
    if(FALSE == bResult)
    {
        Ql_MEM_Free(pParam);
        Ql_MEM_Free(pInfo);

        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SendSMS_Text,FAIL! LIB_SMS_ConvCharSetToAlpha FAIL!");
        return RIL_AT_FAILED;
    }

    bResult = LIB_SMS_EncodeSubmitPDU(pParam,pInfo);
    if(FALSE == bResult)
    {
        Ql_MEM_Free(pParam);
        Ql_MEM_Free(pInfo);

        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SendSMS_Text,FAIL! LIB_SMS_EncodeSubmitPDU FAIL!");
        return RIL_AT_FAILED;
    }

    Ql_MEM_Free(pParam);  //Not need <pParam>,free it directly
    pParam = NULL;

    pPDUStr = (char*)Ql_MEM_Alloc((pInfo->uLen) * 2);
    if(NULL == pPDUStr)
    {
        Ql_MEM_Free(pInfo);
        
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SendSMS_Text,FAIL! Ql_MEM_Alloc FAIL! size:%d",((pInfo->uLen) * 2));
        return RIL_AT_FAILED;
    }

    uPDUStrLen = ((pInfo->uLen) * 2);
    bResult = LIB_SMS_ConvHexOctToHexStr((pInfo->aPDUOct),(pInfo->uLen),pPDUStr,(u16*)&uPDUStrLen);    
    if(NULL == pPDUStr)
    {
        Ql_MEM_Free(pInfo);
        Ql_MEM_Free(pPDUStr);
        
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SendSMS_Text,FAIL! Ql_MEM_Alloc FAIL! size:%d",((pInfo->uLen) * 2));
        return RIL_AT_FAILED;
    }

    //Now send SUBMIT-PDU message
    LIB_SMS_CHECK_SUBMIT_PDU_STR_FOR_SEND(pPDUStr,uPDUStrLen,bResult);
    if(FALSE == bResult)
    {
        Ql_MEM_Free(pInfo);
        Ql_MEM_Free(pPDUStr);
    
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SendSMS_PDU,FAIL! LIB_SMS_CHECK_SUBMIT_PDU_STR_FOR_SEND FAIL!");
        return RIL_AT_INVALID_PARAM;
    }

    iResult = CmdSendPDUMsg(pPDUStr,uPDUStrLen,pMsgRef);

    DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_SendSMS_PDU,SUCCESS. iResult:%d",iResult);

    Ql_MEM_Free(pInfo);
    Ql_MEM_Free(pPDUStr);

    return iResult;
}

/******************************************************************************
* Function:     RIL_SMS_DeleteSMS
*  
* Description:
*               This function deletes SMS messages in current SMS storage.
*
* Parameters:    
*               index:
*                   [In] The index number of SMS message.
*               flag:
*                   [In] Delete flag , which is one value of 'Enum_RIL_SMS_DeleteFlag'.
*
* Return:  
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*                                      and then call Ql_RIL_Initialize to initialize RIL.
******************************************************************************/
s32 RIL_SMS_DeleteSMS(u32 uIndex,Enum_RIL_SMS_DeleteFlag eDelFlag)
{
    s32 iResult = 0;
    u8 uStoType = 0;  //Current memory type,same as: 'Enum_RIL_SMS_SMSStorage'
    u32 uStoTot = 0;   //The total count of current memory
    u32 uCoreIdx = 0;
    
    s32 iLen = 0;
    char aCmd[SMS_CMD_MAX_LEN] = {0,};
    
    //Check parameter <eDelFlag>
    if(eDelFlag > RIL_SMS_DEL_ALL_MSG)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_DeleteSMS,FAIL! eDelFlag is INVALID. eDelFlag:%d", eDelFlag);
        return RIL_AT_INVALID_PARAM;
    }

    //Get the total count of current memory type
    iResult = CmdGetStorageInfo(&uStoType,NULL,&uStoTot);
    if(iResult != RIL_ATRSP_SUCCESS)
    {
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_DeleteSMS,FAIL! CmdGetStorageInfo FAIL! AT+CPMS? execute FAL!");
        return RIL_AT_INVALID_PARAM;
    }

    //Check parameter <uIndex>
    if((RIL_SMS_DEL_INDEXED_MSG == eDelFlag)
        && ((uIndex < 1) || (uIndex > uStoTot))
    )
    {
        DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_DeleteSMS,FAIL! uIndex is INVALID. eDelFlag:%d,uIndex:%u,uCurrStoTot:%u", eDelFlag,uIndex,uStoTot);
        return RIL_AT_INVALID_PARAM;
    }

    //Covert RIL SMS index to core SMS index
    if(RIL_SMS_DEL_INDEXED_MSG == eDelFlag)
    {
        uCoreIdx = ADP_SMS_ConvIdxToCoreIdx(uStoType,uIndex,uStoTot);
        if(0 == uCoreIdx)
        {
            DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_DeleteSMS,FAIL! ADP_SMS_ConvIdxToCoreIdx FAIL! uCurrStoType:%u,uIndex:%u,uCurrStoTot:%u", uStoType,uIndex,uStoTot);
            return RIL_AT_INVALID_PARAM;
        }
    }
    else
    {
        uCoreIdx = uIndex;
    }
    
    //Set to PDU mode
    SMS_SET_PDU_MODE(iResult,"RIL_SMS_DeleteSMS");
    
    //Delete SMS use AT+CMGD=<index>,<delflag>    
    iLen = Ql_sprintf(aCmd, "AT+CMGD=%d,%d", uIndex,eDelFlag);
    iResult = Ql_RIL_SendATCmd(aCmd, iLen, NULL, NULL, 0);
    
    DBG_TRACE(sg_aDbgBuf,"Enter RIL_SMS_DeleteSMS,SUCCESS. CMD: %s,uStoType:%u,uStoTot:%u,eDelFlag:%u,uIndex:%u", aCmd,uStoType,uStoTot,eDelFlag,uIndex);

    return iResult;
}

#endif  //#if (defined(__OCPU_RIL_SUPPORT__) && defined(__OCPU_RIL_SMS_SUPPORT__))

