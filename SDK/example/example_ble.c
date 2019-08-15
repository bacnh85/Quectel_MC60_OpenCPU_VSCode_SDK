#include "custom_feature_def.h"
#ifdef __OCPU_RIL_BLE_SUPPORT__
#ifdef __EXAMPLE_BLUETOOTH_BLE__

#include "ql_stdlib.h"
#include "ql_uart.h"
#include "ql_trace.h"
#include "ql_type.h"
#include "ql_system.h"
#include "ril.h"
#include "ril_bluetooth.h"
#include "ril_ble.h"



#define DEBUG_ENABLE 1
#if DEBUG_ENABLE > 0
#define DEBUG_PORT  UART_PORT1
#define DBG_BUF_LEN   2048
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

#define SERIAL_RX_BUFFER_LEN  (2048)
#define BL_RX_BUFFER_LEN       (1024+1)

static u8 m_RxBuf_Uart1[SERIAL_RX_BUFFER_LEN];
static u8 m_RxBuf_BL[BL_RX_BUFFER_LEN];

#define MSG_GATT_SERVER_REG   (700)
#define MSG_GATT_ADD_REMOVE_SERVICE   (701)

#define MSG_GATT_ADD_CHAR   (702)
#define MSG_GATT_ADD_DESP   (703)
#define MSG_GATT_WREG_RESP   (704)
#define MSG_GATT_RREG_RESP   (705)
#define MSG_GATT_SERVICE_START   (706)


ST_BLE_Server Qserver1= {0};
s32 appearance=0;
s32 string_mode=0;
char manufacture_data[32]={0};
char service_data[32]={0};
char adv_service_uuid[32]={0};

static char bt_name[BT_NAME_LEN] = "ble_test";

#define NAME_LEN		"0B"		//11
#define BLE_NAME		"5155454354454c2d4254"
#define MANUFACTURE_LEN		"09"
#define MANUFACTURE		"4E42313233343536"

u8 ble_adv_set[62]="020106"NAME_LEN"09"BLE_NAME""MANUFACTURE_LEN"ff"MANUFACTURE"";
u8 ble_rsp_data[62]="11066e400001b5a3f393e0a9e50e24dcca9e";

    
//void BLE_Test(void);
static void BLE_COM_Demo(void);
static void CallBack_UART_Hdlr(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara);
static void BLE_Callback(s32 event, s32 errCode, void* param1, void* param2); ;
static s32 ATResponse_Handler(char* line, u32 len, void* userData);

void proc_subtask1(s32 taskId)
{
    ST_MSG msg;
    // Register & open UART port
    Ql_UART_Register(UART_PORT1, CallBack_UART_Hdlr, NULL);
    Ql_UART_Open(UART_PORT1, 115200, FC_NONE);

    Ql_UART_Register(UART_PORT2, CallBack_UART_Hdlr, NULL);
    Ql_UART_Open(UART_PORT2, 115200, FC_NONE);

    APP_DEBUG("\r\n<-- OpenCPU: subtask -->\r\n")

    while (TRUE)
    {
        Ql_OS_GetMessage(&msg);
        switch(msg.message)
        {
            default:
                break;
        }
    }
}


//main function
void proc_main_task(s32 taskId)
{
    s32 ret;
    ST_MSG msg;
    while (TRUE)
    {
        Ql_OS_GetMessage(&msg);
        switch(msg.message)
        {
            case MSG_ID_RIL_READY:
                
                APP_DEBUG("<-- RIL is ready -->\r\n");
                
                Ql_RIL_Initialize();
                
                BLE_COM_Demo();
            break;

            case MSG_ID_USER_START:
                break;
            case MSG_GATT_SERVER_REG:
                ret=RIL_BT_Gatsreg(1,&Qserver1);
                APP_DEBUG("\r\n<--RIL_BT_Gatsreg: ret=%d -->\r\n",ret);
                break;
            case MSG_GATT_ADD_REMOVE_SERVICE:
                ret=RIL_BT_Gatss(1,&Qserver1);
                     
                APP_DEBUG("\r\n<--RIL_BT_Gatss: ret=%d -->\r\n",ret);
                break;
            case MSG_GATT_ADD_CHAR:
                ret=RIL_BT_Gatsc(1,&Qserver1);
                     
                APP_DEBUG("\r\n<--RIL_BT_Gatsc: ret=%d -->\r\n",ret);
                break;
            case MSG_GATT_ADD_DESP:
                ret=RIL_BT_Gatsd(1,&Qserver1);
                 APP_DEBUG("\r\n<--RIL_BT_Gatsd: ret=%d -->\r\n",ret);
                 break;
            case MSG_GATT_SERVICE_START:
                ret=RIL_BT_Gatsst(1,&Qserver1);
                APP_DEBUG("\r\n<--RIL_BT_Gatsst: ret=%d -->\r\n",ret);
                 break;             
             case  MSG_GATT_WREG_RESP:
                {
                    char testdata[32]="ab12";//对写数据回复的数据，可随意设置
               
                    Qserver1.wrreq_param.need_rsp=0;
                     Ql_memcpy(Qserver1.wrreq_param.value,testdata,sizeof(testdata));
				//	 APP_DEBUG("\r\n-response data:%s for write request\r\n",testdata);
                    RIL_BT_Gatsrsp(&Qserver1); //response to clent for write requst
             }
                break;
              case  MSG_GATT_RREG_RESP:
                {
                    char testdata[32]="ab12"; //对读请求，需要回复该attr实际的参数数值，此处只做参考
                    Qserver1.wrreq_param.need_rsp=0;
                    Ql_memcpy(Qserver1.wrreq_param.value,testdata,sizeof(testdata));
                    APP_DEBUG("\r\n-response data:%s\r\n",testdata);
                    RIL_BT_Gatsrsp(&Qserver1);//response to clent for read requst
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
        APP_DEBUG("Fail to read from port[%d]\r\n", port);
        return -99;
    }
    return rdTotalLen;
}


static void CallBack_UART_Hdlr(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara)
{
    s32 ret = RIL_AT_SUCCESS ;


    switch (msg)
    {
    case EVENT_UART_READY_TO_READ:
        {
            if (DEBUG_PORT == port)
            {
                s32 totalBytes = ReadSerialPort(port, m_RxBuf_Uart1, sizeof(m_RxBuf_Uart1));
                
                if (totalBytes <= 0)
                {
                    Ql_Debug_Trace("<-- No data in UART buffer! -->\r\n");
                    return;
                }

                APP_DEBUG("\r\n<-- m_RxBuf_Uart1:%s->\r\n",m_RxBuf_Uart1)
                // register a gatt server                         
                if(0 == Ql_memcmp(m_RxBuf_Uart1,"gatsreg",7)) 
                {                    
                    Ql_OS_SendMessage(main_task_id,MSG_GATT_SERVER_REG,0,0);
                    
                    break;
                }

                //add a service 
                if(0 == Ql_memcmp(m_RxBuf_Uart1,"gatss",5)) 
                { 
                    Ql_OS_SendMessage(main_task_id,MSG_GATT_ADD_REMOVE_SERVICE,0,0);
  
                    break;
                }
      
                //add a charcter
                if(0 == Ql_memcmp(m_RxBuf_Uart1,"gatsc",5))
                {   
                    Ql_OS_SendMessage(main_task_id,MSG_GATT_ADD_CHAR,0,0);
           
                    break;
                }

                //add a descrp
                if(0 == Ql_memcmp(m_RxBuf_Uart1,"gatsd",5)) 
                {  
                    Ql_OS_SendMessage(main_task_id,MSG_GATT_ADD_DESP,0,0);

                    break;
                }

                //start service
                if(0 == Ql_strncmp(m_RxBuf_Uart1,"gatst",5)) 
                {      
                    Ql_OS_SendMessage(main_task_id,MSG_GATT_SERVICE_START,0,0);
                        
                    break;
                }

                //send indicate to client
                if(0 == Ql_strncmp(m_RxBuf_Uart1,"gatsind=",8)) 
                {
                    u8 *p1,*p2;
                    p1 = m_RxBuf_Uart1+8;
                    p2 = Ql_strstr(m_RxBuf_Uart1,"\r\n");
                    if( p1 && p2)
                    {
                        Ql_memset(Qserver1.sind_param.value,0,sizeof(Qserver1.sind_param.value));
                        Ql_strncpy(Qserver1.sind_param.value,(char*)p1,p2-p1);
                    }

                    Qserver1.sid=0;
                    Qserver1.service_id[Qserver1.sid].cid=0;
                    Qserver1.sind_param.attr_handle=Qserver1.service_id[Qserver1.sid].char_id[Qserver1.service_id[Qserver1.sid].cid].char_handle;
                    Qserver1.sind_param.need_cnf=0;

                    if(Qserver1.conn_status.connect_status == 1)
					{
						ret=RIL_BT_Gatsind(&Qserver1);
						APP_DEBUG("\r\n<--RIL_BT_Gatsind: ret=%d -->\r\n",ret);
					}
						   
                    break;
                }                
	
				//add advertising data
				if(0 == Ql_strncmp(m_RxBuf_Uart1,"gatsetadv=",10)) 
				{
					char service_temp[6]={0};
				
					Ql_sscanf(m_RxBuf_Uart1, "%*[^=]=%d,%d,%[^,],%[^,],%s[^\r\n]\r\n",&appearance,&string_mode,manufacture_data,service_data,service_temp); //result

					ret=RIL_BT_QGatSetadv(Qserver1.gserv_id,appearance,string_mode,manufacture_data,service_data,service_temp);

					APP_DEBUG("\r\n<--RIL_BT_QGatSetadv: ret=%d -->\r\n",ret);
						   
					break;
				}	

				//set BT device name
                if(0 == Ql_memcmp(m_RxBuf_Uart1,"name=",5)) 
                {
                    u8 *p1,*p2;
                    p1 = Ql_strstr(m_RxBuf_Uart1,"\"");
                    p2 = Ql_strstr(p1+1,"\"");
                    if( p1 && p2)
                    {
                        Ql_memset(bt_name,0,sizeof(bt_name));
                        Ql_strncpy(bt_name,p1+1,sizeof(bt_name)<=(p2-p1-1)?sizeof(bt_name):(p2-p1-1));
                        ret = RIL_BT_SetName(bt_name,Ql_strlen(bt_name));
                        if(RIL_AT_SUCCESS == ret)
                        {
                            APP_DEBUG("BT device name set successful.\r\n");
							ret = RIL_BT_Gatsl(1,&Qserver1);
							APP_DEBUG("\r\n<--RIL_BT_Gatsl: ret=%d -->\r\n",ret);
                        }
                        else
                        {
                            APP_DEBUG("BT device name set failed,ret=%d.\r\n",ret);
                        }
                    }
                    else
                    {
                        APP_DEBUG("Invalid parameters.\r\n");
                    }
                    break;
                }

				//get BLE device address
				if(0 == Ql_memcmp(m_RxBuf_Uart1,"bleaddr",7)) 
				{
					char bleaddr[13]={0};
					ret = RIL_BT_GetLeLocalAddr(bleaddr,12);

					APP_DEBUG("\r\n<--bleaddr:%s: ret=%d -->\r\n",bleaddr,ret);
					
					break;
				}
				
				//start advertising 
				if(0 == Ql_strncmp(m_RxBuf_Uart1,"gatsl",5)) 
				{
					
					ret = RIL_BT_Gatsl(1,&Qserver1);
					APP_DEBUG("\r\n<--RIL_BT_Gatsl: ret=%d -->\r\n",ret);
						   
					break;
				}

				if(0 == Ql_strncmp(m_RxBuf_Uart1,"qgatsdisc=",9)) 
				{
					s32 conn_id= 0;
					Ql_sscanf(m_RxBuf_Uart1, "%*[^=]=%d",&conn_id);
                    
					ret = RIL_BT_QGatsdisc(conn_id);
					APP_DEBUG("\r\n<--RIL_BT_QGatsdisc: ret=%d,con_id:%d\r\n",ret,conn_id);
						   
					break;
				}
				
                if(0 == Ql_strncmp(m_RxBuf_Uart1,"AT+",3)) 
                {
                ret = Ql_RIL_SendATCmd((char*)m_RxBuf_Uart1, totalBytes, ATResponse_Handler, NULL, 0);
                break;
                }
          
            }
          
            break;
        }
    case EVENT_UART_READY_TO_WRITE:
        break;
    default:
        break;
    }
}

static s32 ATResponse_Handler(char* line, u32 len, void* userData)
{
    Ql_UART_Write(DEBUG_PORT, (u8*)line, len);
    
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


static void BLE_Callback(s32 event, s32 errCode, void* param1, void* param2)
{
    u8 s_index,c_index;
    
      switch(event)
    {
        case MSG_BLE_CONNECT :
        {
            ST_BLE_ConnStatus *conn = (ST_BLE_ConnStatus *)param2;
            if(Ql_StrPrefixMatch((char *)param1, Qserver1.gserv_id))
            {
                Ql_memcpy(&Qserver1.conn_status,conn,sizeof(ST_BLE_ConnStatus));
            }
            APP_DEBUG("sever_id:%s,connect_id:%d,connect_status=%d,Peer_addr:%s\r\n",Qserver1.gserv_id,Qserver1.conn_status.connect_id,Qserver1.conn_status.connect_status,conn->peer_addr);    
		//根据实际应用设置连接参数，数据传输速度和功耗，综合考虑。
		//设置太大，对方获取service UUID 时间会很长。	
		/*	if(conn->connect_status == 1)
			{
				s32 ret;
				ret=RIL_BT_Gatcpu(conn->peer_addr,288,304,600,4);
				APP_DEBUG("\r\n<--RIL_BT_Gatcpu: ret=%d -->\r\n",ret);
			}*/
			break;
        }
        case MSG_BLE_WREG_IND :   
            if(Ql_StrPrefixMatch((char*)param1, Qserver1.gserv_id))
                { 
                    Ql_memcpy(&Qserver1.wrreq_param,(ST_BLE_WRreq*)param2,sizeof(ST_BLE_WRreq));
                    
                    APP_DEBUG("data from clien:%s\r\n",Qserver1.wrreq_param.value);
                    if(Qserver1.wrreq_param.need_rsp==1)
                	{
                		Ql_OS_SendMessage(main_task_id,MSG_GATT_WREG_RESP,&Qserver1.wrreq_param,0); 
                	}
                }
            break;

        case MSG_BLE_RREG_IND:
           if(Ql_StrPrefixMatch((char*)param1, Qserver1.gserv_id))
            {
                Ql_memcpy(&Qserver1.wrreq_param,(ST_BLE_WRreq*)param2,sizeof(ST_BLE_WRreq));
                Ql_OS_SendMessage(main_task_id,MSG_GATT_RREG_RESP,&Qserver1.wrreq_param,0);
            }

            break;
        case MSG_BLE_PXP_CONNECT:
            APP_DEBUG("dev addr:%s\r\n",(char*)param1);
            break;

        case MSG_BLE_FMP_CONNECT:
            APP_DEBUG("dev addr:%s\r\n",(char*)param1);
            break;
         case MSG_BLE_EWREG_IND:
            {
				APP_DEBUG("data from clien end\r\n");
				Ql_OS_SendMessage(main_task_id,MSG_GATT_WREG_RESP,&Qserver1.wrreq_param,0); 
			}
            break;
                  
        default :
            break;
    }

    
}

s32 Ble_adv_set(void)
{
	s32 ret;

//	ret=RIL_BT_QGatSetadv(Qserver1.gserv_id,0,0,manufacture_data,service_data,adv_service_uuid);
	ret=RIL_BT_QGatadvData(Qserver1.gserv_id,ble_adv_set);
	if(ret != 0)
	{
		APP_DEBUG("RIL_BT_QGatSetadv,ret=%d.", ret);
		return ret;
	}
	
	return RIL_BT_QGatScanRsp(Qserver1.gserv_id,ble_rsp_data);
}
static void BLE_COM_Demo(void)
{
    s32 cur_pwrstate = 0 ;
    s32 ret = RIL_AT_SUCCESS ;
	s32 visb_mode = -1;
        u8 s_i=0,c_i=0,d_id=0;

    Ql_memset(&Qserver1,0,sizeof(Qserver1));
    //  1 power on BT device
    ret = RIL_BT_GetPwrState(&cur_pwrstate);
    
    if(RIL_AT_SUCCESS != ret) 
    {
        APP_DEBUG("Get BT device power status failed.\r\n");
        //if run to here, some erros maybe occur, need reset device;
        return;
    }

    if(1 == cur_pwrstate)
    {
        APP_DEBUG("BT device already power on.\r\n");
    }
    else if(0 == cur_pwrstate)
    {
       ret = RIL_BT_Switch(1);
       if(RIL_AT_SUCCESS != ret)
       {
            APP_DEBUG("BT power on failed,ret=%d.\r\n",ret);
            return;
       }
       APP_DEBUG("BT device power on.\r\n");
    }

    RIL_BT_GetPwrState(&cur_pwrstate);
    APP_DEBUG("BT power  cur_pwrstate=%d.\r\n",cur_pwrstate);

	ret = RIL_BT_SetVisble(0,0);   //set BT invisble           
    if(RIL_AT_SUCCESS != ret) 
    {
        APP_DEBUG("visible failed!\r\n");
    }
	
    ret = RIL_BLE_Initialize(BLE_Callback);

    if(RIL_AT_SUCCESS != ret) 
    {
        APP_DEBUG("BT initialization failed.\r\n");
        return;
    }
    APP_DEBUG("BT callback function register successful.\r\n");

//for test register a server, add a service,charackristc
    Qserver1.gserv_id[0]='A';
    Qserver1.gserv_id[1]='B';
    Qserver1.gserv_id[2]='\0';
    Qserver1.sid=0;   // for service index

    Ql_memcpy(Qserver1.service_id[Qserver1.sid].service_uuid,"010102030405060708090A0B0C0D0E0F", 32);
    Qserver1.service_id[Qserver1.sid].num_handles=15;
    Qserver1.service_id[Qserver1.sid].is_primary=1;
    Qserver1.service_id[Qserver1.sid].inst=254;
    Qserver1.service_id[Qserver1.sid].cid=0;
    c_i= Qserver1.service_id[Qserver1.sid].cid;

    c_i=0;
    Ql_memcpy(Qserver1.service_id[Qserver1.sid].char_id[c_i].char_uuid,"010202030405060708090A0B0C0D0E0F", 32);
    Qserver1.service_id[Qserver1.sid].char_id[c_i].inst=1;
    Qserver1.service_id[Qserver1.sid].char_id[c_i].prop=16;
    Qserver1.service_id[Qserver1.sid].char_id[c_i].permission=17;
    Qserver1.service_id[Qserver1.sid].char_id[c_i].did=0;
    d_id =Qserver1.service_id[Qserver1.sid].char_id[c_i].did;
    Ql_memcpy(Qserver1.service_id[Qserver1.sid].char_id[c_i].desc_id[d_id].desc_uuid, "0229", 4);
    Qserver1.service_id[Qserver1.sid].char_id[c_i].desc_id[d_id].inst=1;
    Qserver1.service_id[Qserver1.sid].char_id[c_i].desc_id[d_id].permission=17;

    c_i=1;
    Ql_memcpy(Qserver1.service_id[Qserver1.sid].char_id[c_i].char_uuid,"010302030405060708090A0B0C0D0E0F", 32);
    Qserver1.service_id[Qserver1.sid].char_id[c_i].inst=2;
    Qserver1.service_id[Qserver1.sid].char_id[c_i].prop=6;//read and write without response 
    Qserver1.service_id[Qserver1.sid].char_id[c_i].permission=17;

    Qserver1.service_id[Qserver1.sid].transport=2;

	APP_DEBUG("<--RIL_BT_Gatsreg: gserv_id=%s -->\r\n", Qserver1.gserv_id);

    ret = RIL_BT_Gatsreg(1, &Qserver1);//register server
    if(ret != 0)
    {
    	APP_DEBUG("<--RIL_BT_Gatsreg: ret=%d -->\r\n", ret);
    }

	ret = RIL_BT_Gatss(1, &Qserver1);//add service
    if(ret != 0)
    {
		APP_DEBUG("<--RIL_BT_Gatss: ret=%d -->\r\n", ret);
	}
	Qserver1.service_id[Qserver1.sid].cid=0;
	c_i= Qserver1.service_id[Qserver1.sid].cid;
	ret = RIL_BT_Gatsc(1, &Qserver1);//add character
    if(ret != 0)
    {
		APP_DEBUG("<--RIL_BT_Gatsc: ret=%d -->\r\n", ret);
    }
	Qserver1.service_id[Qserver1.sid].char_id[c_i].did=0;
	ret = RIL_BT_Gatsd(1, &Qserver1);
    if(ret != 0)
    {
		APP_DEBUG("<--RIL_BT_Gatsd: ret=%d -->\r\n", ret);
    }

	Qserver1.service_id[Qserver1.sid].cid=1;
	c_i= Qserver1.service_id[Qserver1.sid].cid;
	ret = RIL_BT_Gatsc(1, &Qserver1);//add character
	if(ret != 0)
	{
		APP_DEBUG("<--RIL_BT_Gatsc: ret=%d -->\r\n", ret);
	}
	ret = RIL_BT_Gatsst(1, &Qserver1);//start service
    if(ret != 0)
    {
		APP_DEBUG("<--RIL_BT_Gatsst: ret=%d -->\r\n", ret);
    }
	//     set_ble_name();
	ret = RIL_BT_Gatsl(1,&Qserver1);
	if(ret != 0)
	{
		APP_DEBUG("\r\n<--RIL_BT_Gatsl: ret=%d -->\r\n",ret);
	}
		
	Ble_adv_set();
	
	return 0;
}

#endif
#endif
