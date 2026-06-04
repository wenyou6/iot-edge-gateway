#include "relay.h"
#include <wiringPi.h>

static uint8_t relay_pin;
static int relay_state = 0;

// 继电器初始化
void Relay_Init(uint8_t pin) {
    relay_pin = pin;
    pinMode(relay_pin, OUTPUT);
    digitalWrite(relay_pin, LOW); // 默认关闭
    relay_state = 0;
}

// 打开继电器
void Relay_On(void) {
    digitalWrite(relay_pin, HIGH);
    relay_state = 1;
}

// 关闭继电器
void Relay_Off(void) {
    digitalWrite(relay_pin, LOW);
    relay_state = 0;
}

// 切换继电器状态
void Relay_Toggle(void) {
    if (relay_state) {
        Relay_Off();
    } else {
        Relay_On();
    }
}

// 获取继电器状态
int Relay_GetState(void) {
    return relay_state;
}
