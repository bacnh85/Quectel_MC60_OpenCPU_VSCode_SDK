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
 *   ril_dtmf.c 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The module defines the information, and APIs related to DTMF.
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
#include "ril_dtmf.h"
#include "ril.h"
#include "ril_util.h"
#include "ql_common.h"
#include "ql_uart.h"
#include "ql_stdlib.h"
#include "ql_error.h"


#define RIL_DTMF_DEBUG_ENABLE 0
#if RIL_DTMF_DEBUG_ENABLE > 0
#define RIL_DTMF_DEBUG_PORT  UART_PORT2
static char DBG_Buffer[100];
#define RIL_DTMF_DEBUG(BUF,...)  QL_TRACE_LOG(RIL_DTMF_DEBUG_PORT,BUF,100,__VA_ARGS__)
#else
#define RIL_DTMF_DEBUG(BUF,...) 
#endif

static CB_ToneDet   callback_ToneDet = NULL;    // get callback funtion pointer for TONEDET URC handle
static CB_WDTMF     callback_WDTMF = NULL;      // get callback funtion pointer for WDTMF URC handle
static u32 *g_low_thresholdPtr = NULL;         // get pointer of low threshold
static u32 *g_high_thresholdPtr = NULL;        // get pointer of high threshold

static s32 ATResponse_Handler(char* line, u32 len, void* userData);

/****************************************************
* DTMF detection          
****************************************************/
/*****************************************************************
* Function:     RIL_ToneDet_Open
* 
* Description:
*               This function is used to Open DTMF detect.
*
* Parameters:
*               cb_ToneDet_hdl:      
*                   [IN] call back function to handle DTMF detected.
*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*                                      and then call Ql_RIL_Initialize to initialize RIL.                
*****************************************************************/
s32 RIL_ToneDet_Open( CB_ToneDet cb_ToneDet_hdl )
{
	s32 ret = RIL_AT_FAILED;
	char strAT[200]; 
    
    callback_ToneDet = cb_ToneDet_hdl;

	Ql_memset(strAT, 0, sizeof(strAT));
	Ql_sprintf(strAT, "AT+QTONEDET=1\r\n");
	ret = Ql_RIL_SendATCmd( strAT, Ql_strlen(strAT), NULL, NULL, 300 ) ;
                  
    RIL_DTMF_DEBUG( DBG_Buffer, "<-- Send AT:%s, ret = %d -->\r\n", strAT, ret );
    
    return ret;
}

/*****************************************************************
* Function:     RIL_ToneDet_Close
* 
* Description:
*               The function is used to close DTMF detect.
*
* Parameters:
*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*                                      and then call Ql_RIL_Initialize to initialize RIL.                
*****************************************************************/
s32 RIL_ToneDet_Close( void )
{
	s32 ret = RIL_AT_FAILED;
	char strAT[200]; 
    
	Ql_memset( strAT, 0, sizeof(strAT) );
	Ql_sprintf( strAT, "AT+QTONEDET=0\r\n" );
	ret = Ql_RIL_SendATCmd( strAT, Ql_strlen(strAT), NULL, NULL, 300 ) ;
                  
    RIL_DTMF_DEBUG( DBG_Buffer, "<-- Send AT:%s, ret = %d -->\r\n", strAT, ret );
    
    return ret;
}

/*****************************************************************
* Function:     RIL_ToneDet_Set
* 
* Description:
*               This function is used to set DTMF detection.
*
* Parameters:
*                <mode>:
*                       [IN] 2-4, select which threshold to set.
*                <pause>:
*                       [IN] prefix pause number.
*                <low>:
*                       [IN] low threshold value.
*                <high>:
*                       [IN] high threshold value.
*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*                                      and then call Ql_RIL_Initialize to initialize RIL.                            
*****************************************************************/
s32 RIL_ToneDet_Set( Enum_ToneDet_Mode mode, u32 pause, u32 low, u32 high )
{
    s32 ret = RIL_AT_FAILED;
	char strAT[200]; 

    if ( mode <= RIL_DETThreshold_Min || mode >= RIL_DETThreshold_Max )
    {
        RIL_DTMF_DEBUG( DBG_Buffer, "<-- ToneDet Set Fail, INVALID PARAM: mode=%d -->\r\n", mode );
        return RIL_AT_INVALID_PARAM;
    }
    
	Ql_memset( strAT, 0, sizeof(strAT) );
	Ql_sprintf( strAT, "AT+QTONEDET=%d,1,%d,%d,%d", mode, pause, low, high );
	ret = Ql_RIL_SendATCmd( strAT, Ql_strlen(strAT), NULL, NULL, 300 ) ;
                  
    RIL_DTMF_DEBUG( DBG_Buffer, "<-- Send AT:%s, ret = %d -->\r\n", strAT, ret );

    return ret;
}

/*****************************************************************
* Function:     RIL_ToneDet_Get
* 
* Description:
*               This function is used to get settings of DTMF detection.
*
* Parameters:
*                <mode>:
*                       [IN] 2-4, select which threshold to get.
*                <low>:
*                       [IN] low threshold value.
*                <high>:
*                       [IN] high threshold value.
*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*                                      and then call Ql_RIL_Initialize to initialize RIL.                              
*****************************************************************/
s32 RIL_ToneDet_Get( Enum_ToneDet_Mode mode, u32 *low, u32 *high )
{
    s32 ret = RIL_AT_FAILED;
	char strAT[200]; 

    if ( mode < RIL_DETThreshold_Min || mode > RIL_DETThreshold_Max 
        || NULL == low
        || NULL == high )
    {
        RIL_DTMF_DEBUG( DBG_Buffer, "<-- ToneDet Set Fail, INVALID PARAM: mode=%d. -->\r\n", mode );
        return RIL_AT_INVALID_PARAM;
    }

    g_low_thresholdPtr = low;
    g_high_thresholdPtr = high;
    
	Ql_memset( strAT, 0, sizeof(strAT) );
	Ql_sprintf( strAT, "AT+QTONEDET=%d,0", mode );
	ret = Ql_RIL_SendATCmd( strAT, Ql_strlen(strAT), ATResponse_Handler, NULL, 300 ) ;
                  
    RIL_DTMF_DEBUG( DBG_Buffer, "<-- Send AT:%s, ret = %d -->\r\n", strAT, ret );

    return ret;
}

/*****************************************************************
* Function:     ATResponse_Handler
* 
* Description:
*               Call back function for handle response of the AT command.
*
* Parameters:
*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*                                      and then call Ql_RIL_Initialize to initialize RIL.               
*****************************************************************/
static s32 ATResponse_Handler(char* line, u32 len, void* userData)
{
    // get TONEDET threshold
    if ( Ql_strstr(line, "+QTONEDET:") )
    {
        if ( NULL != g_low_thresholdPtr && NULL != g_high_thresholdPtr )
        {
            Ql_sscanf( line, "%*[^,],%*[^,],%d,%d", g_low_thresholdPtr, g_high_thresholdPtr );
            RIL_DTMF_DEBUG( DBG_Buffer, "<-- ATResponse_Handler: low=%d, high=%d -->\r\n", *g_low_thresholdPtr, *g_high_thresholdPtr );
        }
    }
    
    
    if (Ql_RIL_FindLine(line, len, "OK"))
    {  
        return  RIL_ATRSP_SUCCESS;
    }
    else if (Ql_RIL_FindLine(line, len, "ERROR"))
    {  
        return  RIL_ATRSP_FAILED;
    }
    else if (Ql_RIL_FindString(line, len, "+CME ERROR"))
    {
        return  RIL_ATRSP_FAILED;
    }
    else if (Ql_RIL_FindString(line, len, "+CMS ERROR:"))
    {
        return  RIL_ATRSP_FAILED;
    }
    return RIL_ATRSP_CONTINUE; //continue wait
}

/*****************************************************************
* Function:     OnURCHandler 
* 
* Description:
*               This function is the entrance for Unsolicited Result Code (URC) Handler.
*
* Parameters:
*               strURC:      
*                   [IN] a URC string terminated by '\0'.
*
*               reserved:       
*                   reserved, can be NULL.
* Return:        
*               
*****************************************************************/
void OnURCHandler_QToneDet( const char* strURC, void* reserved )
{
	char buff[30];
    s32 dtmfCode = -1;
    s32 timems = -1;
        
 	if ( NULL != callback_ToneDet )
 	{
		Ql_strcpy( buff, "\r\n+QTONEDET:\0" );
		if( Ql_StrPrefixMatch(strURC, buff) )
		{
			Ql_sscanf(strURC,"%*[^:]: %[^,]",buff);
			dtmfCode = Ql_atof(buff);
            if ( dtmfCode < 35 || dtmfCode > 70 )   
            {
                return;     // not dtmfCode return.
            }
            else if ( 69 == dtmfCode || 70 == dtmfCode )
            {
                Ql_sscanf( strURC, "%*[^,],%[^\r\n]", buff );
    			timems = Ql_atof( buff );
            }
			
			callback_ToneDet( dtmfCode, timems );
		}
	}
}

/****************************************************
* DTMF send          
****************************************************/
/*****************************************************************
* Function:     RIL_WDTMF_Send
* 
* Description:
*               This function is used to play DTMF tone during the call.
*
* Parameters:
*                <ul_volume>:
*                       [IN] 0-7, uplink channel of the volume.
*                <dl_volume>:
*                       [IN] 0-7, downlink channel of the volume, recommended to be set as 0.
*                <dtmfStr>:
*                       [IN] this string consists of DTMF tone strings, Duration of each DTMF tone(unit is ms) and Mute time (unit is ms).
*                              example:  "0A5,50,50,3,100,50"-->0A5 is DTMF tone strings, continuancetime 50ms, mute time 50ms;
*                                               The remaining three as before. the total lenth of dtmfStr must be less than 400.
*                <cb_WDTMF_hdl>:
*                       [IN] callback function for QWDTMF URC handle.
*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*                                      and then call Ql_RIL_Initialize to initialize RIL.               
*****************************************************************/
s32 RIL_WDTMF_Send( Enum_WDTMF_Vomume ul_volume, Enum_WDTMF_Vomume dl_volume, u8 *dtmfStr, CB_WDTMF cb_WDTMF_hdl )
{
	s32 ret = RIL_AT_FAILED;
	char strAT[200]; 

    if ( ( ul_volume < RIL_WDTMF_VOLUME0 || ul_volume > RIL_WDTMF_VOLUME7 )
        || ( dl_volume < RIL_WDTMF_VOLUME0 || dl_volume > RIL_WDTMF_VOLUME7 )
        || ( NULL == dtmfStr ) )
    {
        RIL_DTMF_DEBUG( DBG_Buffer, "<-- WDTMF Send Fail, INVALID PARAM. -->\r\n");
        return RIL_AT_INVALID_PARAM;
    }

    callback_WDTMF = cb_WDTMF_hdl;
    
	Ql_memset( strAT, 0, sizeof(strAT) );
	Ql_sprintf( strAT, "AT+QWDTMF=%d,%d,\"%s\"\r\n", ul_volume, dl_volume, dtmfStr );
	ret = Ql_RIL_SendATCmd( strAT, Ql_strlen(strAT), NULL, NULL, 0 ) ;
                  
    RIL_DTMF_DEBUG( DBG_Buffer, "<-- Send AT:%s, ret = %d -->\r\n", strAT, ret );
    
    return ret;
}

/*****************************************************************
* Function:     OnURCHandler 
* 
* Description:
*               This function is the entrance for Unsolicited Result Code (URC) Handler.
*
* Parameters:
*               strURC:      
*                   [IN] a URC string terminated by '\0'.
*
*               reserved:       
*                   reserved, can be NULL.
* Return:        
*               
*****************************************************************/
void OnURCHandler_QWDTMF( const char* strURC, void* reserved )
{
	char buff[30];
    s32 result = -1;
        
 	if ( NULL != callback_WDTMF )
 	{
		Ql_strcpy( buff, "\r\n+QWDTMF:\0" );
		if( Ql_StrPrefixMatch(strURC, buff) )
		{
			Ql_sscanf( strURC, "%*[^:]: %[^\r\n]", buff );
			result = Ql_atof( buff );
			
			callback_WDTMF( result );
		}
	}
}