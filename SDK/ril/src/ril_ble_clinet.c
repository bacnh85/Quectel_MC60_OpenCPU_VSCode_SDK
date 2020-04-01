/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Quectel Co., Ltd. 2015
*
*****************************************************************************/
/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ril_bluetooth.c 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The module implements bluetooth related APIs.
 *
 * Author:
 * -------
 * -------
 *  Designed by     :   Stanley YONG
 *  Coded    by     :   Stanley YONG
 *  Tested   by     :   

 *  
 *============================================================================
 *             HISTORY
 *----------------------------------------------------------------------------
 * 
 ****************************************************************************/
#include "custom_feature_def.h"
#ifdef __OCPU_RIL_BLE_SUPPORT__
#include "ql_type.h"
#include "ql_stdlib.h"
#include "ql_trace.h"
#include "ql_error.h"
#include "ql_common.h"
#include "ql_system.h"
#include "ril.h"
#include "ril_util.h"
#include "ril_ble_client.h"

#include "ql_uart.h"

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


static CALLBACK_BLE_IND callback_bt = NULL;

static s32 ATRsp_QBTGatcreg_Hdlr(char* line, u32 len, void* param);
static s32 ATRsp_QBTGatcscan_Hdlr(char* line, u32 len, void* param);
static s32 ATRsp_QBTGatcrn_Hdlr(char* line, u32 len, void* param);
static s32 ATRsp_QBTGatcgdt_Hdlr(char* line, u32 len, void* param);

s32 RIL_BLE_Client_Initialize(CALLBACK_BLE_IND cb)
{
    
    if (!cb)
    {
        return RIL_AT_INVALID_PARAM;
    }
    
    callback_bt = cb;
 
    return RIL_AT_SUCCESS;
}

//AT+QBTGATCREG=<op>,"<gclient_id>"
//AT+QBTGATCREG=1,"B001" 
s32 RIL_BT_Gatcreg(u8 op , ST_BLE_Client* gclie)
{
    char strAT[100];
    s32 ret;

    if (op < GAT_REMOVE || op > GAT_ADD)
    {
        return RIL_AT_INVALID_PARAM;
    }
    if(NULL == gclie)
    {
        return RIL_AT_INVALID_PARAM;
   }
 
    Ql_memset(strAT, 0x0, sizeof(strAT));
    
    Ql_sprintf(strAT, "AT+QBTGATCREG=%d,\"%s\"", op,gclie->gclient_id);

    ret = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QBTGatcreg_Hdlr,(void*)gclie,0);
    
    return  gclie->result;
    
}

//AT+QBTGATCSCAN=1,"B001",5000,30
//+QBTGATCSCAN: 1,0,"B001"
s32 RIL_BT_Gatcscan(u8 op , ST_BLE_Client* gclie,u16 scan_timeout,u16 scan_num)
{
    char strAT[100];
    s32 ret;

    if (op < GAT_REMOVE || op > GAT_ADD || scan_timeout < GAT_REMOVE || scan_num < GAT_REMOVE)
    {
        return RIL_AT_INVALID_PARAM;
    }
    if(NULL == gclie)
    {
        return RIL_AT_INVALID_PARAM;
    }
 
    Ql_memset(strAT, 0x0, sizeof(strAT));

	if((scan_timeout == 0) && (scan_num == 0))
	{
		Ql_sprintf(strAT, "AT+QBTGATCSCAN=%d,\"%s\"", op,gclie->gclient_id);
	}
	else
	{
		Ql_sprintf(strAT, "AT+QBTGATCSCAN=%d,\"%s\",%d,%d", op, gclie->gclient_id,scan_timeout,scan_num);
	}
    ret = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QBTGatcscan_Hdlr,(void*)gclie,0);

    return  gclie->result;
    
}

s32 RIL_BT_Gatccon(u8 op , ST_BLE_Client* gclient,char *peer_address)
{
    char strAT[100];
    s32 ret;

    if (op < GAT_STOP || op > GAT_START)
    {
        return RIL_AT_INVALID_PARAM;
    }
    if(NULL == gclient)
    {
        return RIL_AT_INVALID_PARAM;
    }

    Ql_memset(strAT, 0x0, sizeof(strAT));

	if(op == 1)
	{
		Ql_sprintf(strAT, "AT+QBTGATCCON=%d,\"%s\",%s,1", op,gclient->gclient_id,peer_address);
	}
	else
	{
		Ql_sprintf(strAT, "AT+QBTGATCCON=%d,\"%s\",1", op,gclient->gclient_id);
	}

	APP_DEBUG("\r\nstrAT:%s\r\n",strAT)
	return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL,NULL, 0);
	 
}
#if 0
s32 RIL_BT_Gatccon(u8 op , ST_BLE_Client* gclie)
{
    char strAT[100];
    s32 ret;
    ST_BLE_ConnStatus *conn_status = NULL;

    if (op < GAT_REMOVE || op > GAT_ADD)
    {
        return RIL_AT_INVALID_PARAM;
    }
    if(NULL == gclie)
    {
        return RIL_AT_INVALID_PARAM;
    }

    conn_status = &(gclie->conn_status);
    Ql_memset(strAT, 0x0, sizeof(strAT));
    
    if(op)
    {
        
        Ql_sprintf(strAT, "AT+QBTGATCCON=%d,\"%s\",%s,%d",op,gclie->gclie_id,conn_status->peer_addr,1);

        ret = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL,(void *)gclie, 0);
        
    }
    else
    {
        Ql_sprintf(strAT, "AT+QBTGATCCON=%d,\"%s\",%d",op,gclie->gclie_id,conn_status->connect_id);
        APP_DEBUG("\r\n%s\r\n",strAT);
  
        ret = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL,(void *)gclie, 0);
        
    }
    
    return  gclie->result;
    
}
#endif

s32 RIL_BT_Gatcss(ST_BLE_Client* gclie)
{
    char strAT[100];
    s32 ret;

    if(NULL == gclie)
    {
        return RIL_AT_INVALID_PARAM;
    }
 
    Ql_memset(strAT, 0x0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QBTGATCSS=\"%s\",%d",gclie->gclient_id,gclie->conn_status.connect_id);

    //ret = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL,(void*)gclie,0);
    return  Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT),NULL,NULL,0);   
}


s32 RIL_BT_Gatcgc(ST_BLE_Client* gclie)
{
    char strAT[100];
    s32 ret;
    u8 sid=0,cid; 

	sid=gclie->sid;
    cid =gclie->service_id[sid].cid;
    if (gclie->sid < GAT_REMOVE || gclie->sid > SERVICE_NUM)
    {
        return RIL_AT_INVALID_PARAM;
    }
    if(NULL == gclie)
    {
        return RIL_AT_INVALID_PARAM;
    }
 	Ql_memset(strAT, 0x0, sizeof(strAT));
    if(gclie->service_id[sid].cid == 0)
    {

        Ql_sprintf(strAT, "AT+QBTGATCGC=\"%s\",%d,\"%s\",%d,%d",gclie->gclient_id,gclie->conn_status.connect_id,gclie->service_id[sid].service_uuid,gclie->service_id[sid].service_inst,gclie->service_id[sid].is_primary);
    }
    else
    {
    	cid =cid-1;
        Ql_sprintf(strAT, "AT+QBTGATCGC=\"%s\",%d,\"%s\",%d,%d,\"%s\",%d",gclie->gclient_id,gclie->conn_status.connect_id,gclie->service_id[sid].service_uuid,gclie->service_id[sid].service_inst,gclie->service_id[sid].is_primary,gclie->service_id[sid].char_id[cid].char_uuid,gclie->service_id[gclie->sid].char_id[cid].char_inst);
        APP_DEBUG("\r\n--uuid[%d]:%s\r\n",cid,gclie->service_id[sid].char_id[cid].char_uuid);
    }
    APP_DEBUG("strAT:%s\r\n",strAT);
    
    ret = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL,NULL,0);
    
    return  gclie->result;   
}



//+QBTGATCGD=  [,"desc_uuid>",<desc_inst>]
s32 RIL_BT_Gatcgd(ST_BLE_Client* gclie)
{
    char strAT[160];
    s32 ret;
    s32 sid,cid,did;

    if (gclie->sid < GAT_REMOVE || gclie->sid > SERVICE_NUM)
    {
        return RIL_AT_INVALID_PARAM;
    }
    if(NULL == gclie)
    {
        return RIL_AT_INVALID_PARAM;
    }
 
    Ql_memset(strAT, 0x0, sizeof(strAT));
	sid =gclie->sid;
    cid = gclie->service_id[gclie->sid].cid;
    did = gclie->service_id[gclie->sid].char_id[gclie->service_id[gclie->sid].cid].did;
    //Ql_sprintf(strAT, "AT+QBTGATCGD=\"%s\",%d,\"%s\",%d,%d,\"%s\",%d",gclie->gclient_id,gclie->conn_status.connect_id,gclie->service_id[gclie->sid].service_id,gclie->service_id[gclie->sid].inst,gclie->service_id[gclie->sid].is_primary,gclie->service_id[gclie->sid].char_id[n].char_id,gclie->service_id[gclie->sid].char_id[n].inst);
    APP_DEBUG("\r\n%s\r\n",strAT);
    if (0 == did)
    {

        Ql_sprintf(strAT, "AT+QBTGATCGD=\"%s\",%d,\"%s\",%d,%d,\"%s\",%d",gclie->gclient_id,gclie->conn_status.connect_id,gclie->service_id[sid].service_uuid,gclie->service_id[sid].service_inst,gclie->service_id[sid].is_primary,gclie->service_id[sid].char_id[cid].char_uuid,gclie->service_id[sid].char_id[cid].char_inst);
    }
    else
    {
        Ql_sprintf(strAT, "AT+QBTGATCGD=\"%s\",%d,\"%s\",%d,%d,\"%s\",%d,\"%s\",%d",gclie->gclient_id,gclie->conn_status.connect_id,gclie->service_id[sid].service_uuid,gclie->service_id[sid].service_inst,gclie->service_id[sid].is_primary,gclie->service_id[sid].char_id[cid].char_uuid,gclie->service_id[sid].char_id[cid].char_inst,gclie->service_id[sid].char_id[cid].desc_id[did-1].desc_uuid,gclie->service_id[sid].char_id[cid].desc_id[did-1].inst);
    }
    ret = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL,NULL,0);
    
    return  gclie->result;
    
}

s32 RIL_BT_Gatcrc(ST_BLE_Client* gclie,u8 auth_req)
{
    char strAT[100];
    s32 ret;
    s32 cid;

    if (gclie->sid < GAT_REMOVE || gclie->sid > SERVICE_NUM)
    {
        return RIL_AT_INVALID_PARAM;
    }
    if(NULL == gclie)
    {
        return RIL_AT_INVALID_PARAM;
    }
 
    Ql_memset(strAT, 0x0, sizeof(strAT));
    cid = gclie->service_id[gclie->sid].cid;
    Ql_sprintf(strAT, "AT+QBTGATCRC=\"%s\",%d,\"%s\",%d,%d,\"%s\",%d,%d",gclie->gclient_id,gclie->conn_status.connect_id,gclie->service_id[gclie->sid].service_uuid,gclie->service_id[gclie->sid].service_inst,\
		gclie->service_id[gclie->sid].is_primary,gclie->service_id[gclie->sid].char_id[cid].char_uuid,gclie->service_id[gclie->sid].char_id[cid].char_inst,auth_req);

	 APP_DEBUG("\r\n%s\r\n",strAT);
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL,NULL,0);
    
    
    
}

s32 RIL_BT_Gatcwc(ST_BLE_Client* gclie,u8 write_type,u8 auth_req,u8 *char_value)
{
	ST_BLE_Clie_Char *char_op;
    char strAT[1500];
    s32 ret;
    s32 cid;
    

    if (gclie->sid < GAT_REMOVE || gclie->sid > SERVICE_NUM || NULL == char_value)
    {
        return RIL_AT_INVALID_PARAM;
    }
    if(NULL == gclie)
    {
        return RIL_AT_INVALID_PARAM;
    }
    
    Ql_memset(strAT, 0x0, sizeof(strAT));
    cid = gclie->service_id[gclie->sid].cid;
    char_op =&(gclie->service_id[gclie->sid].char_id[cid]);
	
    Ql_sprintf(strAT, "AT+QBTGATCWC=\"%s\",%d,%d,\"%s\",%d,%d,\"%s\",%d,\"%s\",%d",gclie->gclient_id,gclie->conn_status.connect_id,\
		write_type,gclie->service_id[gclie->sid].service_uuid,gclie->service_id[gclie->sid].service_inst,\
		gclie->service_id[gclie->sid].is_primary,char_op->char_uuid,char_op->char_inst,char_value,auth_req);
	
     APP_DEBUG("\r\n%s\r\n",strAT);
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL,NULL,0);
   
}

s32 RIL_BT_Gatcwd(ST_BLE_Client* gclie,u8 write_type,u8 auth_req,u8 *char_value)
{
    char strAT[200];
    s32 ret;
    s32 cid,did;

    if (gclie->sid < GAT_REMOVE || gclie->sid > SERVICE_NUM)
    {
        return RIL_AT_INVALID_PARAM;
    }
    if(NULL == gclie)
    {
        return RIL_AT_INVALID_PARAM;
    }
 
    Ql_memset(strAT, 0x0, sizeof(strAT));
    cid = gclie->service_id[gclie->sid].cid;
    did = gclie->service_id[gclie->sid].char_id[cid].did;

	
	
    Ql_sprintf(strAT, "AT+QBTGATCWD=\"%s\",%d,%d,\"%s\",%d,%d,\"%s\",%d,\"%s\",%d,\"%s\",%d",gclie->gclient_id,gclie->conn_status.connect_id,\
		write_type,gclie->service_id[gclie->sid].service_uuid,gclie->service_id[gclie->sid].service_inst,gclie->service_id[gclie->sid].is_primary,\
		gclie->service_id[gclie->sid].char_id[cid].char_uuid,gclie->service_id[gclie->sid].char_id[cid].char_inst,\
		gclie->service_id[gclie->sid].char_id[cid].desc_id[did].desc_uuid,gclie->service_id[gclie->sid].char_id[cid].desc_id[did].inst,\
		char_value,auth_req);

	 APP_DEBUG("\r\n%s\r\n",strAT);
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL,NULL,0);
}

s32 RIL_BT_Gatcrd(ST_BLE_Client* gclie,u8 auth_req)
{
    char strAT[160];
    s32 ret;
    s32 n,m;

    if (gclie->sid < GAT_REMOVE || gclie->sid > SERVICE_NUM)
    {
        return RIL_AT_INVALID_PARAM;
    }
    if(NULL == gclie)
    {
        return RIL_AT_INVALID_PARAM;
    }
 
    Ql_memset(strAT, 0x0, sizeof(strAT));
    n = gclie->service_id[gclie->sid].cid;
    m = gclie->service_id[gclie->sid].char_id[gclie->service_id[gclie->sid].cid].did;
	
    Ql_sprintf(strAT, "AT+QBTGATCRD=\"%s\",%d,\"%s\",%d,%d,\"%s\",%d,\"%s\",%d,%d",gclie->gclient_id,gclie->conn_status.connect_id,\
		gclie->service_id[gclie->sid].service_uuid,gclie->service_id[gclie->sid].service_inst,gclie->service_id[gclie->sid].is_primary,\
		gclie->service_id[gclie->sid].char_id[n].char_uuid,gclie->service_id[gclie->sid].char_id[n].char_inst,\
		gclie->service_id[gclie->sid].char_id[n].desc_id[m].desc_uuid,gclie->service_id[gclie->sid].char_id[n].desc_id[m].inst,auth_req);

	 APP_DEBUG("\r\n%s\r\n",strAT);
	return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL,NULL,0);
       
}
s32 RIL_BT_Gatcrn(u8 op , ST_BLE_Client* gclie)
{
    char strAT[160];
    s32 ret,n;

    if(NULL == gclie)
    {
        return RIL_AT_INVALID_PARAM;
    }

    Ql_memset(strAT, 0x0, sizeof(strAT));
    n = gclie->service_id[gclie->sid].cid;

	Ql_sprintf(strAT, "AT+QBTGATCRN=%d,\"%s\",%s,\"%s\",%d,%d,\"%s\",%d",op,gclie->gclient_id,gclie->conn_status.peer_addr,\
		gclie->service_id[gclie->sid].service_uuid,gclie->service_id[gclie->sid].service_inst,gclie->service_id[gclie->sid].is_primary,\
		gclie->service_id[gclie->sid].char_id[n].char_uuid,gclie->service_id[gclie->sid].char_id[n].char_inst);

	APP_DEBUG("\r\n%s\r\n",strAT);
    Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QBTGatcrn_Hdlr,(void*)gclie,0);
    
    return  gclie->result;
    
}
//AT+QBTGATCGDT="ABCD",2711C92E38C6
s32 RIL_BT_Gatcgdt(ST_BLE_Client* gclie)
{
    char strAT[100];
    s32 ret,n;

    if(NULL == gclie)
    {
        return RIL_AT_INVALID_PARAM;
    }

    Ql_memset(strAT, 0x0, sizeof(strAT));

	Ql_sprintf(strAT, "AT+QBTGATCGDT=\"%s\",%s",gclie->gclient_id,gclie->conn_status.peer_addr);
    ret = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QBTGatcgdt_Hdlr,(void*)gclie,0);
    
    return  gclie->result;
    
}

s32 RIL_BT_Gatcl(ST_BLE_Client* gclie,u8 start)
{
    char strAT[100];
    s32 ret;

    if(NULL == gclie)
    {
        return RIL_AT_INVALID_PARAM;
    }
    Ql_memset(strAT, 0x0, sizeof(strAT));
    
    Ql_sprintf(strAT, "AT+QBTGATCL=\"%s\",%d",gclie->gclient_id,start);

    //APP_DEBUG("\r\n%s\r\n",strAT);
    ret = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL,NULL,0);
    
    return  gclie->result;
    
}

//+QBTGATCSCAN:"<gclient_id>",<peer_addr>,<RSSI>,<EIR>
//+QBTGATCSCAN: "ABCD",76DCAB964FEF,40,02011A0B095155454354454C2D4254
void OnURCHandler_BTGatcscan(const char* strURC, void* reserved)
{
    
    s32 err_code = 0;
    char urcHead[100];
    char clie_id[32]={0};
    ST_BLE_Scan scan_info = {0};
	
	//APP_DEBUG("strURC:%s\r\n",strURC);

    Ql_strcpy(urcHead, "\r\n+QBTGATCSCAN:");
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {
        Ql_sscanf(strURC, "%*[^\"]\"%[^\"]%*[^\r\n]\r\n",clie_id );

        Ql_sscanf(strURC, "%*[^:]: %*[^,],%[^,]%*[^\r\n]\r\n",scan_info.peer_addr );
        Ql_sscanf(strURC, "%*[^,],%*[^,],%d,%*[^\r\n]\r\n",&(scan_info.RSSI));
        Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%[^,]\r\n",scan_info.EIR);

//		APP_DEBUG("clie_id:%s\r\n",clie_id);
		
        if((callback_bt != NULL)&&(scan_info.EIR[0]!=0))
        {     
            callback_bt(MSG_BLE_SCAN,0,clie_id, &scan_info);
        }
        //else
        //{
            //APP_DEBUG("client_id:%s\r\n",clie_id);
        //}
      
    }
    
}

//+QBTGATCCON: 1,0,¡°ABCD¡±,2711C92E38C6,1
void OnURCHandler_BTGatccon(const char* strURC, void* reserved)
{
    s32 err_code = 0;
    char urcHead[40];
    char clie_id[32]={0};
    ST_BLE_ConnStatus conn_status = {0};

    APP_DEBUG("strURC:%s\r\n",strURC);
    Ql_strcpy(urcHead, "\r\n+QBTGATCCON:");
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {
        Ql_sscanf(strURC, "%*[^:]: %d,%*[^\r\n]\r\n",&(conn_status.connect_status));
        Ql_sscanf(strURC, "%*[^,],%d,%*[^\r\n]\r\n", &(conn_status.result));
        Ql_sscanf(strURC, "%*[^\"]\"%[^\"]%*[^\r\n]\r\n",clie_id);

        Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%[^,],%*[^\r\n]\r\n", conn_status.peer_addr );
        
        Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%*[^,],%d\r\n", &(conn_status.connect_id) );
        
        if(callback_bt != NULL)
        {            
            callback_bt(MSG_BLE_CCON,0,clie_id, &conn_status);
        }
      
    }
    return ;
}

//+QBTGATCSS: <result>,"<gclient_id>",<conn_id>,"<service_uuid>",<service_inst>,<is_primary>
void OnURCHandler_BTGatcss(const char* strURC, void* reserved)
{
#if 1
    s32 err_code = 0;
    char urcHead[40];
    char clie_id[32]={0};
	ST_BLE_Clie_Service css;

	APP_DEBUG("strURC:%s\r\n",strURC);

    Ql_strcpy(urcHead, "\r\n+QBTGATCSS:");
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {
        
        //APP_DEBUG("%s\r\n",strURC);
        //Ql_sscanf(strURC, "%*[^:]: %d,%*[^\r\n]\r\n",&(client.result));
        Ql_sscanf(strURC, "%*[^\"]\"%[^\"]%*[^\r\n]\r\n",&clie_id );
        //Ql_sscanf(strURC, "%*[^,],%*[^,],%d,%*[^\r\n]\r\n", &(client.conn_status.connect_id));
        Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%*[^,],%d,%*[^\r\n]\r\n", &(css.service_inst));
        Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%d\r\n", &(css.is_primary));
        Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,\"],\"%[^\"],%*[^\r\n]\r\n", css.service_uuid);
        //APP_DEBUG("clie_id = %s,service_uuid = %s,service_inst = %d,is_primary = %d\r\n",clie_id,css.service_uuid,css.inst,css.is_primary);
        //APP_DEBUG("inst = %d,is_primary = %d\r\n",css.inst,css.is_primary);
        
        if(callback_bt != NULL)
        {
            
            callback_bt(MSG_BLE_CSS,0,clie_id,&css );
        }
        #if 0
        Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,\"],\"%[^\"],%*[^\r\n]\r\n", service_uuid);
        Ql_sscanf(strURC, "%*[^:]: %d,%*[^\r\n]\r\n",&result);
        Ql_sscanf(strURC, "%*[^\"]\"%[^\"]%*[^\r\n]\r\n",client_uuid);
        Ql_sscanf(strURC, "%*[^,],%*[^,],%d,%*[^\r\n]\r\n", &conn_id);
        Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%*[^,],%d,%*[^\r\n]\r\n", &inst);
        Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%d\r\n", &is_primary);
        APP_DEBUG("result = %d,client_id = %s,conn_id = %d,service_uuid = %s,service_inst = %d,is_primary = %d\r\n",result,client_uuid,conn_id,service_uuid,service_inst,is_primary);
        #endif
        
    }
	#endif
}

//+QBTGATCGC: <result>,"<gclient_id>",<conn_id>,"<service_uuid>",<service_inst>,<service_is_primary>,"<char_uuid>",<char_inst>,<prop>]
void OnURCHandler_BTGatcgc(const char* strURC, void* reserved)
{
#if 1
    s32 err_code = 0;
    u8  urcHead[40];
    u8  clie_id[33]={0};
    u8  service_uuid[33]={0};
    ST_BLE_Clie_Char cgc = {0};

	APP_DEBUG("strURC:%s\r\n",strURC);
    Ql_strcpy(urcHead, "\r\n+QBTGATCGC:");
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {
        //APP_DEBUG("%s\r\n",strURC); 
        Ql_sscanf(strURC, "%*[^\"]\"%[^\"]%*[^\r\n]\r\n",clie_id);
        Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],\"%[^\"],%*[^\r\n]\r\n",service_uuid);
        Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,\"],\"%[^\"],%*[^\r\n]\r\n",cgc.char_uuid);
        Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%d,%*[^\r\n]\r\n",&(cgc.char_inst));
        Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%d\r\n",&(cgc.prop));
        //APP_DEBUG("char_id:%s,char_inst:%d,prop:%d\r\n",char_id,char_inst,prop); 
        APP_DEBUG("client_id:%s,service_uuid:%s,char_id:%s,char_inst:%d,prop:%d\r\n",clie_id,service_uuid,cgc.char_uuid,cgc.char_inst,cgc.prop);

        if((callback_bt != NULL)&&(strlen(cgc.char_uuid)!=0))
        {
            callback_bt(MSG_BLE_CGC,0,service_uuid,&cgc);
        }
    }
	#endif
}

//+QBTGATCGD: <result>,"<gclient_id>",<conn_id>,"<service_uuid>",<service_inst>,<service_is_primary>,"<char_uuid>",<char_inst>,"<desc_uuid>",<desc_inst>]
void OnURCHandler_BTGatcgd(const char* strURC, void* reserved)
{
    s32 err_code = 0;
    u8  urcHead[40];
    u8  clie_id[32]={0};
	u8  service_uuid[33]={0};
    ST_BLE_Clie_Char cgd = {0};
    
    Ql_strcpy(urcHead, "\r\n+QBTGATCGD:");
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {
        APP_DEBUG("%s\r\n",strURC); 
        Ql_sscanf(strURC, "%*[^\"]\"%[^\"]%*[^\r\n]\r\n",clie_id);
		Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],\"%[^\"],%*[^\r\n]\r\n",service_uuid);
		Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,\"],\"%[^\"],%*[^\r\n]\r\n",cgd.char_uuid);
        Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%d,%*[^\r\n]\r\n",&(cgd.char_inst));
		Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%d\r\n",&(cgd.prop));
        Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,\"],\"%[^\"],%*[^\r\n]\r\n",cgd.desc_id[0].desc_uuid);
        Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%d,%*[^\r\n]\r\n",&(cgd.desc_id[0].inst));
        APP_DEBUG("client_id:%s,char_uuid:%s,desc_uuid:%s,desc_inst:%d\r\n",clie_id,cgd.char_uuid,cgd.desc_id[0].desc_uuid,cgd.desc_id[0].inst);

        if(callback_bt != NULL)
        {
            callback_bt(MSG_BLE_CGD,0,service_uuid,&cgd);
        }
    }
}

//+QBTGATCRC: <result>,"<gclient_id>",<conn_id>,"<service_uuid>",<service_inst>,<service_is_primary>,"<char_uuid>",<char_inst>,"<value>"
void OnURCHandler_BTGatcrc(const char* strURC, void* reserved)
{
    s32 err_code = 0;
    u8  urcHead[40];
    u8  clie_id[32]={0};
    u8  char_value[1024]={0};
	u8  char_uuid[32]={0};
	
    Ql_strcpy(urcHead, "\r\n+QBTGATCRC:");
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {
        //APP_DEBUG("%s\r\n",strURC);
        Ql_sscanf(strURC, "%*[^\"]\"%[^\"]%*[^\r\n]\r\n",clie_id);
        Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,\"],\"%[^\"],%*[^\r\n]\r\n",char_uuid);
        Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,\"],\"%[^\"],%*[^\r\n]\r\n",char_value);
        //APP_DEBUG("client_id:%s,char_value:%s\r\n",clie_id,char_value);
        
        if(callback_bt != NULL)
        {
            callback_bt(MSG_BLE_CRC,0,char_uuid,char_value);
        }
    }
}

//+QBTGATCRD: <result>,"<gclient_id>",<conn_id>,"<service_uuid>",<service_inst>,<service_is_primary>,"<char_uuid>",<char_inst>,"<desc_uuid>",<desc_inst>,"<value>"
void OnURCHandler_BTGatcrd(const char* strURC, void* reserved)
{
    s32 err_code = 0;
    u8  urcHead[40];
    u8  clie_id[32]={0};
	u8  desc_uuid[32]={0};
    u8  desc_value[1024]={0};
    
    Ql_strcpy(urcHead, "\r\n+QBTGATCRD:");
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {
        
        Ql_sscanf(strURC, "%*[^\"]\"%[^\"]%*[^\r\n]\r\n",clie_id);
		Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,\"],%*[^,],%*[^,\"],\"%[^\"],%*[^\r\n]\r\n",desc_uuid);
        Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,\"],\"%[^\"],%*[^\r\n]\r\n",desc_value);
        //APP_DEBUG("client_id:%s,char_value:%s\r\n",clie_id,desc_value);
        
        if(callback_bt != NULL)
        {
            callback_bt(MSG_BLE_CRD,0,desc_uuid,desc_value);
        }
        
    }
}

//+QBTGATCN: "<gclient_id>",<conn_id>,<peer_addr>,<service_uuid>,<service_inst>,<service_is_primary>,"<char_uuid>",<char_inst>,<is_notify>,<value>
void OnURCHandler_BTGatcn(const char* strURC, void* reserved)
{
    s32 err_code = 0;
    u8  urcHead[40];
    u8  clie_id[32]={0};
    ST_BLE_Notify ble_notify;

    APP_DEBUG("%s\r\n",strURC); 
    Ql_strcpy(urcHead, "\r\n+QBTGATCN:");
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {
        
        Ql_sscanf(strURC, "%*[^\"]\"%[^\"]%*[^\r\n]\r\n",clie_id);
		Ql_sscanf(strURC, "%*[^,],%d,%*[^\r\n]\r\n",&ble_notify.conn_id);
		Ql_sscanf(strURC, "%*[^,],%*[^,],%[^,],%*[^\r\n]\r\n", ble_notify.peer_addr );	
		Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],\"%[^\"],%*[^\r\n]\r\n",ble_notify.service_uuid);	
		Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],\"%[^\"],%*[^\r\n]\r\n",ble_notify.char_uuid);	
        Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],\"%[^\"],%*[^\r\n]\r\n",ble_notify.value);
        Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%d,%*[^\r\n]\r\n",&ble_notify.is_notify);

        if(callback_bt != NULL)
        {
            callback_bt(MSG_BLE_CN,0,clie_id,&ble_notify);
        }
    }
}

//+QBTGATCREG:1,0,"B001"
//+QBTGATCREG: <op>,<result>,<gserv_id>
static s32 ATRsp_QBTGatcreg_Hdlr(char* line, u32 len, void* param)
{
    ST_BLE_Client* gclie_op =( ST_BLE_Client*)param;

    char* pHead = Ql_RIL_FindString(line, len, "\r\n+QBTGATCREG:");
 
    if (pHead)
    {
        Ql_sscanf(line, "%*[^,],%d,%*[^,]\r\n",&(gclie_op->result));//result

        return  RIL_ATRSP_CONTINUE;
    }

    pHead = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
    if (pHead)
    {  
        return RIL_ATRSP_SUCCESS;
    }

   gclie_op->result = RIL_ATRSP_FAILED;

    pHead = Ql_RIL_FindLine(line, len, "ERROR");// find <CR><LF>ERROR<CR><LF>, <CR>ERROR<CR>£¬<LF>ERROR<LF>
    if (pHead)
    {  
        return  RIL_ATRSP_FAILED;
    } 
    pHead = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if (pHead)
    {
        return  RIL_ATRSP_FAILED;
    }

    return RIL_ATRSP_FAILED; //not supported
}

static s32 ATRsp_QBTGatcscan_Hdlr(char* line, u32 len, void* param)
{
    ST_BLE_Client* gclie_op =( ST_BLE_Client*)param;

    char* pHead = Ql_RIL_FindString(line, len, "\r\n+QBTGATCSCAN:");

   // APP_DEBUG("line:%s\r\n",line);
    if (pHead)
    {
        Ql_sscanf(line, "%*[^,],%d,%*[^,]\r\n",&(gclie_op->result));//result

        return  RIL_ATRSP_CONTINUE;
    }

    pHead = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
    if (pHead)
    {  
        return RIL_ATRSP_SUCCESS;
    }

    gclie_op->result = RIL_ATRSP_FAILED;

    pHead = Ql_RIL_FindLine(line, len, "ERROR");// find <CR><LF>ERROR<CR><LF>, <CR>ERROR<CR>£¬<LF>ERROR<LF>
    if (pHead)
    {  
        return  RIL_ATRSP_FAILED;
    } 
    pHead = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if (pHead)
    {
        return  RIL_ATRSP_FAILED;
    }

    return RIL_ATRSP_FAILED; //not supported
}

//+QBTGATCRN: <op>,<result>,"<gclient_id>",<peer_addr>,<service_uuid>,<service_inst>,<service_is_primary>,"<char_uuid>",<char_inst>
//+QBTGATCRN: 1,0,"ABCD",2711C92E38C6,"3418",0,1,"332A",0
static s32 ATRsp_QBTGatcrn_Hdlr(char* line, u32 len, void* param)
{
    ST_BLE_Client* gclient_op =( ST_BLE_Client*)param;
	char client_id[32];
	u8	 conn_id;
	u8 service_uuid[32]; 
	u8  service_inst;
	u8  is_primary;
    u8  char_inst;
    u8 char_uuid[32];	
	u32 char_prop;
	u8 result = 0;
	bool used = FALSE;
    char* pHead = Ql_RIL_FindString(line, len, "\r\n+QBTGATCRN:");

	if (pHead)
	{
		
		Ql_sscanf(line, "%*[^,],%d,%*[^,]\r\n",&(gclient_op->result));//result

        return  RIL_ATRSP_CONTINUE;
	}
	
	pHead = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
	if (pHead)
	{  
		return RIL_ATRSP_SUCCESS;
	}
	
	pHead = Ql_RIL_FindLine(line, len, "ERROR");// find <CR><LF>ERROR<CR><LF>, <CR>ERROR<CR>£¬<LF>ERROR<LF>
	if (pHead)
	{  
		return	RIL_ATRSP_FAILED;
	} 
	pHead = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
	if (pHead)
	{
		return	RIL_ATRSP_FAILED;
	}

}

static s32 ATRsp_QBTGatcgdt_Hdlr(char* line, u32 len, void* param)
{
    ST_BLE_Client* gclie_op =( ST_BLE_Client*)param;

    char* pHead = Ql_RIL_FindString(line, len, "\r\n+QBTGATCGDT:");
 /*
    if (pHead)
    {
        Ql_sscanf(line, "%*[^,],%*[^,],%*[^,],%d\r\n",&(gclie_op->dev_type));//result

        return  RIL_ATRSP_CONTINUE;
    }
*/
    pHead = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
    if (pHead)
    {  
        return RIL_ATRSP_SUCCESS;
    }

   gclie_op->result = RIL_ATRSP_FAILED;

    pHead = Ql_RIL_FindLine(line, len, "ERROR");// find <CR><LF>ERROR<CR><LF>, <CR>ERROR<CR>£¬<LF>ERROR<LF>
    if (pHead)
    {  
        return  RIL_ATRSP_FAILED;
    } 
    pHead = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if (pHead)
    {
        return  RIL_ATRSP_FAILED;
    }

    return RIL_ATRSP_FAILED; //not supported
}
#endif  

