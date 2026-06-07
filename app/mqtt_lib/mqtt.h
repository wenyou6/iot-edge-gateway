#ifndef MQTT_C_H
#define MQTT_C_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

enum MQTTErrors {
    MQTT_OK = 0,
    MQTT_ERROR_CONNECTION_REFUSED = 1,
    MQTT_ERROR_SEND_FAILED = 2,
    MQTT_ERROR_RECV_FAILED = 3,
    MQTT_ERROR_NULL_CALLBACK = 4,
    MQTT_ERROR_INVALID_STATE = 5,
    MQTT_ERROR_OUT_OF_MEMORY = 6,
    MQTT_ERROR_MALFORMED_RESPONSE = 7
};

struct mqtt_client {
    void *ctx;
    ssize_t (*send)(void *ctx, const void *buf, size_t len);
    ssize_t (*recv)(void *ctx, void *buf, size_t len);
    void *(*malloc)(size_t sz);
    void (*free)(void *ptr);
    uint8_t *send_buf;
    size_t send_buf_size;
    uint8_t *recv_buf;
    size_t recv_buf_size;
    int error;
    uint16_t msg_id;
    unsigned char keep_alive;
    unsigned char connect_status;
};

struct mqtt_connect_args {
    const char *client_id;
    const char *username;
    const char *password;
    const char *will_topic;
    const char *will_message;
    unsigned int will_message_len;
    unsigned int keep_alive;
    unsigned int clean_session;
};

struct mqtt_publish_args {
    const char *topic;
    const void *payload;
    unsigned int payload_len;
    unsigned int qos;
    unsigned int retain;
};

struct mqtt_disconnect_args {
    // 预留
};

void mqtt_lib_client_init(struct mqtt_client *client, void *ctx,
                      ssize_t (*send)(void *, const void *, size_t),
                      ssize_t (*recv)(void *, void *, size_t),
                      void *(*malloc_fn)(size_t),
                      void (*free_fn)(void *));
void mqtt_lib_connect(struct mqtt_client *client, struct mqtt_connect_args *args);
void mqtt_lib_publish(struct mqtt_client *client, struct mqtt_publish_args *args);
void mqtt_lib_disconnect(struct mqtt_client *client, struct mqtt_disconnect_args *args);

#endif