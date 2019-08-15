#ifndef __MQTT_BUFFER_H__
#define __MQTT_BUFFER_H__


struct MqttExtent {
    struct MqttExtent *next;
    char *payload;   
    unsigned int len;

};



struct MqttBuffer {
    struct MqttExtent *first_ext;
    struct MqttExtent *last_ext;
    unsigned int available_bytes;

    char **allocations;
    char *first_available;
    unsigned int alloc_count;
    unsigned int alloc_max_count;
    unsigned int buffered_bytes;
};

/**
Func:Init the buffer
@param : the buf to save data packet
@return : void
**/

void MqttBuffer_Init(struct MqttBuffer *buf);

void MqttBuffer_Destroy(struct MqttBuffer *buf);
void MqttBuffer_Reset(struct MqttBuffer *buf);




/**
Func:allocate fixed header,variable header ,payload extent ,managed by the buffer
@param : the buf to save data packet ,managed extents
@param : the malloc length of allocated extent
@return : the malloc extent
**/
struct MqttExtent *MqttBuffer_AllocExtent(struct MqttBuffer *buf, unsigned int bytes);

/**
Func:append a extent to a buffer,tail append
@param : the buf to save data packet ,managed extents
@param : the append extent
@return : void
**/

void MqttBuffer_AppendExtent(struct MqttBuffer *buf, struct MqttExtent *ext);

/**
Func:append payload to a buffer
@param : the buf to save data packet ,managed extents
@param : the pointer to payload
@param : the size of payload
@param : whether to copy the payload
@return : see MQTT_ERROR_enum
**/

int MqttBuffer_Append(struct MqttBuffer *buf, char *payload, unsigned int size, int own);

#endif
