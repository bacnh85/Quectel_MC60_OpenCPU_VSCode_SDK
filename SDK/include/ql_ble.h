/*****************************************************************************
*
*****************************************************************************/
/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ql_ble.h 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
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
#ifndef __QL_BLE_H__
#define __QL_BLE_H__
#include "ql_type.h"
typedef struct
{
    u8 bd_addr[6];
    /* rssi of remote device */
    s32 rssi;
	/* length of EIR */
    u8 eir_len;
    /* raw EIR data */
    u8 eir[32];
} st_bcm_gattc_dev;

typedef void (*CallBack_BLE_Scan_Hdlr)(st_bcm_gattc_dev *dev, u8 num, void *customizePara);
void Ql_BLE_Scan_Register(CallBack_BLE_Scan_Hdlr callback_ble_scan,void * customizePara);

#endif  //__QL_BLE_H__
