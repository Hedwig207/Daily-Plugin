# 每日插件 (DailyPlugin) 构建脚本
# 使用 MinGW-w64 的 gcc 编译为 Windows 可执行文件 dp.exe
CC      = gcc
CFLAGS  = -O2 -Wall -Wextra
SRCS    = src/main.c src/util.c src/template.c src/scaffold.c src/package.c src/install.c src/addon.c src/update.c
OBJS    = $(SRCS:.c=.o)
BIN     = dp.exe

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

debug: CFLAGS += -g -O0
debug: all

clean:
	rm -f $(OBJS) $(BIN)

.PHONY: all clean debug