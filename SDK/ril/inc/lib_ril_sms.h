/*==========================================================================
 |               Quectel OpenCPU --  Library header files
 |
 |              Copyright (c) 2013 Quectel Ltd.
 |
 |--------------------------------------------------------------------------
 | File Description
 | ----------------
 |      This file defines interfaces which are used in 'ril_sms.c'.
 |      NOTE: The interfaces are based on portable codes. Therefore they are not related to platform.
 |
 |--------------------------------------------------------------------------
 |
 |  Designed by     :   Vicent GAO
 |  Coded    by     :   Vicent GAO
 |  Tested   by     :   Vicent GAO
 |--------------------------------------------------------------------------
 | Revision History
 | ----------------
 |  2013/11/23       Vicent GAO        Create this file by ROTVG00006-P01
 |  2015/06/03       Vicent GAO        Add support for read/send con-sms by ROTVG00006-P05
 |  2015/06/05       Vicent GAO        Add support for CharSet: IRA by ROTVG00006-P08
 |  2015/06/06       Vicent GAO        Add support for CharSet: 8859-1 by ROTVG00006-P09
 |  ------------------------------------------------------------------------
 \=========================================================================*/
 
#ifndef __LIB_RIL_SMS_H__

#define __LIB_RIL_SMS_H__

/***********************************************************************
 * MACRO CONSTANT DEFINITIONS
************************************************************************/
//Base characters definition
#define LIB_SMS_CHAR_STAR       '*'
#define LIB_SMS_CHAR_POUND      '#'
#define LIB_SMS_CHAR_PLUS       '+'
#define LIB_SMS_CHAR_MINUS      '-'

#define LIB_SMS_CHAR_QM         '?'

#define LIB_SMS_CHAR_A      'A'
#define LIB_SMS_CHAR_B      'B'
#define LIB_SMS_CHAR_C      'C'
#define LIB_SMS_CHAR_D      'D'
#define LIB_SMS_CHAR_E      'E'
#define LIB_SMS_CHAR_F      'F'

#define LIB_SMS_CHAR_a      'a'
#define LIB_SMS_CHAR_b      'b'
#define LIB_SMS_CHAR_c      'c'
#define LIB_SMS_CHAR_d      'd'
#define LIB_SMS_CHAR_e      'e'
#define LIB_SMS_CHAR_f      'f'

#define LIB_SMS_CHAR_0      '0'
#define LIB_SMS_CHAR_1      '1'
#define LIB_SMS_CHAR_2      '2'
#define LIB_SMS_CHAR_3      '3'
#define LIB_SMS_CHAR_4      '4'
#define LIB_SMS_CHAR_5      '5'
#define LIB_SMS_CHAR_6      '6'
#define LIB_SMS_CHAR_7      '7'
#define LIB_SMS_CHAR_8      '8'
#define LIB_SMS_CHAR_9      '9'

#define LIB_SMS_PHONE_NUMBER_MAX_LEN   (20)   //Unit is: one character
#define LIB_SMS_USER_DATA_MAX_LEN      (160)  //Unit is: one character

#define LIB_SMS_PDU_BUF_MAX_LEN     (180)

#define LIB_SMS_PHONE_NUMBER_TYPE_INTERNATIONAL (0x91)   //145
#define LIB_SMS_PHONE_NUMBER_TYPE_NATIONAL      (0xA1)   //161
#define LIB_SMS_PHONE_NUMBER_TYPE_UNKNOWN       (0x81)   //129
//<2015/03/23-ROTVG00006-P04-Vicent.Gao,<[SMS] Segment 4==>Fix issues of RIL SMS LIB.>
#define LIB_SMS_PHONE_NUMBER_TYPE_ALPHANUMERIC  (0x50)   //80
//>2015/03/23-ROTVG00006-P04-Vicent.Gao

#define LIB_SMS_PDU_FO_UDHI_BIT_NO_UDH    (0)
#define LIB_SMS_PDU_FO_UDHI_BIT_HAS_UDH   (1)

#define LIB_SMS_PDU_DCS_NO_MSG_CLASS      (0)
#define LIB_SMS_PDU_DCS_HAS_MSG_CLASS     (1)

#define LIB_SMS_PDU_DEFAULT_PID           (0x00)

#define LIB_SMS_SUBMIT_PDU_FO_SRR_BIT_NO_STATUS_REPORT   (0)
#define LIB_SMS_SUBMIT_PDU_FO_SRR_BIT_HAS_STATUS_REPORT  (1)

#define LIB_SMS_SUBMIT_PDU_DEFAULT_VP_RELATIVE  (167)

#define LIB_SMS_UD_TYPE_CON_DEFAULT   (LIB_SMS_UD_TYPE_CON_6_BYTE)

#define LIB_SMS_DEFAULT_FO_IN_SUBMIT_PDU    \
(   \
        ((LIB_SMS_PDU_TYPE_SUBMIT) << 0)    \
    |   ((LIB_SMS_VPF_TYPE_RELATIVE) << 3)  \
    |   ((LIB_SMS_SUBMIT_PDU_FO_SRR_BIT_NO_STATUS_REPORT) << 5) \
    |   ((LIB_SMS_PDU_FO_UDHI_BIT_NO_UDH) << 6) \
)

/***********************************************************************
 * ENUM TYPE DEFINITIONS
************************************************************************/

//This enum is refer to 'GSM 03.40 Clause 9.2.3.1 TP-Message-Type-Indicator (TP-MTI)'
//Warning: Please NOT-CHANGE this enum!
typedef enum
{
    LIB_SMS_PDU_TYPE_DELIVER = 0x00,
    LIB_SMS_PDU_TYPE_SUBMIT = 0x01,
    LIB_SMS_PDU_TYPE_STATUS_REPORT = 0x02,
    LIB_SMS_PDU_TYPE_RESERVED = 0x03,

    LIB_SMS_PDU_TYPE_INVALID = 0xFF,
} LIB_SMS_PDUTypeEnum;

//This enum is refer to 'GSM 03.40 Clause 9.2.3.3 TP-Validity-Period-Format (TP-VPF)'
//Warning: Please NOT-CHANGE this enum!
typedef enum
{
    LIB_SMS_VPF_TYPE_NOT_PRESENT = 0x00,
    LIB_SMS_VPF_TYPE_RELATIVE = 0x02,
    LIB_SMS_VPF_TYPE_ENHANCED = 0x01,
    LIB_SMS_VPF_TYPE_ABSOLUTE = 0x03,

    VPF_TYPE_INVALID = 0xFF
} LIB_SMS_VPFTypeEnum;

typedef enum
{
    LIB_SMS_DCS_ALPHA_DEFAULT = 0,
    LIB_SMS_DCS_ALPHA_8BIT_DATA = 1,
    LIB_SMS_DCS_ALPHA_UCS2 = 2,
    
    LIB_SMS_DCS_ALPHA_INVALID = 0xFF
} LIB_SMS_DCSAlphaEnum;

typedef enum
{
    LIB_SMS_DCS_MSG_CLASS_0 = 0,  //PLS NOT CHANGE!!
    LIB_SMS_DCS_MSG_CLASS_1 = 1,  //PLS NOT CHANGE!!
    LIB_SMS_DCS_MSG_CLASS_2 = 2,  //PLS NOT CHANGE!!
    LIB_SMS_DCS_MSG_CLASS_3 = 3,  //PLS NOT CHANGE!!
    LIB_SMS_DCS_MSG_CLASS_RESERVED = 4,  //PLS NOT CHANGE!!

    MSG_CLASS_TYPE_INVALID = 0xFF,
} LIB_SMS_DCSMsgClassEnum;

typedef enum
{
    LIB_SMS_DCS_COMP_UNCOMPRESSED = 0x00, //PLS NOT CHANGE!!
    LIB_SMS_DCS_COMP_COMPRESSED = 0x01,   //PLS NOT CHANGE!!

    LIB_SMS_DCS_COMP_INVALID = 0xFF,    
} LIB_SMS_DCSCompressEnum;

//NOTE: This enum ONLY in send/read/write text message.
typedef enum
{
    LIB_SMS_CHARSET_GSM = 0,  //See 3GPP TS 23.038 Clause '6.2.1 GSM 7 bit Default Alphabet'
    LIB_SMS_CHARSET_HEX = 1,
    LIB_SMS_CHARSET_UCS2 = 2,
    LIB_SMS_CHARSET_IRA = 3,
    LIB_SMS_CHARSET_8859_1 = 4,
    
    //==> Warning: Please add new charset upper this line!
    LIB_SMS_CHARSET_INVALID = 0xFF
} LIB_SMS_CharSetEnum;

typedef enum
{
    LIB_SMS_UD_TYPE_NORMAL = 0,
    LIB_SMS_UD_TYPE_CON_6_BYTE = 1,
    LIB_SMS_UD_TYPE_CON_7_BYTE = 2,
    
    //==> Waring: Please add new UDH Type upper thils line!!
    LIB_SMS_UD_TYPE_INVALID = 0xFF
} LIB_SMS_UDTypeEnum;

/***********************************************************************
 * STRUCT TYPE DEFINITIONS
************************************************************************/
typedef struct
{
    u8 uType;
    u8 aNumber[LIB_SMS_PHONE_NUMBER_MAX_LEN];
    u8 uLen;
} LIB_SMS_PhoneNumberStruct;

typedef struct
{
    u8 uYear;
    u8 uMonth;
    u8 uDay;
    u8 uHour;
    u8 uMinute;
    u8 uSecond;
    s8 iTimeZone;
} LIB_SMS_TimeStampStruct;

typedef struct
{
    u8 uMsgType;  //LIB_SMS_UDH_TYPE_CON_8_BIT or LIB_SMS_UDH_TYPE_CON_16_BIT
    u16 uMsgRef;
    u8 uMsgSeg;
    u8 uMsgTot;
} LIB_SMS_ConSMSParamStruct;

typedef struct
{
    u8 aUserData[LIB_SMS_USER_DATA_MAX_LEN];
    u16 uLen;
} LIB_SMS_UserDataStruct;

typedef union
{
    u8 uRelative;
    LIB_SMS_TimeStampStruct sAbsolute;
} LIB_SMS_ValidityPeriodUnion;

typedef struct
{
    LIB_SMS_ConSMSParamStruct sConSMSParam;
    
    LIB_SMS_PhoneNumberStruct sOA;
    u8 uPID;
    u8 uDCS;
    
    LIB_SMS_TimeStampStruct sSCTS;
    LIB_SMS_UserDataStruct sUserData;
} LIB_SMS_DeliverPDUParamStruct;

typedef struct
{
    LIB_SMS_ConSMSParamStruct sConSMSParam;
    
    LIB_SMS_PhoneNumberStruct sDA;
    u8 uPID;
    u8 uDCS;
    
    LIB_SMS_ValidityPeriodUnion sVP;
    LIB_SMS_UserDataStruct sUserData;
} LIB_SMS_SubmitPDUParamStruct;

typedef struct
{
    LIB_SMS_PhoneNumberStruct sSCA;
    u8 uFO;
    
    union
    {
        LIB_SMS_DeliverPDUParamStruct  sDeliverParam;
        LIB_SMS_SubmitPDUParamStruct   sSubmitParam;
    } sParam;
    
} LIB_SMS_PDUParamStruct;

typedef struct
{
    u8 aPDUOct[LIB_SMS_PDU_BUF_MAX_LEN];
    u16 uLen;
} LIB_SMS_PDUInfoStruct;

/***********************************************************************
 * OTHER TYPE DEFINITIONS
************************************************************************/


/***********************************************************************
 * MACRO FUNCTION DEFINITIONS
************************************************************************/
#define LIB_SMS_SET_POINTER_VAL(p,v)  \
    do  \
    {   \
        if((p) != NULL) \
        {   \
            (*(p)) = (v);   \
        }   \
    } while(0)
    
#define LIB_SMS_GET_MSG_TYPE_IN_PDU_FO(FirstOctet)      ((FirstOctet) & 0x03)
#define LIB_SMS_GET_UDHI_IN_PDU(FirstOctet)             (((FirstOctet) & 0x40) >> 6)
#define LIB_SMS_GET_VPF_IN_SUBMIT_PDU_FO(FirstOctet)    (((FirstOctet) & 0x18) >> 3)

#define LIB_SMS_SET_DEFAULT_DCS_IN_SUBMIT_PDU(CharSet,DCS)  \
    do  \
    {   \
        u8 uAlphaZ = 0; \
        \
        if(LIB_SMS_CHARSET_HEX == (CharSet))    \
        {   \
            uAlphaZ = LIB_SMS_DCS_ALPHA_8BIT_DATA;  \
        }   \
        else if(LIB_SMS_CHARSET_UCS2 == (CharSet))  \
        {   \
            uAlphaZ = LIB_SMS_DCS_ALPHA_UCS2;   \
        }   \
        else if(LIB_SMS_CHARSET_IRA == (CharSet))   \
        {   \
            uAlphaZ = LIB_SMS_DCS_ALPHA_DEFAULT;   \
        }   \
        else if(LIB_SMS_CHARSET_8859_1 == (CharSet))   \
        {   \
            uAlphaZ = LIB_SMS_DCS_ALPHA_DEFAULT;   \
        }   \
        else    \
        {   \
            uAlphaZ = LIB_SMS_DCS_ALPHA_DEFAULT;    \
        }   \
            \
        (DCS) = (((uAlphaZ) << 2) | ((LIB_SMS_PDU_DCS_NO_MSG_CLASS) << 4) | ((LIB_SMS_DCS_COMP_UNCOMPRESSED) << 5)); \
    } while(0);
    
#define LIB_SMS_GET_ALPHA_IN_PDU_DCS(DataCodeScheme,Alphabet)    LIB_SMS_DecodeDCS((DataCodeScheme),NULL,&(Alphabet),NULL,NULL);
    
#define LIB_SMS_IS_SUPPORT_CHARSET(CharSet) \
    (   \
        (   \
                (LIB_SMS_CHARSET_GSM == (CharSet))  \
            ||  (LIB_SMS_CHARSET_HEX == (CharSet))  \
            ||  (LIB_SMS_CHARSET_UCS2 == (CharSet)) \
            ||  (LIB_SMS_CHARSET_IRA == (CharSet)) \
            ||  (LIB_SMS_CHARSET_8859_1 == (CharSet)) \
        ) ? TRUE : FALSE    \
    )
    
#define LIB_SMS_CHECK_PDU_STR(PDUStr,PDUStrLen,Result) \
    do  \
    {   \
        LIB_SMS_PDUParamStruct *pTmpZA = NULL;    \
            \
        pTmpZA = (LIB_SMS_PDUParamStruct*)Ql_MEM_Alloc(sizeof(LIB_SMS_PDUParamStruct));    \
        if(NULL == pTmpZA)    \
        {   \
            (Result) = FALSE;   \
            break;  \
        }   \
            \
        (Result) = LIB_SMS_DecodePDUStr((PDUStr),(PDUStrLen),pTmpZA);  \
        Ql_MEM_Free(pTmpZA);  \
        (Result) = TRUE;    \
    } while(0)

#define LIB_SMS_CHECK_SUBMIT_PDU_STR_FOR_SEND(PDUStr,PDUStrLen,Result) \
    do  \
    {   \
        LIB_SMS_PDUParamStruct *pTmpZA = NULL;    \
        LIB_SMS_SubmitPDUParamStruct *pSubmitParamZA = NULL;  \
            \
        pTmpZA = (LIB_SMS_PDUParamStruct*)Ql_MEM_Alloc(sizeof(LIB_SMS_PDUParamStruct));    \
        if(NULL == pTmpZA)    \
        {   \
            (Result) = FALSE;   \
            break;  \
        }   \
            \
        (Result) = LIB_SMS_DecodePDUStr((PDUStr),(PDUStrLen),pTmpZA);  \
        if(FALSE == (Result))   \
        {   \
            Ql_MEM_Free(pTmpZA);  \
            break;  \
        }   \
            \
        if(LIB_SMS_PDU_TYPE_SUBMIT != LIB_SMS_GET_MSG_TYPE_IN_PDU_FO(pTmpZA->uFO))    \
        {   \
            Ql_MEM_Free(pTmpZA);  \
            (Result) = FALSE;   \
            break;  \
        }   \
            \
        pSubmitParamZA = &((pTmpZA->sParam).sSubmitParam);  \
            \
        if(0 == ((pSubmitParamZA->sDA).uLen)) \
        {   \
            Ql_MEM_Free(pTmpZA);  \
            (Result) = FALSE;   \
            break;  \
        }   \
            \
        Ql_MEM_Free(pTmpZA);  \
        (Result) = TRUE;    \
    } while(0)
        
#define LIB_SMS_IS_VALID_ASCII_NUMBER_CHAR(Char)   \
(   \
    (   \
            (((Char) >= LIB_SMS_CHAR_0) && ((Char) <= LIB_SMS_CHAR_9))  \
        ||  (LIB_SMS_CHAR_STAR == (Char))   \
        ||  (LIB_SMS_CHAR_POUND == (Char))  \
        ||  (((Char) >= LIB_SMS_CHAR_A) && ((Char) <= LIB_SMS_CHAR_C))  \
        ||  (((Char) >= LIB_SMS_CHAR_a) && ((Char) <= LIB_SMS_CHAR_c))  \
    ) ? TRUE : FALSE   \
)        
        
/***********************************************************************
 * FUNCTION DECLARATIONS
************************************************************************/
/******************************************************************************
* Function:     LIB_SMS_IsValidHexStr
*  
* Description:
*               This function is used to Check the hex string is VALID or not.
*
* Parameters:    
*               <pHexStr>:
*                    [In] The pointer of hex string
*               <uHexStrLen>:
*                    [In] The length of hex string
*
* Return:  
*              TRUE,  This function works SUCCESS.
*              FALSE, This function works FAIL!
*
* NOTE:
*              1. This is a library function.
*              2. Please ensure all parameters are VALID.
******************************************************************************/
extern bool LIB_SMS_IsValidHexStr(char *pHexStr,u16 uHexStrLen);

/******************************************************************************
* Function:     LIB_SMS_ConvHexOctToHexStr
*  
* Description:
*               This function is used to convert hex octet to hex string.
*
* Parameters:    
*               <pSrc>:
*                    [In] The pointer of source buffer
*               <uSrcLen>:
*                    [In] The length of source buffer
*               <pDest>:
*                    [In] The pointer of destination buffer
*               <pDestLen>:
*                    [In] The length of destination buffer
*
* Return:  
*              TRUE,  This function works SUCCESS.
*              FALSE, This function works FAIL!
*
* NOTE:
*              1. This is a library function.
*              2. Please ensure all parameters are VALID.
*              3. Warning: Before call this function,MUST set <*pDestLen> to a value which specify the rest bytes of <pDest> buffer.
*              4. If this function works SUCCESS,<*pDestLen> will return the bytes of <pDest> buffer that has been written.
******************************************************************************/
extern bool LIB_SMS_ConvHexOctToHexStr(const u8 *pSrc,u16 uSrcLen, char *pDest, u16 *pDestLen);

/******************************************************************************
* Function:     LIB_SMS_ConvHexStrToHexOct
*  
* Description:
*               This function is used to convert hex string to hex octet.
*
* Parameters:    
*               <pSrc>:
*                    [In] The pointer of source buffer
*               <uSrcLen>:
*                    [In] The length of source buffer
*               <pDest>:
*                    [In] The pointer of destination buffer
*               <pDestLen>:
*                    [In] The length of destination buffer
*
* Return:  
*              TRUE,  This function works SUCCESS.
*              FALSE, This function works FAIL!
*
* NOTE:
*              1. This is a library function.
*              2. Please ensure all parameters are VALID.
*              3. Warning: Before call this function,MUST set <*pDestLen> to a value which specify the rest bytes of <pDest> buffer.
*              4. If this function works SUCCESS,<*pDestLen> will return the bytes of <pDest> buffer that has been written.
******************************************************************************/
extern bool LIB_SMS_ConvHexStrToHexOct(const char *pSrc,u16 uSrcLen, u8 *pDest, u16 *pDestLen);

/******************************************************************************
* Function:     LIB_SMS_ConvCharSetToAlpha
*  
* Description:
*               This function is used to convert 'LIB_SMS_CharSetEnum' data to 'LIB_SMS_DCSAlphaEnum' data.
*
* Parameters:    
*               <eCharSet>
*                    [In] Character set,It's value is in 'LIB_SMS_CharSetEnum'
*               <pSrc>:
*                    [In] The pointer of source data
*               <uSrcLen>
*                    [In] The length of source data
*               <uDCS>:
*                    [In] <DCS> element in TPDU
*               <pDest>
*                    [In] The length of destination data
*               <pDestLen>
*                    [In] The length of destination length
* Return:  
*              TRUE,  This function works SUCCESS.
*              FALSE, This function works FAIL!
*
* NOTE:
*              1. This is a library function.
*              2. Please ensure all parameters are VALID.
*              3. Warning: Before call this function,MUST set <*pDestLen> to a value which specify the rest bytes of <pDest> buffer.
*              4. If this function works SUCCESS,<*pDestLen> will return the bytes of <pDest> buffer that has been written.
******************************************************************************/
extern bool LIB_SMS_ConvCharSetToAlpha(LIB_SMS_CharSetEnum eCharSet,u8 *pSrc,u16 uSrcLen,u8 uDCS,u8 *pDest,u16 *pDestLen);

/******************************************************************************
* Function:     LIB_SMS_ConvAlphaToCharSet
*  
* Description:
*               This function is used to convert 'LIB_SMS_DCSAlphaEnum' data to 'LIB_SMS_CharSetEnum' data.
*
* Parameters:    
*               <uDCS>:
*                    [In] <DCS> element in TPDU
*               <pSrc>:
*                    [In] The pointer of source data
*               <uSrcLen>
*                    [In] The length of source data
*               <eCharSet>
*                    [In] Character set,It's value is in 'LIB_SMS_CharSetEnum'
*               <pDest>
*                    [In] The length of destination data
*               <pDestLen>
*                    [In] The length of destination length
* Return:  
*              TRUE,  This function works SUCCESS.
*              FALSE, This function works FAIL!
*
* NOTE:
*              1. This is a library function.
*              2. Please ensure all parameters are VALID.
*              3. Warning: Before call this function,MUST set <*pDestLen> to a value which specify the rest bytes of <pDest> buffer.
*              4. If this function works SUCCESS,<*pDestLen> will return the bytes of <pDest> buffer that has been written.
******************************************************************************/
extern bool LIB_SMS_ConvAlphaToCharSet(u8 uDCS,u8 *pSrc,u16 uSrcLen,LIB_SMS_CharSetEnum eCharSet,u8 *pDest,u16 *pDestLen);

/******************************************************************************
* Function:     LIB_SMS_DecodePDUStr
*  
* Description:
*               This function is used to decode PDU string to 'LIB_SMS_PDUParamStruct' data.
*
* Parameters:    
*               <pPDUStr>:
*                    [In] The pointer of PDU string
*               <uPDUStrLen>:
*                    [In] The length of PDU string
*               <pParam>
*                    [In] The pointer of 'LIB_SMS_PDUParamStruct' data
*
* Return:  
*              TRUE,  This function works SUCCESS.
*              FALSE, This function works FAIL!
*
* NOTE:
*              1. This is a library function.
*              2. Please ensure all parameters are VALID.
******************************************************************************/
extern bool LIB_SMS_DecodePDUStr(char *pPDUStr,u16 uPDUStrLen,LIB_SMS_PDUParamStruct *pParam);

/******************************************************************************
* Function:     LIB_SMS_DecodeDCS
*  
* Description:
*               This function is used to decode <DCS> in TPDU
*
* Parameters:    
*               <uDCS>:
*                    [In] <DCS> element in TPDU
*               <pMsgType>:
*                    [In] The pointer of message type which is same as 'DCSMsgTypeEnum'
*               <pAlpha>
*                    [In] The pointer of alphabet which is same as 'LIB_SMS_DCSAlphaEnum'
*               <pMsgClass>
*                    [In] The pointer of message class which is same as 'DCSMsgClassEnum'
*               <pCompress>
*                    [In] The pointer of compress flag which is same as 'DCSCompressEnum'
* Return:  
*              TRUE,  This function works SUCCESS.
*              FALSE, This function works FAIL!
*
* NOTE:
*              1. This is a library function.
*              2. Please ensure all parameters are VALID.
*              3. If you DONOT want to get certain parameter's value,please set it to NULL.
******************************************************************************/
extern void LIB_SMS_DecodeDCS(u8 uDCS,u8 *pMsgType,u8 *pAlpha,u8 *pMsgClass,u8 *pCompress);

/******************************************************************************
* Function:     LIB_SMS_EncodeSubmitPDU
*  
* Description:
*               This function is used to encode SUBMIT-PDU according to given parameters
*
* Parameters:    
*               <pParam>:
*                    [In] The pointer of 'LIB_SMS_PDUParamStruct' data
*               <pInfo>:
*                    [In] The pointer of 'LIB_SMS_PDUInfoStruct' data
*
* Return:  
*              TRUE,  This function works SUCCESS.
*              FALSE, This function works FAIL!
*
* NOTE:
*              1. This is a library function.
*              2. Please ensure all parameters are VALID.
******************************************************************************/
extern bool LIB_SMS_EncodeSubmitPDU(LIB_SMS_PDUParamStruct *pParam,LIB_SMS_PDUInfoStruct *pInfo);

#endif  //#ifndef __LIB_RIL_SMS_H__
