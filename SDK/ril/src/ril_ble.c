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
#ifdef __OCPU_RIL_BLE_SUPPORT__
#include "ql_type.h"
#include "ql_stdlib.h"
#include "ql_trace.h"
#include "ql_error.h"
#include "ql_common.h"
#include "ql_system.h"
#include "ril.h"
#include "ril_util.h"
#include "ril_ble.h"

static CALLBACK_BLE_IND callback_bt = NULL;

static s32 ATRsp_QBTGatsreg_Hdlr(char* line, u32 len, void* param);
static s32 ATRsp_QBTGatss_Hdlr(char* line, u32 len, void* param);
static s32 ATRsp_QBTGatsi_Hdlr(char* line, u32 len, void* param);
static s32 ATRsp_QBTGatsc_Hdlr(char* line, u32 len, void* param);
static s32 ATRsp_QBTGatsd_Hdlr(char* line, u32 len, void* param);
static s32 ATRsp_QBTGatsst_Hdlr(char* line, u32 len, void* param);
static s32 ATRsp_QBTGatsind_Hdlr(char* line, u32 len, void* param);
static s32 ATRsp_QBTGatsrsp_Hdlr(char* line, u32 len, void* param);
static s32 ATRsp_QBTGatsl_Hdlr(char* line, u32 len, void* param);
static s32 ATRsp_QGatSetadv_Hdlr(char* line, u32 len, void* param);
static void ATRsp_QBTLEADDR_Hdlr(char* line, u32 len, void* param);



/*****************************************************************
* Function:     RIL_BLE_Initialize
* 
* Description:
*               bluetooth initialization after power on ,register callback
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
s32 RIL_BLE_Initialize(CALLBACK_BLE_IND cb)
{
    
    if (!cb)
    {
        return RIL_AT_INVALID_PARAM;
    }
    
    callback_bt = cb;
 
    return RIL_AT_SUCCESS;
}

/*****************************************************************
* Function:     RIL_BT_Gatsreg
* 
* Description:
*               Register a Gatt Server
*
* Parameters:
*               op :[in]  register or deregister
                gserv :[in]  pointer to ST_BLE_Server

*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/

//AT+QBTGATSREG=<op>,"<gserv_id>"
//AT+QBTGATSREG=1,"ABC2" 
s32 RIL_BT_Gatsreg(u8 op , ST_BLE_Server* gserv)
{
    char strAT[100];
    s32 ret;

    if (op < GAT_REMOVE || op > GAT_ADD)
    {
        return RIL_AT_INVALID_PARAM;
    }
    if(NULL == gserv)
    {
        return RIL_AT_INVALID_PARAM;
   }
 
    Ql_memset(strAT, 0x0, sizeof(strAT));
    
    Ql_sprintf(strAT, "AT+QBTGATSREG=%d,\"%s\"", op,gserv->gserv_id);

    ret = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QBTGatsreg_Hdlr,(void*)gserv,0);
    
    return  gserv->result;
    
}

/*****************************************************************
* Function:     RIL_BT_Gatss
* 
* Description:
*               Add or remove a service 
*
* Parameters:
*               op :[in]  add or remove 
                gserv :[in]  pointer to ST_BLE_Server

*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/


//AT+QBTGATSS=1,"ABC2","3418",5,1,254
//AT+QBTGATSS=0,"<gserv_id>",<service_handle>
//AT+QBTGATSS=1,"<gserv_id>","<service_uuid>",<num_handles>,<is_primary>,<inst>
s32 RIL_BT_Gatss(u8 op , ST_BLE_Server* gserv)
{
     char strAT[100];
     s32 ret;
     ST_BLE_Service  *service_op = NULL;

     if (op < GAT_REMOVE || op > GAT_ADD)
     {
        return RIL_AT_INVALID_PARAM;
     }

     if((gserv == NULL)||(gserv->sid>=SERVICE_NUM) || (gserv->sid<0))
     {
        return RIL_AT_INVALID_PARAM;
     }

      service_op = &(gserv->service_id[gserv->sid]);

    
      Ql_memset(strAT, 0x0, sizeof(strAT));     

      if(op)
      {
            if(service_op ->is_used ==1)
           {
                return RIL_AT_INVALID_PARAM;
           }
           Ql_sprintf(strAT, "AT+QBTGATSS=%d,\"%s\",\"%s\",%d,%d,%d",op,gserv->gserv_id,service_op->service_uuid,service_op->num_handles,service_op->is_primary,service_op->inst);
  
           ret = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QBTGatss_Hdlr,(void *)gserv, 0);
           if(gserv->result == 0)
           {
                service_op ->is_used = 1;
           }
      }
      else
      {
           if(service_op ->is_used ==0)
           {
                return RIL_AT_INVALID_PARAM;
           }
          
            Ql_sprintf(strAT, "AT+QBTGATSS=%d,\"%s\",%d",op,gserv->gserv_id,service_op->service_handle);
  
           ret = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QBTGatss_Hdlr,(void *)gserv, 0);
           if(gserv->result == 0)
           {
                service_op ->is_used = 0;
                Ql_memset(service_op,0,sizeof(ST_BLE_Service));
           }
      }

    return gserv->result;
}

/*****************************************************************
* Function:     RIL_BT_Gatsc
* 
* Description:
*               Add characteristic to a service 
*
* Parameters:
*               op :[in]  currently support add 
                gserv :[in]  pointer to ST_BLE_Server

*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/

//AT+QBTGATSC=1,"<gserv_id>",<service_handle>,"<char_uuid>",<inst>,<prop>,<permission>
//AT+QBTGATSC=1,"ABC2",256,"332a",2,58,17
s32 RIL_BT_Gatsc(u8 op , ST_BLE_Server* gserv)
{
    char strAT[100];
    s32 ret;
    u8 s_id=0,c_id; 

    ST_BLE_Char  *char_op = NULL;

    s_id=gserv->sid;
    c_id =gserv->service_id[s_id].cid;

    if (op != GAT_ADD)
    {
        return RIL_AT_INVALID_PARAM;
    }
    if((gserv == NULL)||(s_id>=SERVICE_NUM)||(c_id>=CHAR_NUM)||(s_id < 0)||(c_id < 0))
    {
        return RIL_AT_INVALID_PARAM;
    }


     char_op = &(gserv->service_id[gserv->sid].char_id[c_id]);

      if(char_op ->is_used ==1)
      {
           return RIL_AT_INVALID_PARAM;
      }

     Ql_memset(strAT, 0x0, sizeof(strAT));
     
    Ql_sprintf(strAT, "AT+QBTGATSC=%d,\"%s\",%d,\"%s\",%d,%d,%d", op,gserv->gserv_id, gserv->service_id[s_id].service_handle,\
    char_op->char_uuid, char_op->inst, char_op->prop, char_op->permission);

    ret =  Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QBTGatsc_Hdlr, (void *)gserv, 0);

     if(gserv->result == 0)
     {
          char_op ->is_used = 1;
     }

    return gserv->result;
}

/*****************************************************************
* Function:     RIL_BT_Gatsd
* 
* Description:
*               Add descriptor to char 
*
* Parameters:
*               op :[in]  currently support add 
                gserv :[in]  pointer to ST_BLE_Server

*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/


//AT+QBTGATSD=1,"<gserv_id>",<service_handle>,"<desc_uuid>",<inst>,<permission>
//AT+QBTGATSD=1,"ABC2",256,"1329",1,17
s32 RIL_BT_Gatsd(u8 op , ST_BLE_Server* gserv)
{
    char strAT[100];
    s32 ret;
    u8 s_id,c_id=0,d_id=0;
    ST_BLE_Desc  *des_op = NULL;

    
    s_id=gserv->sid;
    c_id =gserv->service_id[s_id].cid;
    d_id = gserv->service_id[s_id].char_id[c_id].did;
    


    if (op != GAT_ADD)
    {
        return RIL_AT_INVALID_PARAM;
    }

    if((gserv == NULL)||(s_id>=SERVICE_NUM)||(c_id>=CHAR_NUM)||(d_id>=DESC_NUM)||(s_id < 0)||(c_id < 0)||(d_id < 0))
    {
        return RIL_AT_INVALID_PARAM;
    }

    des_op = &(gserv->service_id[gserv->sid].char_id[c_id].desc_id[d_id]);

     if(des_op ->is_used ==1)
      {
           return RIL_AT_INVALID_PARAM;
      }


    Ql_memset(strAT, 0x0, sizeof(strAT));
    
    Ql_sprintf(strAT, "AT+QBTGATSD=%d,\"%s\",%d,\"%s\",%d,%d", op,gserv->gserv_id,gserv->service_id[gserv->sid].service_handle,\
    des_op->desc_uuid,des_op->inst,des_op->permission);

    ret = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QBTGatsd_Hdlr, (void *)gserv, 0);

    if(gserv->result == 0)
     {
          des_op ->is_used = 1;
     }

    return gserv->result;
}

/*****************************************************************
* Function:     RIL_BT_Gatsst
* 
* Description:
*               Start / Stop a service
*
* Parameters:
*               op :[in]  start or stop
                gserv :[in]  pointer to ST_BLE_Server

*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/


//AT+QBTGATSST=<op>,"<gserv_id>",<service_handle>,[<transport>]
//AT+QBTGATSST=1,"ABC2",256,0
s32 RIL_BT_Gatsst(u8 op , ST_BLE_Server* gserv)
{
    char strAT[100];
    s32 ret;
    ST_BLE_Service  *service_op = NULL;


    if (op < 0 || op > 1)
    {
        return RIL_AT_INVALID_PARAM;
    }

    if((gserv == NULL)||(gserv->sid>=SERVICE_NUM)||(gserv->sid < 0))
    {
        return RIL_AT_INVALID_PARAM;
    }

    service_op =&(gserv->service_id[gserv->sid]);
    
    Ql_memset(strAT, 0x0, sizeof(strAT));

    if(op == 1)
    {
          if((service_op->transport < 0) || (service_op->transport > 2))
         {
               return RIL_AT_INVALID_PARAM;
     
         }
          if(service_op->is_started == 1)
         {
              return RIL_AT_INVALID_PARAM;
     
         }

        Ql_sprintf(strAT, "AT+QBTGATSST=%d,\"%s\",%d,%d", op,gserv->gserv_id,service_op->service_handle,service_op->transport);
         ret = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QBTGatsst_Hdlr, (void *)gserv, 0);

         if(gserv->result == 0)
         {
            service_op->is_started = 1;
         }

    }
    else
    {   
        Ql_sprintf(strAT, "AT+QBTGATSST=%d,\"%s\",%d", op,gserv->gserv_id,service_op->service_handle);
        ret = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QBTGatsst_Hdlr, (void *)gserv, 0);
         if(gserv->result == 0)
         {
            service_op->is_started = 0;
         }

    }


    return gserv->result;
}

/*****************************************************************
* Function:     RIL_BT_Gatsind
* 
* Description:
*               Send an indication to client
*
* Parameters:
                gserv :[in]  pointer to ST_BLE_Server

*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/


//AT+QBTGATSIND="<gserv_id>",<conn_id>,<attr_handle>,<need_confirm>,<ind_value>
//AT+QBTGATSIND="ABC2",1,259,1,"74ab" 
s32 RIL_BT_Gatsind(ST_BLE_Server* gserv)
{
    char strAT[1500];
    s32 result;
    ST_BLE_Sind *sind_op = NULL;
    ST_BLE_ConnStatus *conn = NULL;

    if(gserv == NULL)
    {
        return RIL_AT_INVALID_PARAM;
    }

    sind_op = &(gserv->sind_param);
    conn = &(gserv->conn_status);

    Ql_memset(strAT,0,sizeof(strAT));

    Ql_sprintf(strAT, "AT+QBTGATSIND=\"%s\",%d,%d,%d,\"%s\"", gserv->gserv_id,conn->connect_id,sind_op->attr_handle,sind_op->need_cnf,sind_op->value);

	if(sind_op->need_cnf==1)
	{
	    Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QBTGatsind_Hdlr, (void *)gserv, 0);
		return gserv->result;
	}
	else
	{
		return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL,NULL, 0);
	}
	  
}

/*****************************************************************
* Function:     RIL_BT_Gatsrsp
* 
* Description:
*               Send an response to client
*
* Parameters:
                gserv :[in]  pointer to ST_BLE_Server

*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/

//AT+QBTGATSRSP="<gserv_id>",<response>,<conn_id>,<trans_id>,<attr_handle>,<rsp_value>
//AT+QBTGATSRSP="ABC2",0,1,18,259,"adb2"  
s32 RIL_BT_Gatsrsp(ST_BLE_Server* gserv)
{
    char strAT[100];
    s32 result;

    ST_BLE_WRreq *wr_op = NULL;
    ST_BLE_ConnStatus *conn = NULL;

    if(gserv == NULL)
    {
        return RIL_AT_INVALID_PARAM;
    }
    
    wr_op = &(gserv->wrreq_param);
    conn = &(gserv->conn_status);

    Ql_memset(strAT,0,sizeof(strAT));
    
    Ql_sprintf(strAT, "AT+QBTGATSRSP=\"%s\",%d,%d,%d,%d,\"%s\"", gserv->gserv_id,wr_op->need_rsp,conn->connect_id,wr_op->trans_id,wr_op->attr_handle,wr_op->value);
  
    Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QBTGatsrsp_Hdlr, (void *)gserv, 0);

    return gserv->result;
}

/*****************************************************************
* Function:     RIL_BT_Gatsl
* 
* Description:
*               Start/stop gatt server listening 
*
* Parameters:
                op  :[in]  start or stop
                gserv :[in]  pointer to ST_BLE_Server

*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
//AT+QBTGATSL="<gserv_id>",<op>
//AT+QBTGATSL="ABC2",1  
s32 RIL_BT_Gatsl(u8 op , ST_BLE_Server* gserv)
{
    char strAT[100];
    s32 result;

     if (op < GAT_REMOVE || op > GAT_ADD)
    {
        return RIL_AT_INVALID_PARAM;
    }

    if(gserv == NULL)
    {
        return RIL_AT_INVALID_PARAM;
    }

    
    Ql_memset(strAT,0,sizeof(strAT));
    Ql_sprintf(strAT, "AT+QBTGATSL=\"%s\",%d",gserv->gserv_id, op);
    Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QBTGatsl_Hdlr, (void *)gserv, 0);

    return gserv->result;
}


/*****************************************************************
* Function:     RIL_BT_SetLeTxPwr
* 
* Description:
*               Set the Level of BLE Transmission Power
*
* Parameters:
               tx_level : The level of BLE transmission power. The range is 0-7.

*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*****************************************************************/
//AT+QBTLETXPWR=<tx_level>
s32 RIL_BT_SetLeTxPwr(u8 tx_level)
{
    char strAT[100];
    s32 result;

     if (tx_level < 0 || tx_level > 7)
    {
        return RIL_AT_INVALID_PARAM;
    }

 
    Ql_memset(strAT,0,sizeof(strAT));
    Ql_sprintf(strAT, "AT+QBTLETXPWR=%d",tx_level);
    Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL, NULL, 0);

    return 0;
}


/*****************************************************************
* Function:     RIL_BT_QBTFMPsreg
* 
* Description:
*               Start/stop FMP profile
*
* Parameters:
                op  :[in]  start or stop

*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/


//AT+QBTFMPSREG=1
s32 RIL_BT_QBTFMPsreg(u8 op )
{
    char strAT[100];

     if (op < GAT_STOP || op > GAT_START)
    {
        return RIL_AT_INVALID_PARAM;
    }
    
    Ql_memset(strAT,0,sizeof(strAT));
    Ql_sprintf(strAT, "AT+QBTFMPSREG=%d", op);
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL, NULL, 0);
}

/*****************************************************************
* Function:     RIL_BT_QBTPXPsreg
* 
* Description:
*               Start/stop PXP profile
*
* Parameters:
                op  :[in]  start or stop

*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/


//AT+QBTPXPSREG=1
s32 RIL_BT_QBTPXPsreg(u8 op )
{
    char strAT[100];

     if (op < GAT_STOP || op > GAT_START)
    {
        return RIL_AT_INVALID_PARAM;
    }
 
    
    Ql_memset(strAT,0,sizeof(strAT));
    Ql_sprintf(strAT, "AT+QBTPXPSREG=%d", op);
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL, NULL, 0);
}

/*****************************************************************
* Function:     RIL_BT_QBTGatadv
* 
* Description:
*               Set advertising interval gap for gatt client
* Parameters:
                min_interval  :[in]  min interval
                
                max_interval  :[in]  max interval

*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/


//AT+QBTGATADV=<min_interval>,<max_interval>
s32 RIL_BT_QBTGatadv(u16 min_interval,u16 max_interval)
{
    char strAT[100];
    
    Ql_memset(strAT,0,sizeof(strAT));

    Ql_sprintf(strAT, "AT+QBTGATADV=%d,%d", min_interval,max_interval);
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL, NULL, 0);
}

/*****************************************************************
* Function:     RIL_BT_QGatSetadv
* 
* Description:
*               Set advertising data
* Parameters:
                gserv_id : User ID (or the name) of GATT server 
                appearance: The external appearance of the device
                manufacture_data: It is manufacture information.
                service_data: It is service information.
                service_uuid: UUID of this service.
*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
//AT+ QGATSETADV =<gserv_id>,<appearance>,<manufacture_data>,<service_data>,<service_uuid>
s32 RIL_BT_QGatSetadv(char* gserv_id,u16 appearance,u16 string_mode,u8* manufacture_data,u8* service_data,u8* service_uuid)
{
    char strAT[100];
	s32 result;
    
    Ql_memset(strAT,0,sizeof(strAT));

    Ql_sprintf(strAT, "AT+QGATSETADV=\"%s\",%d,%d,\"%s\",\"%s\",\"%s\"", gserv_id,appearance,string_mode,manufacture_data,service_data,service_uuid);

	Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QGatSetadv_Hdlr, &result, 0);

	return result;

}

s32 RIL_BT_QGatadvData(char* gserv_id,u8* adv_data)
{
    char strAT[100];

    Ql_memset(strAT,0,sizeof(strAT));
    Ql_sprintf(strAT, "AT+QGATADVDATA=\"%s\",\"%s\"",gserv_id,adv_data);

    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL, NULL, 0);
}

s32 RIL_BT_QGatScanRsp(char* gserv_id,u8* rsp_data)
{
    char strAT[100];

    Ql_memset(strAT,0,sizeof(strAT));
    Ql_sprintf(strAT, "AT+QGATSCANRSP=\"%s\",\"%s\"", gserv_id,rsp_data);

    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL, NULL, 0);
}


/*****************************************************************
* Function:     RIL_BT_Gatcpu
* 

*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/

//AT+QBTGATCPU=<bt_addr>,<min_interval>,<max_interval>,<timeout>,<latency>
//AT+QBTGATCPU=E62CA017A503,288,304,600,4
s32 RIL_BT_Gatcpu(char* bt_addr,u16 min_interval,u16 max_interval,u16 timeout,u16 latency)
{
    char strAT[100];
  
    Ql_memset(strAT,0,sizeof(strAT));
    Ql_sprintf(strAT, "AT+QBTGATCPU=%s,%d,%d,%d,%d",bt_addr ,min_interval,max_interval,timeout,latency);
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL, NULL, 0);
}

/*****************************************************************
* Function:     RIL_BT_QGatsdisc
* 
* Description:
*               Disconnect the current connection.
* Parameters:
                conn_id:[in] ID of current connection
*
* Return:        
*                RIL_AT_SUCCESS,send AT successfully.
*                RIL_AT_FAILED, send AT failed.
*                RIL_AT_TIMEOUT,send AT timeout.
*                RIL_AT_BUSY,   sending AT.
*                RIL_AT_INVALID_PARAM, invalid input parameter.
*                RIL_AT_UNINITIALIZED, RIL is not ready, need to wait for MSG_ID_RIL_READY
*****************************************************************/
//AT+QBTGATSDISC=<conn_id>
s32 RIL_BT_QGatsdisc(u8 conn_id)
{
    char strAT[20];
	s32 result;
    
    Ql_memset(strAT,0,sizeof(strAT));

    Ql_sprintf(strAT, "AT+QBTGATSDISC=%d",conn_id);

	return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), NULL, &result, 0);
}


//+QBTGATSCON: <op>,<gserv_id>,<result>,<bt_addr>,<conn_id>
//+QBTGATSCON: 1,"ABC2",0,CB2CD7923F46,1
void OnURCHandler_BTGatscon(const char* strURC, void* reserved)
{
    s32 err_code = 0;
    char urcHead[40];
    char serv_id[32]={0};
    ST_BLE_ConnStatus con_status = {0};

    
    Ql_strcpy(urcHead, "\r\n+QBTGATSCON:");
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {
        Ql_sscanf(strURC, "%*[^\"]\"%[^\"]%*[^\r\n]\r\n",serv_id);
        
        Ql_sscanf(strURC, "%*[^:]: %d,%*[^\r\n]\r\n",&(con_status.connect_status));
        
        Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%*[^,],%d\r\n", &(con_status.connect_id) );
        Ql_sscanf(strURC, "%*[^,],%*[^,],%*[^,],%[^,],%*[^\r\n]\r\n", con_status.peer_addr );
        
        Ql_sscanf(strURC, "%*[^,],%*[^,],%d,%*[^\r\n]\r\n", &(con_status.result));
        
        if(callback_bt != NULL)
        {            
            callback_bt(MSG_BLE_CONNECT,0,serv_id, &con_status);
        }
      
    }
        return; 
}

/*****************************************************************
* Function:     RIL_BT_GetLELocalAddr
* 
* Description:
*               get the BLE device address of bluetooth
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

s32 RIL_BT_GetLeLocalAddr(char* ptrAddr,u8 len)
{
    char strAT[] = "AT+QBTLEADDR?\0";
    s32 ret = RIL_AT_SUCCESS;
    char in_addr[BT_ADDR_LEN] = {0};
    
    if (NULL == ptrAddr)
    {
        return RIL_AT_INVALID_PARAM;
    }

	Ql_memset(ptrAddr,0,len);

    ret = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_QBTLEADDR_Hdlr,(void *)in_addr, 0);

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



//+QBTGATWREQ: <gserv_id>,<conn_id>,<trans_id>,<bt_addr>,<attr_handle>,<value>,<need_rsp>,<is_prepare>,<offset>
//+QBTGATWREQ: "ABC2",1,19,CB2CD7923F46,259,37383336,1,0,0
void OnURCHandler_BTGatwreq(const char* strURC, void* reserved)
{
    s32 err_code = 0;
    char urcHead[50];
    char server_id[33];  
    ST_BLE_WRreq  wreg_para;

    Ql_strcpy(urcHead, "\r\n+QBTGATWREQ:");
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {
        Ql_sscanf(strURC, "%*[^\"]\"%[^\"]%*[^\r\n]\r\n",&server_id );
    
        Ql_sscanf(strURC, "%*[^:]: %*[^,],%*[^,],%d,%*[^\r\n]\r\n",&(wreg_para.trans_id) );  
         
        Ql_sscanf(strURC, "%*[^:]: %*[^,],%*[^,],%*[^,],%*[^,],%d,%*[^\r\n]\r\n", &(wreg_para.attr_handle)  );

        Ql_sscanf(strURC, "%*[^:]: %*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%d,%*[^\r\n]\r\n", &(wreg_para.need_rsp)  );
        
        Ql_sscanf(strURC,  "%*[^:]: %*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%[^,],%*[^\r\n]\r\n", (wreg_para.value) );//result

        if(callback_bt != NULL)
        {
            
            callback_bt(MSG_BLE_WREG_IND,0,&server_id, &wreg_para);
        }
      
    }

        return; 
}

//+QBTGATEWREQ: <gserv_id>,<conn_id>,<trans_id>,<bt_addr>,<cancel>
void OnURCHandler_BTGatewreq(const char* strURC, void* reserved)
{
    s32 err_code = 0;
    char urcHead[50];
    char server_id[33];
    ST_BLE_WRreq  wreg_para;
 
    Ql_strcpy(urcHead, "\r\n+QBTGATEWREQ:");
     if (Ql_StrPrefixMatch(strURC, urcHead))
    {
        Ql_sscanf(strURC, "%*[^\"]\"%[^\"]%*[^\r\n]\r\n",&server_id );
    
        Ql_sscanf(strURC, "%*[^:]: %*[^,],%*[^,],%d,%*[^\r\n]\r\n",&(wreg_para.trans_id));  
         
        Ql_sscanf(strURC, "%*[^:]: %*[^,],%*[^,],%*[^,],%*[^,],%d,%*[^\r\n]\r\n", &(wreg_para.attr_handle));
	
		Ql_sscanf(strURC, "%*[^:]: %*[^,],%*[^,],%*[^,],%*[^,],%[^\r\n]\r\n", &(wreg_para.cancel));
       
        if(callback_bt != NULL)
        {
            callback_bt(MSG_BLE_EWREG_IND,0,&server_id, &wreg_para);
        }
      
    }
        return; 
}


//+QBTGATRREQ: <gserv_id>,<conn_id>,<trans_id>,<bt_addr>,<attr_handle>,<is_long>,<offset>
//+QBTGATRREQ: "ABC2",1,18,CB2CD7923F46,259,0,0
void OnURCHandler_BTGatrreq(const char* strURC, void* reserved)
{
    s32 err_code = 0;
    char urcHead[50];
    char server_id[33];
    ST_BLE_WRreq  wreg_para;

    Ql_strcpy(urcHead, "\r\n+QBTGATRREQ:");
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {

        Ql_sscanf(strURC, "%*[^\"]\"%[^\"]%*[^\r\n]\r\n",&server_id );
        //Ql_sscanf(strURC, "%*[^,],%d,%*[^\r\n]\r\n",&(wreg_para.connect_id) );
    
        Ql_sscanf(strURC, "%*[^:]: %*[^,],%*[^,],%d,%*[^\r\n]\r\n",&(wreg_para.trans_id) );
        
        Ql_sscanf(strURC, "%*[^:]: %*[^,],%*[^,],%*[^,],%*[^,],%d,%*[^\r\n]\r\n", &(wreg_para.attr_handle)  );

        if(callback_bt != NULL)
        {
            
            callback_bt(MSG_BLE_RREG_IND,0,&server_id, &wreg_para);
        }
      
    }
        return; 
}

//+QBTPXPSCON: <bt_addr>
void OnURCHandler_BTpxpscon(const char* strURC, void* reserved)
{
    s32 err_code = 0;
    char urcHead[30];
    char bt_addr[13]={0};


    Ql_strcpy(urcHead, "\r\n+QBTGATSCON:");
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {
        Ql_sscanf(strURC, "%*[^:]: %[^\r\n]\r\n",bt_addr);

        if(callback_bt != NULL)
        {   
            callback_bt(MSG_BLE_PXP_CONNECT,0,bt_addr, 0);
        }
        
    }
        return; 
}

//+QBTFMPSCONSREG: <op>,<bt_addr>
void OnURCHandler_BTfmpscon(const char* strURC, void* reserved)
{
    s32 err_code = 0;
    char urcHead[30];
    char bt_addr[13];
    

    if(callback_bt == NULL)
    {
        return ;
    }

    Ql_strcpy(urcHead, "\r\n+QBTFMPSCONSREG:");
    if (Ql_StrPrefixMatch(strURC, urcHead))
    {
        Ql_sscanf(strURC, "%*[^:]: %*[^,],%[^\r\n]\r\n",bt_addr);

        if(callback_bt != NULL)
        {     
            callback_bt(MSG_BLE_FMP_CONNECT,0,bt_addr, 0);
        }
           
    }
        return; 
}



//+QBTGATSREG:1,"ABC2",0  
//+QBTGATSREG: <op>,<gserv_id>,<result>
static s32 ATRsp_QBTGatsreg_Hdlr(char* line, u32 len, void* param)
{
    ST_BLE_Server* gserv_op =( ST_BLE_Server*)param;

    char* pHead = Ql_RIL_FindString(line, len, "\r\n+QBTGATSREG:");
 
    if (pHead)
    {
        Ql_sscanf(line, "%*[^,],%*[^,],%d\r\n",&(gserv_op->result));//result

        return  RIL_ATRSP_CONTINUE;
    }

    pHead = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
    if (pHead)
    {  
        return RIL_ATRSP_SUCCESS;
    }

   gserv_op->result = RIL_ATRSP_FAILED;

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

//+QBTGATSS:1,"ABC2",3418,1,0,254,256
//+QBTGATSS: 0,<gserv_id>,<result>,<service_handle>  
//+QBTGATSS: 1,<gserv_id>,<service_uuid>,<is_primary>,<result>,<inst>,<service_handle>

static s32 ATRsp_QBTGatss_Hdlr(char* line, u32 len, void* param)
{

    ST_BLE_Server* gserv_op =( ST_BLE_Server*)param;
    s32 op;
    
    char* pHead = Ql_RIL_FindString(line, len, "\r\n+QBTGATSS:");

    if (pHead)
    {
        
        Ql_sscanf(line, "%*[^:]: %d,%*[^,],\r\n",&op); 
          
        if(op == 1)  //add service
        {     
            Ql_sscanf(line, "%*[^,],%*[^,],%*[^,],%*[^,],%d,%*[^,],%d\r\n", &(gserv_op->result),&(gserv_op->service_id[gserv_op->sid].service_handle)); 
        }
        else
        {    
            Ql_sscanf(line, "%*[^,],%*[^,],%d,%d\r\n", &(gserv_op->result),&(gserv_op->service_id[gserv_op->sid].service_handle)); 
        }
      
        return  RIL_ATRSP_CONTINUE;
    }

    pHead = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
    if (pHead)
    {  
       return  RIL_ATRSP_SUCCESS;
    }
    gserv_op->result = RIL_ATRSP_FAILED;
    
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


//+QBTGATSC: 1,<gserv_id>,<service_handle>,<char_uuid>,<inst>,<result>,<char_handle>
//+QBTGATSC:1,"ABC2",256,"332A",2,0,259
static s32 ATRsp_QBTGatsc_Hdlr(char* line, u32 len, void* param)
{
    ST_BLE_Server* gserv_op =( ST_BLE_Server*)param;
    u8  s_id=gserv_op->sid;
    u8  c_id =gserv_op->service_id[s_id].cid;

    char* pHead = Ql_RIL_FindString(line, len, "\r\n+QBTGATSC:");
 
    if (pHead)
    {
        Ql_sscanf(line, "%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%d,%d\r\n",&(gserv_op->result),&(gserv_op->service_id[s_id].char_id[c_id].char_handle)); //result

        return  RIL_ATRSP_CONTINUE;
    }

    pHead = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
    if (pHead)
    {  
       return  RIL_ATRSP_SUCCESS;
    }
    
    gserv_op->result = RIL_ATRSP_FAILED;
    
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

//+QBTGATSD: 1, <gserv_id>,<service_handle>,<desc_uuid>,<inst>,<result>,<desc_handle>
//+QBTGATSD:1,"ABC2",256,"1329",1,0,260
static s32 ATRsp_QBTGatsd_Hdlr(char* line, u32 len, void* param)
{
    char* pHead = Ql_RIL_FindString(line, len, "\r\n+QBTGATSD:");

    ST_BLE_Server* gserv_op =( ST_BLE_Server*)param;

    u8  s_id=gserv_op->sid;
    u8  c_id =gserv_op->service_id[s_id].cid;
    u8  d_id = gserv_op->service_id[s_id].char_id[c_id].did;
 
    if (pHead)
    {
       Ql_sscanf(line, "%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%d,%d\r\n",&(gserv_op->result),&(gserv_op->service_id[s_id].char_id[c_id].desc_id[d_id].desc_handle));

        return  RIL_ATRSP_CONTINUE;
    }

    pHead = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
    if (pHead)
    {  
       return  RIL_ATRSP_SUCCESS;
    }
    gserv_op->result = RIL_ATRSP_FAILED;

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

//+QBTGATSST: <op>,<gserv_id>,<result>,<service_handle>
//+QBTGATSST: 1,"ABC2",0,256
static s32 ATRsp_QBTGatsst_Hdlr(char* line, u32 len, void* param)
{
    char* pHead = Ql_RIL_FindString(line, len, "\r\n+QBTGATSST:");

    ST_BLE_Server* gserv_op =( ST_BLE_Server*)param;
    
    if (pHead)
    {
        Ql_sscanf(line, "%*[^,],%*[^,],%d,%*[^\r\n]\r\n", &(gserv_op->result));//result        

        return  RIL_ATRSP_CONTINUE;
    }

    pHead = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
    if (pHead)
    {  
       return  RIL_ATRSP_SUCCESS;
    }
    
    gserv_op->result = RIL_ATRSP_FAILED;

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

//+QBTGATSIND: <result>,<gserv_id>,<conn_id>,<attr_handle>
//+QBTGATSIND:0,"ABC2",1,2
static s32 ATRsp_QBTGatsind_Hdlr(char* line, u32 len, void* param)
{
    char* pHead = Ql_RIL_FindString(line, len, "\r\n+QBTGATSIND:");
    
    ST_BLE_Server* gserv_op =( ST_BLE_Server*)param;

    if (pHead)
    {
        
        Ql_sscanf(line, "%*[^:]: %d,%*[^\r\n]\r\n", &(gserv_op->result));//result
        return  RIL_ATRSP_CONTINUE;
    }

    pHead = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
    if (pHead)
    {  
       return  RIL_ATRSP_SUCCESS;
    }
    gserv_op->result = RIL_ATRSP_FAILED;

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

//+QBTGATSRSP: <result>,<gserv_id>,<conn_id>,<attr_handle>
//+QBTGATSRSP:0,"ABC2",1,259
static s32 ATRsp_QBTGatsrsp_Hdlr(char* line, u32 len, void* param)
{
    char* pHead = Ql_RIL_FindString(line, len, "\r\n+QBTGATSRSP:");
    
    ST_BLE_Server* gserv_op =( ST_BLE_Server*)param;

    if (pHead)
    {
        Ql_sscanf(line, "%*[^:]: %d,%*[^\r\n]\r\n",  &(gserv_op->result));

        return  RIL_ATRSP_CONTINUE;
    }

    pHead = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
    if (pHead)
    {  
       return  RIL_ATRSP_SUCCESS;
    }

    
    gserv_op->result = RIL_ATRSP_FAILED;

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

//+QBTGATSL: "ABC2",0
//+QBTGATSL: <gserv_id>,<result>
static s32 ATRsp_QBTGatsl_Hdlr(char* line, u32 len, void* param)
{
    char* pHead = Ql_RIL_FindString(line, len, "\r\n+QBTGATSL:");

    
    ST_BLE_Server* gserv_op =( ST_BLE_Server*)param;

    if (pHead)
    {
        Ql_sscanf(line, "%*[^,],%d\r\n", &(gserv_op->result));

        return  RIL_ATRSP_CONTINUE;
    }

    pHead = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
    if (pHead)
    {  
       return  RIL_ATRSP_SUCCESS;
    }
    
    gserv_op->result = RIL_ATRSP_FAILED;

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

static s32 ATRsp_QGatSetadv_Hdlr(char* line, u32 len, void* param)
{
    char* pHead = Ql_RIL_FindString(line, len, "\r\n+QGATSETADV:");

    if (pHead)
    {
        Ql_sscanf(line, "%*[^:]: %d\r\n", param);

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
    	*(s32*)param = (s32)RIL_ATRSP_FAILED;
        return  RIL_ATRSP_FAILED;
    } 
    pHead = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if (pHead)
    {
    	Ql_sscanf(line, "%*[^:]: %d\r\n", param);
        return  RIL_ATRSP_FAILED;
    }

    return RIL_ATRSP_FAILED; //not supported
}

static void ATRsp_QBTLEADDR_Hdlr(char* line, u32 len, void* param)
{
	char* pHead = Ql_RIL_FindString(line, len, "\r\n+QBTLEADDR:");

	if (pHead)
	{
		// +QBTADDR: 1488CD1F6261
		Ql_sscanf(line, "%*[^:]: %[^\r\n]\r\n", (char*)param);
		return	RIL_ATRSP_CONTINUE;
	}

	pHead = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
	if (pHead)
	{  
	   return  RIL_ATRSP_SUCCESS;
	}

	pHead = Ql_RIL_FindLine(line, len, "ERROR");// find <CR><LF>ERROR<CR><LF>, <CR>ERROR<CR>£¬<LF>ERROR<LF>
	if (pHead)
	{  
		return	RIL_ATRSP_FAILED;
	} 
	pHead = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
	if (pHead)
	{
		return	RIL_ATRSP_FAILED;
	}

	return RIL_ATRSP_FAILED; //not supported
}

#endif  //__OCPU_RIL_BT_SUPPORT__

