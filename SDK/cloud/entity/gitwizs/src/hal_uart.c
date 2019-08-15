#include "custom_feature_def.h"
#ifdef __OCPU_SMART_CLOUD_SUPPORT__
#ifdef __GITWIZS_SOLUTION__

#include "gagent_typedef.h"
#include "gagent.h"
#include "adapter.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "utils.h"


#define HAL_UART_PORT   (UART_PORT3)
#define HAL_UART_BAUDRATE     (9600)
#define HAL_UART_DATABITS      (DB_8BIT)
#define HAL_UART_PARITY        (PB_NONE)
#define HAL_UART_STOPBITS        (SB_ONE)
#define HAL_UART_FLOWCTRL        (FC_NONE)


#define LOCAL_HAL_REC_SYNCHEAD1     1
#define LOCAL_HAL_REC_SYNCHEAD2     2
#define LOCAL_HAL_REC_DATALEN1      3
#define LOCAL_HAL_REC_DATALEN2      4
#define LOCAL_HAL_REC_DATA          5






u8 *hal_RxBuffer=NULL;

static u32 pos_start = 0;
static u32 pos_current = 0;
static u8  gu8LocalHalStatus = LOCAL_HAL_REC_SYNCHEAD1;
static u16 gu16LocalPacketDataLen;
static u32 guiLocalPacketStart;
static u8  halRecKeyWord = 0;

extern s32 g_main_task_id;
extern s32 g_cloud_task_id;
extern s32 g_mcu_task_id;

extern int g_mcu_hearbeat_count;









pfMasterMCU_ReciveData PF_ReceiveDataformMCU = NULL;
pfMasertMCU_SendData   PF_SendData2MCU = NULL;






static void Local_CallBack_UART_Hdlr( Enum_SerialPort port, Enum_UARTEventType event, bool pinLevel,void *customizePara);

/****************************************************************
FunctionName    :   Hal_UartReg_RcvHook
Description     :   register recieve hook func for handle rcv data
fun             :   hook func
return          :   void
****************************************************************/
void Hal_UartReg_RcvHook(pfMasterMCU_ReciveData fun)
{
    PF_ReceiveDataformMCU = fun;
}

/****************************************************************
FunctionName    :   Hal_UartReg_SendHook
Description     :   register send hook func for send data
fun             :   hook func
return          :   void
****************************************************************/
void Hal_UartReg_SendHook(pfMasertMCU_SendData fun)
{
    PF_SendData2MCU = fun;
}


/****************************************************************
FunctionName    :   get_available_buf_space
Description     :   get buf of availabel size of buf
pos_current     :   current position  
pos_start       :   start position 
return          :   available size
****************************************************************/
static s32 get_available_buf_space(s32 pos_current, s32 pos_start)
{
    pos_current = pos_current & HAL_BUF_MASK;
    pos_start = pos_start & HAL_BUF_MASK;
    
    if(pos_current >= pos_start)
    {
        return HAL_BUF_SIZE - pos_current;
    }
    else
    {
        return pos_start - pos_current;
    }
}




/****************************************************************
FunctionName    :   Hal_Rcv_Data_Handle
Description     :   receive all of data form local io one time.
                    and put data int local cycle buf.This func will
                    change data 0xff55 isn't SYNC HEAD to 0xff
pgc             :   gagent global struct. 
return          :   the bytes actually read.
****************************************************************/
s32 Hal_Rcv_Data_Handle(pgcontext pgc)
{
    int fd;
    s32 available_len =0;
    s32 read_count = 0;
    
    fd = pgc->rtinfo.local.uart_fd;
    
    if(fd < 0)
    {
        return UART_ERROR_INVALID_FD;
    }

 /* step 1.read data into loopbuf */
 //you need to consider big data ,when massive data is coming ,handle the exception
 //TODO
    available_len = get_available_buf_space( pos_current, pos_start );
 
    read_count = Adapter_UART_Read( fd, &hal_RxBuffer[pos_current & HAL_BUF_MASK],available_len );
    
    if(read_count < 0)
    {
        return UART_ERROR_READ;
    }

    pos_current += read_count;

    return read_count;
}


/****************************************************************
FunctionName    :   Hal_Send_Data_Handle
Description     :   send data to local io
fd              :   uart fd. 
buf             :   data need to serial pointer.
buflen          :   data want to write.
return          :   >0 the number of bytes written is returned
                    other error.
****************************************************************/
s32 Hal_Send_Data_Handle(s32 fd,u8 *buf,u32 buflen)
{
    s32 datalen=0;
    
    if(fd < 0)
    {
        return UART_ERROR_INVALID_FD;
    }
    datalen = Adapter_UART_Send( fd ,buf,buflen);

    return datalen;
}






/****************************************************************
FunctionName    :   Hal_UartBuffer_Init
Description     :   init UART recv buf.
return          :   void
****************************************************************/
void Hal_UartBuffer_Init( pgcontext pgc )
{
    int totalCap = BUF_LEN + BUF_HEADLEN;
    int bufCap = BUF_LEN;
    
    hal_RxBuffer = (u8*)Adapter_Mem_Alloc( HAL_BUF_SIZE );
    while( hal_RxBuffer==NULL )
    {
        hal_RxBuffer = (u8*)Adapter_Mem_Alloc( HAL_BUF_SIZE );
        Adapter_Sleep(1);
    }
    pgc->mcu.isBusy = 0;
}

static u8 __halbuf_read(u32 offset)
{
    return hal_RxBuffer[offset & HAL_BUF_MASK];
}

static void __halbuf_write(u32 offset, u8 value)
{
    hal_RxBuffer[offset & HAL_BUF_MASK] = value;
}

/****************************************************************
FunctionName    :   get_data_len
Description     :   get data length from "from " to "to"
return          :   the length of data.
Add by Alex.lin and Johnson     --2015-04-07
****************************************************************/
static s32 get_data_len( s32 from, s32 to )
{ 
    to = to & HAL_BUF_MASK;
    from = from & HAL_BUF_MASK;
    
    if(to >= from)
        return to  - from;
    else
        return HAL_BUF_SIZE - from + to;
}


/****************************************************************
FunctionName    :   move_data_backward
Description     :   move data backward 
buf             :   the pointer of hal_buf   
pos_start       :   the pos of start of move. 
pos_end         :   the pos of end of move.
move_len        :   the  length of data need to move.
return          :   NULL
****************************************************************/
void HAL_Move_Data_Backward( u8 *buf, s32 pos_start, s32 pos_end, s32 move_len)
{
    s32 k;
    s32 pos_new_start;
    s32 move_data_len = get_data_len(pos_start, pos_end);

    pos_new_start = pos_start + (-1)*move_len;
    
    for(k=0; k < move_data_len; k++)
    {
        __halbuf_write(pos_new_start + k, __halbuf_read(pos_start + k));
    }
}



/****************************************************************
FunctionName    :   HAL_Local_ExtractOnePacket
Description     :   extract one packet from local cycle buf, and 
                    put data into buf.Will change pos_start
buf             :   dest buf
return          :   >0 the local packet data length.
                    <0 don't have one whole packet data
****************************************************************/
s32 HAL_Local_ExtractOnePacket(u8 *buf)
{
    u8 data;
    u32 i = 0;

    while((pos_start & HAL_BUF_MASK) != (pos_current & HAL_BUF_MASK))
    {
        data = __halbuf_read(pos_start);

        if(LOCAL_HAL_REC_SYNCHEAD1 == gu8LocalHalStatus)
        {
            if(MCU_HDR_FF == data)
            {
                gu8LocalHalStatus = LOCAL_HAL_REC_SYNCHEAD2;

                guiLocalPacketStart = pos_start;
            }
        }
        else if(LOCAL_HAL_REC_SYNCHEAD2 == gu8LocalHalStatus)
        {
            if(MCU_HDR_FF == data)
            {
                gu8LocalHalStatus = LOCAL_HAL_REC_DATALEN1;
            }
            else
            {
                gu8LocalHalStatus = LOCAL_HAL_REC_SYNCHEAD1;
            }
        }
        else
        {
            if(halRecKeyWord)
            {
                halRecKeyWord = 0;
                if(0x55 == data)
                {
                    data = 0xff;
                    HAL_Move_Data_Backward(hal_RxBuffer, pos_start + 1, pos_current, 1);
                    pos_current--;
                    pos_start--;

                }
                else if(MCU_HDR_FF == data)
                {
                    gu8LocalHalStatus = LOCAL_HAL_REC_DATALEN1;
                    guiLocalPacketStart = pos_start - 1;
                    pos_start++;
                    continue;
                }
                else
                {
                    if(LOCAL_HAL_REC_DATALEN1 == gu8LocalHalStatus)
                    {
                        guiLocalPacketStart = pos_start - 2;
                    }
                    else
                    {
                        gu8LocalHalStatus = LOCAL_HAL_REC_SYNCHEAD1;
                        pos_start++;
                        continue;
                    }
                }

            }
            else
            {
                if(MCU_HDR_FF == data)
                {
                    halRecKeyWord = 1;
                    pos_start++;
                    continue;
                }
            }

            if(LOCAL_HAL_REC_DATALEN1 == gu8LocalHalStatus)
            {
                gu16LocalPacketDataLen = data;
                gu16LocalPacketDataLen = (gu16LocalPacketDataLen << 8) & 0xff00;
                gu8LocalHalStatus = LOCAL_HAL_REC_DATALEN2;
            }
            else if(LOCAL_HAL_REC_DATALEN2 == gu8LocalHalStatus)
            {
                gu16LocalPacketDataLen += data;
                gu8LocalHalStatus = LOCAL_HAL_REC_DATA;

                if(0 == gu16LocalPacketDataLen)
                {
                    /* invalid packet */
                    gu8LocalHalStatus = LOCAL_HAL_REC_SYNCHEAD1;
                }
            }
            else
            {
                /* Rec data */
                gu16LocalPacketDataLen--;
                if(0 == gu16LocalPacketDataLen)
                {
                    pos_start++;
                    i = 0;
                    while(guiLocalPacketStart != pos_start)
                    {
                        buf[i] = __halbuf_read(guiLocalPacketStart++);
                        i++;
                    }

                    return i;
                }
            }
            
        }
        
        pos_start++;
    }

    return RET_FAILED;
}



/****************************************************************
Function    :   Hal_Uart_Init
Description :   uart init
pgc         :   global staruc pointer.
return      :   void
****************************************************************/
void Hal_Uart_Init(pgcontext pgc)
{
    s32 ret;
    ST_UARTDCB dcb;

    dcb.baudrate = HAL_UART_BAUDRATE;
    dcb.dataBits = HAL_UART_DATABITS;
    dcb.parity = HAL_UART_PARITY;
    dcb.stopBits = HAL_UART_STOPBITS;
    dcb.flowCtrl = HAL_UART_FLOWCTRL;
    
    ret = Ql_UART_Register(HAL_UART_PORT, Local_CallBack_UART_Hdlr, (void *)pgc);
    if (ret < QL_RET_OK)
    {
        APP_DEBUG("Fail to register serial port[%d], ret=%d\r\n", HAL_UART_PORT, ret);
    }
    
    ret = Ql_UART_OpenEx(HAL_UART_PORT,&dcb);
    if (ret < QL_RET_OK)
    {
        APP_DEBUG("Fail to open serial port[%d], ret=%d\r\n", HAL_UART_PORT, ret);
    }
    pgc->rtinfo.local.uart_fd = HAL_UART_PORT;

}

/****************************************************************
FunctionName    :   Local_Ack2MCUwithP0.
Description     :   ack to mcu with P0 after receive mcu data.
pbuf            :   the data send to MCU
fd              :   local data fd.
sn              :   receive local data sn .
cmd             :   ack to mcu cmd.
****************************************************************/
void HAL_Ack2MCUwithP0( ppacket pbuf, s32 fd,u8 sn,u8 cmd )
{
    u16 datalen = 0;
    u16 flag = 0;
    u16 sendLen = 0; 
    pbuf->phead = (pbuf->ppayload)-8;
    
    /* head(0xffff)| len(2B) | cmd(1B) | sn(1B) | flag(2B) |  payload(xB) | checksum(1B) */
    pbuf->phead[0] = MCU_HDR_FF;
    pbuf->phead[1] = MCU_HDR_FF;
    datalen = pbuf->pend - pbuf->ppayload + 5;    //p0 + cmd + sn + flag + checksum
    *(u16 *)(pbuf->phead + 2) = EndianConvertLToB(datalen);
    pbuf->phead[4] = cmd;
    pbuf->phead[5] = sn;
    *(u16 *)(pbuf->phead + 6) = EndianConvertLToB(flag);
    *( pbuf->pend )  = GAgent_SetCheckSum(pbuf->phead, (pbuf->pend)-(pbuf->phead) );
    pbuf->pend += 1;  /* add 1 Byte of checksum */

    sendLen = (pbuf->pend) - (pbuf->phead);
    sendLen = Gagent_Local_DataAdapter( (pbuf->phead)+2,( (pbuf->pend) ) - ( (pbuf->phead)+2 ) );
    HAL_Local_SendData( fd, pbuf->phead,sendLen );
    
    return;
}


static void Hal_Local_ErrHandle(pgcontext pgc, ppacket pRxbuf)
{
    u8 cmd;
    u8 sn;
    
    if(NULL == pRxbuf)
    {
        return ;
    }

    sn = pRxbuf->phead[MCU_SN_POS];
    cmd = pRxbuf->phead[MCU_CMD_POS];
    switch(cmd)
    {
        case MCU_INFO_CMD_ACK:
        case MCU_CTRL_CMD_ACK:
        case MODULE_PING2MCU_ACK:
            /* do nothing, return immediately */
            break;
        case MCU_REPORT:
            pRxbuf->ppayload[0] = GAGENT_MCU_CHECKSUM_ERROR;
            pRxbuf->pend = pRxbuf->ppayload + 1;
            HAL_Ack2MCUwithP0( pRxbuf, pgc->rtinfo.local.uart_fd, sn, MCU_DATA_ILLEGAL );
            break;
        default :
            pRxbuf->ppayload[0] = GAGENT_MCU_CMD_ERROR;
            pRxbuf->pend = pRxbuf->ppayload + 1;
            HAL_Ack2MCUwithP0( pRxbuf, pgc->rtinfo.local.uart_fd, sn, MCU_DATA_ILLEGAL );
            break;
    }
}

static void Hal_Local_ExtractInfo(pgcontext pgc, ppacket pRxBuf)
{
    u16 *pTime=NULL;
    u16 *pplength=NULL;
    u8 * Rxbuf=NULL;
    s32 pos=0;
    s8 length =0;
    
    Rxbuf = pgc->rtinfo.Rxbuf->phead;

    pplength = (u16*)&((pgc->rtinfo.Rxbuf->phead +2)[0]);
    length = EndianConvertLToB(*pplength);

    
    pos+=8;
    Adapter_Strncpy( pgc->mcu.protocol_ver, Rxbuf+pos, MCU_PROTOCOLVER_LEN );
    pgc->mcu.protocol_ver[MCU_PROTOCOLVER_LEN] = '\0';
    pos += MCU_PROTOCOLVER_LEN;

    Adapter_Strncpy( pgc->mcu.p0_ver,Rxbuf+pos, MCU_P0VER_LEN);
    pgc->mcu.p0_ver[MCU_P0VER_LEN] = '\0';
    pos+=MCU_P0VER_LEN;

    Adapter_Strncpy( pgc->mcu.hard_ver,Rxbuf+pos,MCU_HARDVER_LEN);
    pgc->mcu.hard_ver[MCU_HARDVER_LEN] = '\0';
    pos+=MCU_HARDVER_LEN;

    Adapter_Strncpy( pgc->mcu.soft_ver,Rxbuf+pos,MCU_SOFTVER_LEN);
    pgc->mcu.soft_ver[MCU_SOFTVER_LEN] = '\0';
    pos+=MCU_SOFTVER_LEN;

    Adapter_Strncpy( pgc->mcu.product_key,Rxbuf+pos,PK_LEN);
    pgc->mcu.product_key[PK_LEN] = '\0';
    pos+=PK_LEN;

    pos+=2;


    if( length >= (pos+MCU_MCUATTR_LEN+1 - MCU_P0_LEN - MCU_CMD_LEN) ) //pos+8+1:pos + mcu_attr(8B)+checksum(1B)
    {
        Adapter_Strncpy( pgc->mcu.mcu_attr,Rxbuf+pos, MCU_MCUATTR_LEN);
    }
    else
    {
        Adapter_Memset( pgc->mcu.mcu_attr, 0, MCU_MCUATTR_LEN);
    }

    if( Adapter_Strcmp( (u8 *)pgc->mcu.product_key,pgc->gc.old_productkey )!=0 )
    {
        GAgent_UpdateInfo( pgc,pgc->mcu.product_key );
        APP_DEBUG("2 MCU old product_key:%s.\r\n",pgc->gc.old_productkey);
    }
    HAL_Timer_Stop(HAL_MCU_ACK_TIMER);
    Adapter_SendMsg(g_mcu_task_id,MSG_ID_MCU_ACK_SUCCESS,NULL,NULL);
}

/*****************************************************************
*   
will read the global tx buf of module LOCAL
*******************************************************************/
static int Hal_Local_CheckAck(pgcontext pgc, s32 cmd, s32 sn)
{
    s32 snTx;
    s32 cmdTx;

    cmdTx = pgc->mcu.TxbufInfo.cmd;
    snTx = pgc->mcu.TxbufInfo.sn;
    APP_DEBUG("cmdTx=%d,snTx=%d,cmd=%d,sn=%d\r\n",cmdTx,snTx,cmd,sn);
    if((snTx == sn) && ((cmdTx + 1) == cmd))
    {
       
        /* communicate done */
        pgc->mcu.isBusy = 0;
        return RET_SUCCESS;
    }

    return RET_FAILED;
}

/****************************************************************
FunctionName    :   GAgent_LocalSendData
Description     :   send data to local io.
return          :   NULL
****************************************************************/
s32 HAL_Local_SendData( s32 fd,u8 *pData, u32 bufferMaxLen )
{
    s32 i=0;
    s32 actual_send = 0;

   
    if( PF_SendData2MCU!=NULL )
    {
        APP_DEBUG("local send len = %d:\r\n",bufferMaxLen );
        for( i=0;i<bufferMaxLen;i++ )
        {
            APP_DEBUG(" %02x",pData[i]);
        }
        APP_DEBUG("\r\n");
        //TODO,consider send data incompletely
        actual_send = PF_SendData2MCU( fd,pData,bufferMaxLen );
        
        APP_DEBUG("actual_send = %d\r\n",actual_send);
        if(actual_send < 0)
        {
            return UART_ERROR_SEND_FAILED;
        }
        else
        {
            return actual_send;
        }
    }
    
}




/****************************************************************
FunctionName    :   Local_Ack2MCU.
Description     :   ack to mcu after receive mcu data.
fd              :   local data fd.
sn              :   receive local data sn .
cmd             :   ack to mcu cmd.
****************************************************************/
void HAL_Local_Ack2MCU( s32 fd,u8 sn,u8 cmd )
{
    s32 len = MCU_LEN_NO_PAYLOAD; 
    u16 p0_len = EndianConvertLToB(5);    
    u8 buf[MCU_LEN_NO_PAYLOAD];
    
    Adapter_Memset(buf, 0, len);
    buf[0] = MCU_HDR_FF;
    buf[1] = MCU_HDR_FF;
    Adapter_Memcpy(&buf[MCU_LEN_POS], &p0_len, 2);
    buf[MCU_CMD_POS] = cmd;
    buf[MCU_SN_POS] = sn;
    buf[MCU_LEN_NO_PAYLOAD-1]=GAgent_SetCheckSum( buf, (MCU_LEN_NO_PAYLOAD-1));
    HAL_Local_SendData( fd,buf,len );
    return ;
}



s32 Hal_LocalDataHandle( pgcontext pgc,ppacket Rxbuf,s32 RxLen /*,ppacket Txbuf*/ )
{
    s8 cmd=0;
    u8 sn=0,checksum=0;
    u8 *localRxbuf=NULL;
    u32 ret = 0;
    u8 configType=0;
    //_tm tm;
    s32 piecelen;
    int i;
    if( RxLen>0 )
    {
        localRxbuf = Rxbuf->phead;
        
        cmd = localRxbuf[4];
        sn  = localRxbuf[5];
        checksum = GAgent_SetCheckSum( localRxbuf,RxLen-1 );
        if( checksum!=localRxbuf[RxLen-1] )
        {
            APP_DEBUG("local data cmd=%02x checksum error,calc sum:0x%x,expect:0x%x !\r\n",
                cmd, checksum, localRxbuf[RxLen-1] );
            Hal_Local_ErrHandle( pgc, Rxbuf);
            return UART_ERROR_CHECKSUM;
        }
        switch( cmd )
        {

            case MCU_REPORT:
                HAL_Local_Ack2MCU( pgc->rtinfo.local.uart_fd,sn,cmd+1 );
                Rxbuf->type = SetPacketType( Rxbuf->type,LOCAL_DATA_IN,1 );
                ParsePacket( Rxbuf );
                ret = 1;
                setChannelAttrs(pgc, NULL,1);
                break;
           case MCU_RESET_MODULE:
                HAL_Local_Ack2MCU( pgc->rtinfo.local.uart_fd,sn,cmd+1 );
                GAgent_Clean_Config(pgc);
                Adapter_Sleep(2);
                Adapter_DevReset();
                ret = 0;
                break;

 
            case MCU_INFO_CMD_ACK:
                if(RET_SUCCESS == Hal_Local_CheckAck(pgc, cmd, sn))
                {
                    Hal_Local_ExtractInfo(pgc, Rxbuf);
                }
                ret = 0;
                break;

           case MODULE_PING2MCU_ACK:
                Hal_Local_CheckAck(pgc, cmd, sn);
               // g_mcu_hearbeat_count = 0;
               // APP_DEBUG("\r\n module ping to mcu is ok\r\n");
                HAL_Timer_Stop(HAL_MCU_ACK_TIMER);
                Adapter_SendMsg(g_mcu_task_id,MSG_ID_MCU_ACK_SUCCESS,NULL,NULL);
                
                Adapter_SendMsg(g_main_task_id,MSG_ID_MCU_PING_RSP_OK,NULL,NULL);
                ret = 0 ;
                break;
           case MCU_CTRL_CMD_ACK:
                if(RET_SUCCESS == Hal_Local_CheckAck(pgc, cmd, sn))
                {
                    /* out to app and cloud, for temp */
                    APP_DEBUG("ctrl cmd ack is ok\r\n");
                    HAL_Timer_Stop(HAL_MCU_ACK_TIMER);
                    Adapter_SendMsg(g_mcu_task_id,MSG_ID_MCU_ACK_SUCCESS,NULL,NULL);
                    
                    Adapter_SendMsg(g_cloud_task_id,MSG_ID_MCU_ACK_SUCCESS,NULL,NULL);
                    Rxbuf->type = SetPacketType( Rxbuf->type,LOCAL_DATA_IN,1 );
                    ParsePacket( Rxbuf );
                    Rxbuf->type = SetPacketType( Rxbuf->type,CLOUD_DATA_OUT,0 );

                }
                break;
            
            default:
                ret = 0;
                break;
        }
        //...
    }

  return ret;
}


//uart callback
static void Local_CallBack_UART_Hdlr( Enum_SerialPort port, Enum_UARTEventType event, bool pinLevel,void *customizePara)
{
    u32 recv_len;
    int i;
    s32 localDataLen=0;
    s32 ret;

    
    if(HAL_UART_PORT == port)
    {
        if(EVENT_UART_READY_TO_READ == event)
        {
            recv_len = PF_ReceiveDataformMCU((pgcontext)customizePara);

            if(recv_len < 0)
            {
                APP_DEBUG("read uart error\r\n");
            }

            for( i=0;i<recv_len;i++ )
            {
                APP_DEBUG(" %02x ",hal_RxBuffer[pos_current-recv_len+i]);
            }

            do
            {
                /* extract one packet from local driver cycle buf to app buf */

                localDataLen = HAL_Local_ExtractOnePacket(((pgcontext)customizePara)->rtinfo.Rxbuf->phead);
                APP_DEBUG("localDataLen = %d\r\n",localDataLen);
                if(localDataLen > 0)
                {
                    /* get data.deal */
                    ret = Hal_LocalDataHandle( (pgcontext)customizePara, ((pgcontext)customizePara)->rtinfo.Rxbuf, localDataLen );
                    if(ret > 0)
                    {
                        {
                        /* need transfer local data to other module */
                          dealPacket( (pgcontext)customizePara, ((pgcontext)customizePara)->rtinfo.Rxbuf);
                        }
                    }
                }
            }while(localDataLen > 0);
            
        }
    }
}




#endif
#endif
