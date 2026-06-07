
# IoT Edge Gateway - 智能物联网边缘网关

基于香橙派（Orange Pi）的物联网边缘网关项目，实现温湿度数据采集、本地存储、MQTT 云端上传以及智能温度控制功能。

## 功能特性

- **温湿度采集**：使用 AHT20 传感器实时读取环境温湿度数据
- **本地存储**：使用 SQLite 数据库持久化存储传感器数据
- **MQTT 通信**：通过 MQTT 协议将数据上传到云端平台
- **断网缓存**：支持断网时数据自动缓存，联网后自动补传
- **智能控制**：基于温度阈值自动控制继电器（风扇）开关
- **多线程设计**：传感器采集和 MQTT 上传独立运行

## 技术栈

- **硬件平台**：OrangePi Zero 3（ARM64）
- **操作系统**：Linux
- **编程语言**：C
- **数据库**：SQLite3
- **通信协议**：MQTT
- **GPIO控制**：WiringPi

## 硬件连接

| 设备 | 引脚 | 说明 |
|------|------|------|
| AHT20 SDA | SDA | I2C 数据引脚 |
| AHT20 SCL | SCL | I2C 时钟引脚 |
| LED | GPIO5 | 状态指示灯 |
| 继电器控制 | GPIO9 | 风扇控制信号 |

## 编译与部署

### 环境要求

- 交叉编译器：`aarch64-linux-gnu-gcc`
- Orange Pi 开发板（ARM64架构）

### 编译命令

```bash
cd app
make
```

### 部署到香橙派

```bash
make deploy
```

### 远程运行

```bash
make run
```

### 清理编译产物

```bash
make clean
```

## 配置说明

在 `app/Makefile` 中配置部署信息：

```makefile
# 香橙派部署信息
PI_IP = 10.245.xxx.xxx    # 香橙派 IP 地址
PI_USER = orangepi        # 用户名
PI_PATH = /home/orangepi/projects/iot-edge-gateway  # 部署路径
```

在 `app/main.c` 中配置运行参数：

```c
#define LED_PIN        5       // LED 引脚
#define RELAY_PIN      9       // 继电器引脚
#define TEMP_THRESHOLD 25.0   // 温度阈值（摄氏度）
#define READ_DELAY     2       // 读取间隔（秒）
```

## 运行效果
智能物联网边缘网关启动
温度阈值: 25.0°C
读取间隔: 2秒
[MQTT] 发送 CONNECT 报文，长度: 28
[MQTT] 等待 CONNACK...
[MQTT] 收到 CONNACK: 0x20 0x02 0x00 0x00
[MQTT] 连接成功
Temp:25.38℃ | Hum:70.68%RH
[MQTT] 已上报: {"temperature":25.4, "humidity":70.7}
数据插入成功！温度：25.38℃，湿度：70.68%
温度超过25.0℃,风扇已打开
[MQTT] 已上报: {"temperature":25.4, "humidity":70.6}
Temp:25.38℃ | Hum:70.63%RH
数据插入成功！温度：25.38℃，湿度：70.63%
[MQTT] 已上报: {"temperature":25.4, "humidity":70.6}
Temp:25.33℃ | Hum:70.35%RH
数据插入成功！温度：25.33℃，湿度：70.35%

收到SIGPIPE信号(MQTT连接断开),程序继续运行...
[MQTT] 上报失败，错误码: 2
[MQTT] 数据已缓存，当前缓存数: 1
Temp:25.35℃ | Hum:70.29%RH
数据插入成功！温度：25.35℃，湿度：70.29%
[MQTT] 数据已缓存，当前缓存数: 2
[MQTT] 尝试重新连接...
MQTT 连接失败: Connection refused
Temp:25.32℃ | Hum:70.31%RH
数据插入成功！温度：25.32℃，湿度：70.31%
[MQTT] 数据已缓存，当前缓存数: 3
[MQTT] 尝试重新连接...
MQTT 连接失败: Connection refused
...
Temp:25.25℃ | Hum:70.20%RH
数据插入成功！温度：25.25℃，湿度：70.20%
[MQTT] 数据已缓存，当前缓存数: 13
[MQTT] 尝试重新连接...
[MQTT] 发送 CONNECT 报文，长度: 28
[MQTT] 等待 CONNACK...
[MQTT] 收到 CONNACK: 0x20 0x02 0x00 0x00
[MQTT] 连接成功
[MQTT] 开始补传缓存数据，待补传: 13 条
[MQTT] 已上报: {"temperature":25.3, "humidity":70.4}
[MQTT] 已上报: {"temperature":25.3, "humidity":70.4}
[MQTT] 已上报: {"temperature":25.3, "humidity":70.3}
Temp:25.26℃ | Hum:70.22%RH
数据插入成功！温度：25.26℃，湿度：70.22%
[MQTT] 已上报: {"temperature":25.3, "humidity":70.4}
[MQTT] 已上报: {"temperature":25.3, "humidity":70.5}
[MQTT] 已上报: {"temperature":25.3, "humidity":70.4}
[MQTT] 已上报: {"temperature":25.3, "humidity":70.4}
[MQTT] 已上报: {"temperature":25.3, "humidity":70.4}
[MQTT] 已上报: {"temperature":25.3, "humidity":70.4}
[MQTT] 已上报: {"temperature":25.3, "humidity":70.3}
[MQTT] 已上报: {"temperature":25.3, "humidity":70.3}
[MQTT] 已上报: {"temperature":25.3, "humidity":70.2}
[MQTT] 已上报: {"temperature":25.2, "humidity":70.2}
[MQTT] 补传完成
Temp:25.23℃ | Hum:70.16%RH
数据插入成功！温度：25.23℃，湿度：70.16%
[MQTT] 已上报: {"temperature":25.2, "humidity":70.2}

## 控制逻辑

- 当温度 **高于** 阈值时，自动打开继电器（风扇）
- 当温度 **低于** 阈值时，自动关闭继电器（风扇）
