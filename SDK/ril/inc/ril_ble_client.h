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
 *   ril_bluetooth.h 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The API functions defined in this file are used for managing Bluetooth 
 *   devices and services. 
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
#ifndef __RIL_BLE_CLIENT_H__
#define __RIL_BLE_CLIENT_H__


#ifdef __OCPU_RIL_BLE_SUPPORT__

#define   BT_OFF  (0)
#define   BT_ON   (1)
#define   BT_NAME_LEN       56 /* 18 * 3 + 2 */
#define   BT_ADDR_LEN       13

#define   GAT_REMOVE  (0)
#define   GAT_ADD     (1)
#define   GAT_STOP       (0)
#define   GAT_START    (1)


#define SERVICE_NUM 5
#define CHAR_NUM 2
#define DESC_NUM 1
#define VALUE_NUM 512*2+1
#define SCAN_NUM  20


/* Be device address struct */
typedef struct
{
    u32 lap;    /* Lower Address Part 00..23 */
    u8  uap;    /* upper Address Part 24..31 */
    u16 nap;    /* Non-significant    32..47 */
} ST_BLE_Addr;

typedef enum{
    MSG_BLE_WREG_IND,
    MSG_BLE_EWREG_IND,
    MSG_BLE_RREG_IND,
    MSG_BLE_PXP_CONNECT,
    MSG_BLE_FMP_CONNECT,
    MSG_BLE_CONNECT,
    MSG_BLE_DISCONNECT, 
    MSG_BLE_SCAN,
    MSG_BLE_CCON,
    MSG_BLE_CSS,
    MSG_BLE_CGC,
    MSG_BLE_CGD,
    MSG_BLE_CRC,
    MSG_BLE_CRD,
    MSG_BLE_CN,
}Enum_BLEMsg;


typedef struct
{
     s32 connect_status;
     s32 connect_id;
     s32 result;
     char peer_addr[13];
} ST_BLE_ConnStatus;

typedef struct
{
     s32 RSSI;
     char EIR[256];
     char peer_addr[13];
} ST_BLE_Scan;

typedef struct
{
     s32 conn_id;
	 char peer_addr[13];
     u8 service_uuid[33];
	 u8 char_uuid[33];
     s32 is_notify;
     char value[VALUE_NUM];
}ST_BLE_Notify;


typedef struct
{
    u8  is_used;
    s32  inst;
    u8 desc_uuid[33];
}ST_BLE_Clie_Desc;

typedef struct
{
	s32  is_used;
    s32  char_inst;
    u8 char_uuid[33];
    u32 prop;
    u32 did;
    ST_BLE_Clie_Desc  desc_id[DESC_NUM];   
}ST_BLE_Clie_Char;

typedef struct
{
    s32  is_primary;
    s32  service_inst;
    u8  cid;
    u8  is_used;
    u8 service_uuid[33];  
    ST_BLE_Clie_Char  char_id[CHAR_NUM];   
}ST_BLE_Clie_Service;

typedef struct
{
     u8 sid;
     char gclient_id[33];
	 u8  is_used;
     s32 result;
	 s32 conn_id;
     ST_BLE_Scan scan_info;
     ST_BLE_ConnStatus conn_status;
     ST_BLE_Clie_Service  service_id[SERVICE_NUM];
} ST_BLE_Client;

typedef void (*CALLBACK_BLE_IND)(s32 event, s32 errCode, void* param1, void* param2); 

/*****************************************************************
* Function:     RIL_BLE_Initialize
* 
* Description:
*               bluetooth initialization after power on ,register callback
*
* Parameters:
*               cb: callback to be registered
*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/

s32 RIL_BLE_Client_Initialize(CALLBACK_BLE_IND cb);


// 2018/4/19 ADD by waner
s32 RIL_BT_Gatcreg(u8 op , ST_BLE_Client* gclie);
s32 RIL_BT_Gatcscan(u8 op , ST_BLE_Client* gclie,u16 scan_timeout,u16 scan_num);
s32 RIL_BT_Gatccon(u8 op , ST_BLE_Client* gclient,char *peer_address);
s32 RIL_BT_Gatcss(ST_BLE_Client* gclie);
s32 RIL_BT_Gatcgc(ST_BLE_Client* gclie);
s32 RIL_BT_Gatcgd(ST_BLE_Client* gclie);
s32 RIL_BT_Gatcrc(ST_BLE_Client* gclie,u8 auth_req);
s32 RIL_BT_Gatcwc(ST_BLE_Client* gclie,u8 write_type,u8 auth_req,u8* char_value);
s32 RIL_BT_Gatcwd(ST_BLE_Client* gclie,u8 write_type,u8 auth_req,u8 *char_value);
s32 RIL_BT_Gatcrd(ST_BLE_Client* gclie,u8 auth_req);
s32 RIL_BT_Gatcrn(u8 op , ST_BLE_Client* gclie);
s32 RIL_BT_Gatcgdt(ST_BLE_Client* gclie);
s32 RIL_BT_Gatcl(ST_BLE_Client* gclie,u8 start);

#endif
#endif	//__RIL_BLE_H__

