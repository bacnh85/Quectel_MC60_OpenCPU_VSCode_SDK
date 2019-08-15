#ifndef __UTILS_H__
#define __UTILS_H__

#include "gagent_typedef.h"

void make_rand(s8* data);

/***************************************************
FunctionName    :   resetPacket
Description     :   let ppacket phead ppayload pend pointer 
                    in satrt status.
return          :   void.
***************************************************/
void resetPacket( ppacket pbuf );

/***************************************************
        FunctionName    :   EndianConvertLToB
        Description     :   little endian to big endian convert
        return          :   the converted result.
***************************************************/
u16 EndianConvertLToB(u16 InputNum) ;

/***************************************************
FunctionName    :   SetPacketType
Description     :   set addtype to packet type.
currentType     :   current packet msg type
type            :   type
flag            :   0 reset type. 1 set type.
return          :   the type afet add newtype
***************************************************/
s32 SetPacketType( s32 currentType,s32 type,s8 flag );


/***************************************************
        FunctionName    :   ParsePacket.
        Description     :   set the source phead ppayload
                            pend.
        pbug            :   data source struct.
        return          :   0 ok other fail.
***************************************************/
u32 ParsePacket( ppacket pRxBuf );


s8 dealPacket( pgcontext pgc, ppacket pTxBuf );


s8 isPacketTypeSet( s32 currentType,s32 type );


/*************************************************
*
*       FUNCTION : transtion u16 to 2 byte for mqtt remainlen
*       remainLen: src data to transtion
*       return   : varc
***************************************************/
varc Tran2varc(u32 remainLen);



/***************************************************
FunctionName    :   EndianConvertLToB
Description     :   little endian to big endian convert
return          :   the converted result.
***************************************************/
u16 EndianConvertLToB_Long(u32 InputNum) ;


void setChannelAttrs(pgcontext pgc, stCloudAttrs_t *cloudClient,u8 isBroadCast);



#endif
