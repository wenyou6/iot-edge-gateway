#ifndef __AHT20_H
#define __AHT20_H

#include <stdint.h>

#define AHT20_ADDR    0x38
#define I2C_DEV_PATH  "/dev/i2c-3"

/* 初始化AHT20，打开I2C-3 */
int aht20_init(void);
/* 获取温湿度，返回0成功 */
int aht20_read_data(float *temp, float *humi);
/* 反初始化AHT20，关闭I2C设备 */
void aht20_deinit(void);

#endif