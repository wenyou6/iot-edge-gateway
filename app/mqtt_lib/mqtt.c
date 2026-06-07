#include "mqtt.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MQTT_CONNECT      0x10
#define MQTT_PUBLISH      0x30
#define MQTT_DISCONNECT   0xE0

void mqtt_lib_client_init(struct mqtt_client *client, void *ctx,
                      ssize_t (*send_fn)(void *, const void *, size_t),
                      ssize_t (*recv_fn)(void *, void *, size_t),
                      void *(*malloc_fn)(size_t),
                      void (*free_fn)(void *)) {
    memset(client, 0, sizeof(*client));
    client->ctx = ctx;
    client->send = send_fn;
    client->recv = recv_fn;
    client->malloc = malloc_fn ? malloc_fn : malloc;
    client->free = free_fn ? free_fn : free;
    client->send_buf_size = 2048;
    client->recv_buf_size = 1024;
    client->send_buf = client->malloc(client->send_buf_size);
    client->recv_buf = client->malloc(client->recv_buf_size);
    client->msg_id = 1;
}

static int _mqtt_send(struct mqtt_client *c, uint8_t *buf, size_t len) {
    if (c->send(c->ctx, buf, len) < 0) {
        c->error = MQTT_ERROR_SEND_FAILED;
        return -1;
    }
    return 0;
}

void mqtt_lib_connect(struct mqtt_client *client, struct mqtt_connect_args *args) {
    uint8_t *p = client->send_buf;
    uint8_t *start = p;
    *p++ = MQTT_CONNECT;
    // 剩余长度占位
    p++;
    // 协议名
    *p++ = 0x00; *p++ = 0x04;
    *p++ = 'M'; *p++ = 'Q'; *p++ = 'T'; *p++ = 'T';
    // 协议级别
    *p++ = 0x04;
    // 连接标志
    uint8_t flags = 0x02; // clean session
    if (args->username) flags |= 0x80;
    if (args->password) flags |= 0x40;
    *p++ = flags;
    // keep alive
    *p++ = (args->keep_alive >> 8) & 0xFF;
    *p++ = args->keep_alive & 0xFF;
    // client id
    size_t id_len = args->client_id ? strlen(args->client_id) : 0;
    *p++ = (id_len >> 8) & 0xFF;
    *p++ = id_len & 0xFF;
    if (id_len) {
        memcpy(p, args->client_id, id_len);
        p += id_len;
    }
    // 计算剩余长度
    size_t remaining = (p - start) - 2;
    start[1] = remaining & 0x7F;
    
    printf("[MQTT] 发送 CONNECT 报文，长度: %d\n", (int)(p - start));
    
    // 发送 CONNECT 报文
    if (_mqtt_send(client, start, p - start) != 0) {
        printf("[MQTT] 发送 CONNECT 失败\n");
        return;
    }
    
    printf("[MQTT] 等待 CONNACK...\n");
    
    // 读 CONNACK（带超时处理）
    ssize_t recv_len = client->recv(client->ctx, client->recv_buf, 4);
    if (recv_len <= 0) {
        printf("[MQTT] 接收 CONNACK 失败，长度: %d\n", (int)recv_len);
        client->error = MQTT_ERROR_RECV_FAILED;
        return;
    }
    
    printf("[MQTT] 收到 CONNACK: 0x%02X 0x%02X 0x%02X 0x%02X\n", 
           client->recv_buf[0], client->recv_buf[1], 
           client->recv_buf[2], client->recv_buf[3]);
    
    client->connect_status = client->recv_buf[3];
    if (client->connect_status != 0) {
        printf("[MQTT] Broker 拒绝连接，返回码: %d\n", client->connect_status);
        client->error = MQTT_ERROR_CONNECTION_REFUSED;
    }
}

void mqtt_lib_publish(struct mqtt_client *client, struct mqtt_publish_args *args) {
    uint8_t *p = client->send_buf;
    uint8_t *start = p;
    *p++ = MQTT_PUBLISH;
    size_t topic_len = strlen(args->topic);
    size_t remaining = 2 + topic_len + args->payload_len;
    // 剩余长度（简单编码，假定<128字节）
    *p++ = remaining & 0x7F;
    // topic
    *p++ = (topic_len >> 8) & 0xFF;
    *p++ = topic_len & 0xFF;
    memcpy(p, args->topic, topic_len);
    p += topic_len;
    // payload
    memcpy(p, args->payload, args->payload_len);
    p += args->payload_len;
    _mqtt_send(client, start, p - start);
}

void mqtt_lib_disconnect(struct mqtt_client *client, struct mqtt_disconnect_args *args) {
    (void)args;
    uint8_t buf[2] = {MQTT_DISCONNECT, 0x00};
    _mqtt_send(client, buf, 2);
    if (client->send_buf) { client->free(client->send_buf); client->send_buf = NULL; }
    if (client->recv_buf) { client->free(client->recv_buf); client->recv_buf = NULL; }
}