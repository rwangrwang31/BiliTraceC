# ============================================================
# BiliTraceC Makefile
# B站弹幕溯源工具编译配置
# ============================================================

# 编译器设置
CC = gcc
CFLAGS = -O3 -Wall -Wextra -pthread
LDFLAGS = -lcurl -pthread

# Windows特殊处理 (MinGW/MSYS2)
ifeq ($(OS),Windows_NT)
    # Windows下需要链接ws2_32 (Winsock)
    LDFLAGS += -lws2_32
    TARGET = bilitrace.exe
    RM = del /Q
else
    TARGET = bilitrace
    RM = rm -f
endif

# 源文件
SRCS = main.c cracker.c network.c
OBJS = $(SRCS:.c=.o)
HEADERS = crc32_core.h utils.h cracker.h network.h

# 默认目标
all: $(TARGET)

# 链接
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)
	@echo "========================================="
	@echo " 编译完成: $(TARGET)"
	@echo "========================================="

# 编译规则
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# 调试版本
debug: CFLAGS = -g -Wall -Wextra -pthread -DDEBUG
debug: clean $(TARGET)
	@echo "调试版本编译完成"

# 性能分析版本
profile: CFLAGS = -O3 -pg -Wall -pthread
profile: clean $(TARGET)
	@echo "性能分析版本编译完成"

# 清理
clean:
ifeq ($(OS),Windows_NT)
	-$(RM) $(OBJS) $(TARGET) 2>nul
else
	$(RM) $(OBJS) $(TARGET)
endif
	@echo "清理完成"

# 安装 (Linux/macOS)
install: $(TARGET)
ifneq ($(OS),Windows_NT)
	install -m 755 $(TARGET) /usr/local/bin/
	@echo "已安装到 /usr/local/bin/$(TARGET)"
endif

# 卸载
uninstall:
ifneq ($(OS),Windows_NT)
	rm -f /usr/local/bin/$(TARGET)
	@echo "已卸载"
endif

# 帮助
help:
	@echo "BiliTraceC 编译选项:"
	@echo "  make          - 编译发布版本"
	@echo "  make debug    - 编译调试版本"
	@echo "  make profile  - 编译性能分析版本"
	@echo "  make clean    - 清理编译产物"
	@echo "  make install  - 安装到系统 (Linux/macOS)"
	@echo "  make help     - 显示此帮助"

.PHONY: all clean debug profile install uninstall help
