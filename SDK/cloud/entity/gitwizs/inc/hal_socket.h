#ifndef __HAL_SOCKET_H__
#define __HAL_SOCKET_H__

#include "gagent_typedef.h"
#include "ql_socket.h"
#include "gagent.h"





#if( GAGENT_RELEASE==1)
  #define HTTP_SERVER         "http://api.gizwits.com\0"
  //#define HTTP_SERVER         "http://www.quectel.com/\0"
#else
  #define HTTP_SERVER         "http://api.iotsdk.com/\0"
#endif




//#define HTTP_CONTEXT (0)
#define HTTP_CONTEXT (1)

#define MQTT_CONTEXT (1)



/****************************************************************
FunctionName    :   Hal_Login_MQTT_TCP_Init
Description     :   Init login to mqtt SERVER
return          :   void
****************************************************************/
void Hal_Login_MQTT_TCP_Init(void);


/****************************************************************
FunctionName    :   Hal_Login_Gservice_HTTP_Init
Description     :   Init login to gservice
return          :   void
****************************************************************/
void Hal_Login_Gservice_HTTP_Init(void);

/****************************************************************
FunctionName    :   Hal_Close_PDPContext
Description     :   close dedicated pdp context
pdp_type        :   which pdp context to deactive
return          :   void
****************************************************************/
void Hal_Close_PDPContext(u8 pdp_type);


/****************************************************************
FunctionName    :   Hal_HTTP_RequestTo_Post
Description     :   post msg to http server
strPostMsg      :   the pointer to msg
len             :   msg len
return          :   result code
****************************************************************/
s32 Hal_HTTP_RequestTo_Post(char* strPostMsg, u16 len);


/****************************************************************
FunctionName    :   Hal_HTTP_RequestTo_Get
Description     :   get item from http server
timeout         :   timeout
return          :   result code
****************************************************************/
s32 Hal_HTTP_RequestTo_Get(u8* buf,u32 timeout);

/****************************************************************
FunctionName    :   HAL_SOC_GetIPByHostName
Description     :   convert hostname to ip addr
hostname        :   hostname
ip              :   the converted ip
return          :   result code
****************************************************************/
s32 HAL_SOC_GetIPByHostName(u8* hostname,u8 *ip);














#endif
