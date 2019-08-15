#ifndef __HAL_UART_H__
#define __HAL_UART_H__


#define   HAL_BUF_SIZE 1024
#define   HAL_BUF_MASK  (HAL_BUF_SIZE - 1)

typedef enum
{
    UART_ERROR_INVALID_FD = -200,
    UART_ERROR_READ = -201,
    UART_ERROR_EXTRACT_PACKET = -202,
    UART_ERROR_CHECKSUM = -203,
    UART_ERROR_SEND_FAILED = -204,

}Enum_Uart_ErrorCode;




typedef s32 (*pfMasterMCU_ReciveData)( pgcontext pgc );
typedef s32  (*pfMasertMCU_SendData)( s32 serial_fd,u8 *buf,u32 buflen );




/****************************************************************
Function    :   Hal_Uart_Init
Description :   uart init
pgc         :   global staruc pointer.
return      :   void
****************************************************************/
void Hal_Uart_Init(pgcontext pgc);

/****************************************************************
FunctionName    :   Hal_UartBuffer_Init
Description     :   init UART recv buf.
return          :   void
****************************************************************/
void Hal_UartBuffer_Init( pgcontext pgc );

/****************************************************************
FunctionName    :   Hal_UartReg_RcvHook
Description     :   register recieve hook func for handle rcv data
fun             :   hook func
return          :   void
****************************************************************/
void Hal_UartReg_RcvHook(pfMasterMCU_ReciveData fun);

/****************************************************************
FunctionName    :   Hal_Rcv_Data_Handle
Description     :   receive all of data form local io one time.
                    and put data int local cycle buf.This func will
                    change data 0xff55 isn't SYNC HEAD to 0xff
pgc             :   gagent global struct. 
return          :   the bytes actually read.
****************************************************************/
s32 Hal_Rcv_Data_Handle(pgcontext pgc);


/****************************************************************
FunctionName    :   Hal_UartReg_SendHook
Description     :   register send hook func for send data
fun             :   hook func
return          :   void
****************************************************************/
void Hal_UartReg_SendHook(pfMasertMCU_SendData fun);

/****************************************************************
FunctionName    :   Hal_Send_Data_Handle
Description     :   send data to local io
fd              :   uart fd. 
buf             :   data need to serial pointer.
buflen          :   data want to write.
return          :   >0 the number of bytes written is returned
                    other error.
****************************************************************/
s32 Hal_Send_Data_Handle(s32 fd,u8 *buf,u32 buflen);

/****************************************************************
FunctionName    :   HAL_Local_ExtractOnePacket
Description     :   extract one packet from local cycle buf, and 
                    put data into buf.Will change pos_start
buf             :   dest buf
return          :   >0 the local packet data length.
                    <0 don't have one whole packet data
****************************************************************/
s32 HAL_Local_ExtractOnePacket(u8 *buf);

/****************************************************************
FunctionName    :   move_data_backward
Description     :   move data backward 
buf             :   the pointer of hal_buf   
pos_start       :   the pos of start of move. 
pos_end         :   the pos of end of move.
move_len        :   the  length of data need to move.
return          :   NULL
****************************************************************/
void HAL_Move_Data_Backward( u8 *buf, s32 pos_start, s32 pos_end, s32 move_len);




/****************************************************************
FunctionName    :   Local_Ack2MCU.
Description     :   ack to mcu after receive mcu data.
fd              :   local data fd.
sn              :   receive local data sn .
cmd             :   ack to mcu cmd.
****************************************************************/
void HAL_Local_Ack2MCU( s32 fd,u8 sn,u8 cmd );



/****************************************************************
FunctionName    :   GAgent_LocalSendData
Description     :   send data to local io.
return          :   NULL
****************************************************************/
s32 HAL_Local_SendData( s32 fd,u8 *pData, u32 bufferMaxLen );

s32 Hal_LocalDataHandle( pgcontext pgc,ppacket Rxbuf,s32 RxLen /*,ppacket Txbuf*/ );





#endif
