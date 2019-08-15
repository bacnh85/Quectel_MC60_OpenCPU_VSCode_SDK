#include "custom_feature_def.h"
#ifdef __OCPU_SMART_CLOUD_SUPPORT__


#include "cloud_http.h"


s32 http_set_server_url(char* strURL, u16 len)
{
    return RIL_HTTP_SetServerURL(strURL, len);
}



s32 http_request_post(char* strPostMsg,u16 len)
{
     return RIL_HTTP_RequestToPost(strPostMsg,len);
}


s32 http_request_get(u32 timeout)
{
     return RIL_HTTP_RequestToGet(timeout);
}


s32 http_read_response(u32 timeout, CB_RIL_RcvDataFrmCore cb_rcvData)
{
    return RIL_HTTP_ReadResponse(timeout,cb_rcvData);
}



#endif

