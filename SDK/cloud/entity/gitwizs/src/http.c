#include "custom_feature_def.h"
#ifdef __OCPU_SMART_CLOUD_SUPPORT__
#ifdef __GITWIZS_SOLUTION__

#include "adapter.h"
#include "http.h"
#include "gagent.h"
#include "hal_socket.h"

static u8 g_postBuf[400];
static u8 g_getBuf[200];

/******************************************************
 *
 *   FUNCTION        :   delete device did by http 
 *     
 *   return          :   0--send successful 
 *                       1--fail. 
 *
 ********************************************************/
s32 Http_Delete(const s8 *host,const s8 *did,const s8 *passcode )
{
    s32 ret=0;
    s8 *sendBuf=NULL;
    s8 *url = "/dev/devices"; 
    s8 *Content = NULL;
    s32 ContentLen=0;
    s32 totalLen=0;
    s8 *DELETE=NULL;
    s8 *HOST=NULL;
    s8 Content_Length[20]={0};
    s8 *contentType="Content-Type: application/x-www-form-urlencoded\r\n\r\n";

    DELETE = (s8*)Adapter_Mem_Alloc(Adapter_Strlen("DELETE  HTTP/1.1\r\n")+Adapter_Strlen(url)+1);//+1 for sprintf
    if( DELETE ==NULL ) 
    {
        return -1;
    }
    HOST = (s8*)Adapter_Mem_Alloc(Adapter_Strlen("Host: \r\n")+Adapter_Strlen(host)+1);// +1 for sprintf
    if( HOST==NULL)
    {
      Adapter_Mem_Free(DELETE);
      return -1;
    }
    Content = (s8*)Adapter_Mem_Alloc(Adapter_Strlen("did=&passcode=")+Adapter_Strlen(did)+Adapter_Strlen(passcode)+1);// +1 for sprintf
    if( Content==NULL )
    {
      Adapter_Mem_Free(DELETE);
      Adapter_Mem_Free(HOST);
      return -1;      
    }

    Ql_sprintf(Content,"did=%s&passcode=%s",did,passcode);
    ContentLen=Adapter_Strlen(Content); 
    Ql_sprintf(DELETE,"DELETE %s HTTP/1.1\r\n",url);
    Ql_sprintf(HOST,"Host: %s\r\n","api.gizwits.com");
    Ql_sprintf(Content_Length,"Content-Length: %d\r\n",ContentLen);
    sendBuf = (s8*)Adapter_Mem_Alloc(Adapter_Strlen(DELETE)+Adapter_Strlen(HOST)+Adapter_Strlen(Content_Length)+Adapter_Strlen(contentType)+ContentLen+1);//+1 for sprintf
    if (sendBuf==NULL) 
    {
      Adapter_Mem_Free(DELETE);
      Adapter_Mem_Free(HOST);
      Adapter_Mem_Free(Content);
      return -1;
    }
    Ql_sprintf(sendBuf,"%s%s%s%s%s",DELETE,HOST,Content_Length,contentType,Content);
    totalLen = Adapter_Strlen(sendBuf);
    ret = Adapter_HTTP_Post(sendBuf,totalLen);
    
    APP_DEBUG("totalLen = %d\r\n",totalLen);
    APP_DEBUG("%s\r\n",sendBuf);

    Adapter_Mem_Free(DELETE);
    Adapter_Mem_Free(HOST);
    Adapter_Mem_Free(Content);
    Adapter_Mem_Free(sendBuf);
    return ret;
}



s32 Http_POST(const s8 *host,const s8 *passcode,const s8 *imei,const s8 *product_key )
{
    s32 ret=0;
    s8 *url = "/dev/devices"; 
    s8 Content[100]={0};
    s32 ContentLen=0;
    s32 totalLen=0;
    Adapter_Memset(g_postBuf,0,400);

    APP_DEBUG("passcode: %s\r\n",(char *)passcode); 
    APP_DEBUG("imei: %s\r\n",imei); 
    APP_DEBUG("product_key: %s\r\n",product_key); 




    Ql_sprintf(Content,"passcode=%s&mac=%s&product_key=%s",passcode,imei,product_key);
    ContentLen=Adapter_Strlen(Content);
    
    APP_DEBUG("ContentLen: %d\r\n",ContentLen);    


   Ql_snprintf( (char *)g_postBuf,400,"%s %s %s%s%s %s%s%s%d%s%s%s%s%s",
              "POST" ,url,"HTTP/1.1",kCRLFNewLine,
              "Host:","api.gizwits.com",kCRLFNewLine,
              "Content-Length: ",ContentLen,kCRLFNewLine,
              "Content-Type: application/x-www-form-urlencoded",kCRLFNewLine,
              kCRLFNewLine,
              Content
        );
    totalLen = Adapter_Strlen( (char *)g_postBuf );
    APP_DEBUG("http_post:%s %d\r\n",g_postBuf,totalLen);    
    ret = Adapter_HTTP_Post(g_postBuf,totalLen );
    APP_DEBUG("http_post ret: %d\r\n",ret);    

    return ret;
}



/******************************************************
 *
 *   Http_recevieBuf :   http receive data
 *   return          :   http response code
 *
 ********************************************************/
s32 Http_Response_Code( u8 *Http_recevieBuf )
{
    s32 response_code=0;
    s8* p_start = NULL;
    s8* p_end =NULL; 
    s8 re_code[10] ={0};
    Adapter_Memset(re_code,0,sizeof(re_code));

    p_start = Adapter_StrStr( (char *)Http_recevieBuf,"\r\n" );
    if(NULL == p_start)
    {
        return RET_FAILED;
    }
    p_end = Adapter_StrStr( p_start+2,"\r\n" );
    if(p_end)
    {
        if(p_end - p_start > sizeof(re_code))
        {
            return RET_FAILED;
        }
        Adapter_Memcpy( re_code,p_start,(p_end-p_start) );
    }
    
    response_code = Adapter_Atoi(re_code); 
    return response_code;
}


s32 Http_Response_DID( u8 *Http_recevieBuf,s8 *DID )
{
    s8 *p_start = NULL;
    Adapter_Memset(DID,0,DID_LEN);
    p_start = Adapter_StrStr( (char *)Http_recevieBuf,"did=");
    if( p_start==NULL )
    {
        return -1;
    }
    p_start = p_start+Adapter_Strlen("did=");
    Adapter_Memcpy(DID,p_start,DID_LEN);
    DID[DID_LEN - 2] ='\0';             
    return 0;    
}


s32 Http_GET( const s8 *host,const s8 *did )
{

    s32 totalLen=0;
    s32 ret=0;
    s8 *url = "/dev/devices/";

    Adapter_Memset( g_getBuf,0,200 );
    Ql_snprintf( g_getBuf,200,"%s%s%s",
              HTTP_SERVER,url,did);
    totalLen =Adapter_Strlen( g_getBuf );
    ret = Adapter_HTTP_Get(g_getBuf,GAGENT_HTTP_TIMEOUT);
    APP_DEBUG("Sent provision:\n %s\n", g_getBuf);

     return ret;  
}


s32 Http_getdomain_port( u8 *Http_recevieBuf,s8 *domain,s32 *port )
{
    s8 *p_start = NULL;
    s8 *p_end =NULL;
    s8 Temp_port[10]={0};
    Adapter_Memset( domain,0,100 );
    p_start = Adapter_StrStr( (char *)Http_recevieBuf,"host=");
    if( p_start==NULL ) return -1;
    p_start = p_start+Adapter_Strlen("host=");
    p_end = Adapter_StrStr(p_start,"&");
    if( p_end==NULL )   return -1;
    Adapter_Memcpy( domain,p_start,( p_end-p_start ));
    domain[p_end-p_start] = '\0';
    p_start = Adapter_StrStr((++p_end),"port=");
    if( p_start==NULL ) return -1;
    p_start =p_start+Adapter_Strlen("port=");
    p_end = Adapter_StrStr( p_start,"&" ); 
    Adapter_Memcpy(Temp_port,p_start,( p_end-p_start));
    *port = Adapter_Atoi(Temp_port);
    return 0;
}





#endif
#endif
