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
#ifndef __RIL_BLE_H__
#define __RIL_BLE_H__


#ifdef __OCPU_RIL_BLE_SUPPORT__

#define   BT_OFF  (0)
#define   BT_ON   (1)
#define   BT_NAME_LEN       56 /* 18 * 3 + 2 */
#define   BT_ADDR_LEN       13

#define   GAT_REMOVE  (0)
#define   GAT_ADD        (1)
#define   GAT_STOP       (0)
#define   GAT_START    (1)

#define SERVICE_NUM 3
#define CHAR_NUM 2
#define DESC_NUM 1
#define VALUE_NUM 512*2+1

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
     s32 trans_id;
     s32 need_rsp; 
     s32 attr_handle;
	 s32 cancel;
     char value[VALUE_NUM];
} ST_BLE_WRreq;

typedef struct
{
     s32 trans_id;
     s32 need_cnf;
     s32 attr_handle;
     char value[VALUE_NUM];
}ST_BLE_Sind;

typedef struct
{
    u8  inst;
    u8  is_used;
    u8 desc_uuid[33];
    s32 desc_handle;
    u32 permission;   
}ST_BLE_Desc;

typedef struct
{
    u8  inst;
    u8  is_used;
    u8 char_uuid[33];
    s32 char_handle;
    u32 permission;
    u32 prop;
    u32 did;
    s32 trans_id;
    ST_BLE_Desc  desc_id[DESC_NUM];   
}ST_BLE_Char;

typedef struct
{
    u8  num_handles;
    u8  is_primary;
    u8  inst;
    u8  transport;
    u8  cid;
    u8  is_used;
    u8  is_started;
    u8 service_uuid[33];  
    s32 service_handle;
    s32 in_service;
    ST_BLE_Char  char_id[CHAR_NUM];   
}ST_BLE_Service;

typedef struct
{
     u8 sid;
     char gserv_id[32];
     s32 result;
     ST_BLE_WRreq  wrreq_param;
     ST_BLE_Sind  sind_param;
     ST_BLE_ConnStatus conn_status;
     ST_BLE_Service  service_id[SERVICE_NUM];
} ST_BLE_Server;

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

s32 RIL_BLE_Initialize(CALLBACK_BLE_IND cb);

/*****************************************************************
* Function:     RIL_BT_Gatsreg
* 
* Description:
*               Register a Gatt Server
*
* Parameters:
*               op:[In] Register or deregister a GATT server. 0 means deregister and 1 means register.
                 gserv :[In] Pointer to the ¡°ST_BLE_Server¡± struct

*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
s32 RIL_BT_Gatsreg(u8 op , ST_BLE_Server* gserv);

/*****************************************************************
* Function:     RIL_BT_Gatss
* 
* Description:
*               Add or remove a service 
*
* Parameters:
*                op :[In] Add or remove a service. 0 means remove and 1 means add.
                  gserv :[In] Pointer to the ¡°ST_BLE_Serve¡± struct.

* Return:        
*               RIL_AT_SUCCESS: indicates the AT command is executed successfully, and the response is OK.
*               RIL_AT_FAILED: indicates failed to execute the AT command or the response is ERROR.
*               RIL_AT_TIMEOUT: indicates sending AT command timed out.
*               RIL_AT_BUSY: indicates the AT command is being sent.
*               RIL_AT_INVALID_PARAM: indicates invalid input parameter.
*               RIL_AT_UNINITIALIZED: indicates RIL is not ready, and module needs to wait for MSG_ID_RIL_READY
*               and then call Ql_RIL_Initialize() to initialize RIL
*****************************************************************/
s32 RIL_BT_Gatss(u8 op , ST_BLE_Server* gserv);

/*****************************************************************
* Function:     RIL_BT_Gatsc
* 
* Description:
*               Add characteristic to a service 
*
* Parameters:
*               op :[In] Adding one characteristic at a time is supported and deleting characteristic is not supported currently.
                 gserv :[in]  Pointer to the ¡°ST_BLE_Server¡± struct.

*
* Return:        
*               RIL_AT_SUCCESS: indicates the AT command is executed successfully, and the response is OK.
*               RIL_AT_FAILED: indicates failed to execute the AT command or the response is ERROR.
*               RIL_AT_TIMEOUT: indicates sending AT command timed out.
*               RIL_AT_BUSY: indicates the AT command is being sent.
*               RIL_AT_INVALID_PARAM: indicates invalid input parameter.
*               RIL_AT_UNINITIALIZED: indicates RIL is not ready, and module needs to wait for MSG_ID_RIL_READY
*               and then call Ql_RIL_Initialize() to initialize RIL

*****************************************************************/
s32 RIL_BT_Gatsc(u8 op , ST_BLE_Server* gserv);

/*****************************************************************
* Function:     RIL_BT_Gatsd
* 
* Description:
*               Add descriptor to char 
*
* Parameters:
*               op :[In]Adding one descriptor at a time is supported and deleting descriptor is not supported currently
                gserv :[in]  Pointer to the ¡°ST_BLE_Server¡± struct.

*
* Return:        
*               RIL_AT_SUCCESS: indicates the AT command is executed successfully, and the response is OK.
*               RIL_AT_FAILED: indicates failed to execute the AT command or the response is ERROR.
*               RIL_AT_TIMEOUT: indicates sending AT command timed out.
*               RIL_AT_BUSY: indicates the AT command is being sent.
*               RIL_AT_INVALID_PARAM: indicates invalid input parameter.
*               RIL_AT_UNINITIALIZED: indicates RIL is not ready, and module needs to wait for MSG_ID_RIL_READY
*               and then call Ql_RIL_Initialize() to initialize RIL

*****************************************************************/
s32 RIL_BT_Gatsd(u8 op , ST_BLE_Server* gserv);

/*****************************************************************
* Function:     RIL_BT_Gatsst
* 
* Description:
*               Start / Stop a service
*
* Parameters:
*               op :[in]   Start or stop a service. 0 means stop and 1 means start
                gserv :[in]  Pointer to the ¡°ST_BLE_Server¡± struct.

*
* Return:        
*               RIL_AT_SUCCESS: indicates the AT command is executed successfully, and the response is OK.
*               RIL_AT_FAILED: indicates failed to execute the AT command or the response is ERROR.
*               RIL_AT_TIMEOUT: indicates sending AT command timed out.
*               RIL_AT_BUSY: indicates the AT command is being sent.
*               RIL_AT_INVALID_PARAM: indicates invalid input parameter.
*               RIL_AT_UNINITIALIZED: indicates RIL is not ready, and module needs to wait for MSG_ID_RIL_READY
*               and then call Ql_RIL_Initialize() to initialize RIL

*****************************************************************/
s32 RIL_BT_Gatsst(u8 op , ST_BLE_Server* gserv);

/*****************************************************************
* Function:     RIL_BT_Gatsind
* 
* Description:
*               Send an indication to client
*
* Parameters:
                gserv :[in] Pointer to the ¡°ST_BLE_Server¡± struct.

*
* Return:        
*               RIL_AT_SUCCESS: indicates the AT command is executed successfully, and the response is OK.
*               RIL_AT_FAILED: indicates failed to execute the AT command or the response is ERROR.
*               RIL_AT_TIMEOUT: indicates sending AT command timed out.
*               RIL_AT_BUSY: indicates the AT command is being sent.
*               RIL_AT_INVALID_PARAM: indicates invalid input parameter.
*               RIL_AT_UNINITIALIZED: indicates RIL is not ready, and module needs to wait for MSG_ID_RIL_READY
*               and then call Ql_RIL_Initialize() to initialize RIL

*****************************************************************/
s32 RIL_BT_Gatsind(ST_BLE_Server* gserv);

/*****************************************************************
* Function:     RIL_BT_Gatsrsp
* 
* Description:
*               This function responses to the read or write request from client
*
* Parameters:
                gserv :[in]  Pointer to the ¡°ST_BLE_Server¡± struct.

*
* Return:        
*               RIL_AT_SUCCESS: indicates the AT command is executed successfully, and the response is OK.
*               RIL_AT_FAILED: indicates failed to execute the AT command or the response is ERROR.
*               RIL_AT_TIMEOUT: indicates sending AT command timed out.
*               RIL_AT_BUSY: indicates the AT command is being sent.
*               RIL_AT_INVALID_PARAM: indicates invalid input parameter.
*               RIL_AT_UNINITIALIZED: indicates RIL is not ready, and module needs to wait for MSG_ID_RIL_READY
*               and then call Ql_RIL_Initialize() to initialize RIL
*****************************************************************/
s32 RIL_BT_Gatsrsp(ST_BLE_Server* gserv);

/*****************************************************************
* Function:     RIL_BT_Gatsl
* 
* Description:
*               Start/stop gatt server listening 
*
* Parameters:
                op  :[in]  Start or stop advertising. 0 means stop and 1 means start.
                gserv :[in] Pointer to the ¡°ST_BLE_Server¡± struct.

*
* Return:        
*               RIL_AT_SUCCESS: indicates the AT command is executed successfully, and the response is OK.
*               RIL_AT_FAILED: indicates failed to execute the AT command or the response is ERROR.
*               RIL_AT_TIMEOUT: indicates sending AT command timed out.
*               RIL_AT_BUSY: indicates the AT command is being sent.
*               RIL_AT_INVALID_PARAM: indicates invalid input parameter.
*               RIL_AT_UNINITIALIZED: indicates RIL is not ready, and module needs to wait for MSG_ID_RIL_READY
*               and then call Ql_RIL_Initialize() to initialize RIL
*****************************************************************/
s32 RIL_BT_Gatsl(u8 op , ST_BLE_Server* gserv);
/*****************************************************************
* Function:     RIL_BT_SetLeTxPwr
* 
* Description:
*               Set the Level of BLE Transmission Power
*
* Parameters:
               tx_level : The level of BLE transmission power. The range is 0-7.

*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED: indicates failed to execute the AT command or the response is ERROR.

*****************************************************************/
s32 RIL_BT_SetLeTxPwr(u8 tx_level);

/*****************************************************************
* Function:     RIL_BT_QBTFMPsreg
* 
* Description:
*               Start/stop FMP profile
*
* Parameters:
                op  :[in]  start or stop

*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
s32 RIL_BT_QBTFMPsreg(u8 op) ;

/*****************************************************************
* Function:     RIL_BT_QBTPXPsreg
* 
* Description:
*               Start/stop PXP profile
*
* Parameters:
                op  :[in]  start or stop

*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
s32 RIL_BT_QBTPXPsreg(u8 op);

/*****************************************************************
* Function:     RIL_BT_QBTGatadv
* 
* Description:
*                Set advertising interval gap for gatt client
* Parameters:
                  min_interval:
*                [In] Minimum advertising interval for undirected and low duty cycle directed advertising. The range is
*                32~16384.
*                max_interval:
*                [In] Maximum advertising interval for undirected and low duty cycle directed advertising. The range is
*                32~16384.

*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
s32 RIL_BT_QBTGatadv(u16 min_interval,u16 max_interval);

/*****************************************************************
* Function:     RIL_BT_QGatSetadv
* 
* Description:
*               This function adds  advertising data
* Parameters:
*               gserv_id : User ID  of GATT server 
*               appearance: The external appearance of the device
*                manufacture_data: It is manufacture information.
*               service_data: It is service information.
*               service_uuid: UUID of this service.
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
s32 RIL_BT_QGatSetadv(char* gserv_id,u16 appearance,u16 string_mode,u8* manufacture_data,u8* service_data,u8* service_uuid);


/*****************************************************************
* Function:     RIL_BT_Gatcpu
*
* Parameters:
*               bt_addr:
*               [In] Address of the peer device.
*               min_interval:
*               [In] Minimum value of the connection interval. The range is 6~3200.

*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
s32 RIL_BT_Gatcpu(char* bt_addr,u16 min_interval,u16 max_interval,u16 timeout,u16 latency);

/*****************************************************************
* Function:     RIL_BT_GetLELocalAddr
* 
* Description:
*               get the BLE device address of bluetooth
*
* Parameters:
*               ptrAddr    [out] :  bluetooth addr to get ,length is fixed 13 bytes including '\0'
                 len        [in]  :  sizeof ptrAddr
*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
s32 RIL_BT_GetLeLocalAddr(char* ptrAddr,u8 len);

/*****************************************************************
* Function:     RIL_BT_QGatsdisc
* 
* Description:
*               Disconnect the current connection.
* Parameters:
                conn_id:[in] ID of current connection
*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
s32 RIL_BT_QGatsdisc(u8 conn_id);


s32 RIL_BT_QGatadvData(char* gserv_id,u8* adv_data);

s32 RIL_BT_QGatScanRsp(char* gserv_id,u8* rsp_data);

#endif
#endif	//__RIL_BLE_H__

