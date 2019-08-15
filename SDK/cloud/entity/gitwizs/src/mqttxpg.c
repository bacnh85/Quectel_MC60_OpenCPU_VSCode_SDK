#include "custom_feature_def.h"
#ifdef __OCPU_SMART_CLOUD_SUPPORT__

#include "mqttxpg.h"
#include "adapter.h"
#include "cloud.h"
#include "utils.h"


struct MqttGizwitContext gw_ctx[1];
extern s32 g_cloud_task_id;
extern s32 g_main_task_id;




int Mqttxpg_HandlePingResp(void *arg)
{
    
    Adapter_SendMsg(g_main_task_id,MSG_ID_CLOUD_PING_RSP_OK,NULL,NULL);
    return 0;
}

int Mqttxpg_HandleConnAck(void *arg, char flags, char ret_code)
{
   if(ret_code == 0x00)
   {
      PGC->rtinfo.m2m_result = 0;
      
      Ql_Debug_Trace("%s %d\r\n",__FUNCTION__,__LINE__);
   }
   else
   {
      PGC->rtinfo.m2m_result = -1;
      
      Ql_Debug_Trace("%s %d\r\n",__FUNCTION__,__LINE__);
   }
   Adapter_SendMsg(g_cloud_task_id,MSG_ID_MQTT_READY,NULL,NULL);
  // Adapter_Memcpy(PGC->rtinfo.Cloud_Rxbuf->phead,gw_ctx->mqttctx->bgn,gw_ctx->mqttctx->pos-gw_ctx->mqttctx->bgn);
   return ret_code;
}

unsigned char mqtt_num_rem_len_bytes(const unsigned char* buf)
{
    unsigned char num_bytes = 1;

    if ((buf[1] & 0x80) == 0x80) {
        num_bytes++;
        if ((buf[2] & 0x80) == 0x80) {
            num_bytes ++;
            if ((buf[3] & 0x80) == 0x80) {
                num_bytes ++;
            }
        }
    }
    return num_bytes;
}


int Mqttxpg_HandlePublish(void *arg, unsigned char pkt_id, const char *topic,
                         const char *payload, unsigned int payloadsize,
                         int dup, enum MqttQosLevel qos)
{
    //int bytes ;
    unsigned char varlen=0;
    unsigned char*pTemp;
    unsigned short cmd;
    unsigned short *pcmd=NULL;
    int i,sn,j;
    unsigned char  clientid[PHONECLIENTID + 1];

  
 
     struct MqttGizwitContext * param =(struct MqttGizwitContext *)arg;

     resetPacket( PGC->rtinfo.Cloud_Rxbuf );
     Adapter_Memcpy(PGC->rtinfo.Cloud_Rxbuf->phead,param->mqttctx->bgn,param->mqttctx->pos - param->mqttctx->bgn);
     PGC->rtinfo.m2m_recvlen = param->mqttctx->pos - param->mqttctx->bgn;
     
     Ql_Debug_Trace("m2m_recvlen = %d\r\n",PGC->rtinfo.m2m_recvlen);

     for(i = 0 ;i < PGC->rtinfo.m2m_recvlen; i++)
     {
        
        Ql_Debug_Trace("0x%x ",(PGC->rtinfo.Cloud_Rxbuf->phead)[i]);
     }
     
     Ql_Debug_Trace("\r\n");
     Adapter_SendMsg(g_cloud_task_id,MSG_ID_MQTT_READY,NULL,NULL);
    

         return 0;
     
}

int Mqttxpg_HandlePubAck(void *arg, unsigned short pkt_id)
{
    return 0;
}

int Mqttxpg_HandlePubRec(void *arg, unsigned short pkt_id)
{
    return 0;
}

int Mqttxpg_HandlePubRel(void *arg, unsigned short pkt_id)
{
    return 0;
}

int Mqttxpg_HandlePubComp(void *arg, unsigned short pkt_id)
{
    return 0;
}

int Mqttxpg_HandleSubAck(void *arg, unsigned short pkt_id,
                          const char *codes, unsigned int count)
{
   int ret ;
   
   if(PGC->rtinfo.waninfo.mqttMsgsubid == pkt_id)
   {
      PGC->rtinfo.m2m_result = 0;
      Ql_Debug_Trace("%s %d\r\n",__FUNCTION__,__LINE__);
      ret = 0;
   }
   else
   {
      PGC->rtinfo.m2m_result = -1;
      
      Ql_Debug_Trace("%s %d\r\n",__FUNCTION__,__LINE__);
      ret = -1;
   }
   Adapter_SendMsg(g_cloud_task_id,MSG_ID_MQTT_READY,NULL,NULL);

   return ret;
}


int Mqttxpg_HandleUnsubAck(void *arg, unsigned short packet_id)
{
    return 0;
}

int Mqttxpg_HandleCmd(void *arg, unsigned short pkt_id, const char *cmdid,
                      long timestamp, const char *desc, const char *cmdarg,
                      unsigned int cmdarg_len, int dup, enum MqttQosLevel qos)
{
    return 0;
}






static int init_subTopic(pgcontext pgc,char *Sub_TopicBuf,int Mqtt_Subflag)
{
    int productKeyLen=0,DIdLen=0;
    
    DIdLen = Adapter_Strlen( pgc->gc.DID );
    switch(Mqtt_Subflag)
    {
    case 1:
        //4.6
        productKeyLen = Adapter_Strlen( (const char *)pgc->mcu.product_key );
        Adapter_Memcpy( Sub_TopicBuf,"ser2cli_noti/",Adapter_Strlen("ser2cli_noti/") );
        Adapter_Memcpy( Sub_TopicBuf+Adapter_Strlen("ser2cli_noti/"),pgc->mcu.product_key,productKeyLen );
        Sub_TopicBuf[Adapter_Strlen("ser2cli_noti/")+productKeyLen] = '\0';
        break;
    case 2:
        // 4.7 4.9
        Adapter_Memcpy( Sub_TopicBuf,"ser2cli_res/",Adapter_Strlen("ser2cli_res/") );
        Adapter_Memcpy( Sub_TopicBuf+Adapter_Strlen("ser2cli_res/"),pgc->gc.DID,DIdLen );
        Sub_TopicBuf[Adapter_Strlen("ser2cli_res/")+DIdLen] = '\0';
        break;
    case 3:
        // 4.13
        Adapter_Memcpy(Sub_TopicBuf,"app2dev/",Adapter_Strlen("app2dev/") );
        Adapter_Memcpy(Sub_TopicBuf+Adapter_Strlen("app2dev/"),pgc->gc.DID,DIdLen );
        Sub_TopicBuf[Adapter_Strlen("app2dev/")+DIdLen] = '/';
        Sub_TopicBuf[Adapter_Strlen("app2dev/")+DIdLen+1] = '#';
        Sub_TopicBuf[Adapter_Strlen("app2dev/")+DIdLen+2] = '\0';
        break;
    default:
        break;
    }

    return 0;
}


int Mqtt_DoSubTopic( pgcontext pgc,short mqttstatus )
{
    char ret =0;
    ret = Mqtt_SubLoginTopic(pgc,mqttstatus );
    return ret;
}

int Mqtt_SubLoginTopic(pgcontext pgc,short mqttstatus )
{
    char Sub_TopicBuf[100];
    char Topic[100];
    int ret;

    Adapter_Memset(Sub_TopicBuf,0,100);
    Adapter_Memset(Topic,0,100);
    MqttBuffer_Init(gw_ctx->mqttbuf);
    switch(mqttstatus)
    {
    case MQTT_STATUS_REQ_LOGINTOPIC1:
        init_subTopic(pgc,Sub_TopicBuf,1);
        pgc->rtinfo.waninfo.mqttMsgsubid ++;
        if(Mqtt_PackSubscribePkt(gw_ctx->mqttbuf,pgc->rtinfo.waninfo.mqttMsgsubid,Sub_TopicBuf, MQTT_QOS_LEVEL1)==0)
        {   
            Ql_sprintf(Topic,"LOGIN sub topic1 is:%s",Sub_TopicBuf);
            APP_DEBUG("topic1:%s\r\n",Topic);
            APP_DEBUG("MQTT_STATUS_LOGINTOPIC1\r\n");
        }
        break;            
    case MQTT_STATUS_REQ_LOGINTOPIC2:
        init_subTopic(pgc,Sub_TopicBuf,2);
        pgc->rtinfo.waninfo.mqttMsgsubid ++;
        if(Mqtt_PackSubscribePkt(gw_ctx->mqttbuf,pgc->rtinfo.waninfo.mqttMsgsubid,Sub_TopicBuf, MQTT_QOS_LEVEL1 )==0)
        {    
            Ql_sprintf(Topic,"LOGIN T2 sub topic is:%s",Sub_TopicBuf);                                
            APP_DEBUG("topic2:%s\r\n",Topic);
            APP_DEBUG("MQTT_STATUS_LOGINTOPIC2\r\n");

        }
        break;
    case MQTT_STATUS_REQ_LOGINTOPIC3:
        init_subTopic(pgc,Sub_TopicBuf,3);
        pgc->rtinfo.waninfo.mqttMsgsubid ++;
        if(Mqtt_PackSubscribePkt(gw_ctx->mqttbuf,pgc->rtinfo.waninfo.mqttMsgsubid,Sub_TopicBuf, MQTT_QOS_LEVEL1 )==0)
        {
            Ql_sprintf(Topic,"LOGIN T3 sub topic is:%s",Sub_TopicBuf);
            APP_DEBUG("topic3:%s\r\n",Topic);
            APP_DEBUG("MQTT_STATUS_LOGINTOPIC3\r\n");

        }
        break;
    default:
        break;
    }
    ret = Mqtt_SendPkt(gw_ctx->mqttctx,gw_ctx->mqttbuf ,0);
    
    if (ret < 0)
    {
        APP_DEBUG("mqtt send suscribe packet is failed with:%d\r\n", ret); 
        return -1;
    }
    
    APP_DEBUG("Mqtt_SendSubscribePacket OK, write:%d\r\n", ret); 
    MqttBuffer_Destroy(gw_ctx->mqttbuf);
    return 0;
}


int Mqtt_Login2Server(int socketid,const unsigned char *username,const const unsigned char *password )
{
    if( Mqtt_SendConnetPacket(socketid,(const char*)username,(const char*)password ) == 0)
    {    
        APP_DEBUG(" Mqtt_SendConnetPacket OK!");
        return 0;
    }   
    APP_DEBUG(" Mqtt_SendConnetPacket NO!");    
    
    return -1;
}

void MQTT_HeartbeatTime( void )
{
    //mqtt_ping(&g_stMQTTBroker);
    int ret;
    
    MqttBuffer_Init(gw_ctx->mqttbuf);
    ret = Mqtt_PackPingReqPkt(gw_ctx->mqttbuf);
    if (ret < 0)
    {
        APP_DEBUG("mqtt pack ping packet is failed with:%d\r\n", ret); 
        return -1;
    }

    ret = Mqtt_SendPkt(gw_ctx->mqttctx,gw_ctx->mqttbuf ,0);
    
    if (ret < 0)
    {
        APP_DEBUG("mqtt send ping packet is failed with:%d\r\n", ret); 
        return -1;
    }
    
    APP_DEBUG("MQTT_HeartbeatTime OK, write:%d\r\n", ret); 
    
    MqttBuffer_Destroy(gw_ctx->mqttbuf);
}


/********************************************************************
        Function        :   MQTT_SenData.
        Description     :   send buf data to m2m.
        szDID           :   Device ID.
        buf             :   need to send data pointer.
        buflen          :   buf length .
        return          :   0-ok 
                            other fail.
********************************************************************/
int MQTT_SenData( pgcontext pgc, char *szDID, ppacket pbuf,/*uint8 *buf,*/ int buflen )
{
    unsigned char *sendpack=NULL;
    int i=0,sendpacklen=0,headlen=0;
    short varlen=0;
    volatile varc sendvarc;
    char msgtopic[64]= {0};
    int didlen=0;
    unsigned short cmd = pgc->rtinfo.stChannelAttrs.cloudClient.cmd;
    int sn = pgc->rtinfo.stChannelAttrs.cloudClient.sn;
    char *clienid = pgc->rtinfo.stChannelAttrs.cloudClient.phoneClientId;
    unsigned char pos = 0;
    int clienIdLen;
    int ret;

    
    didlen = Adapter_Strlen(szDID);
    if(didlen!=22)
        return -1;
    
    Adapter_Memcpy( msgtopic,"dev2app/",Adapter_Strlen("dev2app/"));
    pos += Adapter_Strlen("dev2app/");
    Adapter_Memcpy( msgtopic+Adapter_Strlen("dev2app/"),szDID,didlen );
    pos += didlen;
    if(0x0094 == cmd)
    {
        clienIdLen = Adapter_Strlen( (const char *)clienid);
        if(clienIdLen > 0)
        {
            msgtopic[pos] = '/';
            pos++;
            Adapter_Memcpy( msgtopic+pos,clienid,clienIdLen );
            pos+=clienIdLen;
        }
    }
    msgtopic[pos] = '\0';
    
    //protocolVer(4B)+varLen(1~4B)+flag(1B)+cmd(2B)+P0
    varlen = 1+2+buflen;
    if(0x0093 == cmd || 0x0094 == cmd)
    {
        varlen += sizeof(sn);
    }
    sendvarc=Tran2varc(varlen);
    sendpacklen = 4+sendvarc.varcbty+varlen;
    headlen = sendpacklen-buflen;
    
    sendpack = ( (pbuf->ppayload)-headlen );
    //protocolVer
    sendpack[0] = 0x00;
    sendpack[1] = 0x00;
    sendpack[2] = 0x00;
    sendpack[3] = 0x03;
    //varLen
    for(i=0;i<sendvarc.varcbty;i++)
    {
        sendpack[4+i] = sendvarc.var[i];
    }
     //flag   
    sendpack[4+sendvarc.varcbty] = 0x00;
    //CMD
   // sendpack[4+sendvarc.varcbty+1] = (unsigned char)(cmd & 0xff00);
    //sendpack[4+sendvarc.varcbty+2] = (unsigned char)(cmd & 0x00ff);
    *(unsigned short *)&sendpack[4+sendvarc.varcbty+1] = EndianConvertLToB(cmd);
    if(0x0093 == cmd || 0x0094 == cmd)
    {
        *(int *)&sendpack[4+sendvarc.varcbty+1 + 2] = EndianConvertLToB_Long(sn);
    }

    APP_DEBUG("------SEND TO Cloud ------\r\n");
    for(i=0;i<sendpacklen;i++)
        APP_DEBUG(" %02X",sendpack[i] );
    APP_DEBUG(" \r\n");
    MqttBuffer_Init(gw_ctx->mqttbuf);
    //PubMsg( &g_stMQTTBroker,msgtopic,(char *)sendpack,sendpacklen,0 );
    
    pgc->rtinfo.waninfo.mqttMsgsubid ++;
    ret = Mqtt_PackPublishPkt(gw_ctx->mqttbuf, pgc->rtinfo.waninfo.mqttMsgsubid, msgtopic,
                        (const char *)sendpack, sendpacklen,
                        MQTT_QOS_LEVEL0, 0,0);
    if (ret < 0)
    {
       APP_DEBUG("mqtt pack publish packet is failed with:%d\r\n", ret); 
       return -1;
    }
    ret = Mqtt_SendPkt(gw_ctx->mqttctx,gw_ctx->mqttbuf ,0);
       
    if (ret < 0)
    {
        APP_DEBUG("mqtt send publish packet is failed with:%d\r\n", ret); 
        return -1;
    }

    
    MqttBuffer_Destroy(gw_ctx->mqttbuf);
    return 0;
}


/***********************************************************
 *
 *   return :    0 success ,-1 error
 *
 *************************************************************/
int Mqtt_SendConnetPacket(int socketid,const char* username,const char* password )
{       
    int ret;
    
    if( (username==NULL) || (password==NULL) )// 匿名登录 GAgent V4 will not runing this code.
    {
        return -1;
    }
    gw_ctx->mqttfd = socketid;
    
    MqttBuffer_Init(gw_ctx->mqttbuf);

    ret = Mqtt_PackConnectPkt(gw_ctx->mqttbuf, CLOUD_MQTT_SET_ALIVE, username, 1, "WillTopic", "will message", 17,
                              MQTT_QOS_LEVEL0, 0, username,
                              password, Ql_strlen(password));
    if (ret < 0)
    {
        APP_DEBUG("mqtt pack connect packet is failed with:%d\r\n", ret); 
        return -1;
    }

    //consider send imcompletely
    ret = Mqtt_SendPkt(gw_ctx->mqttctx,gw_ctx->mqttbuf ,0);
    
    if (ret < 0)
    {
        APP_DEBUG("mqtt send connect packet is failed with:%d\r\n", ret); 
        return -1;
    }
    
    APP_DEBUG("Mqtt_SendConnetPacket OK, write:%d\r\n", ret); 
    MqttBuffer_Destroy(gw_ctx->mqttbuf);
    return 0;
}


int MQTTclose_socket(void)
{
    Adapter_SOC_Close(gw_ctx->mqttfd);
    gw_ctx->mqttfd = -1;
    return 0;
}

/********************************************************************
 *
 *  FUNCTION   : Mqtt send request packbag to server ask online client
 *               res.
 *   buf       : mqtt msg payload.
 *   return    : none.
 *             add by alex.lin --2014-12-11
 ********************************************************************/
void Mqtt_ResOnlineClient( pgcontext pgc,char *buf,int buflen)
{
    u16 *pWanclient_num;
    u16 wanclient_num=0;
    u16 lanclient_num=0;

    pWanclient_num = (u16*)&buf[6];
    wanclient_num = EndianConvertLToB( *pWanclient_num );
    pgc->rtinfo.waninfo.wanclient_num = wanclient_num;
  //  lanclient_num = pgc->ls.tcpClientNums;
#if 0
    if( 0 != wanclient_num)
    {
        GAgent_SetWiFiStatus( pgc,WIFI_CLIENT_ON,1 );
    }
    else if(0 == (wanclient_num + lanclient_num))
    {
        GAgent_SetWiFiStatus( pgc,WIFI_CLIENT_ON,0 );
    }

    GAgent_Printf(GAGENT_INFO,"wanclient_num = %d",wanclient_num );
#endif
    return ;
}




static unsigned short mqtt_parse_pub_topic_ptr(const unsigned char* buf, const unsigned char **topic_ptr) 
{
    unsigned short len = 0;
	
    //printf("mqtt_parse_pub_topic_ptr\n");

    if(((buf[0]&0xf0)>>4) == MQTT_PKT_PUBLISH) {
        // fixed header length = 1 for "flags" byte + rlb for length bytes
        unsigned char rlb = mqtt_num_rem_len_bytes(buf);
        len = *(buf+1+rlb)<<8;	// MSB of topic UTF
        len |= *(buf+1+rlb+1);	// LSB of topic UTF
        // start of topic = add 1 for "flags", rlb for remaining length, 2 for UTF
        *topic_ptr = (buf + (1+rlb+2));
        Ql_Debug_Trace("%s %d\r\n",__FUNCTION__,__LINE__);
        
    } else {
        *topic_ptr = NULL;
    }
    return len;
}

static unsigned short mqtt_parse_pub_topic(const unsigned char* buf, unsigned char* topic) 
{
    const unsigned char* ptr;
    unsigned short topic_len = mqtt_parse_pub_topic_ptr(buf, &ptr);
	
    //printf("mqtt_parse_pub_topic\n");
	
    if(topic_len != 0 && ptr != NULL) {
        
        Ql_Debug_Trace("%s %d\r\n",__FUNCTION__,__LINE__);
        Adapter_Memcpy(topic, ptr, topic_len);
    }
	
    return topic_len;
}

unsigned short mqtt_parse_rem_len(const unsigned char* buf) 
{
    unsigned short multiplier = 1;
    unsigned short value = 0;
    unsigned char digit;

    buf++;	// skip "flags" byte in fixed header

    do {
        digit = *buf;
        value += (digit & 127) * multiplier;
        multiplier *= 128;
        buf++;
    } while ((digit & 128) != 0);

    return value;
}

static unsigned short mqtt_parse_pub_msg_ptr(const unsigned char* buf, const unsigned char **msg_ptr) 
{
    unsigned short len = 0;

    //printf("mqtt_parse_pub_msg_ptr\n");

    if(((buf[0]&0xf0)>>4) == MQTT_PKT_PUBLISH) {
        // message starts at
        // fixed header length + Topic (UTF encoded) + msg id (if QoS>0)
        unsigned char rlb = mqtt_num_rem_len_bytes(buf);
        unsigned char offset = (*(buf+1+rlb))<<8;	// topic UTF MSB
        offset |= *(buf+1+rlb+1);			// topic UTF LSB
        offset += (1+rlb+2);				// fixed header + topic size

        if(((buf[0]&0x06)>>1) != 0) {
            offset += 2;					// add two bytes of msg id
        }

        *msg_ptr = (buf + offset);
				
        // offset is now pointing to start of message
        // length of the message is remaining length - variable header
        // variable header is offset - fixed header
        // fixed header is 1 + rlb
        // so, lom = remlen - (offset - (1+rlb))
      	len = mqtt_parse_rem_len(buf) - (offset-(rlb+1));
        
        Ql_Debug_Trace("%s %d\r\n",__FUNCTION__,__LINE__);
    } else {
        *msg_ptr = NULL;
    }
    return len;
}
static unsigned short mqtt_parse_publish_msg(const unsigned char* buf, unsigned char** msg) 
{
    unsigned char* ptr;
    unsigned short msg_len = mqtt_parse_pub_msg_ptr(buf, (const unsigned char **)&ptr);

    if(msg_len != 0 && ptr != NULL) {
        //memcpy(msg, ptr, msg_len);
        (*msg)=ptr;
    }

    return msg_len;
}






int Mqtt_DispatchPublishPacket( pgcontext pgc,unsigned char *packetBuffer,int packetLen )
{
    unsigned char topic[128];
    int topiclen;
    unsigned char *pHiP0Data;
    int HiP0DataLen;
    int i;
    unsigned char varlen=0;
    unsigned char  clientid[PHONECLIENTID + 1];
    //int32 clientidlen = 0;
    unsigned char*pTemp;
    unsigned short cmd;
    int sn;
    unsigned short *pcmd=NULL;

    topiclen = mqtt_parse_pub_topic(packetBuffer, topic);
    //HiP0DataLen = packetLen - topiclen;
    topic[topiclen] = '\0';

    HiP0DataLen = mqtt_parse_publish_msg(packetBuffer, &pHiP0Data); 


    if(Adapter_Strncmp((const char*)topic,"app2dev/",Adapter_Strlen("app2dev/"))==0)
    {
    
        varlen = mqtt_num_rem_len_bytes( pHiP0Data+3 );
        if(varlen<1 || varlen>4)
        {
            return 0;
        }

        pcmd = (u16*)&pHiP0Data[4+varlen+1];
        cmd = EndianConvertLToB( *pcmd );  
        pTemp = &topic[Adapter_Strlen("app2dev/")];
        i = 0;
        while (*pTemp != '/')
        {
            i++;
            pTemp++;
        }

        pTemp ++; /* 跳过\/ */
        i=0;
        while (*pTemp != '\0' && i <= PHONECLIENTID)
        {
            clientid[i] = *pTemp;
            i++;
            pTemp++;
        }
        if(i > PHONECLIENTID)
        {
            /* should handle invalid phone client id.don't ack the cmd */
            i = PHONECLIENTID;
        }
        clientid[i]= '\0';
        Adapter_Strncpy( pgc->rtinfo.waninfo.phoneClientId ,(const char*)clientid ,Adapter_Strlen((const char*)clientid));
        pgc->rtinfo.waninfo.srcAttrs.cmd = cmd;
        Adapter_Memcpy( packetBuffer,pHiP0Data,HiP0DataLen );
        APP_DEBUG("Cloud CMD =%04X\r\n",cmd );
        if( cmd==0x0093 )
        {
            sn = *(int *)&pHiP0Data[4+varlen+1 + sizeof(cmd)];
            sn = EndianConvertLToB_Long(sn);
            Cloud_SetClientAttrs(pgc, clientid, cmd, sn);
        }
        else if( cmd == 0x0090 )
        {
            sn = 0;
            Cloud_SetClientAttrs(pgc, clientid, cmd, sn);
        }
        return HiP0DataLen;
    }
    // 订阅最新固件响应
    else if(Adapter_Strncmp((const char*)topic,"ser2cli_res/",Adapter_Strlen("ser2cli_res/"))==0)
    {

        pcmd = (u16*)&pHiP0Data[4];
        cmd = EndianConvertLToB( *pcmd );        
        // pHiP0Data消息体的指针
        // HiP0DataLen消息体的长度 packetBuffer
        switch(cmd)
        {
            /* V4.1 Don't use this cmd */
            case 0x020e:
                break;
            // wan client on line numbers res.
            case 0x0210:
                Mqtt_ResOnlineClient( pgc,(char*)pHiP0Data, HiP0DataLen);
            break;
            case 0x0211:
                //todo MCU OTA.
                //GAgent_Printf( GAGENT_DEBUG,"M2M cmd to check OTA!!! ");
               // GAgent_SetCloudConfigStatus( pgc,CLOUD_RES_GET_SOFTVER); 
            break;
            default:
            break;
        }
        return 0;
    }
    return 0;
}



#if 0
int MQTT_readPacket( int socketid,ppacket pbuf,int bufferLen )
{
    int bytes_rcvd;
    unsigned char *pData;
    int messageLen;
    int varLen;
    int packet_length;
    int ret;

    Adapter_Memset(pbuf->phead, 0, bufferLen);
    pData = pbuf->phead;
    ret  = Mqtt_RecvPkt(gw_ctx->mqttctx);
    if(ret < 0)
    {
        MQTTclose_socket();
        return -1;
    }
    bytes_rcvd = gw_ctx->mqttctx->pos - gw_ctx->mqttctx->bgn;
    //pData = packetBuffer + 0;
    Ql_memcpy(pbuf->phead,gw_ctx->mqttctx->bgn,bytes_rcvd);
    pbuf->pend=pbuf->phead+bytes_rcvd;
    
   
    return bytes_rcvd;
}
#endif


#endif

