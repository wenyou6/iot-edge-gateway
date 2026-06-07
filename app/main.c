#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <wiringPi.h>
#include <sqlite3.h>
#include <pthread.h>
#include "aht20.h"
#include "relay.h"
#include "db_ops.h"
#include "mqtt.h"


#define LED_PIN        5
#define RELAY_PIN      9
#define TEMP_THRESHOLD 25.0   // 温度阈值（摄氏度）
#define READ_DELAY     2      // 读取间隔（秒）
// 全局标志位，用于控制主循环
volatile sig_atomic_t g_running = 1;
sqlite3 *db = NULL;
int relay_state = 0;  // 继电器状态：0-关闭，1-打开

// 信号处理函数
void sig_handler(int signo)
{
    if (signo == SIGINT) {
        printf("\n收到Ctrl+C信号,准备退出...\n");
        g_running = 0;
    } else if (signo == SIGPIPE) {
        // 忽略 SIGPIPE 信号，避免连接断开时程序崩溃
        printf("\n收到SIGPIPE信号(MQTT连接断开),程序继续运行...\n");
    }
}

void* sensor_thread(void *arg) {
    float tmp, hum;
    while(g_running)
    {
        digitalWrite(LED_PIN, HIGH);
        if(aht20_read_data(&tmp, &hum) == 0)
        {
            printf("Temp:%.2f℃ | Hum:%.2f%%RH\n", tmp, hum);
            // 插入数据库
            db_insert(db, tmp, hum);
            // 温度控制逻辑：超过阈值打开继电器，低于阈值关闭
            if (tmp > TEMP_THRESHOLD && relay_state == 0) {
                Relay_On();
                relay_state = 1;
                printf("温度超过%.1f℃,风扇已打开\n", TEMP_THRESHOLD);
            } else if (tmp < TEMP_THRESHOLD && relay_state == 1) {
                Relay_Off();
                relay_state = 0;
                printf("温度低于%.1f℃,风扇已关闭\n", TEMP_THRESHOLD);
            }
        }
        digitalWrite(LED_PIN, LOW);
        sleep(READ_DELAY);
    }
    return NULL;
}
void* mqtt_thread(void *arg) {
    float tmp, hum;
    
    // 初始化 MQTT 客户端
    if (mqtt_init() != 0) {
        printf("[MQTT] 初始化失败，将在循环中尝试重连\n");
    }
    
    while (g_running) {
        // 读取传感器数据
        if (aht20_read_data(&tmp, &hum) == 0) {
            // 使用带缓存的发布函数（断网时自动缓存，联网后自动补传）
            mqtt_cache_publish(tmp, hum);
        }
        sleep(READ_DELAY); // 与传感器读取间隔一致
    }

    // 清理资源
    mqtt_cleanup();
    printf("MQTT 断开连接\n");
    return NULL;
}

int main(void)
{
    pthread_t tid_sensor, tid_mqtt;

    // 注册信号处理函数
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGPIPE, sig_handler);  // 处理 SIGPIPE，避免连接断开时程序崩溃

     if (wiringPiSetup() == -1) {
        printf("初始化失败\n");
        return 1;
    }
    
    // 初始化LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // 初始化继电器
    Relay_Init(RELAY_PIN);

    if(aht20_init() != 0)
    {
        printf("AHT20 init error!\n");
        return -1;
    }

     // 初始化数据库
    db = db_init("./sensor.db");
    if (!db) {
        printf("数据库初始化失败\n");
        return 1;
    }
    
    
    printf("智能物联网边缘网关启动\n");
    printf("温度阈值: %.1f°C\n", TEMP_THRESHOLD);
    printf("读取间隔: %d秒\n", READ_DELAY);

     /* 创建线程 */
    pthread_create(&tid_sensor, NULL, sensor_thread, NULL);
    pthread_create(&tid_mqtt, NULL, mqtt_thread, NULL);

    pthread_join(tid_sensor, NULL);
    pthread_join(tid_mqtt, NULL);
    // 资源回收
    printf("资源回收中...\n");
    db_close(db);           // 关闭数据库连接
    Relay_Off();           // 关闭继电器
    digitalWrite(LED_PIN, LOW);  // 关闭LED
    aht20_deinit();        // 关闭I2C设备
    printf("程序已退出\n");

    return 0;
}