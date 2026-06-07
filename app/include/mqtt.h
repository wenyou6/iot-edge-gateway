#ifndef MQTT_H
#define MQTT_H

#define MQTT_BROKER "10.245.140.187"  // 你的Ubuntu虚拟机IP
#define MQTT_PORT   1883
#define MQTT_TOPIC  "sensor/data"

// 断网缓存配置
#define MQTT_CACHE_SIZE 100  // 最大缓存记录数

// 缓存数据结构
typedef struct {
    float temperature;
    float humidity;
    unsigned long timestamp;
} mqtt_cache_item_t;

int mqtt_init();
void mqtt_publish(float temperature, float humidity);
void mqtt_cleanup();
void mqtt_cache_publish(float temperature, float humidity);  // 带缓存的发布函数

#endif