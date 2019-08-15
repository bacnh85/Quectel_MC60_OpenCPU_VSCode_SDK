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
 *   ql_socket.h 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   socket APIs defines.
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
 

#ifndef __QL_SOCKET_H__
#define __QL_SOCKET_H__

/* Socket Type */ 
typedef enum {
    SOC_TYPE_TCP = 0,  /* TCP */ 
    SOC_TYPE_UDP       /* datagram socket, UDP */ 
} Enum_SocketType;

/****************************************************************************
 * Return Codes Definition for TCP/IP
 ***************************************************************************/
typedef enum {
    SOC_SUCCESS             = 0,
    SOC_ERROR               = -1,
    SOC_WOULDBLOCK          = -2,
    SOC_LIMIT_RESOURCE      = -3,    /* limited resource */
    SOC_INVALID_SOCKET      = -4,    /* invalid socket */
    SOC_INVALID_ACCOUNT     = -5,    /* invalid account id */
    SOC_NAMETOOLONG         = -6,    /* address too long */
    SOC_ALREADY             = -7,    /* operation already in progress */
    SOC_OPNOTSUPP           = -8,    /* operation not support */
    SOC_CONNABORTED         = -9,    /* Software caused connection abort */
    SOC_INVAL               = -10,   /* invalid argument */
    SOC_PIPE                = -11,   /* broken pipe */
    SOC_NOTCONN             = -12,   /* socket is not connected */
    SOC_MSGSIZE             = -13,   /* msg is too long */
    SOC_BEARER_FAIL         = -14,   /* bearer is broken */
    SOC_CONNRESET           = -15,   /* TCP half-write close, i.e., FINED */
    SOC_DHCP_ERROR          = -16,
    SOC_IP_CHANGED          = -17,
    SOC_ADDRINUSE           = -18,
    SOC_CANCEL_ACT_BEARER   = -19    /* cancel the activation of bearer */
} Enum_SocErrCode;

typedef struct {
    void(*callback_socket_connect)(s32 socketId, s32 errCode, void* customParam );
    void(*callback_socket_close)(s32 socketId, s32 errCode, void* customParam );
    void(*callback_socket_accept)(s32 listenSocketId, s32 errCode, void* customParam );
    void(*callback_socket_read)(s32 socketId, s32 errCode, void* customParam );
    void(*callback_socket_write)(s32 socketId, s32 errCode, void* customParam );
}ST_SOC_Callback;

typedef struct {
    s32 optionType;
    void* pOptionParam;
} ST_SocketOption;


/*****************************************************************
* Function:     Ql_SOC_Register
*
* Description:
*               This function registers callback functions for the specified socket.
*
* Parameters:
*               cb:
*                   [In] The pointer of the socket-related callback function.
*
*               customParam:
*                   [In] One customized parameter that can be passed into the callback functions.
*
* Return:
*               The socket ID, or other Error Codes. To get extended
*               information, please see Enum_SocErrCode.
*****************************************************************/
s32 Ql_SOC_Register(ST_SOC_Callback cb, void* customParam);


/*****************************************************************
* Function:     Ql_SOC_Create
*
* Description:
*               This function creates a socket. The maximum number of socket is 6.
*
* Parameters:
*               contxtId:
*                   OpenCPU supports two PDP-contexts to the destination
*                   host at a time. This parameter can be 0 or 1.
*
*               socketType:
*                   A value of socket_type_enum.
*
* Return:
*               The socket ID, or other Error Codes. To get extended
*               information, please see Enum_SocErrCode.
*****************************************************************/
s32     Ql_SOC_Create(u8 contextId, u8 socketType);


/*****************************************************************
* Function:     Ql_SOC_CreateEx
*
* Description:
*               This function creates a socket. The maximum number of socket is 6.
*
* Parameters:
*               contxtid:
*                   OpenCPU supports two PDP-contexts to the destination
*                   host at a time. This parameter can be 0 or 1.
*
*               socketType:
*                   A value of socket_type_enum.
*               
*               taskId:
*                   A taskId 
*
*               cb:
*
* Return:
*               The socket ID, or other Error Codes. To get extended
*               information, please see soc_error_enum.
*****************************************************************/
s32     Ql_SOC_CreateEx(u8 contextId, u8 socketType,s32 taskId, ST_SOC_Callback cb);


/*****************************************************************
* Function:     Ql_SOC_Close
*
* Description:
*               This function closes a socket.
*
* Parameters:
*               socketId:
*                   A socket Id.
*
* Return:
*               This return value will be SOC_SUCCESS (0) if
*               the function succeeds; or a negative number
*               (Error Code) will be returned.
*****************************************************************/
s32   Ql_SOC_Close(s32 socketId);


/*****************************************************************
* Function:     Ql_SOC_Connect
*
* Description:
*               This function connects to TCP server. The server
*               is specified by an IP address and a port number.
*
* Parameters:
*               socketId:
*                   A socket Id.
*
*               remoteIP:
*                   A address pointer to the IPv4 address. Please see the example for usage.
*
*               remotePort:
*                   Socket port.
* Return:
*               SOC_SUCCESS: the function succeeds.
*               SOC_WOULDBLOCK: this return value is for none-blocking mode. 
*                   It mean the operation is in progressing, needs to wait callback_socket_connect() for the result.
*               Other values: error code, please refer to Enum_SocErrCode.
*======================
* Example:
*======================
*    {
*         u8 ip_addr[4] = {10, 85, 116, 131};
*         ret = Ql_SOC_Connect(soc_id,(u32)ip_addr, 6005);
*    }
*****************************************************************/
s32   Ql_SOC_Connect(s32 socketId, u32 remoteIP, u16 remotePort);


/*****************************************************************
* Function:     Ql_SOC_ConnectEx
*
* Description:
*               This function connects to TCP server. The server
*               is specified by an IP address and a port number.
*
*               This function supports two modes:
*               1. non-blocking mode
*                  When the "isBlocking" is set to FALSE, this function works
*                  under non-blocking mode. The result will be returned even if
*                  the operation is not done, and the result will be reported in callback.
*               2. blocking mode
*                  When the "isBlocking" is set to TRUE, this function works
*                  under blocking mode. The result will be returned only after the operation is done.
*
*               If working under non-blocking mode, this function is same as 
*               Ql_SOC_Connect() functionally.
* Parameters:
*               socketId:
*                   A socket Id.
*
*               remoteIP:
*                   Peer IPv4  address.
*
*               remotePort:
*                   Socket port.
*
*               isBlocking:
*                   blocking mode. TRUE=blocking mode, FALSE=non-blocking mode.
* Return:
*               SOC_SUCCESS: the function succeeds.
*               Other values: error code, please refer to Enum_SocErrCode.
*======================
* Example:
*======================
*    {
*         u8 ip_addr[4] = {10, 85, 116, 131};
*         ret = Ql_SOC_Connect(soc_id,(u32)ip_addr, 6005);
*    }
*****************************************************************/
s32   Ql_SOC_ConnectEx(s32 socketId, u32 remoteIP, u16 remotePort, bool isBlocking);

/*****************************************************************
* Function:     Ql_SOC_Send
*
* Description:
*               The function sends data to a connected TCP socket.
*
* Parameters:
*               socketId:
*                   Socket Id.
*
*               pData:
*                   Pointer to the data to send.
*
*               dataLen:
*                   Number of bytes to send.
* Return:
*               If no error occurs, Ql_SOC_Send returns the total
*               number of bytes sent, which can be less than the number
*               requested to be sent in the dataLen parameter.
*               Otherwise, a value of Enum_SocErrCode is returned.
*****************************************************************/
s32   Ql_SOC_Send(s32 socketId, u8* pData, s32 dataLen);


/*****************************************************************
* Function:     Ql_SOC_Recv
*
* Description:
*               The function receives data from a bound socket.
*
* Parameters:
*               socketId:
*                   A socket Id.
*
*               pBuffer:
*                   Point to a buffer that is the storage space
*                   for the received data.
*
*               bufferLen:
*                   Length of pData, in bytes.
* Return:
*               If no error occurs, Ql_SOC_Recv returns the total
*               number of bytes received. Otherwise, a value of
*               Enum_SocErrCode is returned.
*****************************************************************/
s32   Ql_SOC_Recv(s32 socketId, u8* pBuffer, s32 bufferLen);


/*****************************************************************
* Function:     Ql_SOC_GetAckNumber
*
* Description:
*               The function gets the TCP socket ACK number.
*
* Parameters:
*               socket:
*                   [in] Socket Id.
*
*               ackNum:
*                   [out] Point to a u64 type  variable that is the storage
*                   space for the TCP ACK number
* Return:
*               If no error occurs, this return value will be SOC_SUCCESS (0). 
*               Otherwise, a value of "Enum_SocErrCode" is returned.
*****************************************************************/
s32   Ql_SOC_GetAckNumber(s32 socketId, u64* ackNum);


/*****************************************************************
* Function:     Ql_SOC_SendTo
*
* Description:
*               The function sends data to a specific destination
*               through UDP socket.
*
* Parameters:
*               socketId:
*                   [in] Socket Id to send to.
*
*               pData:
*                   [in] Buffer containing the data to be transmitted.
*
*               dataLen:
*                   [in] Length of the data in pData, in bytes.
*
*               remoteIP:
*                   [in] Point to the address of the target socket.
*
*               remotePort:
*                   [in] The target port number.
* Return:
*               If no error occurs, this function returns the number
*               of bytes actually sent. Otherwise, a value of
*               Enum_SocErrCode is returned.
*****************************************************************/
s32   Ql_SOC_SendTo(s32 socketId, u8* pData, s32 dataLen, u32 remoteIP, u16 remotePort);


/*****************************************************************
* Function:     Ql_SocketRecvFrom
*
* Description:
*               The function receives a datagram data through TCP socket.
*
* Parameters:
*               socketId:
*                   [in] Socket Id to receive from.
*
*               pBuffer:
*                   [out] Buffer for the incoming data.
*
*               recvLen:
*                   [out] Length of pData, in bytes.
*
*               remoteIP:
*                   [out] An optional pointer to a buffer that
*                   receives the address of the connecting entity.
*
*               remotePort:
*                   [out] An optional pointer to an integer that
*                   contains the port number of the connecting entity.
* Return:
*               If no error occurs, this function returns the number
*               of bytes received. Otherwise, a value of
*               Enum_SocErrCode is returned.
*****************************************************************/
s32   Ql_SOC_RecvFrom(s32 socketId, u8* pBuffer, s32 recvLen, u32* remoteIP, u16* remotePort);


/*****************************************************************
* Function:     Ql_SOC_Listen
*
* Description:
*               The function places a socket in a state in which
*               it is listening for an incoming connection.
*
* Parameters:
*               listenSocketId:
*                   [in] The listened socket id.
*
*               maxClientNum:
*                   [in] Maximum connection number.Limiting the maximum length of the request queue.
*                            The maximum is 5.
*
* Return:
*               If no error occurs, this function returns SOC_SUCCESS (0).
*               Otherwise, a value of Enum_SocErrCode is returned.
*****************************************************************/
s32   Ql_SOC_Listen(s32 listenSocketId, s32 maxClientNum);


/*****************************************************************
* Function:     Ql_SOC_Bind
*
* Description:
*               This function associates a local address with a socket.
*
* Parameters:
*               socketId:
*                   [In] Descriptor identifying an unbound socket.
*

*               LocalPort:
*                   [in] Socket port number.
*
* Return:
*               If no error occurs, this function returns SOC_SUCCESS (0).
*               Otherwise, a value of Enum_SocErrCode is returned.
*****************************************************************/
s32   Ql_SOC_Bind(s32 socketId, u16 LocalPort);


/*****************************************************************
* Function:     Ql_SOC_Accept
*
* Description:
*               The function permits a connection attempt on a socket.
*
* Parameters:
*               listenSocketId:
*                   [in]  The listened socket id.
*
*               remoteIP:
*                   [out] An optional pointer to a buffer that
*                   receives the address of the connecting entity.
*
*               remotePort:
*                   [out] An optional pointer to an integer that
*                   contains the port number of the connecting entity.
*
* Return:
*               If no error occurs, this function returns a socket Id,the socket Id is used to send data for tcp server.
*               and it is greater than or equal to zero.
*               Otherwise, a value of Enum_SocErrCode is returned.
*****************************************************************/
s32   Ql_SOC_Accept(s32 listenSocketId, u32 * remoteIP, u16* remotePort);


/*****************************************************************
* Function:     Ql_IpHelper _GetIPByHostName
*
* Description:
*               The function retrieves host IP corresponding to a host name.
*
* Parameters:
*               contxtId:
*                   [in] OpenCPU supports two PDP-contexts to the destination
*                   host at a time. This parameter can be 0 or 1.
*
*               hostName:
*                   [in] The host name.
*
*               requestId:
*	             [Out] Embedded in response message.
*
*               errCode:	
*                   [Out] Error code if fail
*
*               ipAddrCnt: 	
*                   [Out] Get address number.
*
*               ipAddr:
*                   [Out] The host IPv4 address.
*
*               callback_GetIpByName:
*                   [in] This callback is called by Core System to notify
*                   whether this function retrieves host IP successfully or not.
*
* Return:
*               If no error occurs, this function returns SOC_SUCCESS (0).
*               Otherwise, a value of Enum_SocErrCode is returned.
*****************************************************************/
typedef void (*Callback_IpHelper_GetIpByName)(u8 contexId, u8 requestId, s32 errCode,  u32 ipAddrCnt, u32* ipAddr);
s32 Ql_IpHelper_GetIPByHostName(u8 contextId, 
                                u8 requestId,
                                u8 *hostName, 
                                Callback_IpHelper_GetIpByName  callback_GetIpByName);

/*****************************************************************
* Function:     Ql_IpHelper_GetIPByHostNameEx
*
* Description:
*               The function gets the IP of the given domain name.
*               it could return multiple IP addresses (max. 5 IP address)
*               This function works under blocking mode.
*
* Parameters:
*               contxtId:
*                   [in] OpenCPU supports two PDP-contexts to the destination
*                   host at a time. This parameter can be 0 or 1.
*
*               requestId:
*	                [out] Embedded in response message.
*
*               hostName:
*                   [in] The host name.
*
*               ipCount:
*                   [out] ip address number.
*
*               ipAddress: 	
*                   [out] The host IPv4 address.
*
* Return:
*               If no error occurs, this function returns SOC_SUCCESS (0).
*               The IP address is in "ipAddresss", and the IP count is in "ipCount".
*
*               Otherwise, a value of Enum_SocErrCode is returned.
*****************************************************************/
s32 Ql_IpHelper_GetIPByHostNameEx(u8 contextId, 
                                  u8 requestId,
                                  u8 *hostName, 
                                  u32* ipCount,
                                  u32* ipAddress);

/*****************************************************************
* Function:     Ql_IpHelper_ConvertIpAddr
*
* Description:
*               This function checks whether an IP address is valid
*               IP address or not. If 'yes', each segment of the
*               IP address string will be converted into integer to
*               store in ipaddr parameter.
*
* Parameters:
*               addrString:
*                   [in] IP address string.
*
*               ipaddr:
*                   [out] Point to a u32 variable, each byte stores the IP digit
*                   converted from the corresponding IP string.
*
* Return:
*               If no error occurs, this function returns SOC_SUCCESS (0).
*               Otherwise, a value of Enum_SocErrCode is returned.
*****************************************************************/
s32 Ql_IpHelper_ConvertIpAddr(u8 *addrString, u32* ipAddr);


/*****************************************************************
* Function:     Ql_SOC_SetOption
*
* Description:
*               This function sets the socket options
*
* Parameters:
*
* Return:
*               If no error occurs, this function returns SOC_SUCCESS (0).
*               Otherwise, a value of Enum_SocErrCode is returned.
*****************************************************************/
s32   Ql_SOC_SetOption(s32 socketId, s32 optionType, u32 optionParam);

#endif //__QL_TCPIP_H__
