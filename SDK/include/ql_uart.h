/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Quectel Co., Ltd. 2013
*
*****************************************************************************/
/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ql_uart.h 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *  Uart APIs defines.
 *
 * Author:
 * -------
 * -------
 *
 *============================================================================
 *             HISTORY
 *----------------------------------------------------------------------------
 * 
 ****************************************************************************/
 

#ifndef __QL_UART_H__
#define __QL_UART_H__
#include "ql_common.h"
#include "ql_system.h"

typedef enum {
    PORT_START = 0,
    VIRTUAL_PORT1 = PORT_START,
    VIRTUAL_PORT2,
    UART_START = 10,
    UART_PORT1 = UART_START,
    UART_PORT2,
    UART_PORT3,
    UART_PORT_END
}Enum_SerialPort;

typedef enum {
    DB_5BIT = 5,
    DB_6BIT,
    DB_7BIT,
    DB_8BIT
} Enum_DataBits;

typedef enum {
    SB_ONE = 1,
    SB_TWO,
    SB_ONE_DOT_FIVE
} Enum_StopBits;

typedef enum {
    PB_NONE = 0,
    PB_ODD,
    PB_EVEN,
    PB_SPACE,
    PB_MARK
} Enum_ParityBits;

typedef enum {
    FC_NONE=1,  // None Flow Control
    FC_HW,      // Hardware Flow Control 
    FC_SW       // Software Flow Control
} Enum_FlowCtrl;

typedef struct {
    u32                 baudrate; 
    Enum_DataBits       dataBits;
    Enum_StopBits       stopBits;
    Enum_ParityBits     parity;
    Enum_FlowCtrl       flowCtrl;
}ST_UARTDCB;

typedef enum {
    UART_VFIFO = 0,         // read and write, only valid on the physical UART PORT1 and physical UART PORT3
    UART_CLR_FE,            // write only, only valid on the physical UART ports
    UART_RX_BYTE_AVAIL,     // read only, valid both on the physical UART and the virtual UART
    UART_RX_BUF_LEFT_ROOM,  // read only,only valid on the virtual UART
    UART_TX_BUF_LEFT_ROOM,   // read only,valid both on the physical UART and the virtual UART
    UART_TX_BUF_SEND_OUT     // read  only, only valid on the physical UART ports
}Enum_UARTOption;

typedef  enum {
    UART_PIN_RI = 0,    // read only, RI read operator only valid on the virtual UART
    UART_PIN_DCD,       // read only, DCD read operator only valid on the virtual UART
} Enum_UARTPinType ;

typedef enum {
    EVENT_UART_READY_TO_READ = 0,
    EVENT_UART_READY_TO_WRITE,
    EVENT_UART_FE_IND,
    EVENT_UART_RI_IND,
    EVENT_UART_DCD_IND,
    EVENT_UART_DTR_IND
} Enum_UARTEventType;

typedef struct {
    u16 txBufDeep;  // for physical UART1 and UART3 only, max  length 2048 byte
    u16 rxBufDeep;  // for physical UART1 and UART3 only, max length 3584 byte
    u16 txBufAlart; // for physical UART1 and UART3 only , should less than txBufDeep
    u16 rxBufAlart; // for physical UART1 and UART3only , should less than rxBufDeep
}ST_UartVfifo;

/*****************************************************************
* Function:     Ql_UART_Register 
* 
* Description:
*               This function registers the callback function for the 
*               specified serial port. 
*               UART callback function used for receiving the UART notification from CORE.
*
* Parameters:
*               [in]port:   
*                       Port name                            
*               [in]callback_uart:     
*                       The pointer of the UART call back function.
*               [in]event:
*                       indication the event type of UART call back, please reference Enum_UARTEventType.
*               [in]pinLevel:
*                       if the event type is EVENT_UART_RI_IND or EVENT_UART_DCD_IND or EVENT_UART_DTR_IND
*                       the level indication the relate pin's current level otherwise this parameter has no meaning, just ignore it.
*               [in]customizePara: 
*                       Customize parameter, if not use just set to NULL.    
*
* Return:        
*               QL_RET_OK indicates success; otherwise failure.
*
*****************************************************************/
typedef void (*CallBack_UART_Notify)( Enum_SerialPort port, Enum_UARTEventType event, bool pinLevel,void *customizePara);
s32 Ql_UART_Register(Enum_SerialPort port, CallBack_UART_Notify callback_uart,void * customizePara);


/*****************************************************************
* Function:     Ql_UART_Open 
* 
* Description:
*               This function opens a specified UART port with the specified 
*               flow control mode. 
*               Which task call this function, which task will own the specified UART port.
*
* Parameters:
*               [in]port:       
*                       Port name
*               [in]baud:      
*                       The baud rate of the UART to be opened
*                       for the physical the baud rate support 75,150,300,600,1200,2400,4800,7200,
*                       9600,14400,19200,28800,38400,57600,115200,230400,460800
*                       for the UART_PORT1 support auto-baud rate, when the baud set to 0.
*
*                       This parameter don't take,effect on the VIRTUAL_PORT1,VIRTUAL_PORT2,just set to 0
*
*               [in]flowCtrl:   
*                       one value of Enum_FlowCtrl, used for the physical UART's flow control.
*                       the hardware flow control(FC_HW)  only supported on the UART_PORT1,
*                       other port not support hardware flow control.
*                       This parameter don't take,effect on the VIRTUAL_PORT1,VIRTUAL_PORT2,just set to FC_NONE
*
* Return:        
*               QL_RET_OK indicates success; otherwise failure.       
*
*****************************************************************/
s32 Ql_UART_Open(Enum_SerialPort port,u32 baudrate, Enum_FlowCtrl flowCtrl);


/*****************************************************************
* Function:     Ql_UART_OpenEx 
* 
* Description:
*               This function opens a specified UART port with the 
*               specified DCB parameters.
*               Which task call this function, which task will own the specified UART port.
*
* Parameters:
*               [in]port:   
*                       Port name
*               [in]dcb:   
*                       The point to the UART dcb struct. 
*                       Include baud rate,data bits,stop bits,parity,and flow control
*                       only physical UART1(UART_PORT1) have the hardware flow control
*
*                       This parameter don't take effect on the VIRTUAL_PORT1,VIRTUAL_PORT2
*                       just set to NULL.
*
* Return:        
*               QL_RET_OK indicates success; otherwise failure.    
*
*****************************************************************/
s32 Ql_UART_OpenEx(Enum_SerialPort port, ST_UARTDCB *dcb);

/*****************************************************************
* Function:     Ql_UART_RX_Enable 
* 
* Description:
*               Enable specified uart rx func ,module can recieve data from terminals
*
* Parameters:
*               [in]port:       
*                       Port name ,limited to physical port ,see Enum_SerialPort
*
* Return:        
*               QL_RET_OK indicates success; otherwise failure.       
*
* Example:      Ql_UART_RX_Enable(UART_PORT1);
*
*
*
*****************************************************************/
s32 Ql_UART_RX_Enable(Enum_SerialPort port);


/*****************************************************************
* Function:     Ql_UART_RX_Disable 
* 
* Description:
*               Disable specified uart rx func,module can't recieve data from terminals
*
* Parameters:
*               [in]port:       
*                       Port name ,limited to physical port ,see Enum_SerialPort
*
* Return:        
*               QL_RET_OK indicates success; otherwise failure.       
*
*****************************************************************/
s32 Ql_UART_RX_Disable(Enum_SerialPort port);


/*****************************************************************
* Function:     Ql_UART_Write 
* 
* Description:
*               This function is used to send data to the specified UART port. 
*
*               When the number of bytes actually write data is less than that to send,
*               Application should stop sending data, and application will receive an event 
*               EVENT_UART_READY_TO_WRITE later. 
*               After receiving this Event Application can continue to send data, 
*               and previously unsent data should be resend.
*
*               NOTE:
*               The RX/TX buffer size of VIRTUAL_PORT1 and VIRTUAL_PORT2 both are 1024-byte,
*               So the number of bytes to write should generally not more than 1024.
*  
* Parameters:
*               [in] port:
*                       Port name
*               [in]data:
*                       Pointer to data to be send
*               [in]writeLen:
*                       The length of the data to be written
*
* Return:        
*               If >= 0, indicates success, and the return value 
*                        is the length of actual write data length
*               If <  0, indicates failure to read      
*
*****************************************************************/
s32 Ql_UART_Write(Enum_SerialPort port, u8* data, u32 writeLen );


/*****************************************************************
* Function:     Ql_UART_Read 
* 
* Description:
*               This function read data from the specified UART port.
*               When the UART callback is called, and the message type is 
*               EVENT_UART_READ_TO_READ,
*               user need read all of the data in the data buffer 
*               which means the real read len is less than the to be read len,
*               otherwise later the UART data is coming, there will be no such event to notify user get data.
*
* Parameters:
*               [in]port:
*                        Port name
*               [out]data:
*                       Point to the read data
*               [in]readLen:
*			    then length of data want to read
*
* Return:        
*               If >= 0, indicates success, and the return value 
*                        is the length of actual read data length
*               If <  0, indicates failure to read
*
*****************************************************************/
s32 Ql_UART_Read(Enum_SerialPort port, u8* data, u32 readLen);


/*****************************************************************
* Function:     Ql_UART_SetDCBConfig 
* 
* Description:
*                 This function used for setting the parameter of the UART port, for example the baud rate
*                 data bits, stop bits, parity,and flow control.
*                 This function is only effect on the physical port(UART_PORT1 UART_PORT2 UART_PORT3).
*                 the virtual port(VIRTUAL_PORT1,VIRTUAL_PORT2) don't support this function.
*
* Parameters:
*               [in]port:
*                        Port name
*               [in]dcb:
*                       the DCB struct to be set of the special UART
*
* Return:        
*               QL_RET_OK indicates success; otherwise failure.   
*
*****************************************************************/
s32  Ql_UART_SetDCBConfig(Enum_SerialPort port, ST_UARTDCB *dcb);


/*****************************************************************
* Function:     Ql_UART_GetDCBConfig 
* 
* Description:
*               This function used for getting the parameter of the UART port, for example the baud rata
*               data bits, stop bits, parity,and flow control.
*               This function is only effect on the physical port(UART_PORT1 UART_PORT2 UART_PORT3).
*                the virtual port(VIRTUAL_PORT1,VIRTUAL_PORT2) don't support this function.
*
* Parameters:
*               [in]port:
*                       Port name
*               [out]dcb:
*                      the DCB parameter of the special UART port
*
* Return:        
*               QL_RET_OK indicates success; otherwise failure.   
*
*****************************************************************/
s32  Ql_UART_GetDCBConfig(Enum_SerialPort port, ST_UARTDCB *dcb);


/*****************************************************************
* Function:     Ql_UART_ClrRxBuffer 
* 
* Description:
*               This function clears receive-buffer of specified UART port
*
* Parameters:
*               [in]port:
*                        Port name
*
* Return:        
*              NONE   
*
*****************************************************************/
void  Ql_UART_ClrRxBuffer(Enum_SerialPort port);


/*****************************************************************
* Function:     Ql_UART_ClrTxBuffer 
* 
* Description:
*               This function clears send-buffer of specified UART port.
*
* Parameters:
*               [in]port:
*                       Port name
*
* Return:        
*               NONE  
*
*****************************************************************/
void  Ql_UART_ClrTxBuffer(Enum_SerialPort port);


/*****************************************************************
* Function:     Ql_UART_GetOption 
* 
* Description:
*               This function used for getting the UART option, such as physical UART port's vfifo,
*               or the send or receive buffer left space on the UART port.
*
* Parameters:
*              [in]port:
*                       Port name
*              [in]opt:
*                       one value of Enum_UARTOption
*                        the get option of UART_VFIFO is only valid on the UART_PORT1 and UART_PORT3
*                        the get option of UART_CLR_FE is only valid on the UART_PORT1 and UART_PORT2 and UART_PORT3 
*                        the get option of UART_RX_BYTE_AVAIL valid both on the vitual or the physical port
*                        the get option of UART_RX_BUF_LEFT_ROOM only valid on the vitual port
*                        the get option of UART_TX_BYTE_AVAIL valid both on the vitual or physical port
*                        the get option of UART_TX_BUF_SEND_OUT is only valid on the UART_PORT1 and UART_PORT2 and UART_PORT3
*              [out]para:
*			    if opt is UART_VFIFO, point to a struct of ST_UartVfifo
*                       if opt is UART_RX_BYTE_AVAIL or UART_RX_BUF_LEFT_ROOM or UART_TX_BUF_LEFT_ROOM
*		           this parameter should a point of u32
*
* Return:        
*               QL_RET_OK indicates success; otherwise failure.     
*
*****************************************************************/
s32 Ql_UART_GetOption(Enum_SerialPort port,Enum_UARTOption opt, void* para);


/*****************************************************************
* Function:     Ql_UART_SetOption 
* 
* Description:
*                This function used for setting the UART option, such as physical UART port's vfifo, 
*                or clear the physical UART's(UART_PORT1 UART_PORT2 and UART_PORT3) frame error.
*
* Parameters:
*               [in]port:
*                         Port name
*               [in]opt:
*                        one value of Enum_UARTOption
*                        the set option of UART_VFIFO is only valid on the UART_PORT1 and UART_PORT3
*                        the set option of UART_CLR_FE is only valid on the UART_PORT1 and UART_PORT2 and UART_PORT3 
*                        other option don't support set operator.                       
*               [in]para:
*           		     if opt is UART_VFIFO, point to a struct of ST_UartVfifo
*                        if opt is UART_CLR_FE,this parameter should be set to NULL.
*
* Return:        
*               QL_RET_OK indicates success; otherwise failure.     
*
*****************************************************************/
s32 Ql_UART_SetOption(Enum_SerialPort port,Enum_UARTOption opt, void* para);


/*****************************************************************
* Function:     Ql_UART_GetPinStatus 
* 
* Description:
*                This function gets the pin status (include RI, DCD, DTR) 
*                of the virtual UART port. It doesn't work for the physical UART ports.
*
* Parameters:
*               port:
*                   [in] virtual UART port.
*               pin:
*                   [in] pin name, one value of Enum_UARTPinType.
*
* Return:        
*               If >= 0, indicates success, and the return special pin level value
*                        0:low level, 1:high level.
*               If <  0, indicates failure   
*
*****************************************************************/
s32 Ql_UART_GetPinStatus(Enum_SerialPort port, Enum_UARTPinType pin);


/*****************************************************************
* Function:     Ql_UART_SetPinStatus 
* 
* Description:
*                This function sets the pin level status of the virtual
*                UART port. It doesn't work for the physical UART ports.
*
* Parameters:
*               port:
*                   [in] virtual UART port.
*               pin:
*                   [in] pin name, one value of Enum_UARTPinType.
*
*               pinLevel:
*                   [in] the level value to set. 0:low level, 1:high level.
* Return:        
*               QL_RET_OK indicates success; otherwise failure.         
*
*****************************************************************/
s32 Ql_UART_SetPinStatus(Enum_SerialPort port, Enum_UARTPinType pin, bool pinLevel);

/*****************************************************************
* Function:     Ql_UART_SendEscap 
* 
* Description:
*               This function notifies the port to quit from Data Mode, and return back to Command Mode.
*               This function only supported on the VIRTUAL_PORT1, VIRTUAL_PORT2.
*
* Parameters:
*              [in] port:
*                        Port name
*                        this function only supported on the VIRTUAL_PORT1, VIRTUAL_PORT2.
*
* Return:        
*               QL_RET_OK indicates success; otherwise failure.          
*****************************************************************/
s32 Ql_UART_SendEscap(Enum_SerialPort port);

/*****************************************************************
* Function:     Ql_UART_Close 
* 
* Description:
*               This function close a specified UART port.
* Parameters:
*               [in] port:
*                      Port name
* Return:        
*             NONE  
*
*****************************************************************/
void Ql_UART_Close(Enum_SerialPort port);

#endif  // End-of __QL_UART_H__

