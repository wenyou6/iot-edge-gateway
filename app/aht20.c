#include "aht20.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

static int i2c_fd = -1;

/**
 * @brief  初始化AHT20温湿度传感器
 * @details 完成I2C设备打开、从机地址设置和传感器初始化操作
 * @return 0 - 初始化成功, -1 - 初始化失败
 */
int aht20_init(void)
{
    // 打开I2C设备文件，以读写方式访问
    i2c_fd = open(I2C_DEV_PATH, O_RDWR);
    if(i2c_fd < 0)
    {
        perror("open /dev/i2c-3 failed");
        return -1;
    }
    
    // 设置I2C从机地址为AHT20的地址
    if(ioctl(i2c_fd, I2C_SLAVE, AHT20_ADDR) < 0)
    {
        perror("ioctl set slave addr fail");
        close(i2c_fd);  // 关闭已打开的设备
        i2c_fd = -1;
        return -1;
    }
    
    // AHT20初始化命令：0xBE为初始化指令，0x08为校准使能
    uint8_t cmd[2] = {0xBE, 0x08};
    write(i2c_fd, cmd, sizeof(cmd));
    
    // 等待传感器完成初始化（至少等待100ms）
    usleep(100000);
    
    return 0;
}

/**
 * @brief  反初始化AHT20温湿度传感器
 * @details 关闭I2C设备文件描述符，释放资源
 */
void aht20_deinit(void)
{
    if(i2c_fd >= 0)
    {
        close(i2c_fd);
        i2c_fd = -1;
    }
}

/**
 * @brief  读取AHT20温湿度传感器数据
 * @details 发送测量命令，读取传感器返回的原始数据，并转换为实际温度和湿度值
 * @param[out] temp 温度指针，用于存储读取的温度值（单位：℃）
 * @param[out] humi 湿度指针，用于存储读取的湿度值（单位：%RH）
 * @return 0 - 读取成功, -1 - 读取失败（传感器未初始化）
 */
int aht20_read_data(float *temp, float *humi)
{
    // 检查传感器是否已初始化
    if(i2c_fd < 0) return -1;

    // 发送测量命令：0xAC为触发测量指令，0x33和0x00为参数
    uint8_t cmd[3] = {0xAC, 0x33, 0x00};
    write(i2c_fd, cmd, sizeof(cmd));
    
    // 等待测量完成（AHT20测量时间最大约80ms）
    usleep(80000);

    // 读取6字节传感器数据
    uint8_t buf[6] = {0};
    read(i2c_fd, buf, sizeof(buf));

    // 解析原始湿度数据：buf[1]<<12 | buf[2]<<4 | buf[3]>>4
    // AHT20湿度原始数据为20位，存储在buf[1]、buf[2]和buf[3]的高4位
    uint32_t raw_h = ((buf[1] << 12) | (buf[2] << 4) | (buf[3] >> 4));
    
    // 解析原始温度数据：(buf[3]&0x0F)<<16 | buf[4]<<8 | buf[5]
    // AHT20温度原始数据为20位，存储在buf[3]的低4位、buf[4]和buf[5]
    uint32_t raw_t = (((buf[3] & 0x0F) << 16) | (buf[4] << 8) | buf[5]);

    // 将原始数据转换为实际湿度值（范围：0% ~ 100%）
    // 计算公式：湿度 = 原始湿度值 * 100 / 2^20
    *humi = raw_h * 100.0f / 1048576.0f;
    
    // 将原始数据转换为实际温度值（范围：-40℃ ~ 85℃）
    // 计算公式：温度 = 原始温度值 * 200 / 2^20 - 50
    *temp = raw_t * 200.0f / 1048576.0f - 50.0f;

    return 0;
}