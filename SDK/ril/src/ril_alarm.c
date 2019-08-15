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
 *   ril_alarm.c 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The module implements RTC alarm related APIs.
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
#include "ril_alarm.h"
#include "ril.h"
#include "ril_util.h"
#include "ql_common.h"
#include "ql_stdlib.h"
#include "ql_error.h"

static s32 ATResponse_QALARM_Handler(char* line, u32 len, void* userData);


s32 RIL_Alarm_Create(ST_Time* dateTime, u8 mode)
{
	char strAT[50] = {"\0"};
    u16  atLength = 0;
	char strTimeZone[10];

	if (NULL == dateTime)
	{
		return QL_RET_ERR_INVALID_PARAMETER;
	}
	Ql_memset(strAT, 0x0, sizeof(strAT));
	if (dateTime->timezone >= 0)
	{
		Ql_sprintf(strTimeZone, "+%02d\0", dateTime->timezone);
	} else {
		Ql_sprintf(strTimeZone, "-%02d\0", dateTime->timezone);
	}
	atLength = Ql_sprintf(strAT, "AT+QALARM=1,\"%02d/%02d/%02d,%02d:%02d:%02d%s\",%d,0",
		dateTime->year, dateTime->month, dateTime->day, dateTime->hour, dateTime->minute, dateTime->second, strTimeZone, mode);
	//Ql_Debug_Trace("%s\r\n", strAT);
	return Ql_RIL_SendATCmd(strAT, atLength, NULL, NULL,0);
}

s32 RIL_Alarm_Query(ST_Time* dateTime)
{
	char strAT[] = "AT+QALARM?\0";
	if (NULL == dateTime)
	{
		return QL_RET_ERR_INVALID_PARAMETER;
	}
	return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATResponse_QALARM_Handler, dateTime, 0);
}

s32 RIL_Alarm_Remove(ST_Time* dateTime)
{
	s32 retRes = 0;
	{
		char strAT[50] = {"\0"};
		char strTimeZone[10];

		if (NULL == dateTime)
		{
			return QL_RET_ERR_INVALID_PARAMETER;
		}
		Ql_memset(strAT, 0x0, sizeof(strAT));
		if (dateTime->timezone >= 0)
		{
			Ql_sprintf(strTimeZone, "+%02d\0", dateTime->timezone);
		} else {
			Ql_sprintf(strTimeZone, "-%02d\0", dateTime->timezone);
		}
		Ql_sprintf(strAT, "AT+QALARM=0,\"%02d/%02d/%02d,%02d:%02d:%02d%s\",0,0", 
			dateTime->year, dateTime->month, dateTime->day, dateTime->hour, dateTime->minute, dateTime->second, strTimeZone);
		//Ql_Debug_Trace("%s\r\n", strAT);
		retRes = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL, NULL,0);
	}
	return retRes;
}

static s32 ATResponse_QALARM_Handler(char* line, u32 len, void* userData)
{
	char* p1 = NULL;
	char* p2 = NULL;
	ST_Time* pDT = (ST_Time*)userData;
	char* head = Ql_RIL_FindString(line, len, "+QALARM:"); //continue wait
	char strTmp[10];

	//Ql_Debug_Trace("Line%d:%s, ", __LINE__, line);

	if (head)
	{
		p1 = Ql_strstr(head, ",");
		if (p1)
		{
			// year
			p1 += 2;
			p2 = Ql_strstr(p1, "/");
			if (p2)
			{
				Ql_memset(strTmp, 0x0, sizeof(strTmp));
				Ql_strncpy(strTmp, p1, 2);
				pDT->year = Ql_atoi(strTmp);
			}

			// month
			p1 = p2 + 1;
			p2 = Ql_strstr(p1, "/");
			if (p2)
			{
				Ql_memset(strTmp, 0x0, sizeof(strTmp));
				Ql_strncpy(strTmp, p1, 2);
				pDT->month = Ql_atoi(strTmp);
			}

			// day
			p1 = p2 + 1;
			p2 = Ql_strstr(p1, ",");
			if (p2)
			{
				Ql_memset(strTmp, 0x0, sizeof(strTmp));
				Ql_strncpy(strTmp, p1, 2);
				pDT->day = Ql_atoi(strTmp);
			}

			// hour
			p1 = p2 + 1;
			p2 = Ql_strstr(p1, ":");
			if (p2)
			{
				Ql_memset(strTmp, 0x0, sizeof(strTmp));
				Ql_strncpy(strTmp, p1, 2);
				pDT->hour = Ql_atoi(strTmp);
			}

			// minute
			p1 = p2 + 1;
			p2 = Ql_strstr(p1, ":");
			if (p2)
			{
				Ql_memset(strTmp, 0x0, sizeof(strTmp));
				Ql_strncpy(strTmp, p1, 2);
				pDT->minute = Ql_atoi(strTmp);
			}

			// second
			p1 = p2 + 1;
			p2 = Ql_strstr(p1, "+");
			if (p2)
			{
				Ql_memset(strTmp, 0x0, sizeof(strTmp));
				Ql_strncpy(strTmp, p1, 2);
				pDT->second = Ql_atoi(strTmp);
			}

			// time zone
			p1 = p2 + 1;
			p2 = Ql_strstr(p1, ",");
			if (p2)
			{
				Ql_memset(strTmp, 0x0, sizeof(strTmp));
				Ql_strncpy(strTmp, p1, 2);
				pDT->timezone = Ql_atoi(strTmp);
			}
		}
		return  RIL_ATRSP_SUCCESS;
	}

	head = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
	if (head)
	{
		return  RIL_ATRSP_SUCCESS;
	}

	head = Ql_RIL_FindLine(line, len, "ERROR");// find <CR><LF>ERROR<CR><LF>, <CR>ERROR<CR>£¬<LF>ERROR<LF>
	if(head)
	{  
		return  RIL_ATRSP_FAILED;
	} 

	return RIL_ATRSP_CONTINUE; //continue wait
}

