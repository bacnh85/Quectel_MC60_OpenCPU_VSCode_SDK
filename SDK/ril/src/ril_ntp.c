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
 *   ril_ntp.c 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The module implements NTP related APIs.
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
#include "ril_ntp.h"
#include "ril.h"
#include "ril_util.h"
#include "ql_stdlib.h"
#include "ql_error.h"
#include "ql_trace.h"

static CB_NTPCMD     callback_NTPCMD = NULL;

void OnURCHandler_NTPCMD(const char* strURC, void* reserved)
{
	char urcHead[] = "\r\n+QNTP:\0";
        
 	if ( NULL != callback_NTPCMD )
 	{
		if( Ql_StrPrefixMatch(strURC, urcHead) )
		{
			callback_NTPCMD(strURC);
		}
	}
}

s32 RIL_NTP_START(u8 *server_addr, u16 server_port, CB_NTPCMD cb_NTPCMD_hdl)
{
	s32 ret = RIL_AT_FAILED;
	char strAT[200]; 

    if (server_addr == NULL)
    {
        return RIL_AT_INVALID_PARAM;
    }

    callback_NTPCMD = cb_NTPCMD_hdl;
    
	Ql_memset( strAT, 0, sizeof(strAT) );
	Ql_sprintf( strAT, "AT+QNTP=\"%s\",%d\r\n", server_addr, server_port);
	ret = Ql_RIL_SendATCmd( strAT, Ql_strlen(strAT), NULL, NULL, 0 ) ;
    
    return ret;
}


