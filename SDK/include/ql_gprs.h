
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
 *   ql_gprs.h 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   GPRS APIs definition.
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


#ifndef __QL_GPRS_H__
#define __QL_GPRS_H__

#define MAX_GPRS_USER_NAME_LEN 32
#define MAX_GPRS_PASSWORD_LEN  32
#define MAX_GPRS_APN_LEN       100


typedef struct {
    void (*Callback_GPRS_Actived)(u8 contexId, s32 errCode, void* customParam);
    void (*CallBack_GPRS_Deactived)(u8 contextId, s32 errCode, void* customParam );
}ST_PDPContxt_Callback;

typedef struct {
    u8 apnName[MAX_GPRS_APN_LEN];
    u8 apnUserId[MAX_GPRS_USER_NAME_LEN]; 
    u8 apnPasswd[MAX_GPRS_PASSWORD_LEN]; 
    u8 authtype; // pap or chap
    void* Reserved1;  // Qos
    void* Reserved2;  //
} ST_GprsConfig;


typedef enum {
    GPRS_PDP_SUCCESS             = 0,
    GPRS_PDP_ERROR               = -1,
    GPRS_PDP_WOULDBLOCK          = -2,
    GPRS_PDP_ALREADY             = -7,    /* operation already in progress */
    GPRS_PDP_INVAL               = -10,   /* invalid argument */
    GPRS_PDP_BEARER_FAIL         = -14,   /* bearer is broken */
} Enum_GPRS_PDPError;


/*****************************************************************
* Function:     Ql_GPRS_GetPDPContextId
*
* Description:
*               This function retrieves an available PDP context Id, 
*               which can be 0 or 1. 
*
* Parameters:
*               None.
* Return:
*               0 or 1 is returned if this function succeeds.
*               GPRS_PDP_ERROR indicates this function fails, or no
*               PDP context Id is available.
*****************************************************************/
s32 Ql_GPRS_GetPDPContextId(void);

/*****************************************************************
* Function:     Ql_GPRS_Register
*
* Description:
*               This function registers the GPRS related callback functions.
*
* Parameters:
*               contextId:
*                   Module supports two PDP-context. It can be 0 or 1. 
*                   User may call Ql_GPRS_GetPDPContextId to get an available PDP context Id.
*                   Within each PDP context, 6 socket links can be created.
*
*               callback_func:
*                   A group of callback functions, including GPRS activate and deactivate, which
*                   is implemented by Embedded Application and is invoked by Core System.
*
*               customParam:
*                   A customized parameter, which can be used in the callback function. 
*                   It may be NULL if no customized parameter needs to be passed in.
* Return:
*               GPRS_PDP_SUCCESS:
*                   This function succeeds.
*
*               GPRS_PDP_INVAL:
*                   Invalid argument.
*
*               GPRS_PDP_ALREADY:
*                   The GPRS network is already initialized.
*****************************************************************/
s32   Ql_GPRS_Register(u8 contextId,  ST_PDPContxt_Callback * callback_func, void* customParam);

/*****************************************************************
* Function:     Ql_GPRS_Config
*
* Description:
*               This function configures GPRS parameters, including 
*               APN name, user name, password and authentication type
*               for the specified PDP context.
*
* Parameters:
*               contextId:
*                   PDP context id, which can be 0 or 1.
*
*               cfg:
*                   PDP configuration structure.
*
* Return:
*               GPRS_PDP_SUCCESS:
*                   This function succeeds.
*
*               GPRS_PDP_INVAL:
*                   Invalid argument.
*======================
* Example:
*======================
*               Take "China Mobile" for example, the APN name for public
*               subscribers is "CMNET", and the user name and password are
*               not necessary. So developer can configure GPRS as below:
*               {
*                   ST_GprsConfig  gprsCfg;
*                   Ql_strcpy(gprsCfg.apnName, "CMNET\0");
*                   Ql_memset(gprsCfg.apnUserId, 0x0, sizeof(gprsCfg.apnUserId));
*                   Ql_memset(gprsCfg.apnPasswd, 0x0, sizeof(gprsCfg.apnPasswd));
*                   gprsCfg.authtype = 0;
*                   gprsCfg.Reserved1 = 0;
*                   gprsCfg.Reserved2 = 0;
*                   Ql_GPRS_Config(0, &gprsCfg);
*               }
*****************************************************************/
s32 Ql_GPRS_Config(u8 contextId, ST_GprsConfig* cfg);

/*****************************************************************
* Function:     Ql_GPRS_Activate
*
* Description:
*               This function activates the specified PDP context.
*               The maximum possible time for Activating GPRS is 180s.
* Parameters:
*               contextId:
*                   PDP context id, which can be 0 or 1.
*
* Return:
*               GPRS_PDP_SUCCESS:
*                   This function succeeds, and activating GPRS succeeds.
*
*               GPRS_PDP_WOULDBLOCK:
*                   The app should wait, till the callback function is called.
*                   The app gets the information of success or failure in callback function.
*                   The maximum possible time for Activating GPRS is 180s.
*
*               GPRS_PDP_INVAL:
*                   Invalid argument.
*
*               GPRS_PDP_ALREADY:
*                   The activating operation is in process.
*
*               GPRS_PDP_BEARER_FAIL:
*                   Bearer is broken.
*======================
* Example:
*======================
*               The following codes show the activating GPRS processing.
*               {
*                   s32 ret;
*                   ret = Ql_GPRS_Activate(0);
*                   if (GPRS_PDP_SUCCESS == ret)
*                   {
*                       // Activate GPRS successfully
*                   }
*                   else if (GPRS_PDP_WOULDBLOCK == ret)
*                   {
*                       // Activating GPRS, need to wait Callback_GPRS_Actived for the result
*                   }
*                   else if (GPRS_PDP_ALREADY == ret)
*                   {
*                       // GPRS has been activating...
*                   }else{
*                       // Fail to activate GPRS, error code is in "ret".
*                       // Developer may retry to activate GPRS, and reset the module after 3 successive failures.
*                   }
*               }
*****************************************************************/
s32   Ql_GPRS_Activate(u8 contextId);

/*****************************************************************
* Function:     Ql_GPRS_ActivateEx
*
* Description:
*               This function activates the specified PDP context.
*               The maximum possible time for Activating GPRS is 180s.
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
*               Ql_GPRS_Activate() functionally.
* Parameters:
*               contextId:
*                   PDP context id, which can be 0 or 1.
*
*               isBlocking:
*                   blocking mode. TRUE=blocking mode, FALSE=non-blocking mode.
*
* Return:
*               GPRS_PDP_SUCCESS:
*                   This function succeeds, and activating GPRS succeeds.
*
*               GPRS_PDP_INVAL:
*                   Invalid argument.
*
*               GPRS_PDP_ALREADY:
*                   The activating operation is in process.
*
*               GPRS_PDP_BEARER_FAIL:
*                   Bearer is broken.
*======================
* Example:
*======================
*               The following codes show the activating GPRS processing.
*               {
*                   s32 ret;
*                   ret = Ql_GPRS_Activate(0, TRUE);
*                   if (GPRS_PDP_SUCCESS == ret)
*                   {
*                       // Activate GPRS successfully
*                   }
*                   else if (GPRS_PDP_ALREADY == ret)
*                   {
*                       // GPRS has been activating...
*                   }else{
*                       // Fail to activate GPRS, error code is in "ret".
*                       // Developer may retry to activate GPRS, and reset the module after 3 successive failures.
*                   }
*               }
*****************************************************************/
s32   Ql_GPRS_ActivateEx(u8 contextId, bool isBlocking);

/*****************************************************************
* Function:     Ql_GPRS_Deactivate
*
* Description:
*               This function deactivates the specified PDP context.
*               The maximum possible time for Activating GPRS is 90s.
*
* Parameters:
*               contextId:
*                   PDP context id that is specified when calling Ql_GPRS_Activate, 
*                   which can be 0 or 1. 
*
* Return:
*               GPRS_PDP_SUCCESS:
*                   This function succeeds, and deactivating GPRS succeeds.
*
*               GPRS_PDP_WOULDBLOCK:
*                   The app should wait, till the callback function is called.
*                   The app gets the information of success or failure in callback function.
*                   The maximum possible time for Activating GPRS is 90s.
*
*               GPRS_PDP_INVAL:
*                   Invalid argument.
*
*               GPRS_PDP_ERROR
*                   GPRS is not in active state, or other error.
*
*======================
* Example:
*======================
*               The following codes show the deactivating GPRS processing.
*               {
*                   s32 ret;
*                   ret = Ql_GPRS_Deactivate(0);
*                   if (GPRS_PDP_SUCCESS == ret)
*                   {
*                       // GPRS is deactivated successfully
*                   }
*                   else if (GPRS_PDP_WOULDBLOCK == ret)
*                   {
*                       // Deactivating GPRS, need to wait Callback_GPRS_Deactived for the result
*                   }else{
*                       // Fail to activate GPRS, error code is in "ret".
*                   }
*               }
*****************************************************************/
s32   Ql_GPRS_Deactivate(u8 contextId);


/*****************************************************************
* Function:     Ql_GPRS_DeactivateEx
*
* Description:
*               This function deactivates the specified PDP context.
*               The maximum possible time for Activating GPRS is 90s.
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
*               Ql_GPRS_Deactivate() functionally.
* Parameters:
*               contextId:
*                   PDP context id that is specified when calling Ql_GPRS_Activate, 
*                   which can be 0 or 1. 
*
*               isBlocking:
*                   blocking mode. TRUE=blocking mode, FALSE=non-blocking mode.
*
* Return:
*               GPRS_PDP_SUCCESS:
*                   This function succeeds, and activating GPRS succeeds.
*
*               GPRS_PDP_INVAL:
*                   Invalid argument.
*
*               GPRS_PDP_ALREADY:
*                   The activating operation is in process.
*
*               GPRS_PDP_BEARER_FAIL:
*                   Bearer is broken.
*======================
* Example:
*======================
*               The following codes show the deactivating GPRS processing.
*               {
*                   s32 ret;
*                   ret = Ql_GPRS_Deactivate(0, TRUE);
*                   if (GPRS_PDP_SUCCESS == ret)
*                   {
*                       // GPRS is deactivated successfully
*                   }else{
*                       // Fail to activate GPRS, error code is in "ret".
*                   }
*               }
*****************************************************************/
s32   Ql_GPRS_DeactivateEx(u8 contextId, bool isBlocking);

/*****************************************************************
* Function:     Ql_GPRS_GetLocalIPAddress
*
* Description:
*               This function retrieves local IP corresponding
*               to the specified PDP context.
*
* Parameters:
*               contextId:
*                   [in] PDP context id that is specified when calling Ql_GPRS_Activate, 
*                        which can be 0 or 1. 
*
*               ipAddr:
*                   [out] Pointer to the buffer that stores the local
*                   IPv4 address.
*
* Return:
*               GPRS_PDP_SUCCESS:
*                   This function succeeds.
*
*               GPRS_PDP_INVAL:
*                   Invalid argument.
*
*               GPRS_PDP_BEARER_FAIL:
*                   Bearer is broken.
*======================
* Example:
*======================
*               The following codes show the deactivating GPRS processing.
*               {
*                   s32 ret;
*                   u8 ip_addr[4];
*                   Ql_memset(ip_addr, 0, sizeof(ip_addr));
*                   ret = Ql_GPRS_GetLocalIPAddress(0, (u32 *)ip_addr);
*                   if (GPRS_PDP_SUCCESS == ret)
*                   {
*                       Ql_Debug_Trace("<-- Get Local Ip successfully,Local Ip=%d.%d.%d.%d -->\r\n", 
*                           ip_addr[0],ip_addr[1],ip_addr[2],ip_addr[3]);
*                   }else{
*                       // Fail to get Local Ip address.
*                   }
*               }
*****************************************************************/
s32   Ql_GPRS_GetLocalIPAddress(u8 contextId, u32* ipAddr);


/*****************************************************************
* Function:     Ql_GPRS_GetDNSAddress
*
* Description:
*               This function retrieves the DNS server's IP address.
*
* Parameters:
*               contextId:
*                   [in] PDP context id that is specified when calling Ql_GPRS_Activate, 
*                        which can be 0 or 1. 
*
*               firstAddr:
*                   [out] Point to the buffer that stores the primary 
*                   DNS server's IP address.
*
*               secondAddr:
*                   [out] Point to the buffer that stores the secondary 
*                   DNS server's IP address.
*
* Return:
*               GPRS_PDP_SUCCESS:
*                   This function succeeds.
*
*               GPRS_PDP_INVAL:
*                   Invalid argument.
*
*               GPRS_PDP_BEARER_FAIL:
*                   Bearer is broken.
*======================
* Example:
*======================
*               The following codes show how to get DNS IP address.
*               {
*                   s32 ret;
*                   u8 ipAddr_1st[4];
*                   u8 ipAddr_2st[4];
*                   Ql_memset(ip_addr, 0, sizeof(ipAddr_1st));
*                   Ql_memset(ip_addr, 0, sizeof(ipAddr_2st));
*                   ret = Ql_GPRS_GetDNSAddress(0, (u32*)ipAddr_1st, (u32*)ipAddr_2st);
*                   if (GPRS_PDP_SUCCESS == ret)
*                   {
*                       Ql_Debug_Trace("<-- Get DNS address successfully,first DNS Addr=%d.%d.%d.%d, second DNS Addr=%d.%d.%d.%d -->\r\n", 
*                           ipAddr_1st[0],ipAddr_1st[1],ipAddr_1st[2],ipAddr_1st[3],ipAddr_2st[0],ipAddr_2st[1],ipAddr_2st[2],ipAddr_2st[3]);            
*                   }
*                   else
*                   {
*                       // Fail to get DNS address.
*                   }
*               }
*****************************************************************/
s32   Ql_GPRS_GetDNSAddress(u8 contextId, u32* firstAddr, u32* secondAddr);


/*****************************************************************
* Function:     Ql_GPRS_SetDNSAddress
*
* Description:
*               This function sets the DNS server's IP address.
*
* Parameters:
*               contextId:
*                   [in] PDP context id that is specified when calling Ql_GPRS_Activate, 
*                        which can be 0 or 1. 
*
*               firstAddr:
*                   [in] A u32 integer that stores the IPv4 address of the first DNS server.
*
*               secondAddr:
*                   [in] A u32 integer that stores the IPv4 address of the second DNS server.
*
* Return:
*               GPRS_PDP_SUCCESS:
*                   This function succeeds.
*
*               GPRS_PDP_INVAL:
*                   Invalid argument.
*
*               GPRS_PDP_BEARER_FAIL:
*                   Bearer is broken.
*======================
* Example:
*======================
*               The following codes show how to set DNS IP address.
*               {
*                   s32 ret;
*                   u32 ip_dns1[5];
*                   u32 ip_dns2[5];
*                   u8 ipAddr_1st[] = "223.5.5.5";
*                   u8 ipAddr_2st[] = "223.6.6.6";
*               
*                   Ql_memset(ip_dns1,0,5);
*                   Ql_memset(ip_dns2,0,5);
*                   Ql_IpHelper_ConvertIpAddr(ipAddr_1st, ip_dns1);
*                   Ql_IpHelper_ConvertIpAddr(ipAddr_2st, ip_dns2);
*                   ret = Ql_GPRS_SetDNSAddress(0, ip_dns1, ip_dns2);
*                   if (GPRS_PDP_SUCCESS == ret)
*                   {
*                       // Set DNS IP address successfully
*                   }
*                   else
*                   {
*                       // Fail to set DNS IP address.
*                   }
*               }
******************************************************************/
s32  Ql_GPRS_SetDNSAddress(u8 contextId, u32 firstAddr, u32 secondAddr);

/*****************************************************************
* Function:     Callback_GPRS_Active
*
* Description:
*               This callback function is invoked by Ql_GPRS_Activate() 
*               when the return value of Ql_GPRS_Activate() is GPRS_PDP_WOULDBLOCK .
*
* Parameters:
*               contextId:
*                   [out] PDP context id that is specified when calling Ql_GPRS_Activate, 
*                        which can be 0 or 1.
*
*               errCode:
*                   Error code, refer to Enum_GPRS_PDPError. It indicates activating GPRS
*                   successfully when it's equal to GPRS_PDP_SUCCESS.
*
*               customParam:
*                   The customized parameter, which is passed in by calling Ql_GPRS_Activate().
* Return:
*              None.
*======================
* Example:
*======================
*               The following code is a possible implementation for Callback_GPRS_Actived.
*               void Callback_GPRS_Actived(u8 contexId, s32 errCode, void* customParam)
*               {
*                   if(GPRS_PDP_SUCCESS == errCode)
*                   {
*                       // Activate GPRS successfully
*                   }else{
*                       // Fail to activate GPRS
*                   }
*               }
*****************************************************************/
typedef void (*Callback_GPRS_Active)(u8 contextId, s32 errCode, void* customParam);


/*****************************************************************
* Function:     CallBack_GPRS_Deactive
*
* Description:
*               This callback function is invoked by Ql_GPRS_Deactivate()
*               when the return value of Ql_GPRS_Deactivate() is GPRS_PDP_WOULDBLOCK.
*
*               Besides, this callback function will be invoked when GPRS drops down.
*
* Parameters:
*               contextId:
*                   [in] OpenCPU supports two PDP-contexts to the destination
*                   host at a time. This parameter can be 0 or 1.
*
*               errCode:
*                   Error code, refer to Enum_GPRS_PDPError. It indicates activating GPRS
*                   successfully when it's equal to GPRS_PDP_SUCCESS.
*
*               customParam:
*                   The customized parameter, which is passed in by calling Ql_GPRS_Activate().
* Return:
*              None.
*======================
* Example:
*======================
*               The following code is a possible implementation for Callback_GPRS_Actived.
*               void CallBack_GPRS_Deactived(u8 contexId, s32 errCode, void* customParam)
*               {
*                   if(GPRS_PDP_SUCCESS == errCode)
*                   {
*                       // Deactivate GPRS successfully
*                   }else{
*                       // Fail to deactivate GPRS
*                   }
*               }
*****************************************************************/
typedef void (*CallBack_GPRS_Deactive)(u8 contextId, s32 errCode, void* customParam);

#endif  //__QL_GPRS_H__
