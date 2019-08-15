#ifndef __GAGENT_H__
#define __GAGENT_H__


#include "gagent_typedef.h"

#define RET_SUCCESS (0)
#define RET_FAILED  (-1)

#define GAGENT_RELEASE 1



#define BUF_LEN        (1024*4)      /* depend on you platform ram size */
#define BUF_HEADLEN    (128)
#define GAGENT_BUF_LEN  1024


#define CLOUD_INIT      (1)
#define CLOUD_CONFIG_OK            (16)
#define CLOUD_REQ_GET_DID          2
#define CLOUD_RES_GET_DID          3
#define CLOUD_REQ_PROVISION        4
#define CLOUD_RES_PROVISION        5
#define CLOUD_REQ_GET_SOFTVER      6
#define CLOUD_RES_GET_SOFTVER      7
#define CLOUD_REQ_OTA              8
#define CLOUD_RES_OTA              9

#define CLOUD_REQ_DISABLE_DID      10
#define CLOUD_RES_DISABLE_DID      11
#define CLOUD_REQ_GET_JD_UUID      12
#define CLOUD_RES_GET_JD_UUID      13
#define CLOUD_REQ_POST_JD_INFO     14
#define CLOUD_RES_POST_JD_INFO     15
#define CLOUD_CONFIG_OK            16
#define CLOUD_REQ_GET_GSERVER_TIME 17


#define INVALID_SOCKET  (-1)

#define ONE_SECOND  (1000)
#define GAGENT_HTTP_TIMEOUT        (5*ONE_SECOND)
#define GAGENT_CLOUDREADD_TIME     (10*ONE_SECOND)

#define CLOUD_MQTT_SET_ALIVE      (120*ONE_SECOND)
#define MCU_HEARTBEAT           (50*ONE_SECOND)
#define CLOUD_HEARTBEAT          (55*ONE_SECOND)


#define LOCAL_DATA_IN     (1<<0)
#define LOCAL_DATA_OUT    (1<<1)
#define CLOUD_DATA_IN     (1<<6)
#define CLOUD_DATA_OUT    (1<<7)







#define GAGENT_MAGIC_NUM    (0x55aa1122)


/*
 * | head(0xffff) | len(2B) | cmd(2B) | SN(1B) | flag(2B) | payload(xB) | checksum(1B) |
 *     0xffff     cmd~checksum                                            len~payload
 */
#define MCU_SYNC_HEAD_LEN      2
#define MCU_LEN_POS            2
#define MCU_CMD_POS            4
#define MCU_SN_POS             5
#define MCU_FLAG_POS           6
#define MCU_ERROR_POS          8
#define MCU_HDR_LEN            8
#define MCU_LEN_NO_PAYLOAD     9
#define MCU_HDR_FF             0xFF
#define MCU_P0_POS             8
#define MCU_BYTES_NO_SUM        3
#define LOCAL_GAGENTSTATUS_MASK 0x1FFF


/*V4 CMD of P0*/
#define MCU_INFO_CMD        0X01
#define MCU_INFO_CMD_ACK    0X02

#define MCU_CTRL_CMD        0X03
#define MCU_CTRL_CMD_ACK    0X04

#define MCU_REPORT          0X05
#define MCU_REPORT_ACK      0X06

#define MODULE_PING2MCU       0X07
#define MODULE_PING2MCU_ACK   0X08

#define MCU_CONFIG_WIFI     0X09
#define MCU_CONFIG_WIFI_ACK 0X0A

#define MCU_RESET_MODULE      0X0B
#define MCU_RESET_MODULE_ACK  0X0C

#define WIFI_STATUS2MCU     0X0D
#define WIFI_STATUS2MCU_ACK 0X0E

#define MCU_DATA_ILLEGAL    0x11
#define MCU_REPLY_GAGENT_DATA_ILLEGAL    0x12

#define WIFI_TEST           0x13
#define WIFI_TEST_ACK       0x14

#define MCU_ENABLE_BIND     0x15
#define MCU_ENABLE_BIND_ACK 0x16

#define MCU_REQ_GSERVER_TIME     0x17
#define MCU_REQ_GSERVER_TIME_ACK 0x18

#define MCU_NEED_UPGRADE     0x19
#define MCU_NEED_UPGRADE_ACK 0x1A

#define MCU_READY_RECV_FIRMWARE    0x1B
#define MCU_READY_RECV_FIRMWARE_ACK 0x1C

#define GAGENT_SEND_UPGRADE     0x1D
#define GAGENT_SEND_UPGRADE_ACK 0x1E

#define GAGENT_STOP_SEND     0x1F
#define GAGENT_STOP_SEND_ACK 0x20

/*For V4, GAgent waiting for MCU response of very CMD send by GAgent, Unit: ms*/
#define MCU_ACK_TIME_MS    1000


#define GAGENT_MCU_CHECKSUM_ERROR  0x01
#define GAGENT_MCU_CMD_ERROR       0x02
#define GAGENT_MCU_OTHER_ERROR     0x03




typedef enum
{
    MSG_ID_START = 100,
    MSG_ID_MCU_ACK_TIMEOUT,
    MSG_ID_MCU_ACK_SUCCESS,
    MSG_ID_GPRS_OK_HINT,
    MSG_ID_CLOUD_CONFIG,
    MSG_ID_MQTT_CONFIG,
    MSG_ID_MQTT_READY,
    MSG_ID_HEARTBEAT_READY,
    MSG_ID_MCU_PROGRESS_READY,
    MSG_ID_PING_REQUEST,
    MSG_ID_MCU_PING_RSP_OK,
    MSG_ID_CLOUD_PING_RSP_OK,
    MSG_ID_CLOUD_SEND_DATA,
    MSG_ID_MCU_SEND_DATA,

 }Enum_GAGENT_MSG_TYPE;



typedef enum
{
    GAGENT_ERROR_MCU_BUSY = -300,
    GAGENT_ERROR_MCU_GETINFO = -301,

}Enum_Gagent_ErrorCode;









extern pgcontext pgContextData;

#define PGC    (pgContextData)
#define PPGC   (&pgContextData)

/****************************************************************
Function    :   GAgent_DevInit
Description :   module init,waiting for ril layer init and network ok
pgc         :   global staruc pointer.
return      :   void
****************************************************************/
void GAgent_DevInit( pgcontext pgc );




/****************************************************************
Function    :   GAgent_Init
Description :   GAgent init 
pgc         :   global staruc pointer.
return      :   void
****************************************************************/
void GAgent_Init( pgcontext *pgc );

/****************************************************************
Function    :   GAgent_NewVar
Description :   malloc New Var 
pgc         :   global staruc pointer.
return      :   void
****************************************************************/
void GAgent_NewVar( pgcontext *pgc );

/****************************************************************
Function    :   GAgent_VarInit
Description :   init global var and malloc memory 
pgc         :   global staruc pointer.
return      :   void
****************************************************************/
void GAgent_VarInit( pgcontext *pgc );

/****************************************************************
Function    :   GAgent_LocalInit
Description :   init local for communication with MCU
pgc         :   global staruc pointer.
return      :   void
****************************************************************/
void GAgent_LocalInit( pgcontext pgc );


/****************************************************************
FunctionName  :     GAgent_LocalGetInfo
Description   :     get localinfo like pk.
return        :     void
****************************************************************/
void GAgent_LocalGetInfo( pgcontext pgc );

/****************************************************************
FunctionName    :   GAgent_LocalDataWriteP0
Description     :   send p0 to local io and add 0x55 after 0xff
                    auto.
cmd             :   MCU_CTRL_CMD or WIFI_STATUS2MCU
return          :   0-ok other -error
****************************************************************/
s32 GAgent_LocalDataWriteP0( pgcontext pgc,s32 fd,ppacket pTxBuf,u8 cmd );

/****************************************************************
Function    :   GAgent_NewSN
Description :   generate a new serial no.
pgc         :   void
return      :   SN
****************************************************************/
u8 GAgent_NewSN(void);

/*********************************************************
*
*       buf     :   need to get checksum buf, need not include the checksum of 
                    received package;
*       bufLen  :   bufLen      
*       return  :   checksum
**********************************************************/
u8 GAgent_SetCheckSum(  u8 *buf,int packLen );


/****************************************************************
FunctionName    :   GAgent_LocalDataAdapter
Dercription     :   the function will add 0x55 after local send data,
pData           :   the source of data need to change.
dataLen         :   the length of source data.
destBuf         :   the data after change.
return          :   the length of destBuf.                                  
****************************************************************/
s32 Gagent_Local_DataAdapter( u8 *pData,s32 dataLen );

/****************************************************************
FunctionName    :   GAgent_MoveOneByte
Description     :   move the array one byte to left or right
pData           :   need to move data pointer.
dataLen         :   data length 
flag            :   0 move right
                    1 move left.
return          :   NULL
****************************************************************/
void GAgent_MoveOneByte( u8 *pData,s32 dataLen,u8 flag );


/******************************************************
 *      FUNCTION        :   update info
 *      new_pk          :   new productkey
 *
 ********************************************************/
void GAgent_UpdateInfo( pgcontext pgc,u8 *new_pk );




/****************************************************************
FunctionName    :   GAgent_Clean_Config
Description     :   GAgent clean the device config                  
pgc             :   global staruc 
return          :   NULL
****************************************************************/
void GAgent_Clean_Config( pgcontext pgc );










#endif
