#ifdef __EXAMPLE_GPS__
/***************************************************************************************************
*   Example:
*       
*           GPS Routine
*
*   Description:
*
*           This example gives an example for GPS operation.it use to get the navigation information.
*           Through Uart port, input the special command, there will be given the response about module operation.
*
*   Usage:
*
*           Precondition:
*
*                   Use "make clean/new" to compile, and download bin image to module.
*           
*           Through Uart port:
*
*               Input "GPSOpen"  to retrieve the current local date and time.
*               Input "GPSClose"  to set the current local date and time.
*               Input "GPSRead=<item>" to get navigation information which you want.
*               Input "GPSCMD=<cmdType>,<cmdString>" to send command to GPS module.
*				Input "GPSEPO=<status>" to enable/disable EPO download.
*				Input "GPSAid" to inject EPO data into GPS module.
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
#include "nema_pro.h"


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

void Callback_GPS_CMD_Hdlr(char *str_URC)
{
	if(str_URC != NULL)
	{
		APP_DEBUG("%s", str_URC);
	}
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

            //APP_DEBUG("command is : %s\r\n",m_RxBuf_Uart);

            /* Power On GPS */
			p = Ql_strstr(m_RxBuf_Uart, "GPSOpen");
            if(p)
            {
                iRet = RIL_GPS_Open(1);
                if(RIL_AT_SUCCESS != iRet) 
                {
                    APP_DEBUG("Power on GPS fail, iRet = %d.\r\n", iRet);
                    break;
                }

                APP_DEBUG("Power on GPS Successful.\r\n");
                break;
            }

            /* Power Off GPS */
            p = Ql_strstr(m_RxBuf_Uart, "GPSClose");
            if(p)
            {
                iRet = RIL_GPS_Open(0);
                if(RIL_AT_SUCCESS != iRet) 
                {
                    APP_DEBUG("Power off GPS fail, iRet = %d.\r\n", iRet);
                    break;
                }

                APP_DEBUG("Power off GPS Successful.\r\n");
                break;
            }

            /* GPSRead=<item> */
            p = Ql_strstr(m_RxBuf_Uart, "GPSRead=<");
            if(p)
            {
                u8 rdBuff[1000];
                u8 item[10] = {0};

                Ql_memset(rdBuff,0,sizeof(rdBuff));
                Ql_strncpy(item,m_RxBuf_Uart+9,3);

                iRet = RIL_GPS_Read(item,rdBuff);
                if(RIL_AT_SUCCESS != iRet)
                {
                    APP_DEBUG("Read %s information failed.\r\n",item);
                    break;
                }
                APP_DEBUG("%s\r\n",rdBuff);
                break;
            }

			/* GPSEPO=<status>*/
            p = Ql_strstr(m_RxBuf_Uart, "GPSEPO");
            if(p)
            {
				s8 status = 0;
				u8 status_buf[8] = {0};
				if (Analyse_Command(m_RxBuf_Uart, 1, '>', status_buf))
				{
					APP_DEBUG("<--Parameter I Error.-->\r\n");
					break;
				}
				status = Ql_atoi(status_buf);
				
				iRet = RIL_GPS_EPO_Enable(status);
				
                if(RIL_AT_SUCCESS != iRet) 
                {
                    APP_DEBUG("Set EPO status to %d fail, iRet = %d.\r\n", status, iRet);
                    break;
                }

                APP_DEBUG("Set EPO status to %d successful, iRet = %d.\r\n", status, iRet);
                break;
            }

			/* GPSAid */
            p = Ql_strstr(m_RxBuf_Uart, "GPSAid");
            if(p)
            {	
				iRet = RIL_GPS_EPO_Aid();
				
                if(RIL_AT_SUCCESS != iRet) 
                {
                    APP_DEBUG("EPO aiding fail, iRet = %d.\r\n", iRet);
                    break;
                }

                APP_DEBUG("EPO aiding successful, iRet = %d.\r\n", iRet);
                break;
            }
			
			/* GPSCMD=<cmdType>,<cmdString> */
            p = Ql_strstr(m_RxBuf_Uart, "GPSCMD=<");
            if(p)
            {
                s8 type;
				u8 cmd_buf[128] = {0};
				u8 type_buf[8] = {0};
				if (Analyse_Command(m_RxBuf_Uart, 1, '>', type_buf))
				{
					APP_DEBUG("<--Parameter I Error.-->\r\n");
					break;
				}
				type = Ql_atoi(type_buf);
                Ql_memset(cmd_buf,0,sizeof(cmd_buf));
				if (Analyse_Command(m_RxBuf_Uart, 2, '>', cmd_buf))
                {
                    APP_DEBUG("<--Parameter II Error.-->\r\n");
                    break;
                }

                iRet = RIL_GPS_CMD_Send(type, cmd_buf, Callback_GPS_CMD_Hdlr);
				
                if(RIL_AT_SUCCESS != iRet)
                {
                    APP_DEBUG("Send command \"%s\" failed! iRet=%d.\r\n",cmd_buf, iRet);
                    break;
                }
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

#endif  //__EXAMPLE_ALARM__

