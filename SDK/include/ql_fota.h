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
 *   ql_fota.h 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   Fota API  defines.
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


#ifndef __QL_FOTA_H__
#define __QL_FOTA_H__

#include "ql_type.h"

typedef struct
{
    s16 Q_gpio_pin1;        //Watchdog GPIO pin 1, If only use one GPIO,you can set other to -1,it means invalid.
    s16 Q_feed_interval1;   //gpio1 time interval for feed dog.
    s16 Q_gpio_pin2;        //Watchdog GPIO pin 2, If only use one GPIO,you can set other to -1,it means invalid.
    s16 Q_feed_interval2;   //gpio2 time interval for feed dog.
    s32 reserved1;          //reserve 1
    s32 reserved2;          //reserve 2
}ST_FotaConfig;


/*****************************************************************
* Function:     Ql_FOTA_Init 
* 
* Description:  Initialize FOTA related functions.
*               It a simple API.Programer only need to pass the
*               simple parameters to this API.
*
* Parameters:
*               pFotaCfg: Initialize fota config include watch dog. 
* Return:        
*               QL_RET_OK indicates this function successes.
*               QL_RET_ERR_PARAM indicates parameter error.
*               Ql_RET_NOT_SUPPORT indicates not support this function.
*               Ql_RET_ERR_RAWFLASH_UNKNOW indicates unknown error.
*****************************************************************/
s32 Ql_FOTA_Init(ST_FotaConfig * pFotaCfg);


/*****************************************************************
* Function:     Ql_FOTA_WriteData 
* 
* Description:
*               FOTA write data API.
*                1. This function is used to write data to spare image pool
*                2. This API only allow sequentially writing mechanism
*                3. Authentication mechanism is executed during writing
* Parameters:
*               length: the length of writing (Unit: Bytes).recommend 512 bytes
*               buffer: point to the start address of buffer
* Return:        
*               QL_RET_OK indicates this function successes.
*               QL_RET_ERR_PARAM indicates parameter error.
*               Ql_RET_NOT_SUPPORT indicates not support this function.
*               Ql_RET_ERR_UNKOWN indicates unknown error.
*               Ql_RET_ERR_RAWFLASH_OVERRANGE indicates over flash range.
*               Ql_RET_ERR_RAWFLASH_UNIITIALIZED indicates uninitialized before write or read flash.
*               Ql_RET_ERR_RAWFLASH_UNKNOW indicates unknown error.
*               Ql_RET_ERR_RAWFLASH_INVLIDBLOCKID indicates block id invalid.
*               Ql_RET_ERR_RAWFLASH_PARAMETER indicates parameter error.
*               Ql_RET_ERR_RAWFLASH_ERASEFlASH indicates erase flash failure.
*               Ql_RET_ERR_RAWFLASH_WRITEFLASH indicates written flash failure.
*               Ql_RET_ERR_RAWFLASH_READFLASH indicates read flash failure.
*               Ql_RET_ERR_RAWFLASH_MAXLENGATH indicates the data length too large.
*****************************************************************/
s32  Ql_FOTA_WriteData(s32 length, s8* buffer);

/*****************************************************************
* Function:     Ql_FOTA_Finish 
* 
* Description:
*               FOTA finalization API.
*                1. compare calculated checksum with image checksum in the header after
*                   whole image is written
*                2. mark the status to UPDATE_NEEDED 
* Parameters:
*               None
* Return:        
*               QL_RET_OK indicates this function successes.
*               Ql_RET_NOT_SUPPORT indicates not support this function.
*               Ql_RET_ERR_UNKOWN indicates unknown error.
*               Ql_RET_ERR_RAWFLASH_OVERRANGE indicates over flash range.
*               Ql_RET_ERR_RAWFLASH_UNIITIALIZED indicates uninitialized before write or read flash.
*               Ql_RET_ERR_RAWFLASH_UNKNOW indicates unknown error.
*               Ql_RET_ERR_RAWFLASH_INVLIDBLOCKID indicates block id invalid.
*               Ql_RET_ERR_RAWFLASH_PARAMETER indicates parameter error.
*               Ql_RET_ERR_RAWFLASH_ERASEFlASH indicates erase flash failure.
*               Ql_RET_ERR_RAWFLASH_WRITEFLASH indicates written flash failure.
*               Ql_RET_ERR_RAWFLASH_READFLASH indicates read flash failure.
*               Ql_RET_ERR_RAWFLASH_MAXLENGATH indicates the data length too large.
*****************************************************************/
s32 Ql_FOTA_Finish(void);

/*****************************************************************
* Function:     Ql_FOTA_ReadData 
* 
* Description:
*               This function reads data from the data region which
*               Ql_FOTA_WriteData writes to.
*               If Developer needs to check the whole data package after
*               writing, this API can read back the data.
* Parameters:
*               offset:  the offset value to the data region
*               len:     the length to read (Unit: Byte).recommend 512 bytes
*               pBuffer: point to the buffer to store read data.
* Return:        
*               QL_RET_ERR_PARAM, indicates parameter error.
*               >0,  the real read number of bytes.
*****************************************************************/
s32 Ql_FOTA_ReadData(u32 offset, u32 len, u8* pBuffer);

/*****************************************************************
* Function:     Ql_Fota_Update 
* 
* Description:
*               Starts FOTA Update.
* Parameters:
*               None.
* Return:        
*               QL_RET_OK indicates this function successes.
*               QL_RET_ERR_INVALID_OP indicates invalid operation.
*               Ql_RET_NOT_SUPPORT indicates not support this function.
*               Ql_RET_ERR_RAWFLASH_PARAMETER indicates parameter error.
*               Ql_RET_ERR_RAWFLASH_ERASEFlASH indicates erase flash failure.
*               Ql_RET_ERR_RAWFLASH_WRITEFLASH indicates written flash failure.
*****************************************************************/
s32 Ql_FOTA_Update(void);

#endif  // End-of __QL_FOTA_H__
