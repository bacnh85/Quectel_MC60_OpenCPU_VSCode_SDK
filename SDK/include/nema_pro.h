#ifndef __NMEA_PRO_H__
#define __NMEA_PRO_H__

#include "ql_type.h"
#include "ql_stdlib.h"
#define MAX_I2C_BUF_SIZE	255
#define MAX_NMEA_LEN		255
#define IIC_DEV_ADDR        0x20


#define MSG_ID_IIC_READ_INDICATION  0x1011
#define MSG_ID_OUTPUT_INDICATION  0x1012

#define TIMERID1 100
#define INTERVAL500MS 500
#define INTERVAL5MS 5
#define NMEA_TX_MAX 2048




typedef enum 
{ 
    RXS_DAT_HBD, // receive HBD data 
    RXS_PRM_HBD2, // receive HBD preamble 2 
    RXS_PRM_HBD3, // receive HBD preamble 3 
    RXS_DAT, // receive NMEA data 
    RXS_DAT_DBG, // receive DBG data 
    RXS_ETX, // End-of-packet 
} RX_SYNC_STATE_T;

typedef struct 
{ 
    s16 inst_id; // 1 - NMEA, 2 - DBG, 3 - HBD 
    s16 dat_idx; 
    s16 dat_siz; 
}st_queue;

extern u8 rd_buf[MAX_I2C_BUF_SIZE+1];
extern u8 tx_buf[NMEA_TX_MAX];
extern s32 tx_data_len;
extern s32 tx_size;

bool iop_init_pcrx( void );
void iop_pcrx_nmea( u8 data );
void iop_get_inst(s16 idx, s16 size, void *data);
bool iop_inst_avail(s16 *inst_id, s16 *dat_idx, s16 *dat_siz) ;
bool iop_init_pcrx( void ) ;

void get_nmea(void);
void extract_nmea(void);
bool read_gps_I2C_buffer(void);
s32 IIC_ReadBytes(u32 chnnlNo,u8 slaveAddr,u8 *buf,u32 len);
s32 IIC_WriteBytyes(u32 chnnlNo,u8 slaveAddr,u8 *pdata,u32 len);

#endif

