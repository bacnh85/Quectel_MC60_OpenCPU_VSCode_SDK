#ifndef __HTTP_H__
#define __HTTP_H__


#include "gagent_typedef.h"


#define kCRLFNewLine     "\r\n"
#define kCRLFLineEnding  "\r\n\r\n"



s32 Http_POST(const s8 *host,const s8 *passcode,const s8 *imei,const s8 *product_key );

/******************************************************
 *
 *   Http_recevieBuf :   http receive data
 *   return          :   http response code
 *
 ********************************************************/
s32 Http_Response_Code( u8 *Http_recevieBuf );


s32 Http_Response_DID( u8 *Http_recevieBuf,s8 *DID );

s32 Http_GET( const s8 *host,const s8 *did );

s32 Http_getdomain_port( u8 *Http_recevieBuf,s8 *domain,s32 *port );


/******************************************************
 *
 *   FUNCTION        :   delete device did by http 
 *     
 *   return          :   0--send successful 
 *                       1--fail. 
 *
 ********************************************************/
s32 Http_Delete(const s8 *host,const s8 *did,const s8 *passcode );



#endif
