#include "custom_feature_def.h"
#ifdef __OCPU_SMART_CLOUD_SUPPORT__
#ifdef __GITWIZS_SOLUTION__

#include "gagent.h"
#include "adapter.h"
#include "cloud.h"
#include <stdlib.h>

extern s32 g_cloud_task_id;
extern s32 g_mcu_task_id;




void make_rand(s8* data)
{
    int i;
    Adapter_Memset(data,0,10);
	
    for(i=0;i<10;i++)
    {
        data[i] = 65+rand()%(90-65);
    }
    data[10] = '\0';
    APP_DEBUG("passcode:%s\r\n",data );
}


/***************************************************
FunctionName    :   resetPacket
Description     :   let ppacket phead ppayload pend pointer 
                    in satrt status.
return          :   void.
***************************************************/
void resetPacket( ppacket pbuf )
{
    pbuf->phead  = pbuf->allbuf+BUF_HEADLEN;
    pbuf->ppayload = pbuf->allbuf+BUF_HEADLEN;
    pbuf->pend   = pbuf->allbuf+BUF_HEADLEN;
    Adapter_Memset( pbuf->allbuf,0,pbuf->totalcap );
}

/***************************************************
FunctionName    :   EndianConvertLToB
Description     :   little endian to big endian convert
return          :   the converted result.
***************************************************/
u16 EndianConvertLToB(u16 InputNum) 
{
    return (((InputNum & 0x00ff)<<8) | ((InputNum & 0xff00)>>8));
}



/***************************************************
FunctionName    :   EndianConvertLToB
Description     :   little endian to big endian convert
return          :   the converted result.
***************************************************/
u16 EndianConvertLToB_Long(u32 InputNum) 
{
    return (((InputNum & 0x000000ff)<<24) | ((InputNum & 0x0000ff00)<<8)|((InputNum & 0x00ff0000)>>8)|((InputNum & 0xff000000)>>24));
}



/***************************************************
FunctionName    :   SetPacketType
Description     :   set addtype to packet type.
currentType     :   current packet msg type
type            :   type
flag            :   0 reset type. 1 set type.
return          :   the type afet add newtype
***************************************************/
s32 SetPacketType( s32 currentType,s32 type,s8 flag )
{
    if( flag==0 )
    {
        currentType &=~ type;
    }
    else
    {
        currentType |=type;
    }
    return currentType;
}
extern unsigned short mqtt_parse_rem_len(const unsigned char* buf) ;
extern unsigned char mqtt_num_rem_len_bytes(const unsigned char* buf);


/***************************************************
        FunctionName    :   ParsePacket.
        Description     :   set the source phead ppayload
                            pend.
        pbug            :   data source struct.
        return          :   0 ok other fail.
***************************************************/
u32 ParsePacket( ppacket pRxBuf )
{
    s32 varlen=0;
    s32 datalen=0;
    u16 cmd=0;
    u16 *pcmd=NULL;
    APP_DEBUG("\r\n");
    APP_DEBUG("IN %s packet type : %04x",__FUNCTION__ ,pRxBuf->type );
    if( ((pRxBuf->type)&(CLOUD_DATA_IN)) == CLOUD_DATA_IN )
    {
        datalen = mqtt_parse_rem_len( pRxBuf->phead+3 ); 
        varlen = mqtt_num_rem_len_bytes( pRxBuf->phead+3 );
        
        pcmd = (u16*)&(pRxBuf->phead[4+varlen+1]);
        cmd = EndianConvertLToB(*pcmd);  

        APP_DEBUG("CLOUD_DATA_IN cmd : %04X", cmd );
        if( cmd == 0x0090 )
        {
            pRxBuf->ppayload = pRxBuf->phead+4+varlen+1+2;
        }
        if( cmd == 0x0093 )
        {//with sn.
            pRxBuf->ppayload = pRxBuf->phead+4+varlen+1+2+4;          
        }

        pRxBuf->pend   = pRxBuf->phead+4+varlen+datalen;  

        APP_DEBUG(" ReSet Data Type : %04X - CLOUD_DATA_IN\r\n", pRxBuf->type );
        pRxBuf->type = SetPacketType( pRxBuf->type,CLOUD_DATA_IN,0 );
        pRxBuf->type = SetPacketType( pRxBuf->type,LOCAL_DATA_OUT,1 );
        APP_DEBUG(" Set Data Type : %04X - LOCAL_DATA_OUT\r\n", pRxBuf->type );
    }
    else if( ((pRxBuf->type)&(LOCAL_DATA_IN)) == LOCAL_DATA_IN )
    {
        /* head(0xffff)| len(2B) | cmd(1B) | sn(1B) | flag(2B) |  payload(xB) | checksum(1B) */
        pRxBuf->ppayload = pRxBuf->phead+8;   /* head + len + cmd + sn + flag */
        datalen = ( (s32)EndianConvertLToB( *(u16 *)(pRxBuf->phead + 2) ) ) & 0xffff;
       // datalen = ( (s32) *(u16 *)(pRxBuf->phead + 2)  ) & 0xffff;
       APP_DEBUG("data_len;%d\r\n",datalen);
        pRxBuf->pend =  (pRxBuf->phead )+( datalen+4-1 ); /* datalen + head + len -checksum */

        APP_DEBUG(" ReSet Data Type : %04X - LOCAL_DATA_IN\r\n", pRxBuf->type );
        pRxBuf->type = SetPacketType( pRxBuf->type,LOCAL_DATA_IN,0 );
        pRxBuf->type = SetPacketType( pRxBuf->type,CLOUD_DATA_OUT,1 );
        APP_DEBUG(" Set Data Type : %04X - CLOUD_DATA_OUT \r\n",pRxBuf->type );
    }
    else
    {
        APP_DEBUG( "Data Type error,wite :%04X\r\n ", pRxBuf->type );
        return -1;
    }
    APP_DEBUG("OUT packet type : %04X\r\n",pRxBuf->type );
    return 0;
}

s8 dealPacket( pgcontext pgc, ppacket pTxBuf )
{


    
    
    APP_DEBUG("\r\n");
    APP_DEBUG("IN %s packet type : %04x",__FUNCTION__ ,pTxBuf->type );
   
    if( ((pTxBuf->type)&(LOCAL_DATA_OUT)) == LOCAL_DATA_OUT )
    {
        APP_DEBUG("packet Type : LOCAL_DATA_OUT ");
        GAgent_LocalDataWriteP0( pgc,pgc->rtinfo.local.uart_fd, pTxBuf,MCU_CTRL_CMD ); 
        
        pTxBuf->type = SetPacketType(pTxBuf->type, LOCAL_DATA_OUT, 0);
       // Adapter_SendMsg(g_mcu_task_id,MSG_ID_MCU_SEND_DATA,(void *)pTxBuf,NULL);
        APP_DEBUG("ReSetpacket Type : LOCAL_DATA_OUT ");     
    }
    if( ((pTxBuf->type)&(CLOUD_DATA_OUT)) == CLOUD_DATA_OUT )
    {
       
        APP_DEBUG("packet Type : CLOUD_DATA_OUT \r\n");

        if(pgc->rtinfo.waninfo.mqttstatus == MQTT_STATUS_RUNNING)
        {
          Cloud_SendData(  pgc,pTxBuf,(pTxBuf->pend)-(pTxBuf->ppayload) );
        }
      // Adapter_SendMsg(g_cloud_task_id,MSG_ID_CLOUD_SEND_DATA,(void *)pTxBuf,NULL);
       // APP_DEBUG("ReSetpacket Type : CLOUD_DATA_OUT \r\n");
       
        pTxBuf->type = SetPacketType(pTxBuf->type, CLOUD_DATA_OUT, 0);
    }

    APP_DEBUG("OUT packet type : %04X\r\n",pTxBuf->type );
    return 0;
}


/* return 1 type set */
s8 isPacketTypeSet( s32 currentType,s32 type )
{
    if( ( currentType & type ) == type )
    {
        return 1;
    }
    else
    { 
        return 0;
    }
}

void setChannelAttrs(pgcontext pgc, stCloudAttrs_t *cloudClient,u8 isBroadCast)
{
    if(isBroadCast)
    {

        pgc->rtinfo.stChannelAttrs.cloudClient.phoneClientId[0] = '\0';
        pgc->rtinfo.stChannelAttrs.cloudClient.cmd = GAGENT_CMD_CTL_93;
        pgc->rtinfo.stChannelAttrs.cloudClient.sn = 0;

    }
    else
    {
        if(NULL != cloudClient)
        {
            pgc->rtinfo.stChannelAttrs.cloudClient = *cloudClient;
            pgc->rtinfo.stChannelAttrs.cloudClient.cmd += 1;
            Cloud_ClearClientAttrs(pgc, &pgc->rtinfo.waninfo.srcAttrs);
        }
    }
}



/*************************************************
*
*       FUNCTION : transtion u16 to 2 byte for mqtt remainlen
*       remainLen: src data to transtion
*       return   : varc
***************************************************/
varc Tran2varc(u32 remainLen)
{
    int i;
	varc Tmp;
	
    Adapter_Memset(&Tmp, 0x0, sizeof(Tmp));

	Tmp.varcbty = 1;
	for(i = 0; i < 4; i++)
	{
		Tmp.var[i] = remainLen % 128;
		remainLen >>= 7;
		if(remainLen)
		{
			Tmp.var[i] |= 0x80;
			Tmp.varcbty++;
		}
		else
		{
			break;
		}
	}
    return Tmp;
}



#endif
#endif
