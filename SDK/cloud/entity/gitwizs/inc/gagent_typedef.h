#ifndef __GAGENT_TYPEDEF_H__
#define __GAGENT_TYPEDEF_H__



#include "ql_type.h"

#define M2MSERVER_LEN         (128)
#define IMEI_LEN               (30)
#define PHONECLIENTID          (23)
#define PASSCODE_MAXLEN          32
#define PASSCODE_LEN            10
#define MCU_PROTOCOLVER_LEN    (8)
#define MCU_P0VER_LEN          (8)
#define MCU_HARDVER_LEN        (8)
#define MCU_SOFTVER_LEN         (8)
#define MCU_MCUATTR_LEN         (8)
#define PK_LEN                   (32)
#define DID_LEN                  (24)
#define FIRMWARELEN              (32)
#define FIRMWARE_LEN_MAX            32
#define IP_LEN_MAX               (15) 
#define IP_LEN_MIN               (7) 
#define MCU_P0_LEN                2
#define MCU_CMD_LEN               2


/* use for mqtt var len */
typedef struct _varc
{
    char var[2];//the value of mqtt
    char varcbty;// 1b-4b B=Bit,b=byte
} varc;




typedef struct modeinfo_t
{
    s32 m2m_Port;    //m2m port no
    s8  m2m_SERVER[M2MSERVER_LEN+1];  //m2m server ip
    u8  imei[IMEI_LEN+1];  //module IMEI no
}modeinfo, *pmodeinfo;


typedef struct _gprsStatus
{
   s32 devSignalrssi; //Signal Quality of module
   s32 devSignalber;
}signalstatus;

typedef struct
{
    s32 sn;
    u16 cmd;
    s8  phoneClientId[PHONECLIENTID+1];
}stCloudAttrs_t;


typedef struct _waninfo
{
    stCloudAttrs_t srcAttrs;
    u32 send2HttpLastTime;
    u32 send2MqttLastTime;
    u32 ReConnectMqttTime;
    u32 ReConnectHttpTime;
    u32 firstConnectHttpTime;
    s32 http_socketid;
    s32 m2m_socketid;
    s32 wanclient_num;

    u16 CloudStatus;
    u16 mqttstatus;
    u8 mqttpackType;
    u16 mqttMsgsubid;
    s8  phoneClientId[PHONECLIENTID+1];
    s8  cloudPingTime;
    s8  httpCloudPingTime; 
}WanInfo;

typedef struct _localmodule
{
    u8  timeoutCnt;
    u32 oneShotTimeout;
    s32 uart_fd;


}localmodule;

typedef struct _packet
{
    s32   totalcap;  
    s32   remcap;   
    u8    *allbuf;  
    s32   bufcap;   
    
    u32   type;    
    u8    *phead;    
    u8    *ppayload;  
    u8    *pend;    
} packet,*ppacket;

typedef struct
{
    u32  type;
    stCloudAttrs_t cloudClient;
}stChannelAttrs_t;






typedef struct runtimeinfo_t
{
    u32 clock;   //for heartbeat
    u32 updatestatusinterval;   
    u32 filelen;
    s32 m2m_result;
    u32 m2m_recvlen;

    u32 gprs_status;

    u32 GAgentStatus;/* gagentStatus */
    u16 lastGAgentStatus;
   
    s8  firstStartUp;
    s8  *MD5;

    signalstatus devSignalStatus;
    
    WanInfo waninfo;
    localmodule local;
    ppacket Cloud_Rxbuf;
    ppacket Cloud_Txbuf;
    ppacket Txbuf;/* send data to local buf */
    ppacket Rxbuf;/* receive data from local buf */
    stChannelAttrs_t stChannelAttrs;    /* the attrs of channel connected to GAgent */

}runtimeinfo, *pruntimeinfo;

typedef struct 
{
   u8 cmd;
   u8 sn;
   u32 local_send_len;
}localTxbufInfo;


typedef struct _XPG_MCU
{
    /* XPG_Ping_McuTick */
    //uint32 XPG_PingTime;
    u32     oneShotTimeout;
    u8      timeoutCnt;
    volatile u8 isBusy;
    //int8 loseTime;
    /* 8+1('\0') for print32f. */
    u8      protocol_ver[MCU_PROTOCOLVER_LEN+1];
    u8      p0_ver[MCU_P0VER_LEN+1];
    u8      hard_ver[MCU_HARDVER_LEN+1];
    u8      soft_ver[MCU_SOFTVER_LEN+1];
    u8      product_key[PK_LEN+1];
    u8      mcu_attr[MCU_MCUATTR_LEN];
    localTxbufInfo TxbufInfo;   
}XPG_MCU;

typedef struct GAGENT_CONFIG
{
    u32   magicNumber;
    u32   flag;
    s8    DID[DID_LEN]; /* Device, generate by server, unique for devices */
    s8    FirmwareVerLen[2];
    s8    FirmwareVer[FIRMWARELEN+1];
    s8    modulepasscode[PASSCODE_MAXLEN+1];
    s8    old_modulepasscode[PASSCODE_MAXLEN+1];
    s8    old_did[DID_LEN];
    s8    old_productkey[PK_LEN + 1];    /* Add 1byte '\0' */
    u8    m2m_ip[IP_LEN_MAX + 1];        /* Add 1byte '\0' */
    u8    GServer_ip[5];    /* Add 1byte '\0' */
    s8    rsvd[256];
}GAGENT_CONFIG_S, gconfig, *pgconfig;



/* global context, or gagent context */
typedef struct gagentcontext_t
{
    /* modeinfo mi; */
    modeinfo minfo;
    runtimeinfo rtinfo;
    /* mcuinfo mcui; */
    XPG_MCU mcu;
    /* logman lm; */
    gconfig gc;
}gcontext, *pgcontext;



#endif

