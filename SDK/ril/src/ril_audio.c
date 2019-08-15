/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Quectel Co., Ltd. 2014
*
*****************************************************************************/
/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ril_audio.c 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The APIs are used to afford the audio related operations,based on RIL.
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
#include "ril_audio.h"


#ifdef __OCPU_RIL_SUPPORT__ 

#define IS_SUPPORT_AUD_CHANNEL(audChannel)    \
(   \
    (   \
            (AUD_CHANNEL_NORMAL == (audChannel))    \
         || (AUD_CHANNEL_HEADSET == (audChannel))   \
         || (AUD_CHANNEL_LOUD_SPEAKER == (audChannel))    \
    ) ? TRUE : FALSE    \
)

#define IS_SUPPORT_VOLUME_TYPE(volume_type)    \
(   \
    (   \
            (VOL_TYPE_CTN == (volume_type))    \
         || (VOL_TYPE_GMI == (volume_type))   \
         || (VOL_TYPE_KEY == (volume_type))    \
         || (VOL_TYPE_MEDIA == (volume_type))   \
         || (VOL_TYPE_MIC == (volume_type))    \
         || (VOL_TYPE_SID == (volume_type))   \
         || (VOL_TYPE_SPH == (volume_type)) \
    ) ? TRUE : FALSE    \
)
static Enum_AudChannel s_channel_qmic = AUD_CHANNEL_HEADSET;
static s32 ATResponse_AUD_handler(char* line, u32 len, void* userdata);


/*
+QAUDCH: 0

*/
s32 RIL_AUD_GetChannel(Enum_AudChannel *pchannel)
{
    s32 ret = RIL_AT_SUCCESS;
    char strAT[20];
 
    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QAUDCH?\r\n");
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_AUD_handler,pchannel,0);
    return ret;
}

s32 RIL_AUD_SetChannel(Enum_AudChannel audChannel)
{
    s32 ret = RIL_AT_SUCCESS;
    char strAT[20];
 
    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QAUDCH=%d\r\n",(char)audChannel);
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),NULL,NULL,0);
    return ret;
    
}

s32  RIL_AUD_SetVolume(Enum_VolumeType volType, u8 volLevel)
{
    s32 ret = RIL_AT_SUCCESS;
    char strAT[40];
    Enum_AudChannel channel ; 
   
    Ql_memset(strAT, 0, sizeof(strAT));
    switch(volType)
    {
    case VOL_TYPE_SPH:
        {
            Ql_sprintf(strAT, "AT+CLVL=%d\r\n",(char)volLevel);
        }
        break;
    case VOL_TYPE_MIC:
        {
            //level :0-15
            if(RIL_AT_SUCCESS != (ret = RIL_AUD_GetChannel(&channel)))
            {
                return ret;   
            }
            Ql_sprintf(strAT, "AT+QMIC=%d,%d\r\n",channel,(char)volLevel);
        }
        break;
    case VOL_TYPE_MEDIA:
        {
            Ql_sprintf(strAT, "AT+QMEDVL=%d\r\n",(char)volLevel);
        }
        break;
    default:
        {
            return RIL_AT_INVALID_PARAM;
        }
        break;
    }
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),NULL,NULL,0);

    return ret;
}
s32  RIL_AUD_GetVolume(Enum_VolumeType volType,u8* pVolLevel) 
{
    s32 ret = RIL_AT_SUCCESS;
    char strAT[20];
   
    Ql_memset(strAT, 0, sizeof(strAT));
    switch(volType)
    {
    case VOL_TYPE_SPH:
        {
            Ql_sprintf(strAT, "AT+CLVL?\r\n");
        }
        break;
    case VOL_TYPE_MIC:
        {
            if(RIL_AT_SUCCESS != (ret = RIL_AUD_GetChannel(&s_channel_qmic)))
            {
                return ret;
            }
            Ql_sprintf(strAT, "AT+QMIC?\r\n");
            
        }
        break;
    case VOL_TYPE_MEDIA:
        {
            Ql_sprintf(strAT, "AT+QMEDVL?\r\n");

        }
        break;
    default:
        {
            return RIL_AT_INVALID_PARAM;
        }
        break;
    }
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_AUD_handler,pVolLevel,0);
  
    return ret;
}

/*
File suffix can be AMR,WAV or MP3
*/
s32 RIL_AUD_PlayFile(char* filePath, bool isRepeated)
{
    s32 ret = RIL_AT_SUCCESS;
    char strAT[255];
    char repeat = 0;
    u8 vol_level = 0;
    Enum_AudChannel channel;
    (isRepeated)?(repeat = 1):(repeat = 0);
    if(RIL_AT_SUCCESS != (ret=RIL_AUD_GetVolume(VOL_TYPE_MEDIA, &vol_level)))
    {
        return ret;
    }
    if(RIL_AT_SUCCESS != (ret=RIL_AUD_GetChannel(&channel)))
    {
        return ret;
    }
    
    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QAUDPLAY=\"%s\",%d,%d,%d\r\n",filePath,repeat,vol_level,channel);
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_AUD_handler,NULL,0);
    return ret;
}

s32 RIL_AUD_StopPlay(void)
{
    s32 ret = RIL_AT_SUCCESS;
    char strAT[20];
    
    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QAUDSTOP\r\n");

    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_AUD_handler,NULL,0);
    return ret;
}

/*
reference to file :GSM_Recording_AT_Commands_Manual_V3.0.pdf
    AMR = 3,
    WAV_PCM16 =13,
    WAV_ALAW, 14
    WAV_ULAW, 15
    WAV_ADPCM 16
*/
static u8 tool_convert_format(Enum_AudRecordFormat format)
{
    u8 real=255;
    if(AUD_RECORD_FORMAT_AMR==format)
    {
        real = 3;
    }
    else if(AUD_RECORD_FORMAT_WAV_PCM16 == format)
    {
        real = 13;
    }
    else if(AUD_RECORD_FORMAT_WAV_ALAW == format)
    {
        real = 14;
    }
    else if(AUD_RECORD_FORMAT_WAV_ULAW == format)
    {
        real = 15;
    }
    else if(AUD_RECORD_FORMAT_WAV_ADPCM == format)
    {
        real = 16;
    }
    return real;
}

/*  
start record audio, use the func tool_convert_format to convert the num
 */
s32 RIL_AUD_StartRecord(char* fileName, Enum_AudRecordFormat format)
{
    s32 ret = RIL_AT_SUCCESS;
    char strAT[255];
    u8 real_format = tool_convert_format(format);

    if(real_format == 255)
    {
        return RIL_AT_INVALID_PARAM;
    }
    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QAUDRD=1,\"%s\",%d\r\n",fileName,real_format);
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_AUD_handler,NULL,0);
    
    return ret;
}
s32 RIL_AUD_StopRecord(void)
{
    s32 ret = RIL_AT_SUCCESS;
    char strAT[20];
    
    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QAUDRD=0\r\n");
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_AUD_handler,NULL,0);
    return ret;
}

s32 RIL_AUD_PlayMem(u32 mem_addr, u32 mem_size, u8 aud_format, bool repeat)
{
    s32 at_result = RIL_AT_SUCCESS;
    char strAT[60];
    
    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QPLAYMEM=0x%x,%d,%d,%d",mem_addr,mem_size,aud_format,repeat);
    at_result = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_AUD_handler,NULL,0);

    return at_result;
}

s32 RIL_AUD_StopPlayMem(void)
{
    char strAT[20];
    
    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QSTOPRES");
    return Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_AUD_handler,NULL,0);
}

s32 RIL_AUD_PlayMemBg(u32 mem_addr, u32 mem_size, u8 aud_format, u8 vol_ul, u8 vol_dl)
{
    char strAT[60];
    u32 atLen;

    // AT+QPMEMBG=<1>,<addr(hex)>,<size>,<format(1-3)>,<vol_ul(0-7)>,<vol_dl(0-7)>
    //  format: 1 - mp3, 2 - amr, 3 - wav
    //  ex.: AT+QPMEMBG=1,0x102C706C,88836,2,5,5 (request to play amr audio data)
    //       OK
    //       +QPRESBG: 0,5 (when finished)
    Ql_memset(strAT, 0, sizeof(strAT));
    atLen =Ql_sprintf(strAT, "AT+QPMEMBG=1,0x%x,%d,%d,%d,%d",mem_addr,mem_size,aud_format,vol_ul,vol_dl);
    return Ql_RIL_SendATCmd(strAT, atLen, ATResponse_AUD_handler, NULL, 0);
}

s32 RIL_AUD_StopPlayMemBg(void)
{
    char strAT[20];
    u32 atLen;
    
    Ql_memset(strAT, 0, sizeof(strAT));
    atLen = Ql_sprintf(strAT, "AT+QPMEMBG=0");
    return Ql_RIL_SendATCmd(strAT, atLen, ATResponse_AUD_handler, NULL, 0);
}

s32 RIL_AUD_GetRecordState(u8* pState)
{
    s32 at_result = RIL_AT_SUCCESS;
    char strAT[20];
    u8 state = 0;
    
    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QAUDRD?\r\n");
    at_result = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_AUD_handler,&state,0);
    if(at_result == RIL_ATRSP_SUCCESS)
    {
        *pState = state;
    }
    return at_result;
}
static u8 parse_cmd_qmic(char *pcmd,u8 channel)
{
    char strTmp[10];
    char* phead = NULL;
    char* ptail= NULL;
    u8 temp1 =0,temp2=0,temp3=0;
    Ql_memset(strTmp, 0x0, sizeof(strTmp));
    if(NULL == (phead  = Ql_strstr(pcmd, ":")))
        goto error;
    phead +=2;
    if(NULL == (ptail = Ql_strstr(phead, ",")))
        goto error;
    Ql_memcpy(strTmp, phead, ptail - phead);
    temp1 = Ql_atoi(strTmp);

    phead = ptail+1;
    if(NULL == (ptail  = Ql_strstr(phead , ",")))
        goto error;
    Ql_memcpy(strTmp, phead, ptail - phead);
    temp2 = Ql_atoi(strTmp);
    
    phead = ptail+1;    
    if(NULL == (ptail = Ql_strstr(phead, "\r\n")))
        goto error;
    Ql_memcpy(strTmp, phead, ptail - phead);
    temp3 = Ql_atoi(strTmp);
    
    if(channel == 0)return temp1;
    else if(1 == channel) return temp2;
    else if(2 == channel) return temp3;
  
error:
    return 255;
}
static s32 ATResponse_AUD_handler(char* line, u32 len, void* userdata)
{
    char *head = NULL;

    if((head =Ql_RIL_FindString(line, len, "+QAUDCH:")) || \
            (head =Ql_RIL_FindString(line, len, "+QMEDVL:")) || (head =Ql_RIL_FindString(line, len, "+CLVL:")) || \
            (head =Ql_RIL_FindString(line, len, "+QSIDET:")) || (head =Ql_RIL_FindString(line, len, "+QAUDRD:")) )
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
                *(u8* )userdata = Ql_atoi(strTmp);
            }
        return  RIL_ATRSP_CONTINUE;
    }
    else if(head =Ql_RIL_FindString(line, len, "+QMIC:"))
    {
        *(u8* )userdata = parse_cmd_qmic(head,(u8)s_channel_qmic);
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

RIL_AUD_PLAY_IND cb_aud_play = NULL;
s32 RIL_AUD_RegisterPlayCB(RIL_AUD_PLAY_IND audCB)
{
    if (NULL == audCB)
    {
        return QL_RET_ERR_INVALID_PARAMETER;
    }
    cb_aud_play = audCB;
    return QL_RET_OK;
}

#endif

