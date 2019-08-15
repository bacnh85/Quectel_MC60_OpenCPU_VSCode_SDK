#include "custom_feature_def.h"
#ifdef __OCPU_SMART_CLOUD_SUPPORT__


#include "mqttbuffer.h"
#include "ql_type.h"
#include "ql_memory.h"
#include "ql_stdlib.h"
#include "mqttlib_ext.h"
#include "ql_trace.h"

static const unsigned int MQTT_MIN_EXTENT_SIZE = 1024;

/**
Func:Init the buffer
@param : the buf to save data packet
@return : void
**/

void MqttBuffer_Init(struct MqttBuffer *buf)
{
    buf->first_ext = NULL;
    buf->last_ext = NULL;
    buf->available_bytes = 0;
    buf->allocations = NULL;
    buf->alloc_count = 0;
    buf->alloc_max_count = 0;
    buf->first_available = NULL;
    buf->buffered_bytes = 0;
}

void MqttBuffer_Destroy(struct MqttBuffer *buf)
{
    MqttBuffer_Reset(buf);
}

void MqttBuffer_Reset(struct MqttBuffer *buf)
{
    unsigned int i;
    for(i = 0; i < buf->alloc_count; ++i) {
        Ql_MEM_Free(buf->allocations[i]);
    }

    MqttBuffer_Init(buf);
}



/**
Func:allocate fixed header,variable header ,payload extent ,managed by the buffer
@param : the buf to save data packet ,managed extents
@param : the malloc length of allocated extent
@return : the malloc extent
**/
struct MqttExtent *MqttBuffer_AllocExtent(struct MqttBuffer *buf, unsigned int bytes)
{
    struct MqttExtent *ext;


    bytes += sizeof(struct MqttExtent);

    if(buf->available_bytes < bytes) {
        unsigned int alloc_bytes;
        char *chunk;

        if(buf->alloc_count == buf->alloc_max_count) {
            unsigned int max_count = buf->alloc_max_count * 2 + 1;
            char **tmp = (char**)Ql_MEM_Alloc(max_count * sizeof(char**));
            if(NULL == tmp) {
                return NULL;
            }

            Ql_memset(tmp, 0, max_count * sizeof(char**));
            Ql_memcpy(tmp, buf->allocations, buf->alloc_max_count);
            Ql_MEM_Free(buf->allocations);

            buf->alloc_max_count = max_count;
            buf->allocations = tmp;
        }

        alloc_bytes = bytes < MQTT_MIN_EXTENT_SIZE ? MQTT_MIN_EXTENT_SIZE : bytes;
        chunk = (char*)Ql_MEM_Alloc(alloc_bytes);
        if(NULL == chunk) {
            return NULL;
        }

        buf->alloc_count += 1;
        buf->allocations[buf->alloc_count - 1] = chunk;
        buf->available_bytes = alloc_bytes;
        buf->first_available = chunk;
    }

    if(buf->available_bytes < bytes || buf->alloc_count <= 0)
    {
        return NULL;
    }
 #if 0
    if(((unsigned int)(buf->first_available)&& 0xff) != 0)
    {
        buf->first_available = (char *)(((unsigned int)buf->first_available + 3)&~2);
    }
#endif
    if(((unsigned int)(buf->first_available)%2) != 0)
    {
       buf->first_available +=3;
    }

    ext = (struct MqttExtent*)(buf->first_available);
    ext->len = bytes - sizeof(struct MqttExtent);
    ext->payload = buf->first_available + sizeof(struct MqttExtent);
    ext->next = NULL;
    
    buf->first_available += bytes;

    buf->available_bytes -= bytes;

    return ext;
}

/**
Func:append a extent to a buffer,tail append
@param : the buf to save data packet ,managed extents
@param : the append extent
@return : void
**/

void MqttBuffer_AppendExtent(struct MqttBuffer *buf, struct MqttExtent *ext)
{
   struct MqttExtent *cursor;
       int i;
   
    ext->next = NULL;
    if(NULL != buf->last_ext) {
         buf->last_ext->next = ext;
         
         buf->last_ext = ext;
         
    }
    else {
        if(NULL != buf->first_ext || buf->alloc_count < 1)
        {
            return;
        }
       
        buf->first_ext = ext;
        buf->last_ext = ext;
    }

    buf->buffered_bytes += ext->len;
   
    
}

/**
Func:append payload to a buffer
@param : the buf to save data packet ,managed extents
@param : the pointer to payload
@param : the size of payload
@param : whether to copy the payload
@return : see MQTT_ERROR_enum
**/

int MqttBuffer_Append(struct MqttBuffer *buf, char *payload, unsigned int size, int own)
{
    const unsigned int bytes = own ? size : sizeof(struct MqttExtent);

    struct MqttExtent *ext = MqttBuffer_AllocExtent(buf, bytes);
    if(NULL == ext) {
        return MQTTERR_OUTOFMEMORY;
    }

    if(own) {
        ext->payload = ((char*)ext) + sizeof(struct MqttExtent);
        Ql_memcpy(ext->payload, payload, size);
    }
    else {
        ext->payload = payload;
        ext->len = size;
    }

    MqttBuffer_AppendExtent(buf, ext);
    return MQTTERR_NOERROR;
}


#endif
