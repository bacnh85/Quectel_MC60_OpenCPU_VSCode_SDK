#ifndef __MQTTLIB_EXT_H__
#define __MQTTLIB_EXT_H__

#include "mqttbuffer.h"

#ifdef __GITWIZS_SOLUTION__
#define MQTT_PROTOCOL_V3
#endif


#ifdef __ONENET_SOLUTION__
#define MQTT_PROTOCOL_V4
#endif

/** MQTT QOS Level */
enum MqttQosLevel {
    MQTT_QOS_LEVEL0,  /**< at most once */
    MQTT_QOS_LEVEL1,  /**< at least once  */
    MQTT_QOS_LEVEL2   /**< exactly once  */
};

/**MQTT ErrorCode**/
enum MqttError {
    MQTTERR_NOERROR                  = 0,  /**Success*/
    MQTTERR_OUTOFMEMORY              = -1, /**Out of Memory*/
    MQTTERR_ENDOFFILE                = -2, /**End of File*/
    MQTTERR_IO                       = -3, /**< I/O Error */
    MQTTERR_ILLEGAL_PKT              = -4, /**< Illegal Packet*/
    MQTTERR_ILLEGAL_CHARACTER        = -5, /**< Illegal Chacracter*/
    MQTTERR_NOT_UTF8                 = -6, /**< UTF-8 Error */
    MQTTERR_INVALID_PARAMETER        = -7, /**< Invalid Parameter */
    MQTTERR_PKT_TOO_LARGE            = -8, /**< Too Large Packet */
    MQTTERR_BUF_OVERFLOW             = -9, /**< Buffer Overflow */
    MQTTERR_EMPTY_CALLBACK           = -10,/**< Empty Callback */
    MQTTERR_INTERNAL                 = -11,/**< Sys Internal Error*/
    MQTTERR_NOT_IN_SUBOBJECT         = -12,
    MQTTERR_INCOMPLETE_SUBOBJECT     = -13,
    MQTTERR_FAILED_SEND_RESPONSE     = -14,
    MQTTERR_ASSERT_ERROR             = -15,
};

/** MQTT Connect Flag */
enum MqttConnectFlag {
    MQTT_CONNECT_CLEAN_SESSION  = 0x02,
    MQTT_CONNECT_WILL_FLAG      = 0x04,
    MQTT_CONNECT_WILL_QOS0      = 0x00,
    MQTT_CONNECT_WILL_QOS1      = 0x08,
    MQTT_CONNECT_WILL_QOS2      = 0x10,
    MQTT_CONNECT_WILL_RETAIN    = 0x20,
    MQTT_CONNECT_PASSORD        = 0x40,
    MQTT_CONNECT_USER_NAME      = 0x80
};


/** MQTT data type */
enum MqttPacketType {
    MQTT_PKT_CONNECT = 1, /**< Connect Request */
    MQTT_PKT_CONNACK,     /**< Connect Ack */
    MQTT_PKT_PUBLISH,     /**< Publish Data */
    MQTT_PKT_PUBACK,      /**< Publish Ack */
    MQTT_PKT_PUBREC,      /**< Publish Recieve£¬Qos 2Ê±£¬Response to MQTT_PKT_PUBLISH */
    MQTT_PKT_PUBREL,      /**< Publish Release£¬ Qos 2Ê±£¬Response to MQTT_PKT_PUBREC */
    MQTT_PKT_PUBCOMP,     /**< Publish Complete£¬ Qos 2Ê±£¬Response to MQTT_PKT_PUBREL */
    MQTT_PKT_SUBSCRIBE,   /**< Subscribe Topic */
    MQTT_PKT_SUBACK,      /**< Subscribe Ack */
    MQTT_PKT_UNSUBSCRIBE, /**< Unsubscribe Topic */
    MQTT_PKT_UNSUBACK,    /**< Unsubscribe Ack */
    MQTT_PKT_PINGREQ,     /**< Ping Request */
    MQTT_PKT_PINGRESP,    /**< Ping Response*/
    MQTT_PKT_DISCONNECT   /**< Disconnect Request*/
};

struct iovec {
    void *iov_base;
    unsigned int iov_len;
};


/** MQTT Running Context */
struct MqttContext {
    char *bgn;
    char *end;
    char *pos;
    void *read_func_arg;
    int (*read_func)(void *arg, unsigned char *buf,int count);
    void *writev_func_arg;
    int (*writev_func)(void *arg, const struct iovec *iov, int iovcnt);
    void *handle_ping_resp_arg;
    int (*handle_ping_resp)(void *arg);

    void *handle_conn_ack_arg;
    int (*handle_conn_ack)(void *arg, char flags, char ret_code);

    void *handle_publish_arg; 
    int (*handle_publish)(void *arg, unsigned char pkt_id, const char *topic,
                          const char *payload, unsigned int payloadsize,
                          int dup, enum MqttQosLevel qos);
    void *handle_pub_ack_arg;
    int (*handle_pub_ack)(void *arg, unsigned short pkt_id);
    void *handle_pub_rec_arg; 
    int (*handle_pub_rec)(void *arg, unsigned short pkt_id);
    void *handle_pub_rel_arg; 
    int (*handle_pub_rel)(void *arg, unsigned short pkt_id);
    void *handle_pub_comp_arg;
    int (*handle_pub_comp)(void *arg, unsigned short pkt_id);
    void *handle_sub_ack_arg; 
    int (*handle_sub_ack)(void *arg, unsigned short pkt_id,
                          const char *codes, unsigned int count);

    void *handle_unsub_ack_arg;
    int (*handle_unsub_ack)(void *arg, unsigned short packet_id);

    void *handle_cmd_arg; 
    int (*handle_cmd)(void *arg, unsigned short pkt_id, const char *cmdid,
                      long timestamp, const char *desc, const char *cmdarg,
                      unsigned int cmdarg_len, int dup, enum MqttQosLevel qos);

};

/**
*Func : Init MQTT Context
*@param : ctx : the mqtt context
*@param : buf_size : recieve buffer size
*@return :refer to enum MqttError
**/


int Mqtt_InitContext(struct MqttContext *ctx, unsigned int buf_size);

/**
*Func : Destroy MQTT Context
*@param : ctx : the mqtt context
*@return :refer to enum MqttError
**/

void Mqtt_DestroyContext(struct MqttContext *ctx);


/**
*Func : Set packet dup flag
*@param : buf : the buf to save data packet
*@return :refer to enum MqttError
**/

int Mqtt_SetPktDup(struct MqttBuffer *buf);



/**
*Func:Packing MQTT Connect Packet
*@param buf : the buf to save data packet
*@param keep_alive : keep alive internal ,unit in seconds
*@param id : the client indentifier
*@param clean_session : 0 -- continue to use last session,if there is no last session ,create a new one
                        1 -- delete the last session and create a new one
*@param will_topic : when the client drops unexpectedly,the will msg will be sent to this topic
*@param will_msg : the msg sent to the will topic when client drops unexpectedly
*@param msg_len : the length of will msg
*@param qos : the level of quality of service
*@param will_retain : 0 -- the server will delete the will_msg when send out it
                      1 -- the server will store the will_msg when send out it
*@param user : the username for authentication
*@param password : the password for authentication
*@param pswd_len : the length of password
*@return refer to enum MqttError
**/
int Mqtt_PackConnectPkt(struct MqttBuffer *buf, unsigned short keep_alive, const char *id,
                        int clean_session, const char *will_topic,
                        const char *will_msg, unsigned short msg_len,
                        enum MqttQosLevel qos, int will_retain, const char *user,
                        const char *password, unsigned short pswd_len);



/**
*Func : Recieve MQTT Packet
*@param : ctx : the mqtt context
*@return :refer to enum MqttError
**/

int Mqtt_RecvPkt(struct MqttContext *ctx);

/**
*Func : Send MQTT Packet
*@param : ctx : the mqtt context
*@param : buf : the buf to save data packet
*@param : offset : the offset of buf start
*@return :refer to enum MqttError
**/
int Mqtt_SendPkt(struct MqttContext *ctx, const struct MqttBuffer *buf, unsigned int offset);

/**
*Func:Packing MQTT Publish Packet
*@param buf : the buf to save data packet
*@param pkt_id : packet id ,not zero
*@param topic : the topic publish to
*@param payload : payload start addr
*@param size : payload size ,in bytes
*@param qos : the level of quality of service
*@param retain :      0 -- the server will delete the msg to given topic
                      1 -- the server will store the msg to given topic
*@param own : whether to copy payload to buffer,0 -- no !0 --yes
              when set to 0.keep the payload valid before reset or destroy
*@return refer to enum MqttError
**/


int Mqtt_PackPublishPkt(struct MqttBuffer *buf, unsigned short pkt_id, const char *topic,
                        const char *payload, unsigned int size,
                        enum MqttQosLevel qos, int retain, int own);

/**
*Func:Packing MQTT Pubrelease Packet
*@param buf : the buf to save data packet
*@param pkt_id : packet id ,not zero
*@return refer to enum MqttError
**/

int Mqtt_PackPubRelPkt(struct MqttBuffer *buf, unsigned short pkt_id);

/**
*Func:Packing MQTT Subscribe Packet
*@param buf : the buf to save data packet
*@param pkt_id : packet id ,not zero
*@param topic : the topic subscribe
*@param qos : the level of quality of service
*@return refer to enum MqttError
**/

int Mqtt_PackSubscribePkt(struct MqttBuffer *buf, unsigned short pkt_id,
                          const char *topic, enum MqttQosLevel qos);



/**
*Func:Packing MQTT Unsubscribe Packet
*@param buf : the buf to save data packet
*@param pkt_id : packet id ,not zero
*@param topic : the topic to unsubscribe
*@return refer to enum MqttError
**/

int Mqtt_PackUnsubscribePkt(struct MqttBuffer *buf, unsigned short pkt_id, const char *topic);


/**
*Func:Packing MQTT Ping Packet
*@param buf : the buf to save data packet
*@return refer to enum MqttError
**/

int Mqtt_PackPingReqPkt(struct MqttBuffer *buf);

/**
*Func:Packing MQTT Disconnect Packet
*@param buf : the buf to save data packet
*@return refer to enum MqttError
**/

int Mqtt_PackDisconnectPkt(struct MqttBuffer *buf);


/**
*Func:Packing MQTT Pubrecieve Packet
*@param buf : the buf to save data packet
*@param pkt_id : the packet id
*@return refer to enum MqttError
**/

int Mqtt_PackPubRecPkt(struct MqttBuffer *buf, unsigned short pkt_id);

/**
*Func:Packing MQTT Puback Packet
*@param buf : the buf to save data packet
*@param pkt_id : the packet id
*@return refer to enum MqttError
**/

int Mqtt_PackPubAckPkt(struct MqttBuffer *buf, unsigned short pkt_id);

/**
*Func:Packing MQTT Pubcomplish Packet
*@param buf : the buf to save data packet
*@param pkt_id : the packet id
*@return refer to enum MqttError
**/


int Mqtt_PackPubCompPkt(struct MqttBuffer *buf, unsigned short pkt_id);

/**
*Func : Append Subscribe Topic
*@param : buf : the buf to save data packet
*@param : topic : the topic want to append
*@param : qos   : the qos level when subscribe topic
*@return :refer to enum MqttError
**/


int Mqtt_AppendSubscribeTopic(struct MqttBuffer *buf, const char *topic, enum MqttQosLevel qos);

/**
*Func : Append UnSubscribe Topic
*@param : buf : the buf to save data packet
*@param : topic : the topic want to append
*@return :refer to enum MqttError
**/

int Mqtt_AppendUnsubscribeTopic(struct MqttBuffer *buf, const char *topic);


#endif
