#include "custom_feature_def.h"
#ifdef __OCPU_SMART_CLOUD_SUPPORT__


#include "mqttlib_ext.h"
#include "mqttbuffer.h"
#include <ctype.h>
#include "ql_stdlib.h"
#include "ql_memory.h"
#include "ql_trace.h"

#define CMD_TOPIC_PREFIX "$SYS/cmdreq/"
#define CMD_TOPIC_PREFIX_LEN 12 // strlen(CMD_TOPIC_PREFIX)



static const char Mqtt_TrailingBytesForUTF8[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};


static int Mqtt_CheckClentIdentifier(const char *id);
static int Mqtt_CheckUtf8(const char *str, unsigned int len);
static int Mqtt_IsLegalUtf8(const char *first, int len);
static int Mqtt_DumpLength(unsigned int len, char *buf);
static void Mqtt_WB16(unsigned short v, char *out);
static void Mqtt_WB32(unsigned int v, char *out);

static void Mqtt_PktWriteString(char **buf, const char *str, unsigned short len);
static int Mqtt_ReadLength(const char *stream, int size, unsigned int *len);
static int Mqtt_Dispatch(struct MqttContext *ctx, char fh,  char *pkt, unsigned int size);
static int Mqtt_HandleConnAck(struct MqttContext *ctx, char flags,
                              char *pkt, unsigned int size);

static unsigned short Mqtt_RB16(const char *v);
static unsigned long long Mqtt_RB64(const char *v);

static int Mqtt_HandlePubAck(struct MqttContext *ctx, char flags,
                             char *pkt, unsigned int size);

static int Mqtt_HandlePubRec(struct MqttContext *ctx, char flags,
                             char *pkt, unsigned int size);
static int Mqtt_HandlePubComp(struct MqttContext *ctx, char flags,
                              char *pkt, unsigned int size);

static int Mqtt_HandleSubAck(struct MqttContext *ctx, char flags,
                             char *pkt, unsigned int size);
static int Mqtt_HandleUnsubAck(struct MqttContext *ctx, char flags,
                               char *pkt, unsigned int size);
static int Mqtt_AppendLength(struct MqttBuffer *buf, unsigned int len);

static int Mqtt_EraseLength(struct MqttBuffer *buf, unsigned int len);

static int Mqtt_HandlePublish(struct MqttContext *ctx, char flags,
                              char *pkt, unsigned int size);



static int Mqtt_EraseLength(struct MqttBuffer *buf, unsigned int len)
{
    struct MqttExtent *fix_head = buf->first_ext;
    unsigned int pkt_len;

    if(NULL == fix_head)
    {
        return MQTTERR_ASSERT_ERROR;
    }

    if(Mqtt_ReadLength(fix_head->payload + 1, 4, &pkt_len) < 0) {
        return MQTTERR_INVALID_PARAMETER;
    }

    if(pkt_len < len) {
        // critical bug
        return MQTTERR_INTERNAL;
    }

    pkt_len -= len;
    buf->buffered_bytes -= len;

    fix_head->len = Mqtt_DumpLength(pkt_len, fix_head->payload + 1) + 1;
    if(fix_head->len < 2)
    {
        return MQTTERR_ASSERT_ERROR;
    }

    return MQTTERR_NOERROR;
}


static int Mqtt_CheckClentIdentifier(const char *id)
{
    int len;
    for(len = 0; '\0' != id[len]; ++len) {
        if(!isalnum(id[len])) {
            return -1;
        }
    }

    return len;
}

static int Mqtt_CheckUtf8(const char *str, unsigned int len)
{
    unsigned int i;

    for(i = 0; i < len;) {
        int ret;
        char utf8_char_len = Mqtt_TrailingBytesForUTF8[(unsigned char)str[i]] + 1;

        if(i + utf8_char_len > len) {
            return MQTTERR_NOT_UTF8;
        }

        ret = Mqtt_IsLegalUtf8(str, utf8_char_len);
        if(ret != MQTTERR_NOERROR) {
            return ret;
        }

        i += utf8_char_len;
        if('\0'== str[i]) {
            break;
        }
    }

    return (int)i;
}

static int Mqtt_IsLegalUtf8(const char *first, int len)
{
    unsigned char bv;
    const unsigned char *tail = (const unsigned char *)(first + len);

    switch(len) {
    default:
        return MQTTERR_NOT_UTF8;

    case 4:
        bv = *(--tail);
        if((bv < 0x80) || (bv > 0xBF)) {
            return MQTTERR_NOT_UTF8;
        }
    case 3:
        bv = *(--tail);
        if((bv < 0x80) || (bv > 0xBF)) {
            return MQTTERR_NOT_UTF8;
        }
    case 2:
        bv = *(--tail);
        if((bv < 0x80) || (bv > 0xBF)) {
            return MQTTERR_NOT_UTF8;
        }
        switch(*(const unsigned char *)first) {
        case 0xE0:
            if(bv < 0xA0) {
                return MQTTERR_NOT_UTF8;
            }
            break;

        case 0xED:
            if(bv > 0x9F) {
                return MQTTERR_NOT_UTF8;
            }
            break;

        case 0xF0:
            if(bv < 0x90) {
                return MQTTERR_NOT_UTF8;
            }
            break;

        case 0xF4:
            if(bv > 0x8F) {
                return MQTTERR_NOT_UTF8;
            }
            break;

        default:
            break;
        }
    case 1:
        if(((*first >= 0x80) && (*first < 0xC2)) || (*first > 0xF4)) {
            return MQTTERR_NOT_UTF8;
        }
    }

    return MQTTERR_NOERROR;
}


static int Mqtt_DumpLength(unsigned int len, char *buf)
{
    int i;
    for(i = 1; i <= 4; ++i) {
        *((unsigned char*)buf) = len % 128;
        len /= 128;
        if(len > 0) {
            *buf |= 128;
            ++buf;
        }
        else {
            return i;
        }
    }

    return -1;
}

static int Mqtt_AppendLength(struct MqttBuffer *buf, unsigned int len)
{
    struct MqttExtent *fix_head = buf->first_ext;
    unsigned int pkt_len;

    if(NULL == fix_head)
    {
        return MQTTERR_ASSERT_ERROR;
    }

    if(Mqtt_ReadLength(fix_head->payload + 1, 4, &pkt_len) < 0) {
        return MQTTERR_INVALID_PARAMETER;
    }

    pkt_len += len;

    fix_head->len = Mqtt_DumpLength(pkt_len, fix_head->payload + 1) + 1;
    if(fix_head->len < 2) {
        return MQTTERR_PKT_TOO_LARGE;
    }

    return MQTTERR_NOERROR;
}


static void Mqtt_WB16(unsigned short v, char *out)
{
    unsigned char *uo = (unsigned char*)out;
    uo[0] = (unsigned char)(v >> 8);
    uo[1] = (unsigned char)(v);
}

static void Mqtt_WB32(unsigned int v, char *out)
{
    unsigned char *uo = (unsigned char*)out;
    uo[0] = (unsigned char)(v >> 24);
    uo[1] = (unsigned char)(v >> 16);
    uo[2] = (unsigned char)(v >> 8);
    uo[3] = (unsigned char)(v);
}


static void Mqtt_PktWriteString(char **buf, const char *str, unsigned short len)
{
    Mqtt_WB16(len, *buf);
    Ql_memcpy(*buf + 2, str, len);
    *buf += 2 + len;
}

static int Mqtt_ReadLength(const char *stream, int size, unsigned int *len)
{
    int i;
    const unsigned char *in = (const unsigned char*)stream;
    unsigned int multiplier = 1;

    *len = 0;
    for(i = 0; i < size; ++i) {
        *len += (in[i] & 0x7f) * multiplier;

        if(!(in[i] & 0x80)) {
            return i + 1;
        }

        multiplier *= 128;
        if(multiplier >= 128 * 128 * 128) {
            return -2; // error, out of range
        }
    }

    return -1; // not complete
}

static int Mqtt_HandleConnAck(struct MqttContext *ctx, char flags,
                              char *pkt, unsigned int size)
{
    char ack_flags, ret_code;

    if((0 != flags) || (2 != size)) {
        return MQTTERR_ILLEGAL_PKT;
    }

    ack_flags = pkt[0];
    ret_code = pkt[1];

    if(((ack_flags & 0x01) && (0 != ret_code)) || (ret_code > 5)) {
        return MQTTERR_ILLEGAL_PKT;
    }

    return ctx->handle_conn_ack(ctx->handle_conn_ack_arg, ack_flags, ret_code);
}

static unsigned short Mqtt_RB16(const char *v)
{
    const unsigned char *uv = (const unsigned char*)v;
    return (((unsigned short)(uv[0])) << 8) | uv[1];
}

static unsigned long long Mqtt_RB64(const char *v)
{
    const unsigned char *uv = (const unsigned char*)v;
    return ((((unsigned long long)(uv[0])) << 56) |
            (((unsigned long long)(uv[1])) << 48) |
            (((unsigned long long)(uv[2])) << 40) |
            (((unsigned long long)(uv[3])) << 32) |
            (((unsigned long long)(uv[4])) << 24) |
            (((unsigned long long)(uv[5])) << 16) |
            (((unsigned long long)(uv[6])) << 8)  |
            uv[7]);

}



static int Mqtt_HandlePubAck(struct MqttContext *ctx, char flags,
                             char *pkt, unsigned int size)
{
    unsigned short pkt_id;

    if((0 != flags) || (2 != size)) {
        return MQTTERR_ILLEGAL_PKT;
    }

    pkt_id = Mqtt_RB16(pkt);
    if(0 == pkt_id) {
        return MQTTERR_ILLEGAL_PKT;
    }

    return ctx->handle_pub_ack(ctx->handle_pub_ack_arg, pkt_id);
}

static int Mqtt_HandlePubRec(struct MqttContext *ctx, char flags,
                             char *pkt, unsigned int size)
{
    unsigned short pkt_id;
    int err;

    if((0 != flags) || (2 != size)) {
        return MQTTERR_ILLEGAL_PKT;
    }

    pkt_id = Mqtt_RB16(pkt);
    if(0 == pkt_id) {
        return MQTTERR_ILLEGAL_PKT;
    }

    err = ctx->handle_pub_rec(ctx->handle_pub_rec_arg, pkt_id);
    if(err >= 0) {
        struct MqttBuffer response[1];
        MqttBuffer_Init(response);

        err = Mqtt_PackPubRelPkt(response, pkt_id);
        if(MQTTERR_NOERROR == err) {
            if(Mqtt_SendPkt(ctx, response, 0) != response->buffered_bytes) {
                err = MQTTERR_FAILED_SEND_RESPONSE;
            }
        }

        MqttBuffer_Destroy(response);
    }

    return err;
}

static int Mqtt_HandlePubComp(struct MqttContext *ctx, char flags,
                              char *pkt, unsigned int size)
{
    unsigned short pkt_id;

    if((0 != flags) || (2 != size)) {
        return MQTTERR_ILLEGAL_PKT;
    }

    pkt_id = Mqtt_RB16(pkt);
    if(0 == pkt_id) {
        return MQTTERR_ILLEGAL_PKT;
    }

    return ctx->handle_pub_comp(ctx->handle_pub_comp_arg, pkt_id);
}

static int Mqtt_HandleSubAck(struct MqttContext *ctx, char flags,
                             char *pkt, unsigned int size)
{
    unsigned short pkt_id;
    char *code;

    if((0 != flags) || (size < 2)) {
        return MQTTERR_ILLEGAL_PKT;
    }

    pkt_id = Mqtt_RB16(pkt);
    if(0 == pkt_id) {
        return MQTTERR_ILLEGAL_PKT;
    }

    for(code = pkt + 2; code < pkt + size; ++code ) {
        if(*code & 0x7C) {
            return MQTTERR_ILLEGAL_PKT;
        }
    }

    return ctx->handle_sub_ack(ctx->handle_sub_ack_arg, pkt_id, pkt + 2, size - 2);
}

static int Mqtt_HandleUnsubAck(struct MqttContext *ctx, char flags,
                               char *pkt, unsigned int size)
{
    unsigned short pkt_id;

    if((0 != flags) || (2 != size)) {
        return MQTTERR_ILLEGAL_PKT;
    }

    pkt_id = Mqtt_RB16(pkt);
    if(0 == pkt_id) {
        return MQTTERR_ILLEGAL_PKT;
    }

    return ctx->handle_unsub_ack(ctx->handle_unsub_ack_arg, pkt_id);
}

static int Mqtt_HandlePingResp(struct MqttContext *ctx, char flags,
                               char *pkt, unsigned int size)
{
    if((0 != flags) || (0 != size)) {
        return MQTTERR_ILLEGAL_PKT;
    }

    return ctx->handle_ping_resp(ctx->handle_ping_resp_arg);
}

static int Mqtt_HandlePublish(struct MqttContext *ctx, char flags,
                              char *pkt, unsigned int size)
{
    const char dup = flags & 0x08;
    const char qos = ((unsigned char)flags & 0x06) >> 1;
    const char retain = flags & 0x01;
    unsigned short topic_len = 0;
    unsigned short pkt_id = 0;
    unsigned int payload_len;
    char *payload;
    char *topic, *cursor;
    int err = MQTTERR_NOERROR;

    if(size < 2) {
        
        Ql_Debug_Trace("lebron:aaaaa\r\n");
        return MQTTERR_ILLEGAL_PKT;
    }

    if(retain) {
        
        Ql_Debug_Trace("lebron:bbbbb\r\n");
        return MQTTERR_ILLEGAL_PKT;
    }

    topic_len = Mqtt_RB16(pkt);
    Ql_Debug_Trace("size = %d,topic_len = %d\r\n",size,topic_len);
    if(size < (unsigned short)(2 + topic_len)) {
        
        Ql_Debug_Trace("lebron:cccccc\r\n");
        return MQTTERR_ILLEGAL_PKT;
    }

    switch(qos) {
    case MQTT_QOS_LEVEL0: // qos0 have no packet identifier
        if(0 != dup) {
            
            Ql_Debug_Trace("lebron:dddddd\r\n");
            return MQTTERR_ILLEGAL_PKT;
        }

        //Ql_memmove(pkt, pkt + 2, topic_len); // reuse the space to store null terminate
        topic = pkt+2;

        payload_len = size - 4 - topic_len;
        payload = pkt + 2 + topic_len;
        break;

    case MQTT_QOS_LEVEL1:
    case MQTT_QOS_LEVEL2:
        topic = pkt + 2;
        if(topic_len + 4 > size) {
            
            Ql_Debug_Trace("lebron:eeeee\r\n");
            return MQTTERR_ILLEGAL_PKT;
        }

        pkt_id = Mqtt_RB16(pkt + topic_len + 2);
        if(0 == pkt_id) {
            
            Ql_Debug_Trace("lebron:ffffff\r\n");
            return MQTTERR_ILLEGAL_PKT;
        }
        payload_len = size - 4 - topic_len;
        payload = pkt + 4 + topic_len;
        break;

    default:
        
        Ql_Debug_Trace("lebron:gggggg\r\n");
        return MQTTERR_ILLEGAL_PKT;
    }

    if(NULL == topic)
    {
        
        Ql_Debug_Trace("lebron:hhhhhh\r\n");
        return MQTTERR_ASSERT_ERROR;
    }
    topic[topic_len] = '\0';

    if(Mqtt_CheckUtf8(topic, topic_len) != topic_len) {
        
        Ql_Debug_Trace("lebron:iiiiii\r\n");
        return MQTTERR_ILLEGAL_PKT;
    }

    cursor = topic;
 #ifdef __ONENET_SOLUTION__ 
    while('\0' != *cursor) {

        if(('+' == *cursor) || ('#' == *cursor)) {
            
            return MQTTERR_ILLEGAL_PKT;
        }
        ++cursor;
    }

    if('$' == *topic) {
        if(topic == Ql_strstr(topic, CMD_TOPIC_PREFIX)) {
            const char *cmdid = topic + CMD_TOPIC_PREFIX_LEN; // skip the $CMDREQ
            char *arg = payload + 1;
            unsigned int arg_len = payload_len - 1;
            long long ts = 0;
            char *desc = "";

            if((payload_len < 1) || ((*payload & 0x1f) != 0x5)) {
                return MQTTERR_ILLEGAL_PKT;
            }

            if(*payload & 0x40) {
                if(arg_len < 8) {
                    return MQTTERR_ILLEGAL_PKT;
                }
                ts = (long long)Mqtt_RB64(arg);
                arg += 8;
                arg_len -= 8;
            }

            if(*payload & 0x20) {
                unsigned short desc_len;

                if(arg_len < 2) {
                    return MQTTERR_ILLEGAL_PKT;
                }

                desc_len = Mqtt_RB16(arg);
                if(arg_len < 2 + desc_len) {
                    return MQTTERR_ILLEGAL_PKT;
                }

                Ql_memmove(arg, arg + 2, desc_len);
                desc = arg;
                desc[desc_len] = '\0';

                arg += desc_len + 2;
                arg_len -= desc_len - 2;
            }

            err = ctx->handle_cmd(ctx->handle_cmd_arg, pkt_id, cmdid,
                                  ts, desc, arg, arg_len, dup,
                                  (enum MqttQosLevel)qos);
        }
    }
    else {
        err = ctx->handle_publish(ctx->handle_publish_arg, pkt_id, topic,
                                  payload, payload_len, dup,
                                  (enum MqttQosLevel)qos);
    }
#endif
#ifdef __GITWIZS_SOLUTION__ 

   err = ctx->handle_publish(ctx->handle_publish_arg, pkt_id, topic,
                                  payload, payload_len, dup,
                                  (enum MqttQosLevel)qos);
#endif
    // send the publish response.
    if(err >= 0) {
        struct MqttBuffer response[1];
        MqttBuffer_Init(response);

        switch(qos) {
        case MQTT_QOS_LEVEL2:
            
            Ql_Debug_Trace("lebron:22222\r\n");
            if(0 == pkt_id)
            {
                return MQTTERR_ASSERT_ERROR;
            }
            err = Mqtt_PackPubRecPkt(response, pkt_id);
            break;

        case MQTT_QOS_LEVEL1:
            
            Ql_Debug_Trace("lebron:333333\r\n");
            if(0 == pkt_id)
            {
                 return MQTTERR_ASSERT_ERROR;
            }
            err = Mqtt_PackPubAckPkt(response, pkt_id);
            break;

        default:
            Ql_Debug_Trace("lebron:1111\r\n");
            break;
        }

        if((MQTTERR_NOERROR == err) && (MQTT_QOS_LEVEL0 != qos)) {
            if(Mqtt_SendPkt(ctx, response, 0) != response->buffered_bytes) {
                err = MQTTERR_FAILED_SEND_RESPONSE;
            }
        }
        else {
            err = MQTTERR_NOERROR;
        }

        MqttBuffer_Destroy(response);
    }

    return err;
}

static int Mqtt_HandlePubRel(struct MqttContext *ctx, char flags,
                             char *pkt, unsigned int size)
{
    unsigned short pkt_id;
    int err;

    if((2 != flags) || (2 != size)) {
        return MQTTERR_ILLEGAL_PKT;
    }

    pkt_id = Mqtt_RB16(pkt);
    if(0 == pkt_id) {
        return MQTTERR_ILLEGAL_PKT;
    }

    err = ctx->handle_pub_rel(ctx->handle_pub_rel_arg, pkt_id);
    if(err >= 0) {
        struct MqttBuffer response[1];
        MqttBuffer_Init(response);
        err = Mqtt_PackPubCompPkt(response, pkt_id);
        if(MQTTERR_NOERROR == err) {
            if(Mqtt_SendPkt(ctx, response, 0) != response->buffered_bytes) {
                err = MQTTERR_FAILED_SEND_RESPONSE;
            }
        }
        MqttBuffer_Destroy(response);
    }

    return err;
}


static int Mqtt_Dispatch(struct MqttContext *ctx, char fh,  char *pkt, unsigned int size)
{
    const char flags = fh & 0x0F;

    switch(((unsigned char)fh) >> 4) {
    case MQTT_PKT_PINGRESP:
        return Mqtt_HandlePingResp(ctx, flags, pkt, size);

    case MQTT_PKT_CONNACK:
        return Mqtt_HandleConnAck(ctx, flags, pkt, size);

    case MQTT_PKT_PUBLISH:
        
        Ql_Debug_Trace("lebron:4444444\r\n");
        return Mqtt_HandlePublish(ctx, flags, pkt, size);

    case MQTT_PKT_PUBACK:
        
        Ql_Debug_Trace("lebron:555555\r\n");
        return Mqtt_HandlePubAck(ctx, flags, pkt, size);

    case MQTT_PKT_PUBREC:
        
        Ql_Debug_Trace("lebron:666666\r\n");
        return Mqtt_HandlePubRec(ctx, flags, pkt, size);

    case MQTT_PKT_PUBREL:
        
        Ql_Debug_Trace("lebron:777777\r\n");
        return Mqtt_HandlePubRel(ctx, flags, pkt, size);

    case MQTT_PKT_PUBCOMP:
        
        Ql_Debug_Trace("lebron:888888\r\n");
        return Mqtt_HandlePubComp(ctx, flags, pkt, size);

    case MQTT_PKT_SUBACK:
        return Mqtt_HandleSubAck(ctx, flags, pkt, size);

    case MQTT_PKT_UNSUBACK:
        return Mqtt_HandleUnsubAck(ctx, flags, pkt, size);

    default:
        
        Ql_Debug_Trace("lebron:999999\r\n");
        break;
    }
    
    Ql_Debug_Trace("lebron:00000\r\n");
    return MQTTERR_ILLEGAL_PKT;
}

/**
*Func : Init MQTT Context
*@param : ctx : the mqtt context
*@param : buf_size : recieve buffer size
*@return :refer to enum MqttError
**/


int Mqtt_InitContext(struct MqttContext *ctx, unsigned int buf_size)
{
    Ql_memset(ctx, 0, sizeof(*ctx));

    ctx->bgn = (char*)Ql_MEM_Alloc(buf_size);
    if(NULL == ctx->bgn) {
        return MQTTERR_OUTOFMEMORY;
    }
    Ql_memset(ctx->bgn,0,buf_size);

    ctx->end = ctx->bgn + buf_size;
    ctx->pos = ctx->bgn;

    return MQTTERR_NOERROR;
}


/**
*Func : Destroy MQTT Context
*@param : ctx : the mqtt context
*@return :refer to enum MqttError
**/

void Mqtt_DestroyContext(struct MqttContext *ctx)
{
    Ql_MEM_Free(ctx->bgn);
    Ql_memset(ctx, 0, sizeof(*ctx));
}

/**
*Func : Set packet dup flag
*@param : buf : the buf to save data packet
*@return :refer to enum MqttError
**/

int Mqtt_SetPktDup(struct MqttBuffer *buf)
{
    struct MqttExtent *fix_head = buf->first_ext;
    unsigned char pkt_type = ((unsigned char)buf->first_ext->payload[0]) >> 4;
    if(!fix_head || (MQTT_PKT_PUBLISH != pkt_type)) {
        return MQTTERR_INVALID_PARAMETER;
    }

    buf->first_ext->payload[0] |= 0x08;
    return MQTTERR_NOERROR;
}

/**
*Func : Append Subscribe Topic
*@param : buf : the buf to save data packet
*@param : topic : the topic want to append
*@param : qos   : the qos level when subscribe topic
*@return :refer to enum MqttError
**/


int Mqtt_AppendSubscribeTopic(struct MqttBuffer *buf, const char *topic, enum MqttQosLevel qos)
{
    struct MqttExtent *fixed_head = buf->first_ext;
    struct MqttExtent *ext;
    unsigned int topic_len;
    unsigned int remaining_len;
    char *cursor;
    int ret;
    const char sub_type = (char)(MQTT_PKT_SUBSCRIBE << 4 | 0x02);
    if(!fixed_head || (sub_type != fixed_head->payload[0]) || !topic) {
        return MQTTERR_INVALID_PARAMETER;
    }

    topic_len = Ql_strlen(topic);
    ext = MqttBuffer_AllocExtent(buf, topic_len + 3);
    if(!ext) {
        return MQTTERR_OUTOFMEMORY;
    }

    cursor = ext->payload;
    Mqtt_PktWriteString(&cursor, topic, topic_len);
    cursor[0] = qos;

    if(Mqtt_ReadLength(fixed_head->payload + 1, 4, &remaining_len) < 0) {
        return MQTTERR_INVALID_PARAMETER;
    }

    remaining_len += topic_len + 3;
    ret = Mqtt_DumpLength(remaining_len, fixed_head->payload + 1);
    if(ret < 0) {
        return MQTTERR_PKT_TOO_LARGE;
    }

    fixed_head->len = ret + 1;
    MqttBuffer_AppendExtent(buf, ext);
    return MQTTERR_NOERROR;
}


/**
*Func : Append UnSubscribe Topic
*@param : buf : the buf to save data packet
*@param : topic : the topic want to append
*@return :refer to enum MqttError
**/

int Mqtt_AppendUnsubscribeTopic(struct MqttBuffer *buf, const char *topic)
{
    struct MqttExtent *fixed_head = buf->first_ext;
    struct MqttExtent *ext;
    unsigned int topic_len;
    unsigned int remaining_len;
    char *cursor;
    int ret;
    const char unsub_type =(char)(MQTT_PKT_UNSUBSCRIBE << 4 | 0x02);
    if(!fixed_head || (unsub_type != fixed_head->payload[0]) || !topic) {
        return MQTTERR_INVALID_PARAMETER;
    }

    topic_len = Ql_strlen(topic);
    ext = MqttBuffer_AllocExtent(buf, topic_len + 2);
    if(!ext) {
        return MQTTERR_OUTOFMEMORY;
    }

    cursor = ext->payload;
    Mqtt_PktWriteString(&cursor, topic, topic_len);

    if(Mqtt_ReadLength(fixed_head->payload + 1, 4, &remaining_len) < 0) {
        return MQTTERR_INVALID_PARAMETER;
    }

    remaining_len += topic_len + 2;
    ret = Mqtt_DumpLength(remaining_len, fixed_head->payload + 1);
    if(ret < 0) {
        return MQTTERR_PKT_TOO_LARGE;
    }
    fixed_head->len = ret + 1;

    MqttBuffer_AppendExtent(buf, ext);
    return MQTTERR_NOERROR;
}



/**
*Func : Recieve MQTT Packet
*@param : ctx : the mqtt context
*@return :refer to enum MqttError
**/

int Mqtt_RecvPkt(struct MqttContext *ctx)
{
    int bytes;
    unsigned int remaining_len = 0;
    char *pkt, *cursor;
    int i;

    
    Ql_Debug_Trace("bgn = 0x%x\r\n",ctx->bgn);
    Ql_Debug_Trace("pos = 0x%x\r\n",ctx->pos);
    Ql_Debug_Trace("end = 0x%x\r\n",ctx->end);

    bytes = ctx->read_func(ctx->read_func_arg, ctx->pos, ctx->end - ctx->pos);

    Ql_Debug_Trace("bytes = %d\r\n",bytes);

    for(i = 0 ; i < bytes;i++)
    {
        Ql_Debug_Trace("0x%x ",(ctx->bgn)[i]);
    }
    Ql_Debug_Trace("\r\n");


    if(0 == bytes) {
        ctx->pos = ctx->bgn; // clear the buffer
        return MQTTERR_ENDOFFILE;
    }

    if(bytes < 0) {
        return MQTTERR_IO;
    }

    ctx->pos += bytes;
    if(ctx->pos > ctx->end) {
        return MQTTERR_BUF_OVERFLOW;
    }

    cursor = ctx->bgn;
    while(1) {
        int errcode;

        if(ctx->pos - cursor  < 2) {
            break;
        }

        bytes = Mqtt_ReadLength(cursor + 1, ctx->pos - cursor - 1, &remaining_len);

        if(-1 == bytes) {
            break;
        }
        else if(-2 == bytes) {
            return MQTTERR_ILLEGAL_PKT;
        }

        // one byte for the fixed header
        if(cursor + remaining_len + bytes + 1 > ctx->pos) {
            break;
        }

        pkt = cursor + bytes + 1; 
        Ql_Debug_Trace("remain_len = %d\r\n",remaining_len);
        errcode = Mqtt_Dispatch(ctx, cursor[0], pkt, remaining_len);
        if(errcode < 0) {
            return errcode;
        }
		
		cursor += bytes + 1 + remaining_len;
    }
   // Ql_memset(ctx->bgn,0,ctx->end-ctx->bgn);
   // ctx->pos = ctx->bgn;
#if 1
    if(cursor > ctx->bgn) {
        unsigned int movebytes = cursor - ctx->bgn;
        Ql_memmove(ctx->bgn, cursor, movebytes);
        ctx->pos -= movebytes;
        
        Ql_Debug_Trace("pos_after = 0x%x\r\n",ctx->pos);
        if(ctx->pos < ctx->bgn)
        {
            return MQTTERR_ASSERT_ERROR;
        }
    }
#endif
    return MQTTERR_NOERROR;
}

/**
*Func : Send MQTT Packet
*@param : ctx : the mqtt context
*@param : buf : the buf to save data packet
*@param : offset : the offset of buf start
*@return :refer to enum MqttError
**/
int Mqtt_SendPkt(struct MqttContext *ctx, const struct MqttBuffer *buf, unsigned int offset)
{
    const struct MqttExtent *cursor;
    const struct MqttExtent *first_ext;
    unsigned int bytes;
    int ext_count;
    int i;
    struct iovec *iov;

    if(offset >= buf->buffered_bytes) {
        return 0;
    }

    cursor = buf->first_ext;
    bytes = 0;
    
    while(cursor && bytes < offset) {
        bytes += cursor->len;
        cursor = cursor->next;
    }

    first_ext = cursor;
    ext_count = 0;
    for(; cursor; cursor = cursor->next) {
        ++ext_count;
    }

    if(0 == ext_count) {
        return 0;
    }

    if(NULL == first_ext)
    {
        return MQTTERR_ASSERT_ERROR;
    }

    iov = (struct iovec*)Ql_MEM_Alloc(sizeof(struct iovec) * ext_count);
    if(!iov) {
        return MQTTERR_OUTOFMEMORY;
    }
 
    iov[0].iov_base = first_ext->payload + (offset - bytes);
    iov[0].iov_len = first_ext->len - (offset - bytes);
    

    i = 1;
    for(cursor = first_ext->next; cursor; cursor = cursor->next) {
        iov[i].iov_base = cursor->payload;
        iov[i].iov_len = cursor->len;
        ++i;
    }
    

    i = ctx->writev_func(ctx->writev_func_arg, iov, ext_count);
    Ql_MEM_Free(iov);

    return i;
}




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
                        const char *password, unsigned short pswd_len)
{
     struct MqttExtent *fix_head, *variable_head, *payload;
     unsigned short variable_head_len;
     unsigned int total_len;
     unsigned short id_len;
     char flags = 0;
     unsigned short wt_len;
     unsigned short user_len;
     int ret;
     char *cursor;
     
     
     fix_head = MqttBuffer_AllocExtent(buf, 5);
     if(NULL == fix_head) 
     {
        return MQTTERR_OUTOFMEMORY;
     }
 #ifdef MQTT_PROTOCOL_V4
     variable_head_len = 10 ;
 #endif

 #ifdef MQTT_PROTOCOL_V3
     variable_head_len = 12 ;
 #endif

     variable_head = MqttBuffer_AllocExtent(buf, variable_head_len);
     if(NULL == variable_head) 
     {
          return MQTTERR_OUTOFMEMORY;
     }

     total_len = variable_head_len; // length of the variable header
     id_len = Mqtt_CheckClentIdentifier(id);
     if(id_len < 0) 
     {
         return MQTTERR_ILLEGAL_CHARACTER;
     }
     total_len += id_len + 2;
     if(clean_session) 
     {
        flags |= MQTT_CONNECT_CLEAN_SESSION;
     }

     if(will_msg && !will_topic) 
     {
        return MQTTERR_INVALID_PARAMETER;
     }
     wt_len = 0;
     if(will_topic) 
     {
        flags |= MQTT_CONNECT_WILL_FLAG;
        wt_len = Ql_strlen(will_topic);
        if(Mqtt_CheckUtf8(will_topic, wt_len) != wt_len) {
            
            return MQTTERR_NOT_UTF8;
        }
     }

      switch(qos) {
        case MQTT_QOS_LEVEL0:
            flags |= MQTT_CONNECT_WILL_QOS0;
            break;
        case MQTT_QOS_LEVEL1:
            flags |= (MQTT_CONNECT_WILL_FLAG | MQTT_CONNECT_WILL_QOS1);
            break;
        case MQTT_QOS_LEVEL2:
            flags |= (MQTT_CONNECT_WILL_FLAG | MQTT_CONNECT_WILL_QOS2);
            break;
        default:
            return MQTTERR_INVALID_PARAMETER;
        }

      if(will_retain) {
        flags |= (MQTT_CONNECT_WILL_FLAG | MQTT_CONNECT_WILL_RETAIN);
      }

      if(flags & MQTT_CONNECT_WILL_FLAG) {
        total_len += 4 + wt_len + msg_len;
      }

      if(!user && password) {
        
        return MQTTERR_INVALID_PARAMETER;
      }

      user_len = 0;
      if(user) {
        flags |= MQTT_CONNECT_USER_NAME;
        user_len = Ql_strlen(user);
        ret = Mqtt_CheckUtf8(user, user_len);
        if(user_len != ret) {
            
            return MQTTERR_NOT_UTF8;
        }

        total_len += user_len + 2;
      }

    if(password) {
        flags |= MQTT_CONNECT_PASSORD;
        total_len += pswd_len + 2;
    }
    payload = MqttBuffer_AllocExtent(buf, total_len - variable_head_len);

    fix_head->payload[0] = MQTT_PKT_CONNECT << 4;

    ret = Mqtt_DumpLength(total_len, fix_head->payload + 1);
    if(ret < 0) {
        return MQTTERR_PKT_TOO_LARGE;
    }
    fix_head->len = ret + 1; // ajust the length of the extent
    
#ifdef MQTT_PROTOCOL_V4
    variable_head->payload[0] = 0;
    variable_head->payload[1] = 4;
    variable_head->payload[2] = 'M';
    variable_head->payload[3] = 'Q';
    variable_head->payload[4] = 'T';
    variable_head->payload[5] = 'T';
    variable_head->payload[6] = 4; // protocol level 4
    variable_head->payload[7] = flags;
    Mqtt_WB16(keep_alive, variable_head->payload + 8);
#endif

#ifdef MQTT_PROTOCOL_V3
    variable_head->payload[0] = 0;
    variable_head->payload[1] = 6;
    variable_head->payload[2] = 'M';
    variable_head->payload[3] = 'Q';
    variable_head->payload[4] = 'I';
    variable_head->payload[5] = 's';
    variable_head->payload[6] = 'd';
    variable_head->payload[7] = 'p';
    variable_head->payload[8] = 3; // protocol level 3
    variable_head->payload[9] = flags;
    Mqtt_WB16(keep_alive, variable_head->payload + 10);
#endif

    cursor = payload->payload;
    Mqtt_PktWriteString(&cursor, id, id_len);

    if(flags & MQTT_CONNECT_WILL_FLAG) {
        if(!will_msg) {
            will_msg = "";
            msg_len = 0;
        }

        Mqtt_PktWriteString(&cursor, will_topic, wt_len);
        Mqtt_PktWriteString(&cursor, will_msg, msg_len);
    }

    if(flags & MQTT_CONNECT_USER_NAME) {
           Mqtt_PktWriteString(&cursor, user, user_len);
    }
    
    if(flags & MQTT_CONNECT_PASSORD) {
           Mqtt_PktWriteString(&cursor, password, pswd_len);
    }

    MqttBuffer_AppendExtent(buf, fix_head);
    MqttBuffer_AppendExtent(buf, variable_head);
    MqttBuffer_AppendExtent(buf, payload);

    return MQTTERR_NOERROR;

}


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
                        enum MqttQosLevel qos, int retain, int own)
{
    int ret;
    unsigned int topic_len, total_len;
    struct MqttExtent *fix_head, *variable_head;
    char *cursor;

    if(0 == pkt_id) {
        return MQTTERR_INVALID_PARAMETER;
    }

    for(topic_len = 0; '\0' != topic[topic_len]; ++topic_len) {
        #ifdef __ONENET_SOLUTION__
        if(('#' == topic[topic_len]) || ('+' == topic[topic_len])) {
            return MQTTERR_INVALID_PARAMETER;
        }
        #endif
    }

    if(Mqtt_CheckUtf8(topic, topic_len) != topic_len) {
        return MQTTERR_NOT_UTF8;
    }

    fix_head = MqttBuffer_AllocExtent(buf, 5);
    if(NULL == fix_head) {
        return MQTTERR_OUTOFMEMORY;
    }

    fix_head->payload[0] = MQTT_PKT_PUBLISH << 4;

    if(retain) {
        fix_head->payload[0] |= 0x01;
    }

    total_len = topic_len + size + 2;
    switch(qos) {
    case MQTT_QOS_LEVEL0:
        break;
    case MQTT_QOS_LEVEL1:
        fix_head->payload[0] |= 0x02;
        total_len += 2;
        break;
    case MQTT_QOS_LEVEL2:
        fix_head->payload[0] |= 0x04;
        total_len += 2;
        break;
    default:
        return MQTTERR_INVALID_PARAMETER;
    }

    ret = Mqtt_DumpLength(total_len, fix_head->payload + 1);
    if(ret < 0) {
        return MQTTERR_PKT_TOO_LARGE;
    }
    fix_head->len = ret + 1;

    variable_head = MqttBuffer_AllocExtent(buf, total_len - size);
    if(NULL == variable_head) {
        return MQTTERR_OUTOFMEMORY;
    }
    cursor = variable_head->payload;

    Mqtt_PktWriteString(&cursor, topic, topic_len);
    if(MQTT_QOS_LEVEL0 != qos) {
        Mqtt_WB16(pkt_id, cursor);
    }

    MqttBuffer_AppendExtent(buf, fix_head);
    MqttBuffer_AppendExtent(buf, variable_head);
    if(0 != size) {
        MqttBuffer_Append(buf, (char*)payload, size, own);
    }
    
    return MQTTERR_NOERROR;
}

/**
*Func:Packing MQTT Pubrelease Packet
*@param buf : the buf to save data packet
*@param pkt_id : packet id ,not zero
*@return refer to enum MqttError
**/

int Mqtt_PackPubRelPkt(struct MqttBuffer *buf, unsigned short pkt_id)
{
    struct MqttExtent *ext;

    if(0 == pkt_id) {
        return MQTTERR_INVALID_PARAMETER;
    }

    ext = MqttBuffer_AllocExtent(buf, 4);
    ext->payload[0]= MQTT_PKT_PUBREL << 4 | 0x02;
    ext->payload[1] = 2;
    Mqtt_WB16(pkt_id, ext->payload + 2);
    MqttBuffer_AppendExtent(buf, ext);

    return MQTTERR_NOERROR;
}

/**
*Func:Packing MQTT Subscribe Packet
*@param buf : the buf to save data packet
*@param pkt_id : packet id ,not zero
*@param topic : the topic subscribe
*@param qos : the level of quality of service
*@return refer to enum MqttError
**/

int Mqtt_PackSubscribePkt(struct MqttBuffer *buf, unsigned short pkt_id,
                          const char *topic, enum MqttQosLevel qos)
{
    int ret;
    int i;
    unsigned int topic_len, remaining_len;
    struct MqttExtent *fixed_head, *ext;
    char *cursor;

    if((0 == pkt_id) || (NULL == topic)) {
        return MQTTERR_INVALID_PARAMETER;
    }

    topic_len = Ql_strlen(topic);
    if(Mqtt_CheckUtf8(topic, topic_len) != topic_len) {
        return MQTTERR_NOT_UTF8;
    }

    fixed_head = MqttBuffer_AllocExtent(buf, 5);
    if(NULL == fixed_head) {
        return MQTTERR_OUTOFMEMORY;
    }
    fixed_head->payload[0] = (char)((MQTT_PKT_SUBSCRIBE << 4) | 0x02);

    remaining_len = 5 + topic_len;  // 2 bytes packet id, 1 bytes qos and topic length
    ext = MqttBuffer_AllocExtent(buf, remaining_len);
    if(NULL == ext) {
        return MQTTERR_OUTOFMEMORY;
    }

    ret = Mqtt_DumpLength(remaining_len, fixed_head->payload + 1);
    if(ret < 0) {
        return MQTTERR_PKT_TOO_LARGE;
    }
    fixed_head->len = ret + 1;

    cursor = ext->payload;
    Mqtt_WB16(pkt_id, cursor);
    cursor += 2;

    Mqtt_PktWriteString(&cursor, topic, topic_len);
    cursor[0] = qos;

    MqttBuffer_AppendExtent(buf, fixed_head);
    MqttBuffer_AppendExtent(buf, ext);

    return MQTTERR_NOERROR;
}

/**
*Func:Packing MQTT Unsubscribe Packet
*@param buf : the buf to save data packet
*@param pkt_id : packet id ,not zero
*@param topic : the topic to unsubscribe
*@return refer to enum MqttError
**/

int Mqtt_PackUnsubscribePkt(struct MqttBuffer *buf, unsigned short pkt_id, const char *topic)
{
    struct MqttExtent *fixed_head, *ext;
    unsigned int topic_len;
    unsigned int  remaining_len;
    char *cursor;
    int ret;

    if((0 == pkt_id) || !topic) {
        return MQTTERR_INVALID_PARAMETER;
    }

    topic_len = Ql_strlen(topic);
    if(Mqtt_CheckUtf8(topic, topic_len) != topic_len) {
        return MQTTERR_NOT_UTF8;
    }
    remaining_len = topic_len + 4; // 2 bytes for packet id

    fixed_head = MqttBuffer_AllocExtent(buf, 5);
    if(!fixed_head) {
        return MQTTERR_OUTOFMEMORY;
    }

    fixed_head->payload[0] = (char)(MQTT_PKT_UNSUBSCRIBE << 4 | 0x02);
    ret = Mqtt_DumpLength(remaining_len, fixed_head->payload + 1);
    if(ret < 0) {
        return MQTTERR_PKT_TOO_LARGE;
    }
    fixed_head->len = ret + 1;

    ext = MqttBuffer_AllocExtent(buf, remaining_len);
    if(!ext) {
        return MQTTERR_OUTOFMEMORY;
    }

    cursor = ext->payload;
    Mqtt_WB16(pkt_id, cursor);
    cursor += 2;

    Mqtt_PktWriteString(&cursor, topic, topic_len);
    MqttBuffer_AppendExtent(buf, fixed_head);
    MqttBuffer_AppendExtent(buf, ext);

    return MQTTERR_NOERROR;
}

/**
*Func:Packing MQTT Ping Packet
*@param buf : the buf to save data packet
*@return refer to enum MqttError
**/

int Mqtt_PackPingReqPkt(struct MqttBuffer *buf)
{
    struct MqttExtent *ext = MqttBuffer_AllocExtent(buf, 2);
    if(!ext) {
        return MQTTERR_OUTOFMEMORY;
    }

    ext->payload[0] = (char)(MQTT_PKT_PINGREQ << 4);
    ext->payload[1] = 0;
    MqttBuffer_AppendExtent(buf, ext);

    return MQTTERR_NOERROR;
}

/**
*Func:Packing MQTT Disconnect Packet
*@param buf : the buf to save data packet
*@return refer to enum MqttError
**/

int Mqtt_PackDisconnectPkt(struct MqttBuffer *buf)
{
    struct MqttExtent *ext = MqttBuffer_AllocExtent(buf, 2);
    if(!ext) {
        return MQTTERR_OUTOFMEMORY;
    }

    ext->payload[0] = (char)(MQTT_PKT_DISCONNECT << 4);
    ext->payload[1] = 0;
    MqttBuffer_AppendExtent(buf, ext);

    return MQTTERR_NOERROR;
}

/**
*Func:Packing MQTT Pubrecieve Packet
*@param buf : the buf to save data packet
*@param pkt_id : the packet id
*@return refer to enum MqttError
**/

int Mqtt_PackPubRecPkt(struct MqttBuffer *buf, unsigned short pkt_id)
{
    struct MqttExtent *ext;

    if(0 == pkt_id) {
        return MQTTERR_INVALID_PARAMETER;
    }

    ext = MqttBuffer_AllocExtent(buf, 4);
    if(!ext) {
        return MQTTERR_OUTOFMEMORY;
    }

    ext->payload[0]= MQTT_PKT_PUBREC << 4;
    ext->payload[1] = 2;
    Mqtt_WB16(pkt_id, ext->payload + 2);
    MqttBuffer_AppendExtent(buf, ext);

    return MQTTERR_NOERROR;
}

/**
*Func:Packing MQTT Puback Packet
*@param buf : the buf to save data packet
*@param pkt_id : the packet id
*@return refer to enum MqttError
**/

int Mqtt_PackPubAckPkt(struct MqttBuffer *buf, unsigned short pkt_id)
{
    struct MqttExtent *ext;

    if(0 == pkt_id)  {
        return MQTTERR_INVALID_PARAMETER;
    }

    ext = MqttBuffer_AllocExtent(buf, 4);
    if(!ext) {
        return MQTTERR_OUTOFMEMORY;
    }

    ext->payload[0]= MQTT_PKT_PUBACK << 4;
    ext->payload[1] = 2;
    Mqtt_WB16(pkt_id, ext->payload + 2);
    MqttBuffer_AppendExtent(buf, ext);

    return MQTTERR_NOERROR;
}

/**
*Func:Packing MQTT Pubcomplish Packet
*@param buf : the buf to save data packet
*@param pkt_id : the packet id
*@return refer to enum MqttError
**/


int Mqtt_PackPubCompPkt(struct MqttBuffer *buf, unsigned short pkt_id)
{
    struct MqttExtent *ext;

    if(0 == pkt_id) {
        return MQTTERR_INVALID_PARAMETER;
    }

    ext = MqttBuffer_AllocExtent(buf, 4);
    if(!ext) {
        return MQTTERR_OUTOFMEMORY;
    }

    ext->payload[0]= MQTT_PKT_PUBCOMP << 4;
    ext->payload[1] = 2;
    Mqtt_WB16(pkt_id, ext->payload + 2);
    MqttBuffer_AppendExtent(buf, ext);

    return MQTTERR_NOERROR;
}



#endif

