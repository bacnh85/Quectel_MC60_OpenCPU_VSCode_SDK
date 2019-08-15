#ifndef __MQTT_XPG_H__
#define __MQTT_XPG_H__

#include "mqttlib_ext.h"
#include "mqttbuffer.h"
#include "gagent.h"



struct MqttGizwitContext
{
    int mqttfd;
    unsigned int sendedbytes;
    struct MqttContext mqttctx[1];
    struct MqttBuffer mqttbuf[1];

    const char *host;
    unsigned short port;

    const char *proid;
    const char *devid;
    const char *apikey;

    int dup;
    enum MqttQosLevel qos;
    int retain;

    unsigned short pkt_to_ack;
    char cmdid[1024];
};




extern struct MqttGizwitContext gw_ctx[1];


int Mqttxpg_HandlePingResp(void *arg);
int Mqttxpg_HandleConnAck(void *arg, char flags, char ret_code);
int Mqttxpg_HandlePublish(void *arg, unsigned char pkt_id, const char *topic,
                         const char *payload, unsigned int payloadsize,
                         int dup, enum MqttQosLevel qos);
int Mqttxpg_HandlePubAck(void *arg, unsigned short pkt_id);
int Mqttxpg_HandlePubRec(void *arg, unsigned short pkt_id);
int Mqttxpg_HandlePubRel(void *arg, unsigned short pkt_id);
int Mqttxpg_HandlePubComp(void *arg, unsigned short pkt_id);
int Mqttxpg_HandleSubAck(void *arg, unsigned short pkt_id,
                          const char *codes, unsigned int count);
int Mqttxpg_HandleUnsubAck(void *arg, unsigned short packet_id);
int Mqttxpg_HandleCmd(void *arg, unsigned short pkt_id, const char *cmdid,
                      long timestamp, const char *desc, const char *cmdarg,
                      unsigned int cmdarg_len, int dup, enum MqttQosLevel qos);












int Mqtt_Login2Server(int socketid,const unsigned char *username,const unsigned char *password );

/***********************************************************
 *
 *   return :    0 success ,1 error
 *
 *************************************************************/
int Mqtt_SendConnetPacket(int  socketid,const char* username,const char* password );


int MQTT_readPacket( int socketid,ppacket pbuf, int bufferLen);

int MQTTclose_socket(void);

int Mqtt_DoSubTopic( pgcontext pgc,short mqttstatus );

int Mqtt_SubLoginTopic(pgcontext pgc,short mqttstatus );

void MQTT_HeartbeatTime( void );

int Mqtt_DispatchPublishPacket( pgcontext pgc,unsigned char *packetBuffer,int packetLen );

/********************************************************************
 *
 *  FUNCTION   : Mqtt send request packbag to server ask online client
 *               res.
 *   buf       : mqtt msg payload.
 *   return    : none.
 *             add by alex.lin --2014-12-11
 ********************************************************************/
void Mqtt_ResOnlineClient( pgcontext pgc,char *buf,int buflen);


/********************************************************************
        Function        :   MQTT_SenData.
        Description     :   send buf data to m2m.
        szDID           :   Device ID.
        buf             :   need to send data pointer.
        buflen          :   buf length .
        return          :   0-ok 
                            other fail.
********************************************************************/
int MQTT_SenData( pgcontext pgc, char *szDID, ppacket pbuf,/*uint8 *buf,*/ int buflen );

#endif
