# 交叉编译器
CC = aarch64-linux-gnu-gcc
CFLAGS = -Wall -g

# 头文件和库文件路径
INC = -I./include
LIB_PATH = -L./lib
LIBS = -lwiringPi -lpthread

# 项目配置
TARGET = main
SRC = main.c aht20.c relay.c 

# 香橙派部署信息
PI_IP = 10.245.140.77
PI_USER = orangepi
PI_PATH = /home/orangepi/projects/iot-edge-gateway
# 默认目标：编译
all: $(TARGET)
# 编译目标
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(INC) $(LIB_PATH) -o $(TARGET) $(SRC) $(LIBS)
# 部署到香橙派
deploy: $(TARGET)
	scp $(TARGET) $(PI_USER)@$(PI_IP):$(PI_PATH)
# 远程运行（GPIO需要root权限）
run: deploy
	ssh $(PI_USER)@$(PI_IP) "cd $(PI_PATH) && chmod +x $(TARGET) "
	rm -f $(TARGET)
# 清理产物
clean:
	rm -f $(TARGET)