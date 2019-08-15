/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Quectel Co., Ltd. 2015
*
*****************************************************************************/
/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ril_bluetooth.c 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The module implements bluetooth related APIs.
 *
 * Author:
 * -------
 * -------
 *  Designed by     :   Stanley YONG
 *  Coded    by     :   Stanley YONG
 *  Tested   by     :   

 *  
 *============================================================================
 *             HISTORY
 *----------------------------------------------------------------------------
 * 
 ****************************************************************************/
#include "custom_feature_def.h"
#ifdef __OCPU_RIL_BT_SUPPORT__
#include "ql_type.h"
#include "ql_stdlib.h"
#include "ql_trace.h"
#include "ql_error.h"
#include "ql_common.h"
#include "ql_system.h"
#include "ril.h"
#include "ril_util.h"
#include "ril_bluetooth.h"

static ST_BT_DevInfo* m_arrBTDev[MAX_BT_DEV_CNT] = {NULL};
static CALLBACK_BT_IND callback_bt = NULL;
static u8* m_ptrSppData = NULL;
static u32 m_nSppDataLenToSnd = 0;
static u8* m_ptrSppDataBuf = NULL;
static u32 m_nSppDataLenToRd = 0;
static u32 m_nSppRealReadLen = 0;
static char *pf_name[]={"SPP","OBEX_PBA_PROFILE_CLIENT","OBEX_PBA_PROFILE","OBEX_OBJECT_PUSH_SERVICE",\
    "OBEX_OBJECT_PUSH_CLIENT","HF_PROFILE","HFG_PROFILE",NULL};




static s32 ATRsp_QBTPWR_Hdlr(char* line, u32 len, void* param);
static s32 ATRsp_QBTADDR_Hdlr(char* line, u32 len, void* param);
static s32 ATRsp_QBTSTATE_Hdlr(char* line, u32 len, void* param);
static s32 ATRsp_QBTSPPREAD_Hdlr(char* line, u32 len, void* param);
static s32 ATRsp_QBTSPPSEND_Hdlr(char* line, u32 len, void* param);
static s32 ATRsp_QBTVISB_Hdlr(char* line, u32 len, void* param);
static s32 ATRsp_QBTNAME_Hdlr(char* line, u32 len, void* param);
static s32 AtRsp_QBTCONND_Hdlr(char* line, u32 len, void* userData);
static s32 ATRsp_QBTGPROF_Hdlr(char* line, u32 len, void* param);


extern u32 Ql_GenHash(char* strSrc, u32 len);



// Clean the scanned bt devices
static void BT_DevMngmt_Clean(void)
{
    u16 i;
    
    for (i = 0; i < MAX_BT_DEV_CNT; i++)
    {
        if (m_arrBTDev[i] != NULL)
        {
            Ql_MEM_Free((void *)m_arrBTDev[i]);
            m_arrBTDev[i] = NULL;
        }
    }
}
//
// Append a bt device
static void BT_DevMngmt_Append(ST_BT_DevInfo* pstBtDev)
{
    u16 i;
    for (i = 0; i < MAX_BT_DEV_CNT; i++)
    {
        if (NULL == m_arrBTDev[i])
        {
            m_arrBTDev[i] = pstBtDev;
            break;
        }
    }
}



static bool BT_DevMngmt_isFull(void)
{
    u16 i;
    u16 count = 0 ;
    for (i = 0; i < MAX_BT_DEV_CNT; i++)
    {
        if (NULL != m_arrBTDev[i])
        {
            count++;
        }
    }

    if(count >= MAX_BT_DEV_CNT)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//


static bool BT_PairUpdateConfirm(const s32 pairid,BT_DEV_HDL devHdl,const char *name)
{
    u16 i;

    for (i = 0; i < MAX_BT_DEV_CNT; i++)
    {
        if (NULL != m_arrBTDev[i])
        {
            if(devHdl == m_arrBTDev[i]->btDevice.devHdl)
            {
                m_arrBTDev[i]->pairId = pairid;
				Ql_memset(m_arrBTDev[i]->btDevice.name,0,BT_NAME_LEN);
                Ql_strcpy(m_arrBTDev[i]->btDevice.name,name);
                return TRUE;
            }
            
        }
    }
    return FALSE;

}


static bool BT_ConnectUpdateConfirm(const s32 connid,const s32 profileId,BT_DEV_HDL devHdl,const char *name)
{
    u16 i;

    for (i = 0; i < MAX_BT_DEV_CNT; i++)
    {
        if (NULL != m_arrBTDev[i])
        {
            if(devHdl == m_arrBTDev[i]->btDevice.devHdl)
            {
                m_arrBTDev[i]->connId = connid;
                m_arrBTDev[i]->profileId = profileId;
				Ql_memset(m_arrBTDev[i]->btDevice.name,0,BT_NAME_LEN);
                Ql_strcpy(m_arrBTDev[i]->btDevice.name,name);
                return TRUE;
            }
            
        }
    }
    return FALSE;

}


static bool BT_ScanUpdateConfirm(BT_DEV_HDL devHdl,const s32 devid,const char *name)
{
    u16 i;
    for (i = 0; i < MAX_BT_DEV_CNT; i++)
    {
        if (NULL != m_arrBTDev[i])
        {
            if(devHdl == m_arrBTDev[i]->btDevice.devHdl)
            {
                m_arrBTDev[i]->devId = devid;
				Ql_memset(m_arrBTDev[i]->btDevice.name,0,BT_NAME_LEN);
                Ql_strcpy(m_arrBTDev[i]->btDevice.name,name);
                return TRUE;
            }
            
        }
    }
    return FALSE;

}



//
// Update pair id
static void BT_DevMngmt_UpdatePairId(const u32 hdl, const s32 pairId)
{
    u16 i;
    for (i = 0; i < MAX_BT_DEV_CNT; i++)
    {
        if (hdl == m_arrBTDev[i]->btDevice.devHdl)
        {
            m_arrBTDev[i]->pairId= pairId;
            break;
        }
    }
}
//
// Update connect id
static void BT_DevMngmt_UpdateConnId(const u32 hdl, const s32 connId)
{
    u16 i;
    for (i = 0; i < MAX_BT_DEV_CNT; i++)
    {
        if (hdl == m_arrBTDev[i]->btDevice.devHdl)
        {
            m_arrBTDev[i]->connId= connId;
            break;
        }
    }
}

static void BT_DevMngmt_UpdateProfileId(const u32 hdl, const s32 profileId)
{
    u16 i;
    for (i = 0; i < MAX_BT_DEV_CNT; i++)
    {
        if (hdl == m_arrBTDev[i]->btDevice.devHdl)
        {
            m_arrBTDev[i]->profileId= profileId;
            break;
        }
    }
}



static s32 BT_DevMngmt_ProfileNameToId(const char *profile_name)
{
    u16 i = 0;

    if(NULL == profile_name)
    {
        return -1;
    }
    
    while (pf_name[i] != NULL)
    {
       if(0 == Ql_strncmp(profile_name,pf_name[i],Ql_strlen(profile_name)))
       {
           return i ;
       }
       i++;
    }

    return -1;

     
}




static BT_DEV_HDL BT_DevMngmt_GetDevHdl(const s32 connId)
{
    u16 i;
    for (i = 0; i < MAX_BT_DEV_CNT; i++)
    {
        if (connId == m_arrBTDev[i]->connId)
        {
            return m_arrBTDev[i]->btDevice.devHdl;
        }
    }
    return 0;
}


s32 BT_DevMngmt_GetDeviceId(const u32 hdl)
{
    u16 i;
    for (i = 0; i < MAX_BT_DEV_CNT; i++)
    {
        if (hdl == m_arrBTDev[i]->btDevice.devHdl)
        {
            return m_arrBTDev[i]->devId;
        }
    }
    return 0;
}


s32 BT_DevMngmt_GetPairedId(const u32 hdl)
{
    u16 i;
    for (i = 0; i < MAX_BT_DEV_CNT; i++)
    {
        if (hdl == m_arrBTDev[i]->btDevice.devHdl)
        {
            return m_arrBTDev[i]->pairId;
        }
    }
    return 0;
}
s32 BT_DevMngmt_GetConnId(const u32 hdl)
{
    u16 i;
    for (i = 0; i < MAX_BT_DEV_CNT; i++)
    {
        if (hdl == m_arrBTDev[i]->btDevice.devHdl)
        {
            return m_arrBTDev[i]->connId;
        }
    }
    return 0;
}

s32 BT_DevMngmt_GetProfileId(const u32 hdl)
{
    u16 i;
    for (i = 0; i < MAX_BT_DEV_CNT; i++)
    {
        if (hdl == m_arrBTDev[i]->btDevice.devHdl)
        {
            return m_arrBTDev[i]->profileId;
        }
    }
    return 0;
}

char *BT_DevMngmt_GetDevName(const u32 hdl)
{
    u16 i;
    for (i = 0; i < MAX_BT_DEV_CNT; i++)
    {
        if (hdl == m_arrBTDev[i]->btDevice.devHdl)
        {
            return m_arrBTDev[i]->btDevice.name;
        }
    }
    return NULL;
}


char * BT_DevMngmt_GetDevAddr(const u32 hdl)
{
    u16 i;
    for (i = 0; i < MAX_BT_DEV_CNT; i++)
    {
        if (hdl == m_arrBTDev[i]->btDevice.devHdl)
        {
            return m_arrBTDev[i]->btDevice.addr;
        }
    }
    return NULL;
}


/*****************************************************************
* Function:     RIL_BT_Switch
* 
* Description:
*               Turn on /off bluetooth
*
* Parameters:
*               on_off [in] :  0 --- off ,1 ----- on
*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
s32 RIL_BT_Switch(u8 on_off)
{
    char strAT[20];

    if (on_off < BT_OFF || on_off > BT_ON)
    {
        return RIL_AT_INVALID_PARAM;
    }
    
    Ql_memset(strAT, 0x0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QBTPWR=%d", on_off);
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL, NULL, 0);
}


/*****************************************************************
* Function:     RIL_BT_GetPwrState
* 
* Description:
*               query current bluetooth power state
*
* Parameters:
*               p_on_off [out] :  0 --- off ,1 ----- on
*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
s32 RIL_BT_GetPwrState(s32 *p_on_off)
{
    char strAT[] = "AT+QBTPWR?\0";
    
    if (NULL == p_on_off)
    {
        return RIL_AT_INVALID_PARAM;
    }
    
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QBTPWR_Hdlr, (void*)p_on_off, 0);
}


/*****************************************************************
* Function:     RIL_BT_Initialize
* 
* Description:
*               bluetooth initialization after power on ,register callback and update paired info
*
* Parameters:
*               cb: callback to be registered
*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
s32 RIL_BT_Initialize(CALLBACK_BT_IND cb)
{
    
    if (!cb)
    {
        return RIL_AT_INVALID_PARAM;
    }
    
    BT_DevMngmt_Clean();
    
    callback_bt = cb;

    RIL_BT_QueryState(NULL); //update paired items
    
    return RIL_AT_SUCCESS;
}



/*****************************************************************
* Function:     RIL_BT_SetName
* 
* Description:
*               set the name of bluetooth
*
* Parameters:
*               name [in] :  bluetooth name to set
*               len  [in] :  length of bluetooth name,max length 56 bytes (18 * 3+2)
*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
s32 RIL_BT_SetName(char *name,u8 len)
{
    char strAT[80];
    
    if(NULL == name || (len > BT_NAME_LEN - 2)) 
    {
        return RIL_AT_INVALID_PARAM ;
    }

    Ql_memset(strAT, 0x0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QBTNAME=\"%s\"", name);
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL, NULL, 0);
}


/*****************************************************************
* Function:     RIL_BT_GetName
* 
* Description:
*               get the name of bluetooth
*
* Parameters:
*               name    [out] :  bluetooth name to get
                len     [in]  :  sizeof of param 1
*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
s32 RIL_BT_GetName(char *name/*char addr[BT_NAME_LEN]*/,u8 len)

{
    char strAT[20]="AT+QBTNAME?\0";
    s32 ret = RIL_AT_SUCCESS;
    char in_name[BT_NAME_LEN] = {0};
    
    
    if(NULL == name )
    {
        return RIL_AT_INVALID_PARAM ;
    }

	Ql_memset(name,0,len);

    ret = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QBTNAME_Hdlr,(void *)in_name, 0);

    if(ret == RIL_AT_SUCCESS)
    {
        if(len < Ql_strlen(in_name))
        {
            Ql_strncpy(name,in_name,len-1);
        }
        else
        {
            Ql_strncpy(name,in_name,Ql_strlen(in_name));
        }
    }

    return ret;
}



/*****************************************************************
* Function:     RIL_BT_GetLocalAddr
* 
* Description:
*               get the device address of bluetooth
*
* Parameters:
*               ptrAddr    [out] :  bluetooth addr to get ,length is fixed 13 bytes including '\0'
                len        [in]  :  sizeof param 1
*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/

s32 RIL_BT_GetLocalAddr(char* ptrAddr/*char addr[BT_ADDR_LEN]*/,u8 len)
{
    char strAT[] = "AT+QBTADDR?\0";
    s32 ret = RIL_AT_SUCCESS;
    char in_addr[BT_ADDR_LEN] = {0};
    
    if (NULL == ptrAddr)
    {
        return RIL_AT_INVALID_PARAM;
    }

	Ql_memset(ptrAddr,0,len);

    ret = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QBTADDR_Hdlr,(void *)in_addr, 0);

    if(ret == RIL_AT_SUCCESS)
    {
        if(len < Ql_strlen(in_addr))
        {
            Ql_strncpy(ptrAddr,in_addr,len-1);
        }
        else
        {
            Ql_strncpy(ptrAddr,in_addr,Ql_strlen(in_addr));
        }
    }

    return ret;
}



/*****************************************************************
* Function:     RIL_BT_SetVisble
* 
* Description:
*               set the bluetooth to be viewed or not
*
* Parameters:
*               mode    [in] : visible mode 0 :invisble 1: visible forever 2.visibility temporary on,see Enum_VisibleMode
*               timeout [in] : when mode is set to 2 ,this param decide during which time bluetooth can be found by others
*                              unit second,can be 1-255,after timout,MSG_BT_INVISIBLE will be triggered ,other mode is ignored
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/

s32 RIL_BT_SetVisble(Enum_VisibleMode mode,u8 timeout)
{
    char strAT[30];
    
    if(mode < BT_INVISIBLE || mode >= BT_VISIBLE_END)
    {
        return RIL_AT_INVALID_PARAM;
    }

    Ql_memset(strAT, 0x0, sizeof(strAT));

    if(BT_INVISIBLE == mode || BT_VISIBLE_FOREVER == mode)
    {
         Ql_sprintf(strAT, "AT+QBTVISB=%d", mode);
    }
    else
    {
         Ql_sprintf(strAT, "AT+QBTVISB=%d,%d", mode,timeout);
    }
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL, NULL, 0);
    
}


void OnURCHandler_BTVisible(const char* strURC, void* reserved)
{
    s32 err_code = 0;
    char urcHead[30];

	if(callback_bt == NULL)
	{
	   return;
	}

    if (Ql_StrPrefixMatch(strURC, urcHead))
    {
        Ql_sscanf(strURC, "%*[^:]: %d\r\n",&err_code);
        if(0 == err_code)
        {
           callback_bt(MSG_BT_VISIBLE_IND, URC_BT_INVISIBLE, NULL, NULL);
        }
		else
		{
		   callback_bt(MSG_BT_VISIBLE_IND, err_code, NULL, NULL);
		}
        return;
    }
    Ql_strcpy(urcHead, "+CME ERROR:\0");
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {
        Ql_sscanf(strURC, "%*[^:]: %d\r\n", &err_code);
        callback_bt(MSG_BT_VISIBLE_IND, err_code, NULL, NULL);
        return;
    }
}



/*****************************************************************
* Function:     RIL_BT_GetVisble
* 
* Description:
*               get the current bluetooth visble mode
*
* Parameters:
*               mode    [out] : visible mode 0 :invisble 1: visible forever 2.visibility temporary on ,see Enum_VisibleMode
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
s32 RIL_BT_GetVisble(s32 *mode)

{
        
    char strAT[] = "AT+QBTVISB?\0";
    
    if(NULL == mode)
    {
        return RIL_AT_INVALID_PARAM;
    }
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QBTVISB_Hdlr, (void*)mode, 0);
}



/*****************************************************************
* Function:     RIL_BT_StartScan
* 
* Description:
*               start scan the around bt devices
*
* Parameters:
*              
*               @maxDevCount: 0-20, default 20
*               @CoD: 0-255, default 0
*               @timeout: unit in second. 1-255, default 60s
*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/

s32 RIL_BT_StartScan(u16 maxDevCount, u16 CoD, u16 timeout)
{
    char strAT[20];

    if(maxDevCount < 0 || maxDevCount > MAX_BT_SCAN_CNT)
    {
        return  RIL_AT_INVALID_PARAM ;
    }

    if(CoD < 0 || CoD > MAX_BT_SCAN_COD)
    {
        return  RIL_AT_INVALID_PARAM ;
    }

    if(timeout < 1 || timeout > MAX_BT_SCAN_TIMEOUT)
    {
        return  RIL_AT_INVALID_PARAM ;
    }

    BT_DevMngmt_Clean();

    RIL_BT_QueryState(NULL);

    // Start to scan bt devices
    Ql_memset(strAT, 0x0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QBTSCAN=%d,%d,%d", timeout, maxDevCount, CoD);
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL, NULL, 0);
}


void OnURCHandler_BTScan(const char* strURC, void* reserved)
{
    // +QBTSCAN:3,"M26-test",397D0D816261
    // +QBTSCAN:0
    char urcHead[] = "\r\n+QBTSCAN:\0";
    s32 deviceId = 0;
    ST_BT_DevInfo* pstrNewBtDev = NULL;
    bool ret = FALSE;
    s32 err_code = 0;

	if(callback_bt == NULL)
    {
       return;
	}
		
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {
        Ql_sscanf(strURC, "%*[^:]: %d%*[^\r\n]\r\n",&deviceId);
        
        if (0 == deviceId)  // scan finished
        {
            callback_bt(MSG_BT_SCAN_IND,URC_BT_SCAN_FINISHED, NULL, NULL);
            
        }else{  // scan new bt device
            // Create a new bt device
            pstrNewBtDev = (ST_BT_DevInfo*)Ql_MEM_Alloc(sizeof(ST_BT_DevInfo));
            Ql_memset(pstrNewBtDev, 0x0, sizeof(ST_BT_DevInfo));
            pstrNewBtDev->devId = deviceId;
            pstrNewBtDev->pairId = -1;
            pstrNewBtDev->connId = -1;
            pstrNewBtDev->profileId = -1;

            Ql_sscanf(strURC, "%*[^\"]\"%[^\"]%*[^\r\n]\r\n",pstrNewBtDev->btDevice.name);
            Ql_sscanf(strURC, "%*[^,],%*[^,],%[^\r\n]\r\n",pstrNewBtDev->btDevice.addr);
            pstrNewBtDev->btDevice.devHdl = Ql_GenHash(pstrNewBtDev->btDevice.addr, Ql_strlen(pstrNewBtDev->btDevice.addr));
            ret = BT_ScanUpdateConfirm(pstrNewBtDev->btDevice.devHdl,pstrNewBtDev->devId,pstrNewBtDev->btDevice.name);
			callback_bt(MSG_BT_SCAN_IND,URC_BT_SCAN_FOUND,&(pstrNewBtDev->btDevice), NULL);//customer own management
            if(ret == FALSE)
            {
	            if(!BT_DevMngmt_isFull())
	            {
	               BT_DevMngmt_Append(pstrNewBtDev);
	            }
	            else
	            {
	                Ql_MEM_Free(pstrNewBtDev);
	            }
            }
			else
			{
			    Ql_MEM_Free(pstrNewBtDev);
			}
            
       
        }
        return;
    }

    Ql_strcpy(urcHead, "+CME ERROR:\0");
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {
        Ql_sscanf(strURC, "%*[^:]: %d\r\n", &err_code);
        callback_bt(MSG_BT_SCAN_IND, err_code, NULL, NULL);
        return;
    }
}


/*****************************************************************
* Function:     RIL_BT_GetDevListInfo
* 
* Description:
*               get current bt devices info

                ADDR    DEVID    HANDLER    PAIRID   CONNID   PROFILE  NAME
*
* Parameters:
*              
*              void
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/

s32 RIL_BT_GetDevListInfo(void)
{
    u16 i;
    for (i = 0; i < MAX_BT_DEV_CNT; i++)
    {
        if (NULL != m_arrBTDev[i])
        {
            Ql_Debug_Trace("%s %2d 0x%08x %2d %2d %2d %s\r\n",m_arrBTDev[i]->btDevice.addr,m_arrBTDev[i]->devId,\
                m_arrBTDev[i]->btDevice.devHdl,m_arrBTDev[i]->pairId,m_arrBTDev[i]->connId,m_arrBTDev[i]->profileId,m_arrBTDev[i]->btDevice.name);
        }
    }
    return RIL_AT_SUCCESS;
}


/*****************************************************************
* Function:     RIL_BT_GetDevListPointer
* 
* Description:
*               get the dev list pointer ,then you can use it to get items of the list
                before you use the pointer ,RIL_BT_GetDevListInfo can help you see valid
                items to operate
*
* Parameters:
*              
*              void
* Return:        
*               the pointer to dev list
                ST_BT_DevInfo ** ptr;
                ptr = RIL_BT_GetDevListPointer();
                ptr[i]->btDevice.devHdl
*****************************************************************/

ST_BT_DevInfo ** RIL_BT_GetDevListPointer(void)
{
    return m_arrBTDev;
}




/*****************************************************************
* Function:     RIL_BT_StopScan
* 
* Description:
*               stop the scan process
*
* Parameters:
*               void
*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
s32 RIL_BT_StopScan(void)
{
    char strAT[] = "AT+QBTSCANC\0";
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL, NULL, 0);
}


/*****************************************************************
* Function:     RIL_BT_QueryState
* 
* Description:
*               query current bluetooth state and update paired items
*
* Parameters:
*               status [out] : current BT status see Enum_BTDevStatus
*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
s32 RIL_BT_QueryState(s32 *status)
{
    char strAT[] = "AT+QBTSTATE\0";
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QBTSTATE_Hdlr, (void *)status, 0);
}



/*****************************************************************
* Function:     RIL_BT_PairReq
* 
* Description:
*               request to pair a bt device ,for paired items ,ignore this step ,directly to connect
*               if passive pair ,see +QBTIND
*
* Parameters:
*               hdlDevice :[in]  the bt handler to pair
*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
s32 RIL_BT_PairReq(BT_DEV_HDL hdlDevice)
{
    char strAT[20];
    s32  devId ;
    
    devId = BT_DevMngmt_GetDeviceId(hdlDevice);
    if(devId == 0)
    {
        return RIL_AT_INVALID_PARAM;
    }    
    Ql_sprintf(strAT, "AT+QBTPAIR=%d", devId);
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL, NULL, 0);
}




void OnURCHandler_BTPair(const char* strURC, void* reserved)
{
    s32 err_code = 0;
    char urcHead[30];
    char pinCode[BT_PIN_LEN] = {0};
    ST_BT_BasicInfo bt_dev;
    char temp[30]={0};

    Ql_memset(&bt_dev, 0x0, sizeof(ST_BT_BasicInfo));
    Ql_memset(pinCode, 0x0, sizeof(pinCode));

	if(callback_bt == NULL)
    {
       return;
	}

    // +QBTPAIR: "H60-L01",F4E3FBE47920,724242

    
    Ql_strcpy(urcHead, "\r\n+QBTPAIR:\0");
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {
        Ql_sscanf(strURC, "%*[^\"]\"%[^\"]%*[^\r\n]\r\n", bt_dev.name);
        Ql_sscanf(strURC, "%*[^,],%[^\r\n]\r\n", temp);
       
        if(Ql_strlen(temp) == (BT_ADDR_LEN - 1))
        {
             Ql_sscanf(strURC, "%*[^,],%[^\r\n]\r\n", bt_dev.addr);
             bt_dev.devHdl = Ql_GenHash(bt_dev.addr, Ql_strlen(bt_dev.addr));
             callback_bt(MSG_BT_PAIR_IND, URC_BT_NEED_PASSKEY, &bt_dev, NULL); // need paaskey
        }
        else
        {
            Ql_sscanf(strURC, "%*[^,],%[^,]%*[^\r\n]\r\n",bt_dev.addr);
            Ql_sscanf(strURC, "%*[^,],%*[^,],%[^\r\n]\r\n",pinCode);
            bt_dev.devHdl = Ql_GenHash(bt_dev.addr, Ql_strlen(bt_dev.addr));
            callback_bt(MSG_BT_PAIR_IND, URC_BT_NO_NEED_PASSKEY, &bt_dev, pinCode); //direct confirm
        }   
        return;
    }

    Ql_strcpy(urcHead, "+CME ERROR:\0");
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {
        Ql_sscanf(strURC, "%*[^:]: %d\r\n", &err_code);
        callback_bt(MSG_BT_PAIR_IND, err_code, NULL, NULL);
        return;
    }

}




/*****************************************************************
* Function:     RIL_BT_PairConfirm
* 
* Description:
*               confirm to pair 
*
* Parameters:
*               accept :[in]  whether to accpect the pair request--- 0 :reject 1:accept
*               pincode :[in]  the passkey used to pair,4bytes
*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
s32 RIL_BT_PairConfirm(bool accept, char* pinCode)
{
    char strAT[20] = {0};

    if(accept != 0 && accept != 1)
    {
        return RIL_AT_INVALID_PARAM;
    }

    if (NULL != pinCode)//not numeric code ,is passkey(4bit digits)
    {
        Ql_sprintf(strAT, "AT+QBTPAIRCNF=%d,\"%s\"", (u8)accept, pinCode);
    }else{
        Ql_sprintf(strAT, "AT+QBTPAIRCNF=%d", (u8)accept);
    }
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL, NULL, 0);
}


void OnURCHandler_BTPairCnf(const char* strURC, void* reserved)
{
    s32 err_code = 0;
    s32  pairedId;
    s32  is1stPaired;
    char urcHead[30];
    ST_BT_BasicInfo bt_dev;

    Ql_memset(&bt_dev,  0x0, sizeof(ST_BT_BasicInfo));

    if(callback_bt == NULL)
    {
       return;
	}

    
    // +QBTPAIRCNF:1,1,1,"H60-L01",F4E3FBE47920
    Ql_strcpy(urcHead, "\r\n+QBTPAIRCNF:\0");
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {
        Ql_sscanf(strURC, "%*[^:]: %d%*[^\r\n]\r\n", &err_code);
        if (err_code > 0)
        {
			Ql_sscanf(strURC, "%*[^,],%d%*[^\r\n]\r\n",&pairedId);
			Ql_sscanf(strURC, "%*[^,]%*[^,],%d%*[^\r\n]\r\n",&is1stPaired);
            Ql_sscanf(strURC, "%*[^\"]\"%[^\"]%*[^\r\n]\r\n",bt_dev.name);
            Ql_sscanf(strURC, "%*[^\"]\"%*[^\"]\",%[^\r\n]\r\n",bt_dev.addr);
            bt_dev.devHdl = Ql_GenHash(bt_dev.addr, Ql_strlen(bt_dev.addr));
            RIL_BT_QueryState(NULL);
            callback_bt(MSG_BT_PAIR_CNF_IND, URC_BT_PAIR_CNF_SUCCESS, &bt_dev, NULL);
        }else{
            Ql_sscanf(strURC, "%*[^,],%[^\r\n]\r\n",bt_dev.addr);
            bt_dev.devHdl = Ql_GenHash(bt_dev.addr, Ql_strlen(bt_dev.addr));
            BT_DevMngmt_UpdatePairId(bt_dev.devHdl,-1);
            callback_bt(MSG_BT_PAIR_CNF_IND, URC_BT_PAIR_CNF_FAIL, NULL, NULL);
        }
        
        return;
    }

    Ql_strcpy(urcHead, "+CME ERROR:\0");
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {
        Ql_sscanf(strURC, "%*[^:]: %d[^\r\n]", &err_code);
        callback_bt(MSG_BT_PAIR_CNF_IND, err_code, NULL, NULL);
        return;
    }
}


/*****************************************************************
* Function:     RIL_BT_Unpair
* 
* Description:
*               unpair a paired bt device
*
* Parameters:
*               hdlDevice :[in]  the bt handler to unpair
*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
s32 RIL_BT_Unpair(BT_DEV_HDL hdlDevice)
{
    char strAT[20];
    s32  pairedId = -1 ;
    s32 ret = RIL_AT_SUCCESS ;
    
    pairedId = BT_DevMngmt_GetPairedId(hdlDevice);

    if(pairedId <=0 || pairedId >MAX_BT_PAIRED_CNT)
    {
        return RIL_AT_INVALID_PARAM;
    }
    
    Ql_sprintf(strAT, "AT+QBTUNPAIR=%d", pairedId);
    ret = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL, NULL, 0);


    if(RIL_AT_SUCCESS == ret)
    {
         BT_DevMngmt_UpdatePairId(hdlDevice,-1);
         BT_DevMngmt_UpdateConnId(hdlDevice,-1);
         BT_DevMngmt_UpdateProfileId(hdlDevice,-1);
    }

    return ret ;
}



/*****************************************************************
* Function:     RIL_BT_GetSupportedProfile
* 
* Description:
*               returns the profiles suppoerted both by the local device and the other side device,for paired items
*
* Parameters:
*               hdlDevice :[in]  the bt handler to get support profile
                profile_support :[out] the supported profile get for both sides ,see Enum_BTProfileId
                len :[in] the array length
*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
s32 RIL_BT_GetSupportedProfile(BT_DEV_HDL hdlDevice,s32 *profile_support,u8 len)
{
    char strAT[20];
    s32  pairedId = -1 ;
	s32 ret;
	s32 in_profile_get[BT_PROFILE_END] = {-1};
    
    pairedId = BT_DevMngmt_GetPairedId(hdlDevice);

    if(pairedId <= 0 || pairedId > MAX_BT_PAIRED_CNT || NULL == profile_support || len <= 0 )
    {
        return RIL_AT_INVALID_PARAM;
    }

	Ql_memset(profile_support,0,len*sizeof(profile_support[0]));
    
    Ql_sprintf(strAT, "AT+QBTGPROF=%d", pairedId);
    ret =  Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QBTGPROF_Hdlr, (void *)in_profile_get, 0);

	if(RIL_AT_SUCCESS == ret)
	{
	   if(len < BT_PROFILE_END)
	   {
	      Ql_memcpy(profile_support,in_profile_get,len*sizeof(in_profile_get[0]));
	   }
	   else
	   {
	      Ql_memcpy(profile_support,in_profile_get,BT_PROFILE_END*sizeof(in_profile_get[0]));
	   }
	}
	return ret;
}




/*****************************************************************
* Function:     RIL_BT_ConnReq
* 
* Description:
*               request to connect a paired bt device
*
* Parameters:
*               hdlDevice :[in]  the bt handler to connect
*               profileId :[in]  profile type when connect ,see Enum_BTProfileId
*               mode      :[in]  connect type ,see Enum_BT_SPP_ConnMode
*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
s32 RIL_BT_ConnReq(BT_DEV_HDL hdlDevice, u8 profileId, u8 mode)
{
    char strAT[20];
    s32 pairedId;
    
    if (profileId < BT_PROFILE_SPP || profileId >= BT_PROFILE_END)
    {
        return RIL_AT_INVALID_PARAM;
    }

    if(mode < BT_SPP_CONN_MODE_AT || mode > BT_SPP_CONN_MODE_TRANS)
    {
        return RIL_AT_INVALID_PARAM;
    }
    
    pairedId = BT_DevMngmt_GetPairedId(hdlDevice);

    if(pairedId <= 0 || pairedId > MAX_BT_PAIRED_CNT)
    {
        return RIL_AT_INVALID_PARAM;
    }

    Ql_sprintf(strAT, "AT+QBTCONN=%d,%d,%d", pairedId, profileId, mode);
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL, NULL, 0);
    
}

void OnURCHandler_BTConn(const char* strURC, void* reserved)
{
    s32 err_code = 0;
    s32  connId;
    char profile_name[40] = {0};
    char urcHead[30];
    ST_BT_BasicInfo bt_dev;
    
    Ql_memset(&bt_dev,  0x0, sizeof(ST_BT_BasicInfo));

	if(callback_bt == NULL)
	{
	   return ;
	}

    
    //+QBTCONN:1,1,H60-L01,F4E3FBE47920,SPP
    //+QBTDISC:F4E3FBE47920,SPP
    //+QBTCONN:0
    Ql_strcpy(urcHead, "\r\n+QBTCONN:\0");
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {
        Ql_sscanf(strURC, "%*[^:]: %d%*[^\r\n]\r\n", &err_code);
        if (err_code > 0)
        {  
            //+QBTCONN:1,1,H30-T00,786A89ECCEC7,HF_PROFILE
            Ql_sscanf(strURC, "%*[^,],%d%*[^\r\n]\r\n",&connId);
            Ql_sscanf(strURC, "%*[^\"]\"%[^\"]%*[^\r\n]\r\n",bt_dev.name);
            Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%[^,]%*[^\r\n]\r\n",bt_dev.addr);
            Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%*[^,],\"%[^\"]%*[^\r\n]\r\n",profile_name);
            bt_dev.devHdl = Ql_GenHash(bt_dev.addr, Ql_strlen(bt_dev.addr));
            RIL_BT_QueryState(NULL);
            callback_bt(MSG_BT_SPP_CONN_IND, URC_BT_CONN_SUCCESS, &bt_dev, NULL);
        }else{
            Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%[^,]%*[^\r\n]\r\n",bt_dev.addr);
            bt_dev.devHdl = Ql_GenHash(bt_dev.addr, Ql_strlen(bt_dev.addr));
            BT_DevMngmt_UpdateConnId(bt_dev.devHdl, -1);
            BT_DevMngmt_UpdateProfileId(bt_dev.devHdl,-1);
            callback_bt(MSG_BT_SPP_CONN_IND, URC_BT_CONN_FAIL, NULL, NULL);
        }
        return;
    }

    Ql_strcpy(urcHead, "+CME ERROR:\0");
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {
        Ql_sscanf(strURC, "%*[^:]: %d\r\n", err_code);
        callback_bt(MSG_BT_SPP_CONN_IND, err_code, NULL, NULL);
        return;
    }
}


/*****************************************************************
* Function:     RIL_BT_SPP_DirectConn
* 
* Description:
*               use addr to make a direct connect ,so you don't need to scan or concern the paring process
*               only support SPP connection
*
* Parameters:
*               btMacAddr :[in]  the bt addr to connect
*               mode      :[in]  connect type ,see Enum_BT_SPP_ConnMode
*               pinCode :  [in]  pairkey


                AT+QBTCONND=F4E3FBE47920,0,"0000"

                OK

                +QBTIND: "pair",H60-L01,F4E3FBE47920,339056

                +QBTPAIRCNF:1,1,1,"H60-L01",F4E3FBE47920

                +QBTCONN:1,1,H60-L01,F4E3FBE47920,SPP



*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
s32 RIL_BT_SPP_DirectConn(char* btMacAddr, u8 mode, char* pinCode)
{

    char strAT[20];

    if(NULL == btMacAddr)
    {
        return RIL_AT_INVALID_PARAM;
    }
    
    if(mode < BT_SPP_CONN_MODE_AT || mode > BT_SPP_CONN_MODE_TRANS)
    {
        return RIL_AT_INVALID_PARAM;
    }
    
    Ql_sprintf(strAT, "AT+QBTCONND=%s,%d,\"%s\"", btMacAddr, mode, pinCode);
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), AtRsp_QBTCONND_Hdlr, NULL, 0);
}

static s32 AtRsp_QBTCONND_Hdlr(char* line, u32 len, void* userData)
{
    if (Ql_RIL_FindLine(line, len, "OK"))
    {  
        return  RIL_ATRSP_SUCCESS;
    }else{
        //todo
        return RIL_ATRSP_CONTINUE;
    }
}


/*****************************************************************
* Function:     RIL_BT_ConnAccept
* 
* Description:
*               accept to connect
*
* Parameters:
*               accept      :[in]  0--reject 1---accept
                mode        :[in]  connect type ,see Enum_BT_SPP_ConnMode

*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
s32 RIL_BT_ConnAccept(bool accept , u8 mode)
{
    char strAT[20];

    if(accept != 0 && accept != 1)
    {
        return RIL_AT_INVALID_PARAM;
    }

    if(mode < BT_SPP_CONN_MODE_AT || mode > BT_SPP_CONN_MODE_TRANS)
    {
        return RIL_AT_INVALID_PARAM;
    }
    
    if(1 == accept)
    {
        Ql_sprintf(strAT, "AT+QBTACPT=1,%d", mode);
    }
    else
    {
        Ql_sprintf(strAT, "AT+QBTACPT=0", mode);
    }
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL, NULL, 0);
}


void OnURCHandler_BTConnCnf(const char* strURC, void* reserved)
{
    s32 err_code = 0;
    s32  connId;
    char urcHead[30];
    char profile_name[40] = {0};
    //s32 profileId;
    ST_BT_BasicInfo bt_dev;
    
    Ql_memset(&bt_dev,  0x0, sizeof(ST_BT_BasicInfo));

	if(callback_bt == NULL)
	{
	   return ;
	}


    // AT+QBTACPT=1
    //+QBTACPT:1,2,H60-L01,F4E3FBE47920,SPP
    //
    // AT+QBTACPT=0
    //+QBTDISC:F4E3FBE47920,SPP
    Ql_strcpy(urcHead, "\r\n+QBTACPT:\0");
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {// Response for succeeding in connecting
        Ql_sscanf(strURC, "%*[^:]: %d[^\r\n]\r\n", &err_code);
        if (err_code > 0)
        {
            Ql_sscanf(strURC, "%*[^,],%d%*[^\r\n]\r\n",&connId);
            Ql_sscanf(strURC, "%*[^\"]%[^\"]%*[^\r\n]\r\n",bt_dev.name);
            Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%[^,]%*[^\r\n]\r\n",bt_dev.addr);
            Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%*[^,],\"%[^\"]%*[^\r\n]\r\n",profile_name);
            bt_dev.devHdl = Ql_GenHash(bt_dev.addr, Ql_strlen(bt_dev.addr));
            RIL_BT_QueryState(NULL);
            callback_bt(MSG_BT_SPP_CONN_IND, URC_BT_CONN_SUCCESS, &bt_dev, NULL);
        }else{
            Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%[^,]%*[^\r\n]\r\n",bt_dev.addr);
            bt_dev.devHdl = Ql_GenHash(bt_dev.addr, Ql_strlen(bt_dev.addr));
            BT_DevMngmt_UpdateConnId(bt_dev.devHdl, -1);
            BT_DevMngmt_UpdateProfileId(bt_dev.devHdl,-1);
            callback_bt(MSG_BT_SPP_CONN_IND, URC_BT_CONN_FAIL, NULL, NULL);
        }
        return;
    }
    Ql_strcpy(urcHead, "\r\n+QBTDISC:\0");
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {// Fail to connect or reject connection req
        Ql_sscanf(strURC, "%*[^:]: %[^,]%*[^\r\n]\r\n",bt_dev.addr);
        bt_dev.devHdl = Ql_GenHash(bt_dev.addr, Ql_strlen(bt_dev.addr));
        BT_DevMngmt_UpdateConnId(bt_dev.devHdl, -1);
        BT_DevMngmt_UpdateProfileId(bt_dev.devHdl,-1);
        callback_bt(MSG_BT_SPP_CONN_IND, URC_BT_CONN_FAIL, NULL, NULL);
        return;
    }

    Ql_strcpy(urcHead, "+CME ERROR:\0");
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {
        Ql_sscanf(strURC, "%*[^:]: %d\r\n", err_code);
        callback_bt(MSG_BT_SPP_CONN_IND, err_code, NULL, NULL);
        return;
    }
}


/*****************************************************************
* Function:     RIL_BT_Disconnect
* 
* Description:
*               disconnect a bt connection
*
* Parameters:
*               hdlDevice :[in]  the handler to disconnect

*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
s32 RIL_BT_Disconnect(BT_DEV_HDL hdlDevice)
{
    char strAT[20];
    s32  connId ;
    
    connId = BT_DevMngmt_GetConnId(hdlDevice);

    if(-1 == connId)
    {
        return RIL_AT_INVALID_PARAM ;
    }
    
    Ql_sprintf(strAT, "AT+QBTDISCONN=%d", connId);
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL, NULL, 0);
}

void OnURCHandler_BTDisconn(const char* strURC, void* reserved)
{
    // +QBTDISCONN:2,2,H60-L01,F4E3FBE47920,SPP
    // +CME ERROR: 3518
    ST_BT_BasicInfo bt_dev;
    char urcHead[30];
    s32  connId;
    s32  pairedId;
    s32  profileId;
    s32 err_code = 0;
    char profile_name[40] = {0};    
    
    Ql_memset(&bt_dev,  0x0, sizeof(ST_BT_BasicInfo));
	if(callback_bt == NULL)
	{
	   return ;
	}

    Ql_strcpy(urcHead, "\r\n+QBTDISCONN:\0");
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {
        Ql_sscanf(strURC, "%*[^:]: %d,%d%*[^\r\n]\r\n", &connId,&pairedId);
        Ql_sscanf(strURC, "%*[^,],%*[^,],\"%[^\"]%*[^\r\n]\r\n",bt_dev.name);
        Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%[^,]%*[^\r\n]\r\n",bt_dev.addr);
        Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%*[^,],\"%[^\"]%*[^\r\n]\r\n",profile_name);
        bt_dev.devHdl = Ql_GenHash(bt_dev.addr, Ql_strlen(bt_dev.addr)); 
        //Ql_Debug_Trace("connid = %d pairid = %d name = %s addr = %s ,handle = 0x%08x\r\n",connId,pairedId,bt_dev.name,bt_dev.addr,bt_dev.devHdl);
        BT_DevMngmt_UpdateConnId(bt_dev.devHdl, -1);
        BT_DevMngmt_UpdateProfileId(bt_dev.devHdl,-1);
 
        callback_bt(MSG_BT_DISCONN_IND, URC_BT_DISCONNECT_POSITIVE, &bt_dev, NULL);
        return;
    }
    
    Ql_strcpy(urcHead, "\r\n+CME ERROR: \0");
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {
        Ql_sscanf(strURC, "%*[^: ]: %d\r\n", &err_code);
        callback_bt(MSG_BT_DISCONN_IND, err_code, &bt_dev, NULL);
        return;
    }
}


/*****************************************************************
* Function:     RIL_BT_SPP_Send
* 
* Description:
*               send data in SPP mode
*
* Parameters:
*               hdlDevice :[in]  the handler to send
                ptrData   :[in]  the ptr to data to be sent
                lenToSend :[in]  the length of data to be sent

*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
s32 RIL_BT_SPP_Send(BT_DEV_HDL hdlDevice, u8* ptrData, u32 lenToSend,u32* actualSend)
{
    char strAT[20];
    s32  connId ;

    if(NULL == ptrData || lenToSend < 0)
    {
        return RIL_AT_INVALID_PARAM;
    }
    
    connId = BT_DevMngmt_GetConnId(hdlDevice);

    if(-1 == connId)
    {
        return RIL_AT_INVALID_PARAM;
    }

    m_ptrSppData = ptrData;
    m_nSppDataLenToSnd   = lenToSend;
    Ql_sprintf(strAT, "AT+QSPPSEND=%d,%d", connId, lenToSend);
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QBTSPPSEND_Hdlr,(void *)actualSend, 0);
}


/*****************************************************************
* Function:     RIL_BT_SPP_Read
* 
* Description:
*               read data in SPP mode
*
* Parameters:
*               hdlDevice :[in]  the handler to read
                ptrBuffer   :[in]  the ptr to store the readed data
                lenToRead :[in]  the length of data to read
                actualReadlen: [out] the actually length readed

*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
s32 RIL_BT_SPP_Read(BT_DEV_HDL hdlDevice, u8* ptrBuffer, u32 lenToRead ,u32 *actualReadlen)
{
    char strAT[20];
    s32  connId;

    if(NULL == ptrBuffer || lenToRead < 0)
    {
        return RIL_AT_INVALID_PARAM;
    }
    
    connId = BT_DevMngmt_GetConnId(hdlDevice);

    
    if(-1 == connId)
    {
       return RIL_AT_INVALID_PARAM;
    }
    
    m_ptrSppDataBuf = ptrBuffer;
    m_nSppDataLenToRd = lenToRead;
    m_nSppRealReadLen = 0;
    Ql_sprintf(strAT, "AT+QSPPREAD=%d,%d", connId, lenToRead);
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QBTSPPREAD_Hdlr, (void *)actualReadlen, 0);
}




void OnURCHandler_BTIndication(const char* strURC, void* reserved)
{
    // +QBTIND: "pair",H60-L01,F4E3FBE47920,760429
    // +QBTIND: "conn","H60-L01",F4E3FBE47920,"SPP"
    // +QBTIND: "disc",2,"H60-L01",F4E3FBE47920,"SPP"
    char urcHead[30];
    char urcType[10];
    char pinCode[BT_PIN_LEN]={0};
    ST_BT_BasicInfo bt_dev;
    s32 connId;
    char temp[30]={0};
	

    Ql_memset(&bt_dev,  0x0, sizeof(ST_BT_BasicInfo));
    Ql_memset(urcType,  0x0, sizeof(urcType));

	if(callback_bt == NULL)
    {
       return;
	}


    Ql_strcpy(urcHead, "\r\n+QBTIND:\0");
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {
        // +QBTIND: "pair",H60-L01,F4E3FBE47920,760429
        Ql_sscanf(strURC, "%*[^\"]\"%[^\"]%*[^\r\n]\r\n", urcType);
        if (Ql_strcmp(urcType, "pair") == 0)
        {
            Ql_sscanf(strURC, "%*[^,],%*[^,],%[^\r\n]\r\n", temp);
            if(Ql_strlen(temp) == (BT_ADDR_LEN - 1))
            {
                 Ql_sscanf(strURC, "%*[^,],%*[^,],%[^\r\n]\r\n", bt_dev.addr);
                 bt_dev.devHdl = Ql_GenHash(bt_dev.addr, Ql_strlen(bt_dev.addr));
                 callback_bt(MSG_BT_PAIR_REQ, URC_BT_NEED_PASSKEY, &bt_dev, NULL); // need paaskey
            }
            else
            {
                Ql_sscanf(strURC, "%*[^,],%*[^,],%[^,]%*[^\r\n]\r\n",bt_dev.addr);
                Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%[^\r\n]\r\n",pinCode);
                bt_dev.devHdl = Ql_GenHash(bt_dev.addr, Ql_strlen(bt_dev.addr));
                callback_bt(MSG_BT_PAIR_REQ, URC_BT_NO_NEED_PASSKEY, &bt_dev, pinCode); //direct confirm
            }   
        }
        else if (Ql_strcmp(urcType, "conn") == 0)
        {
            // +QBTIND: "conn","H60-L01",F4E3FBE47920,"SPP"
           Ql_sscanf(strURC, "%*[^,],\"%[^\"]%*[^\r\n]\r\n",bt_dev.name);
           Ql_sscanf(strURC, "%*[^,],%*[^,],%[^,]%*[^\r\n]\r\n",bt_dev.addr);
           bt_dev.devHdl = Ql_GenHash(bt_dev.addr, Ql_strlen(bt_dev.addr));
           callback_bt(MSG_BT_CONN_REQ,URC_BT_CONN_REQ ,&bt_dev, NULL);
        }
        else if (Ql_strcmp(urcType, "disc") == 0)
        {
            // +QBTIND: "disc",2,2,"H60-L01",F4E3FBE47920,"SPP"
            Ql_sscanf(strURC, "%*[^,],%*[^\"]\"%*[^,],%[^,]%*[^\r\n]\r\n",bt_dev.addr);
            Ql_sscanf(strURC, "%*[^,],%*[^\"]\"%[^\"]%*[^\r\n]\r\n",bt_dev.name);
            bt_dev.devHdl = Ql_GenHash(bt_dev.addr, Ql_strlen(bt_dev.addr));
            BT_DevMngmt_UpdateConnId(bt_dev.devHdl, -1);
            BT_DevMngmt_UpdateProfileId(bt_dev.devHdl, -1);
            callback_bt(MSG_BT_DISCONN_IND, URC_BT_DISCONNECT_PASSIVE, &bt_dev, NULL);
        }else if (Ql_strcmp(urcType, "recv") == 0)
        {

            Ql_sscanf(strURC, "%*[^,],%d\r\n",&connId);
            bt_dev.devHdl = BT_DevMngmt_GetDevHdl(connId);
            callback_bt(MSG_BT_RECV_IND, URC_BT_DATA_RECIEVE, &connId, &bt_dev);
        }
    }else{
        // error
    }
}

static s32 ATRsp_QBTADDR_Hdlr(char* line, u32 len, void* param)
{
    char* pHead = Ql_RIL_FindString(line, len, "\r\n+QBTADDR:");

    if (pHead)
    {
        // +QBTADDR: 1488CD1F6261
        Ql_sscanf(line, "%*[^:]: %[^\r\n]\r\n", (char*)param);
        return  RIL_ATRSP_CONTINUE;
    }

    pHead = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
    if (pHead)
    {  
       return  RIL_ATRSP_SUCCESS;
    }

    pHead = Ql_RIL_FindLine(line, len, "ERROR");// find <CR><LF>ERROR<CR><LF>, <CR>ERROR<CR>£¬<LF>ERROR<LF>
    if (pHead)
    {  
        return  RIL_ATRSP_FAILED;
    } 
    pHead = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if (pHead)
    {
        return  RIL_ATRSP_FAILED;
    }

    return RIL_ATRSP_FAILED; //not supported
}
static s32 ATRsp_QBTSPPREAD_Hdlr(char* line, u32 len, void* param)
{
    // +CME ERROR: 8021
    // +QSPPREAD: 0
    // +QSPPREAD: 20
    // <XXXX> 
    // OK
    static bool sppReadMode = FALSE;
    char* pHead = NULL;


    // The coming data is spp data
    if (sppReadMode)
    {
        Ql_memcpy((void*)(m_ptrSppDataBuf), (const void*)line, m_nSppRealReadLen);
	    if(NULL != param)
       {
           *(u32 *)param = m_nSppRealReadLen ;
       }

		sppReadMode = FALSE;
		if(len>m_nSppRealReadLen)
    	{
	    	pHead = Ql_RIL_FindLine(line+m_nSppRealReadLen, len-m_nSppRealReadLen, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
			if (pHead)
			{  
			   return  RIL_ATRSP_SUCCESS;
			}
		}
		
		return  RIL_ATRSP_CONTINUE;

    }
    
    pHead = Ql_RIL_FindString(line, len, "\r\n+QSPPREAD: ");
    if (pHead)
    {
        Ql_sscanf(line, "%*[^:]: %d", &m_nSppRealReadLen);
        if (m_nSppRealReadLen > 0)
        {
            sppReadMode = TRUE;
        }else{  // no more data
            sppReadMode = FALSE;
            // do nothing, just wait for "OK"
        }
        return  RIL_ATRSP_CONTINUE;
    }

    pHead = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
    if (pHead)
    {  
	   
       return  RIL_ATRSP_SUCCESS;
    }

    pHead = Ql_RIL_FindLine(line, len, "ERROR");// find <CR><LF>ERROR<CR><LF>, <CR>ERROR<CR>£¬<LF>ERROR<LF>
    if (pHead)
    {  
        return  RIL_ATRSP_FAILED;
    } 
    pHead = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if (pHead)
    {
        return  RIL_ATRSP_FAILED;
    }

    return RIL_ATRSP_FAILED; //not supported
}
static s32 ATRsp_QBTSPPSEND_Hdlr(char* line, u32 len, void* param)
{
    
    char* pHead = Ql_RIL_FindString(line, len, ">\r\n");
    u32 ret = 0;
    
    if (pHead)
    {
        
        ret = Ql_RIL_WriteDataToCore(m_ptrSppData, m_nSppDataLenToSnd);

        if(NULL != param && ret >= 0 )
        {
            *(u32 *)param = ret ;
        }
        return  RIL_ATRSP_CONTINUE;
    }

    pHead = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
    if (pHead)
    {  
        return  RIL_ATRSP_SUCCESS;
    }

    pHead = Ql_RIL_FindLine(line, len, "ERROR");// find <CR><LF>ERROR<CR><LF>, <CR>ERROR<CR>£¬<LF>ERROR<LF>
    if (pHead)
    {  
        return  RIL_ATRSP_FAILED;
    } 

    pHead = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if (pHead)
    {
        return  RIL_ATRSP_FAILED;
    }

    return RIL_ATRSP_CONTINUE; //continue wait
}

static s32 ATRsp_QBTSTATE_Hdlr(char* line, u32 len, void* param)
{
    char* pHead = Ql_RIL_FindString(line, len, "+QBTSTATE:"); //continue wait
    bool ret = FALSE ;
    char profile_name[40] = {0};
    static bool is_first_prompt = TRUE;
	s32 status = -1;
    ST_BT_DevInfo* pstrNewBtDev = NULL;

    if (pHead)
    {
        // +QBTSTATE:0,1,"H60-L01",F4E3FBE47920
        // +QBTSTATE:1,2,"H60-L01",F4E3FBE47920,SPP

		if(is_first_prompt)
		{
		    if(NULL != param)
		    {
		       Ql_sscanf(pHead, "%*[^:]: %d\r\n", (s32 *)param);
		    }
			is_first_prompt = FALSE;
		}
		else
		{
             Ql_sscanf(pHead, "%*[^:]: %d%*[^\r\n]\r\n", &status);
		}
        
        if (0 == status)
        {
            // paired device
             pstrNewBtDev = (ST_BT_DevInfo*)Ql_MEM_Alloc(sizeof(ST_BT_DevInfo));
             Ql_memset(pstrNewBtDev, 0x0, sizeof(ST_BT_DevInfo));
             pstrNewBtDev->pairId = -1;
             pstrNewBtDev->connId = -1;
             pstrNewBtDev->profileId = -1;
             Ql_sscanf(line, "%*[^,],%d%*[^\r\n]\r\n",&(pstrNewBtDev->pairId));
             Ql_sscanf(line, "%*[^\"]\"%[^\"]%*[^\r\n]\r\n",pstrNewBtDev->btDevice.name);
             Ql_sscanf(line, "%*[^\"]\"%*[^,],%[^\r\n]\r\n",pstrNewBtDev->btDevice.addr);
             pstrNewBtDev->btDevice.devHdl = Ql_GenHash(pstrNewBtDev->btDevice.addr, Ql_strlen(pstrNewBtDev->btDevice.addr));
             ret = BT_PairUpdateConfirm(pstrNewBtDev->pairId,pstrNewBtDev->btDevice.devHdl,pstrNewBtDev->btDevice.name);
             if(FALSE == ret)
             {
                if(!BT_DevMngmt_isFull())
                {
                   BT_DevMngmt_Append(pstrNewBtDev);
                }
                else
                {
                    Ql_MEM_Free (pstrNewBtDev);
                }
             }
             else
             {
                Ql_MEM_Free (pstrNewBtDev);
             }
            
        }
        else if (1 == status)
        {
            // connected device  +QBTSTATE:1,2,"H60-L01",F4E3FBE47920,SPP
             pstrNewBtDev = (ST_BT_DevInfo*)Ql_MEM_Alloc(sizeof(ST_BT_DevInfo));
             Ql_memset(pstrNewBtDev, 0x0, sizeof(ST_BT_DevInfo));
             pstrNewBtDev->pairId = -1;
             pstrNewBtDev->connId = -1;
             pstrNewBtDev->profileId = -1;
             Ql_sscanf(line, "%*[^,],%d%*[^\r\n]\r\n",&(pstrNewBtDev->connId));
             Ql_sscanf(line, "%*[^\"]\"%[^\"]%*[^\r\n]\r\n",pstrNewBtDev->btDevice.name);
             Ql_sscanf(line, "%*[^\"]\"%*[^,],%[^,]%*[^\r\n]\r\n",pstrNewBtDev->btDevice.addr);
             Ql_sscanf(line, "%*[^\"]\"%*[^\"]\"%*[^\"]\"%[^\"]%*[^\r\n]\r\n",profile_name);
			 pstrNewBtDev->profileId = BT_DevMngmt_ProfileNameToId(profile_name);  
             pstrNewBtDev->btDevice.devHdl = Ql_GenHash(pstrNewBtDev->btDevice.addr, Ql_strlen(pstrNewBtDev->btDevice.addr));
             ret = BT_ConnectUpdateConfirm(pstrNewBtDev->connId,pstrNewBtDev->profileId,pstrNewBtDev->btDevice.devHdl,pstrNewBtDev->btDevice.name);
             if(FALSE == ret)
             {
                if(!BT_DevMngmt_isFull())
                {
                   BT_DevMngmt_Append(pstrNewBtDev);
            
                }
                else
                {
                    Ql_MEM_Free (pstrNewBtDev);
                }
             }
             else
             {
                Ql_MEM_Free (pstrNewBtDev);
             }
        }
        
        return  RIL_ATRSP_CONTINUE;
    }

    pHead = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
    if (pHead)
    {  
        is_first_prompt = TRUE;
        return  RIL_ATRSP_SUCCESS;
    }

    pHead = Ql_RIL_FindLine(line, len, "ERROR");// find <CR><LF>ERROR<CR><LF>, <CR>ERROR<CR>£¬<LF>ERROR<LF>
    if (pHead)
    {  
        return  RIL_ATRSP_FAILED;
    } 

    pHead = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if (pHead)
    {
        return  RIL_ATRSP_FAILED;
    }

    return RIL_ATRSP_CONTINUE; //continue wait
}


static s32 ATRsp_QBTPWR_Hdlr(char* line, u32 len, void* param)
{

    char* pHead = Ql_RIL_FindString(line, len, "\r\n+QBTPWR:");
  
    if (pHead)
    {
        Ql_sscanf(line, "%*[^:]: %d\r\n", (s32*)param);

        return  RIL_ATRSP_CONTINUE;
    }

    pHead = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
    if (pHead)
    {  
       return  RIL_ATRSP_SUCCESS;
    }

    pHead = Ql_RIL_FindLine(line, len, "ERROR");// find <CR><LF>ERROR<CR><LF>, <CR>ERROR<CR>£¬<LF>ERROR<LF>
    if (pHead)
    {  
        return  RIL_ATRSP_FAILED;
    } 
    pHead = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if (pHead)
    {
        return  RIL_ATRSP_FAILED;
    }

    return RIL_ATRSP_FAILED; //not supported
}



static s32 ATRsp_QBTNAME_Hdlr(char* line, u32 len, void* param)
{

    char* pHead = Ql_RIL_FindString(line, len, "\r\n+QBTNAME:");

    if (pHead)
    {
        Ql_sscanf(line, "%*[^\"]\"%[^\"]%*[^\r\n]\r\n", (char*)param);

        return  RIL_ATRSP_CONTINUE;
    }

    pHead = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
    if (pHead)
    {  
       return  RIL_ATRSP_SUCCESS;
    }

    pHead = Ql_RIL_FindLine(line, len, "ERROR");// find <CR><LF>ERROR<CR><LF>, <CR>ERROR<CR>£¬<LF>ERROR<LF>
    if (pHead)
    {  
        return  RIL_ATRSP_FAILED;
    } 
    pHead = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if (pHead)
    {
        return  RIL_ATRSP_FAILED;
    }

    return RIL_ATRSP_FAILED; //not supported
}


static s32 ATRsp_QBTVISB_Hdlr(char* line, u32 len, void* param)
{

    char* pHead = Ql_RIL_FindString(line, len, "\r\n+QBTVISB:");

    if (pHead)
    {
        Ql_sscanf(line, "%*[^:]: %d\r\n", (s32 *)param);

        return  RIL_ATRSP_CONTINUE;
    }

    pHead = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
    if (pHead)
    {  
       return  RIL_ATRSP_SUCCESS;
    }

    pHead = Ql_RIL_FindLine(line, len, "ERROR");// find <CR><LF>ERROR<CR><LF>, <CR>ERROR<CR>£¬<LF>ERROR<LF>
    if (pHead)
    {  
        return  RIL_ATRSP_FAILED;
    } 
    pHead = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if (pHead)
    {
        return  RIL_ATRSP_FAILED;
    }

    return RIL_ATRSP_FAILED; //not supported
}


static s32 ATRsp_QBTGPROF_Hdlr(char* line, u32 len, void* param)
{
    char* pHead = Ql_RIL_FindString(line, len, "\r\n+QBTGPROF:");
	static u8 index = 0 ;

    if (pHead)
    {
        Ql_sscanf(line, "%*[^:]: %d[^\r\n]\r\n", (s32 *)(param)+index);

        return  RIL_ATRSP_CONTINUE;
    }

    pHead = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
    if (pHead)
    {  
       index = 0 ;
       return  RIL_ATRSP_SUCCESS;
    }

    pHead = Ql_RIL_FindLine(line, len, "ERROR");// find <CR><LF>ERROR<CR><LF>, <CR>ERROR<CR>£¬<LF>ERROR<LF>
    if (pHead)
    {  
        return  RIL_ATRSP_FAILED;
    } 
    pHead = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if (pHead)
    {
        return  RIL_ATRSP_FAILED;
    }
    
    return RIL_ATRSP_CONTINUE; //continue wait
}



#endif  //__OCPU_RIL_BT_SUPPORT__

