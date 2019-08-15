#ifndef __CLOUD_H__
#define __CLOUD_H__

#include "gagent_typedef.h"


#define MQTT_STATUS_START               1
#define MQTT_STATUS_REQ_LOGIN           2
#define MQTT_STATUS_RES_LOGIN           3
#define MQTT_STATUS_REQ_LOGINTOPIC1     4
#define MQTT_STATUS_RES_LOGINTOPIC1     5
#define MQTT_STATUS_REQ_LOGINTOPIC2     6
#define MQTT_STATUS_RES_LOGINTOPIC2     7
#define MQTT_STATUS_REQ_LOGINTOPIC3     8
#define MQTT_STATUS_RES_LOGINTOPIC3     9
#define MQTT_STATUS_RUNNING             10


#define GAGENT_CMD_CTL_93  0x0093


typedef enum
{
   CLOUD_ERROR_NETWORK = -400,
   CLOUD_ERROR_CONFIG_ALREADY = -401,

}Enum_Cloud_ErrorCode;




/****************************************************************
*
*   function    :   gagent do cloud config.
*   cloudstatus :   gagent cloud status.
*   return      :   0 successful other fail.
****************************************************************/
u32 Cloud_ConfigDataHandle( pgcontext pgc /*int32 cloudstatus*/ );


/****************************************************************
*       FunctionName    :   Cloud_ReqProvision
*       Description     :   send provision req to host.
*       host            :   GServer host,like "api.gizwits.com"
*       return          :   0 success other error.
****************************************************************/
//u32 Cloud_ReqProvision( pgcontext pgc );


/****************************************************************
        Function    :   Cloud_ReqRegister
        description :   sent register data to cloud
        Input       :   NULL;
        return      :   0-send register data ok.
                        other fail.
****************************************************************/
u32 Cloud_ReqRegister( pgcontext pgc );

/* 
    will get the device id.
*/
s8 Cloud_ResRegister( u8 *cloudConfiRxbuf,s8 *pDID,s32 respondCode );



/****************************************************************
*       FunctionName    :   Cloud_ReqProvision
*       Description     :   send provision req to host.
*       host            :   GServer host,like "api.gizwits.com"
*       return          :   0 success other error.
****************************************************************/
u32 Cloud_ReqProvision( pgcontext pgc );

/****************************************************************
*       FunctionName    :   Cloud_ResProvision.
*       Description     :   data form server after provision.
*       szm2mhost       :   m2m server like: "m2m.gizwits.com"
*       port            :   m2m port .
*       respondCode     :   http respond code.
*       return          :   0 success other fail.
****************************************************************/
u32 Cloud_ResProvision( s8 *szdomain,s32 *port,u8 *cloudConfiRxbuf,s32 respondCode );


/****************************************************************
        FunctionName        :   Cloud_M2MDataHandle.
        Description         :   Receive cloud business data .
        xpg                 :   global context.
        return              :   >0 have business data,and need to 
                                   handle.
                                other,no business data.
****************************************************************/
s32 Cloud_M2MDataHandle(  pgcontext pgc);


/****************************************************************
        Function    :   Cloud_ReqConnect
        Description :   send req m2m connect packet.
        username    :   username.
        password    :   username.
        return      :   0: send req connect packet ok
                        other req connect fail.
****************************************************************/
u32 Cloud_ReqConnect( pgcontext pgc,const u8 *username,const u8 *password );


/****************************************************************
        Function    :   Cloud_ResConnect
        Description :   handle packet form mqtt req connect 
        buf         :   data form mqtt.
        return      :   0: req connect ok
                        other req connect fail.
****************************************************************/
u32 Cloud_ResConnect( u8* buf );


u32 Cloud_ReqSubTopic( pgcontext pgc,u16 mqttstatus );

/****************************************************************
        Function        :   Cloud_ResSubTopic
        Description     :   check sub topic respond.
        buf             :   data form mqtt.
        msgsubId        :   sub topic messages id
        return          :   0 sub topic ok.
                            other fail.
****************************************************************/
u32 Cloud_ResSubTopic( const u8* buf,u16 msgsubId );



/****************************************************************
        FunctionName    :   GAgent_Cloud_SendData
        Description     :   send buf data to M2M server.
        return          :   0-ok 
                            other fail.
****************************************************************/
u32 Cloud_SendData( pgcontext pgc,ppacket pbuf,s32 buflen );


void Cloud_ClearClientAttrs(pgcontext pgc, stCloudAttrs_t *client);

void Cloud_SetClientAttrs(pgcontext pgc, u8 *clientid, u16 cmd, s32 sn);



#endif
