#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include "aht20.h"
#include "relay.h"
#include "wiringPi.h"

#define LED_PIN        5
#define RELAY_PIN      9
#define TEMP_THRESHOLD 25.0   // 温度阈值（摄氏度）
// 全局标志位，用于控制主循环
volatile sig_atomic_t g_running = 1;

// 信号处理函数
void sig_handler(int signo)
{
    if (signo == SIGINT) {
        printf("\n收到Ctrl+C信号,准备退出...\n");
        g_running = 0;
    }
}

int main(void)
{
    float tmp, hum;

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

    // 注册信号处理函数
    signal(SIGINT, sig_handler);

    while(g_running)
    {
        digitalWrite(LED_PIN, HIGH);
        if(aht20_read_data(&tmp, &hum) == 0)
        {
            printf("Temp:%.2f℃ | Hum:%.2f%%RH\n", tmp, hum);
            
            // 温度控制逻辑：超过阈值打开继电器，低于阈值关闭
            if (tmp > TEMP_THRESHOLD) {
                Relay_On();
                printf("温度超过%.1f℃,风扇已打开\n", TEMP_THRESHOLD);
            } else if (tmp < TEMP_THRESHOLD) {
                Relay_Off();
                printf("温度低于%.1f℃,风扇已关闭\n", TEMP_THRESHOLD);
            }
        }
        digitalWrite(LED_PIN, LOW);
        sleep(2);
    }

    // 资源回收
    printf("资源回收中...\n");
    Relay_Off();           // 关闭继电器
    digitalWrite(LED_PIN, LOW);  // 关闭LED
    aht20_deinit();        // 关闭I2C设备
    printf("程序已退出\n");

    return 0;
}