#ifdef __EXAMPLE_EPO_CALLBACK__
/***************************************************************************************************
*   Example:
*       
*           GPS Routine with callback and EPO
*
*   Description:
*
*           This example shows how to use GPS with callback and EPO. It will download EPO files and
*       inject it to GNSS module automatically.
*           Customers need to configurate the parameters of NTP server and APN and whether to use 
        reference location obtained by LBS.
*   Usage:
*
*           Precondition:
*
*                   Use "make clean/new" to compile, and download bin image to module.
*       
****************************************************************************************************/
#include "ril.h"
#include "ril_util.h"
#include "ql_type.h"
#include "ql_trace.h"
#include "ql_system.h"
#include "ql_uart.h"
#include "ql_stdlib.h"
#include "ql_error.h"
#include "ril_gps.h"
#include "ql_gnss.h"
#include "ql_gprs.h"
#include "ql_socket.h"
#include "ril_location.h"

#include "ril_network.h"

/***********************Customer needs to configure below parameters***********************/
#define     NTP_SERVER_FOR_TEST         "cn.pool.ntp.org\0"
#define     NTP_SERVER_PORT             (u16)123

#define     APN                         "CMNET\0"
#define     USERID                      ""
#define     PASSWD                      ""
#define     ENABLE_REFERENCE_LOCTION    TRUE   // can be modified to FALSE to disable using reference location
/***********************Customer needs to configure above parameters***********************/

#define     MSG_ID_GPS_MSG_START        MSG_ID_USER_START+100
#define     MSG_ID_GPS_POWERON_REQ      MSG_ID_GPS_MSG_START+1
#define     MSG_ID_GPS_NMEA_IND         MSG_ID_GPS_MSG_START+2
#define     MSG_ID_GPS_EPO_ENABLE_REQ   MSG_ID_GPS_MSG_START+3
#define     MSG_ID_GPS_QNTP_CNF         MSG_ID_GPS_MSG_START+4
#define     MSG_ID_GPS_TIME_SYNC_REQ    MSG_ID_GPS_MSG_START+5
#define     MSG_ID_GPS_CELL_LOC_REQ     MSG_ID_GPS_MSG_START+6
#define     MSG_ID_GPS_SET_REF_LOC_REQ  MSG_ID_GPS_MSG_START+7

#define     INDEX_DATA_VALID_IN_RMC     2
#define     INDEX_LATITUDE_IN_RMC       3
#define     INDEX_NORTH_SOUTH_IN_RMC    4
#define     INDEX_LONGITUDE_IN_RMC      5
#define     INDEX_EAST_WEST_IN_RMC      6

static st_pos   g_my_pos;
static bool     g_stask1_timer1_timeout = FALSE;
static bool     g_stask1_timer2_timeout = FALSE;
static u8       g_pdpCntxtId;
static u32      main_task_timer1 = 0x102;
static u32      sub_task1_timer1 = 0x103;
static u32      sub_task1_timer2 = 0x104;
static u32      gsm_register_timeout = 10000; // unit: ms, using for network registation timeout
static u32      cell_location_timeout = 10000; // unit: ms, using for LBS timeout
static u32      qntp_timeout = 10000; // unit: ms, using for LBS timeout
static void Customer_NMEA_Hdlr(u8 *multi_nmea, u16 len, void *customizePara);
static void Callback_NTP_Hdlr(char *strURC);
static void Main_Task_Timer1_Timeout_Hdlr(u32 timerId, void* param);
static void Cell_Location_Timeout_Hdlr(u32 timerId, void* param);
static void Qntp_Timeout_Hdlr(u32 timerId, void* param);
static void Cell_Location_Program(void);
static void Callback_Cell_Location_Hdlr(s32 result, ST_LocInfo* loc_info);

#define DEBUG_ENABLE 1
#if DEBUG_ENABLE > 0
#define DEBUG_PORT  UART_PORT1
#define DBG_BUF_LEN   512
static char DBG_BUFFER[DBG_BUF_LEN];
#define APP_DEBUG(FORMAT,...) {\
    Ql_memset(DBG_BUFFER, 0, DBG_BUF_LEN);\
    Ql_sprintf(DBG_BUFFER,FORMAT,##__VA_ARGS__); \
    if (UART_PORT2 == (DEBUG_PORT)) \
    {\
        Ql_Debug_Trace(DBG_BUFFER);\
    } else {\
        Ql_UART_Write((Enum_SerialPort)(DEBUG_PORT), (u8*)(DBG_BUFFER), Ql_strlen((const char *)(DBG_BUFFER)));\
    }\
}
#else
#define APP_DEBUG(FORMAT,...) 
#endif

#define SERIAL_RX_BUFFER_LEN  2048
static u8 m_RxBuf_Uart[SERIAL_RX_BUFFER_LEN];
static void Callback_UART_Hdlr(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* param);
static void Callback_GPS_CMD_Hdlr(char *str_URC);


//extern
extern s32 Analyse_Command(u8* src_str,s32 symbol_num,u8 symbol, u8* dest_buf);

/************************************************************************/
/* The entrance for this example application                            */
/************************************************************************/
void proc_main_task(s32 taskId)
{
    s32 ret;
    ST_MSG msg;

    // Register & open UART port
    ret = Ql_UART_Register(UART_PORT1, Callback_UART_Hdlr, NULL);
    if (ret < QL_RET_OK)
    {
        Ql_Debug_Trace("Fail to register serial port[%d], ret=%d\r\n", UART_PORT1, ret);
    }
    
    ret = Ql_UART_Open(UART_PORT1, 115200, FC_NONE);
    if (ret < QL_RET_OK)
    {
        Ql_Debug_Trace("Fail to open serial port[%d], ret=%d\r\n", UART_PORT1, ret);
    }
    
    APP_DEBUG("\r\n<-- OpenCPU: GPS Example -->\r\n");

    // Start message loop of this task
    while (TRUE)
    {
        Ql_OS_GetMessage(&msg);

        switch(msg.message)
        {
            case MSG_ID_RIL_READY:
                Ql_RIL_Initialize();
                APP_DEBUG("<-- RIL is ready -->\r\n");
                break;
            case MSG_ID_USER_START:
                break;
            case MSG_ID_URC_INDICATION:
            {
                switch (msg.param1)
                {
                    case URC_SIM_CARD_STATE_IND:
                        if (SIM_STAT_READY == msg.param2)
                        {
                            s32 ret = Ql_Timer_Register(main_task_timer1, Main_Task_Timer1_Timeout_Hdlr, NULL);
                            if(ret <0)
                            {
                                APP_DEBUG("\r\n<--failed!!, Ql_Timer_Register: timer(%d) fail ,ret = %d -->\r\n",main_task_timer1,ret);
                            }

                            ret = Ql_Timer_Start(main_task_timer1,gsm_register_timeout,FALSE);
                            if(ret < 0)
                            {
                                APP_DEBUG("\r\n<--failed!! stack timer Ql_Timer_Start ret=%d-->\r\n",ret);        
                            }
                            APP_DEBUG("<-- SIM card is ready -->\r\n");
                        }
                        else
                        {
                            APP_DEBUG("<-- SIM card is not available, cause:%d -->\r\n", msg.param2);
                            Ql_OS_SendMessage(subtask1_id, MSG_ID_GPS_POWERON_REQ, NULL, NULL);  
                        }
                        break;
                    case URC_GSM_NW_STATE_IND:
                        APP_DEBUG("<-- GSM network status:%d -->\r\n", msg.param2);
                        if (NW_STAT_REGISTERED == msg.param2 || NW_STAT_REGISTERED_ROAMING == msg.param2)
                        {
                            
                            s32 ret = Ql_Timer_Stop(main_task_timer1);
                            if(ret < 0)
                            {
                                  APP_DEBUG("\r\n<--failed!! stack timer Ql_Timer_Stop ret=%d-->\r\n",ret);           
                            }
                        }
                        break;
                    case URC_GPRS_NW_STATE_IND:
                        APP_DEBUG("<-- GPRS Network Status:%d -->\r\n", msg.param2);
                         if((NW_STAT_REGISTERED == msg.param2)||(NW_STAT_REGISTERED_ROAMING == msg.param2))
                        {
                            if(ENABLE_REFERENCE_LOCTION)
                            {
                                Ql_OS_SendMessage(subtask1_id, MSG_ID_GPS_CELL_LOC_REQ, NULL, NULL);
                            }
                            else
                            {
                                Ql_OS_SendMessage(subtask1_id, MSG_ID_GPS_TIME_SYNC_REQ, NULL, NULL);
                            }
                        }
                        break;
                    default:
                        break;
                }
            }
                break;
            default:
                break;
        }
    }
}

static s32 ReadSerialPort(Enum_SerialPort port, /*[out]*/u8* pBuffer, /*[in]*/u32 bufLen)
{
    s32 rdLen = 0;
    s32 rdTotalLen = 0;
    if (NULL == pBuffer || 0 == bufLen)
    {
        return -1;
    }
    Ql_memset(pBuffer, 0x0, bufLen);
    while (1)
    {
        rdLen = Ql_UART_Read(port, pBuffer + rdTotalLen, bufLen - rdTotalLen);
        if (rdLen <= 0)  // All data is read out, or Serial Port Error!
        {
            break;
        }
        rdTotalLen += rdLen;
        // Continue to read...
    }
    if (rdLen < 0) // Serial Port Error!
    {
        APP_DEBUG("<--Fail to read from port[%d]-->\r\n", port);
        return -99;
    }
    return rdTotalLen;
}


static void Callback_UART_Hdlr(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* param)
{
    s32 iRet = 0;
    switch (msg)
    {
        case EVENT_UART_READY_TO_READ:
        {
            char* p = NULL;
            s32 totalBytes = ReadSerialPort(port, m_RxBuf_Uart, sizeof(m_RxBuf_Uart));
            if (totalBytes <= 0)
            {
                break;
            }

            APP_DEBUG("Invalid command...\r\n");
        }break;

        case EVENT_UART_READY_TO_WRITE:
        {
            //...
        }break;

        default:
        break;
    }
}

void proc_subtask1(s32 TaskId)
{
    bool keepGoing = TRUE;
    ST_MSG subtask1_msg;
    
    Ql_Debug_Trace("<--multitask: example_task1_entry-->\r\n");
    while(keepGoing)
    {    
        Ql_OS_GetMessage(&subtask1_msg);
        switch(subtask1_msg.message)
        {
            case MSG_ID_USER_START:
                break;
            case MSG_ID_GPS_CELL_LOC_REQ:
            {
                s32 ret = Ql_Timer_Register(sub_task1_timer1, Cell_Location_Timeout_Hdlr, NULL);
                if(ret <0)
                {
                    APP_DEBUG("\r\n<--failed!!, Ql_Timer_Register: timer(%d) fail ,ret = %d -->\r\n",main_task_timer1,ret);
                }

                ret = Ql_Timer_Start(sub_task1_timer1,cell_location_timeout,FALSE);
                if(ret < 0)
                {
                    APP_DEBUG("\r\n<--failed!! stack timer Ql_Timer_Start ret=%d-->\r\n",ret);        
                }
                Cell_Location_Program();
            }
                break;
            case MSG_ID_GPS_SET_REF_LOC_REQ:
            {
                st_pos *pos = (st_pos*)subtask1_msg.param1;
                if(!g_stask1_timer1_timeout)
                {
                    s32 ret;
                    ret = Ql_Timer_Stop(sub_task1_timer1);
                    if(ret < 0)
                    {
                          APP_DEBUG("\r\n<--failed!! stack timer Ql_Timer_Stop ret=%d-->\r\n",ret);           
                    }
                }
                APP_DEBUG("<-- set reference location [%f,%f] -->\r\n", pos->lat, pos->lon);
                RIL_GPS_SetRefLoc(pos->lat, pos->lon);
                Ql_OS_SendMessage(subtask1_id, MSG_ID_GPS_TIME_SYNC_REQ, NULL, NULL);
            }
                break;
            case MSG_ID_GPS_TIME_SYNC_REQ:
            {    
                u8 time_sync=0;
                RIL_GPS_Read_TimeSync_Status(&time_sync);

                if(time_sync)
                {
                    APP_DEBUG("<-- NITZ synchronized time succeed -->\r\n");
                    Ql_OS_SendMessage(subtask1_id, MSG_ID_GPS_EPO_ENABLE_REQ, NULL, NULL);
                }
                else
                {
                    APP_DEBUG("<-- NITZ synchronized time failed! -->\r\n");
                    s32 ret = Ql_Timer_Register(sub_task1_timer2, Qntp_Timeout_Hdlr, NULL);
                    if(ret <0)
                    {
                        APP_DEBUG("\r\n<--failed!!, Ql_Timer_Register: timer(%d) fail ,ret = %d -->\r\n",main_task_timer1,ret);
                    }

                    ret = Ql_Timer_Start(sub_task1_timer2, qntp_timeout,FALSE);
                    if(ret < 0)
                    {
                        APP_DEBUG("\r\n<--failed!! stack timer Ql_Timer_Start ret=%d-->\r\n",ret);        
                    }
                    APP_DEBUG("<-- Start QNTP -->\r\n");
                    RIL_NTP_START(NTP_SERVER_FOR_TEST, NTP_SERVER_PORT, Callback_NTP_Hdlr);
                }
            }
                break;
            case MSG_ID_GPS_QNTP_CNF:
            {
                if(!g_stask1_timer2_timeout)
                {
                    s32 ret = Ql_Timer_Stop(sub_task1_timer2);
                    if(ret < 0)
                    {
                          APP_DEBUG("\r\n<--failed!! stack timer Ql_Timer_Stop ret=%d-->\r\n",ret);           
                    }
                }
                if(subtask1_msg.param1==0)
                {
                    APP_DEBUG("<-- QNTP succeed -->\r\n");
                    Ql_OS_SendMessage(subtask1_id, MSG_ID_GPS_EPO_ENABLE_REQ, NULL, NULL);
                }
                else 
                {
                    s32 status = 0;
                    APP_DEBUG("<-- QNTP failed, ret=%d! -->\r\n", subtask1_msg.param1);
                    RIL_GPS_GetPowerState(&status);
                    if(status == 0)
                    {
                        Ql_OS_SendMessage(subtask1_id, MSG_ID_GPS_POWERON_REQ, NULL, NULL);
                    }
                }
            }
                break;
            case MSG_ID_GPS_EPO_ENABLE_REQ:
            {
                s32 status = 0;
                APP_DEBUG("<-- Set APN for context 2 -->\r\n");
                RIL_GPS_EPO_Config_APN(APN, USERID, PASSWD);
                APP_DEBUG("<-- Set PDP context id back to %d -->\r\n", g_pdpCntxtId);
                RIL_NW_SetGPRSContext(g_pdpCntxtId);

                APP_DEBUG("<-- Enable EPO -->\r\n");
                RIL_GPS_EPO_Enable(1);
                RIL_GPS_GetPowerState(&status);
                if(status == 0)
                {
                    Ql_OS_SendMessage(subtask1_id, MSG_ID_GPS_POWERON_REQ, NULL, NULL);
                }
                else if(status == 1)    // GPS has been powered on
                {
                    APP_DEBUG("<-- GPS power on already, aiding EPO to GPS -->\r\n");
                    RIL_GPS_EPO_Aid();
                }
            }
                break;
            case MSG_ID_GPS_POWERON_REQ:
            {
                s32 iRet = Ql_GNSS_PowerOn(RMC_EN | GGA_EN, Customer_NMEA_Hdlr, NULL);  // Also can use ALL_NMEA_EN to enable receiving all NMEA sentences.
                if(RIL_AT_SUCCESS != iRet) 
                {
                    APP_DEBUG("<-- Power on GNSS failed! -->\r\n", iRet);
                    break;
                }
                APP_DEBUG("<-- Power on GNSS succeed -->\r\n");
            }
                break;
            case MSG_ID_GPS_NMEA_IND:
            {
                if(subtask1_msg.param1==TRUE)
                {
                    st_pos* pos = (st_pos*)subtask1_msg.param2;
                    APP_DEBUG("<-- GPS fixed, Position[%0.4f,%0.4f] -->\r\n", pos->lat, pos->lon);
                }
                else
                {
                    APP_DEBUG("<-- GPS unfix -->\r\n");
                }
            }
                break;
            default:
                break;
        }
    }    
}

static void Callback_NTP_Hdlr(char *strURC)
{
	char *p1 = NULL;
	char *p2 = NULL;
	char strResult[10] = {0};
	u8   ntpResult = 0;
	p1 = Ql_strstr(strURC, ":");
    p2 = Ql_strstr(p1, "\r\n");
	if(p1 && p2)
	{
		Ql_memset(strResult, 0x0, sizeof(strResult));
    	Ql_strncpy(strResult, p1 + 1, p2 - p1 - 1);
		ntpResult = Ql_atoi(strResult);

        Ql_OS_SendMessage(subtask1_id, MSG_ID_GPS_QNTP_CNF, ntpResult, NULL);
	}
}

// timer callback function
static void Main_Task_Timer1_Timeout_Hdlr(u32 timerId, void* param)
{
    if(main_task_timer1 == timerId)
    {
        s32 ret;
        s32 gsm_state=0;
        ret = Ql_Timer_Stop(main_task_timer1);
        if(ret < 0)
        {
              APP_DEBUG("\r\n<--failed!! stack timer Ql_Timer_Stop ret=%d-->\r\n",ret);           
        }
        
        RIL_NW_GetGSMState(&gsm_state);
        if(gsm_state!=NW_STAT_REGISTERED && gsm_state!=NW_STAT_REGISTERED_ROAMING)
        {
            APP_DEBUG("\r\n<-- GSM network registation timeout! -->\r\n");
            Ql_OS_SendMessage(subtask1_id, MSG_ID_GPS_POWERON_REQ, NULL, NULL);
        }
    }
}

static void Cell_Location_Timeout_Hdlr(u32 timerId, void* param)
{
    if(sub_task1_timer1 == timerId)
    {
        s32 ret;
        ret = Ql_Timer_Stop(sub_task1_timer1);
        if(ret < 0)
        {
              APP_DEBUG("\r\n<--failed!! stack timer Ql_Timer_Stop ret=%d-->\r\n",ret);           
        }
        g_stask1_timer1_timeout = TRUE;
        APP_DEBUG("\r\n<-- GSM cell location timeout! -->\r\n");
        Ql_OS_SendMessage(subtask1_id, MSG_ID_GPS_TIME_SYNC_REQ, NULL, NULL);
    }
}

static void Qntp_Timeout_Hdlr(u32 timerId, void* param)
{
    if(sub_task1_timer2 == timerId)
    {
        s32 ret;
        ret = Ql_Timer_Stop(sub_task1_timer2);
        if(ret < 0)
        {
              APP_DEBUG("\r\n<--failed!! stack timer Ql_Timer_Stop ret=%d-->\r\n",ret);           
        }
        g_stask1_timer2_timeout = TRUE;
        APP_DEBUG("\r\n<-- GSM cell location timeout! -->\r\n");
        Ql_OS_SendMessage(subtask1_id, MSG_ID_GPS_POWERON_REQ, NULL, NULL);
    }
}

static void Cell_Location_Program(void)
{
	s32 ret;

	u8 location_mode = 0;
	ST_CellInfo GetLocation;
	GetLocation.cellId = 22243;
	GetLocation.lac = 21764;
	GetLocation.mnc = 01;
	GetLocation.mcc = 460;
	GetLocation.rssi = 0;
	GetLocation.timeAd = 0;

	// Set PDP context
	ret = Ql_GPRS_GetPDPContextId();
	APP_DEBUG("<-- The PDP context id available is: %d (can be 0 or 1)-->\r\n", ret);
	if (ret >= 0)
	{
	    g_pdpCntxtId = (u8)ret;
	} else {
    	APP_DEBUG("<-- Fail to get PDP context id, try to use PDP id(0) -->\r\n");
	    g_pdpCntxtId = 0;
	}

	ret = RIL_NW_SetGPRSContext(g_pdpCntxtId);
	APP_DEBUG("<-- Set PDP context id to %d -->\r\n", g_pdpCntxtId);
	if (ret != RIL_AT_SUCCESS)
	{
	    APP_DEBUG("<-- Ql_RIL_SendATCmd error  ret=%d-->\r\n",ret );
	}

	// Set APN
	ret = RIL_NW_SetAPN(1, APN, USERID, PASSWD);
	APP_DEBUG("<-- Set APN for context %d -->\r\n", g_pdpCntxtId);

	// Request to get location
	APP_DEBUG("<-- Getting module location... -->\r\n");
	if(location_mode==0)
	{
		APP_DEBUG("<-- Get reference location by cell info -->\r\n");
		ret = RIL_GetLocation(Callback_Cell_Location_Hdlr);
		if(ret!=RIL_AT_SUCCESS)
		{
			APP_DEBUG("<-- Ql_Getlocation error  ret=%d-->\r\n",ret );
		}
	}
	else if(location_mode==1)
	{
		APP_DEBUG("<--Ql_GetlocationByCell-->\r\n");
		ret = RIL_GetLocationByCell(&GetLocation,Callback_Cell_Location_Hdlr);
		if(ret!=RIL_AT_SUCCESS)
		{
			APP_DEBUG("<-- Ql_GetlocationByCell error  ret=%d-->\r\n",ret );
		}
	}
}

static void Callback_Cell_Location_Hdlr(s32 result, ST_LocInfo* loc_info)
{
    APP_DEBUG("<-- Cell Location succeed -->\r\n");
    g_my_pos.lat = loc_info->latitude;
    g_my_pos.lon = loc_info->longitude;
    Ql_OS_SendMessage(subtask1_id, MSG_ID_GPS_SET_REF_LOC_REQ, (u32)&g_my_pos, NULL);
}

void Callback_GPS_CMD_Hdlr(char *str_URC)
{
	if(str_URC != NULL)
	{
		APP_DEBUG("%s", str_URC);
	}
}

/*****************************************************************
* Function:     isXdigit 
* 
* Description:
*               This function is used to check whether the 
*               input character number is a uppercase hexadecimal digital.
*
* Parameters:
*               [in]ch:   
*                       A character num.
*
* Return:        
*               TRUE  - The input character is a uppercase hexadecimal digital.
*               FALSE - The input character is not a uppercase hexadecimal digital.
*
*****************************************************************/
bool isXdigit(char ch)
{
    if((ch>='0' && ch<='9'
) || (ch >= 'A' && ch<='F'))
    {
        return TRUE;
    }

    return FALSE;
}

/*****************************************************************
* Function:     Ql_check_nmea_valid 
* 
* Description:
*               This function is used to check whether the 
*               input nmea is a valid nmea which contains '$' and <CR>.
*
* Parameters:
*               [in]nmea_in:   
*                       string of a sigle complete NMEA sentence
*
* Return:        
*               TRUE  - nmea_in is a valid NMEA sentence.
*               FALSE - nmea_in is an invalid NMEA sentence.
*
*****************************************************************/
bool Ql_check_nmea_valid(char *nmea_in)
{
    char *p=NULL, *q=NULL;
    
    u8 len = Ql_strlen(nmea_in);
    
    if(len > MAX_NMEA_LEN)                              // validate NMEA length
    {
        return FALSE;
    }
    
    p = nmea_in;
    if(*p == '$')                                       // validate header
    {
        q=Ql_strstr(p, "\r");                           // validate tail
        if(q)
        {
            if(isXdigit(*(q-1)) && isXdigit(*(q-2)))    // validate checksum is a hex digit
            {
                u8 cs_cal = 0;
                char cs_str[3]={0};
                while(*++p!='*')
                {
                    cs_cal ^= *p;
                }
                Ql_sprintf(cs_str, "%X", cs_cal);        // checksum is always uppercase.
                if(!Ql_strncmp(cs_str, q-2, 2))          // validate checksum(2 bits)
                {
                    return TRUE;
                }
                else
                {
                    return FALSE;
                }
            }
            else
            {
                return FALSE;
            }
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
    
    return FALSE;
}

/*****************************************************************
* Function:     Ql_get_nmea_segment_by_index 
* 
* Description:
*               This function is used to get the specified segment from a string
*               of a sigle complete NMEA sentence which contains '$' and <CR>.
*
* Parameters:
*               [in]nmea_in:   
*                       String of a sigle complete NMEA sentence
*               [in]index:     
*                       The index of the segment to get.
*               [out]segment: 
*                       The buffer to save the specified segment.
*               [in]max_len: 
*                       The length of the segment buffer.
*
* Return:        
*               >0 - Get segment successfully and it's the length of the segment.
*               =0 - Segment is empty.
*               <0 - Error number.
*
*****************************************************************/
char Ql_get_nmea_segment_by_index(char *nmea_in, u8 index, char *segment, u8 max_len)
{
    char *p=NULL, *q=NULL;
    u8 i=0;
    
    p = nmea_in;
    q = NULL;
    
    for(i=0; i<index; i++)
    {
        q=Ql_strstr(p, ",");
        if(q)
        {
            p = q+1;
        }
        else
        {
            break;
        }
    }
    
    q = NULL;
    if(i<index)
    {
        return -1;                  // Segment is not exist;
    }
    else
    {
        if((q=Ql_strstr(p, ",")) || (q=Ql_strstr(p, "*")))
        {
            u8 dest_len = q-p;
            if((*p==',') && (dest_len==1))
            {
                return 0;           // Segment is empty which is between ',' and ',' or ',' and '*'.
            }            
            else if(dest_len<max_len)
            {
                Ql_memset(segment, 0, max_len);
                Ql_memcpy(segment, p, dest_len);
                return dest_len;
            }
            else
            {
                return -2;          // Segment is longer than max_len!
            }
        }
        else
        {
            return -1;             // Segment is not exist;
        }
    }
    return -100;                   // Unknow error!
}

static void Customer_NMEA_Hdlr(u8 *multi_nmea, u16 len, void *customizePara)
{
    u8 *p=NULL, *q=NULL;
    char one_nmea[MAX_NMEA_LEN+1];
    char dest_segment[32];
    bool fix_status = FALSE;
    s32 ret;

    //RMC
    p=NULL; q=NULL;
    p = Ql_strstr(multi_nmea, "$GNRMC");
    if(p)
    {
        q = Ql_strstr(p, "\r");
        if(q)
        {
            Ql_memset(one_nmea, 0, sizeof(one_nmea));
            Ql_memcpy(one_nmea, p, q-p+1);
            if(Ql_check_nmea_valid(one_nmea))
            {
                //fix status
                char ret=-1;
                ret = Ql_get_nmea_segment_by_index(one_nmea, INDEX_DATA_VALID_IN_RMC, dest_segment, sizeof(dest_segment));
                if(ret > 0)
                {
                    if(!Ql_strcmp(dest_segment, "A"))
                    {
                        fix_status = TRUE;
                    }
                    else
                    {
                        fix_status = FALSE;
                    }
                }
                
                //Latitude
                ret = -1;
                ret = Ql_get_nmea_segment_by_index(one_nmea, INDEX_LATITUDE_IN_RMC, dest_segment, sizeof(dest_segment));
                if(ret > 0)
                {
                    g_my_pos.lat = Ql_atof(dest_segment);
                }

                // N/S
                ret = -1;
                ret = Ql_get_nmea_segment_by_index(one_nmea, INDEX_NORTH_SOUTH_IN_RMC, dest_segment, sizeof(dest_segment));
                if(ret > 0)
                {
                    if(!Ql_strcmp(dest_segment, "S"))
                    {
                        g_my_pos.lat = -g_my_pos.lat;
                    }
                }

                //Longitude -index 5
                ret=-1;
                ret = Ql_get_nmea_segment_by_index(one_nmea, INDEX_LONGITUDE_IN_RMC, dest_segment, sizeof(dest_segment));
                if(ret > 0)
                {
                    g_my_pos.lon = Ql_atof(dest_segment);
                }

                // E/W
                ret = -1;
                ret = Ql_get_nmea_segment_by_index(one_nmea, INDEX_EAST_WEST_IN_RMC, dest_segment, sizeof(dest_segment));
                if(ret > 0)
                {
                    if(!Ql_strcmp(dest_segment, "W"))
                    {
                        g_my_pos.lon = -g_my_pos.lon;
                    }
                }
            }
        }
    }
    
    Ql_OS_SendMessage(subtask1_id, MSG_ID_GPS_NMEA_IND, fix_status, (u32)&g_my_pos);
}

#endif  //__EXAMPLE_ALARM__

