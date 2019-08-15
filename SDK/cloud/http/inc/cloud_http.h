#ifndef __CLOUD_HTTP_H__
#define __CLOUD_HTTP_H__

#include "ql_type.h"
#include "ril.h"
#include "ril_http.h"


s32 http_request_post(char* strPostMsg,u16 len);

s32 http_request_get(u32 timeout);

s32 http_set_server_url(char* strURL, u16 len);

s32 http_read_response(u32 timeout, CB_RIL_RcvDataFrmCore cb_rcvData);


#endif
