#ifndef RELAY_H
#define RELAY_H

#include <stdint.h>

// 继电器操作函数声明
void Relay_Init(uint8_t pin);
void Relay_On(void);
void Relay_Off(void);
void Relay_Toggle(void);
int Relay_GetState(void);

#endif // RELAY_H
