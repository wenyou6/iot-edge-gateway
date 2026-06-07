#include "mqtt.h"
#include "mqtt_lib/mqtt.h"  // MQTT-C 库放在 mqtt_lib 子目录下
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <time.h>

/* 注意：如果 mqtt.h 直接放在当前目录，则 include "mqtt_c/mqtt.h" 改为 include "mqtt.h" */
/* 以下按 MQTT-C 开源库路径调整，这里假设我们把它放在子目录 mqtt_c 里避免与我们的 mqtt.h 冲突 */
// 如果你的项目结构是 mqtt.h(我们的) 和 mqtt_c/mqtt.h(MQTT-C的)，请相应调整

static struct mqtt_client client;
static int sockfd = -1;

// 断网缓存实现
static mqtt_cache_item_t cache[MQTT_CACHE_SIZE];
static int cache_head = 0;
static int cache_tail = 0;
static int cache_count = 0;

// 检查缓存是否为空
static int cache_is_empty() {
    return cache_count == 0;
}

// 检查缓存是否已满
static int cache_is_full() {
    return cache_count >= MQTT_CACHE_SIZE;
}

// 添加数据到缓存
static void cache_add(float temperature, float humidity) {
    if (cache_is_full()) {
        // 缓存满了，覆盖最老的数据
        printf("[MQTT] 缓存已满，覆盖最老数据\n");
        cache_head = (cache_head + 1) % MQTT_CACHE_SIZE;
        cache_count--;
    }
    
    cache[cache_tail].temperature = temperature;
    cache[cache_tail].humidity = humidity;
    cache[cache_tail].timestamp = time(NULL);
    cache_tail = (cache_tail + 1) % MQTT_CACHE_SIZE;
    cache_count++;
    
    printf("[MQTT] 数据已缓存，当前缓存数: %d\n", cache_count);
}

// 从缓存获取一条数据
static int cache_get(mqtt_cache_item_t *item) {
    if (cache_is_empty()) {
        return -1;
    }
    
    *item = cache[cache_head];
    cache_head = (cache_head + 1) % MQTT_CACHE_SIZE;
    cache_count--;
    
    return 0;
}

// 补传缓存数据
static void cache_replay() {
    if (cache_is_empty()) {
        return;
    }
    
    printf("[MQTT] 开始补传缓存数据，待补传: %d 条\n", cache_count);
    
    mqtt_cache_item_t item;
    while (!cache_is_empty()) {
        if (cache_get(&item) == 0) {
            mqtt_publish(item.temperature, item.humidity);
            // 检查是否再次断开
            if (sockfd < 0) {
                printf("[MQTT] 补传中断，连接再次断开\n");
                // 将当前数据重新放回缓存（放到队首）
                cache_head = (cache_head - 1 + MQTT_CACHE_SIZE) % MQTT_CACHE_SIZE;
                cache[cache_head] = item;
                cache_count++;
                break;
            }
            usleep(100000); // 100ms 间隔，避免过快
        }
    }
    
    printf("[MQTT] 补传完成\n");
}

/* 网络发送回调 */
static ssize_t socket_send(void* ctx, const void* buf, size_t len) {
    (void)ctx;
    return send(sockfd, buf, len, 0);
}

/* 网络接收回调 */
static ssize_t socket_recv(void* ctx, void* buf, size_t len) {
    (void)ctx;
    return recv(sockfd, buf, len, 0);
}

int mqtt_init() {
    /* 创建 TCP socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    /* 连接 MQTT Broker */
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(MQTT_PORT);
    inet_pton(AF_INET, MQTT_BROKER, &addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("MQTT 连接失败");
        close(sockfd);
        sockfd = -1;
        return -1;
    }

    /* 初始化 MQTT 客户端 */
    mqtt_lib_client_init(&client, &sockfd,
                     socket_send, socket_recv,
                     NULL, NULL);   /* 不需要自定义内存分配 */

    /* 发送 CONNECT 报文 */
    struct mqtt_connect_args conn_args = {
        .client_id = "orange_gateway",
        .keep_alive = 60,
        .clean_session = 1
    };
    mqtt_lib_connect(&client, &conn_args);

    /* 检查连接成功 */
    if (client.error != MQTT_OK) {
        printf("[MQTT] 连接失败，错误码: %d\n", client.error);
        close(sockfd);
        sockfd = -1;
        return -1;
    } else {
        printf("[MQTT] 连接成功\n");
        return 0;
    }
}

void mqtt_publish(float temperature, float humidity) {
    if (sockfd < 0) {
        printf("[MQTT] 未连接\n");
        return;
    }

    char payload[128];
    snprintf(payload, sizeof(payload),
             "{\"temperature\":%.1f, \"humidity\":%.1f}",
             temperature, humidity);

    struct mqtt_publish_args pub = {
        .topic = MQTT_TOPIC,
        .payload = (uint8_t*)payload,
        .payload_len = strlen(payload),
        .qos = 0,
        .retain = 0
    };
    mqtt_lib_publish(&client, &pub);

    /* 检查发送是否成功 */
    if (client.error != MQTT_OK) {
        printf("[MQTT] 上报失败，错误码: %d\n", client.error);
        close(sockfd);
        sockfd = -1;
    } else {
        printf("[MQTT] 已上报: %s\n", payload);
    }
}

// 带缓存的发布函数
void mqtt_cache_publish(float temperature, float humidity) {
    // 如果已连接，直接发布
    if (sockfd >= 0) {
        mqtt_publish(temperature, humidity);
        
        // 如果发送失败，保存到缓存
        if (sockfd < 0) {
            cache_add(temperature, humidity);
        }
        return;
    }
    
    // 未连接，保存到缓存
    cache_add(temperature, humidity);
    
    // 尝试重连
    printf("[MQTT] 尝试重新连接...\n");
    if (mqtt_init() == 0) {
        // 重连成功，开始补传
        cache_replay();
    }
}

void mqtt_cleanup() {
    if (sockfd >= 0) {
        struct mqtt_disconnect_args disc;
        mqtt_lib_disconnect(&client, &disc);
        close(sockfd);
        sockfd = -1;
    }
    
    // 打印剩余缓存数据
    if (cache_count > 0) {
        printf("[MQTT] 程序退出，仍有 %d 条数据未发送\n", cache_count);
    }
}