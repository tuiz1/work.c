# Makefile — 网络编程与并发编程实验
# 在容器中执行: make all  或  ./oslab.sh make all

CC      = gcc
CFLAGS  = -Wall -Wextra -g
LDFLAGS = -lpthread

# 需要链接 pthread 的目标
THREAD_APPS = server_thread server_threadpool

# 不需要链接 pthread 的目标
NORMAL_APPS = client server server_process

# 公共对象文件
COMMON_OBJ = calculate.o

.PHONY: all clean

all: $(NORMAL_APPS) $(THREAD_APPS)

# 普通程序
client: client.c $(COMMON_OBJ) common.h
	$(CC) $(CFLAGS) -o $@ client.c $(COMMON_OBJ)

server: server.c $(COMMON_OBJ) common.h
	$(CC) $(CFLAGS) -o $@ server.c $(COMMON_OBJ)

server_process: server_process.c $(COMMON_OBJ) common.h
	$(CC) $(CFLAGS) -o $@ server_process.c $(COMMON_OBJ)

# 需要 pthread 的程序
server_thread: server_thread.c $(COMMON_OBJ) common.h
	$(CC) $(CFLAGS) -o $@ server_thread.c $(COMMON_OBJ) $(LDFLAGS)

server_threadpool: server_threadpool.c $(COMMON_OBJ) common.h
	$(CC) $(CFLAGS) -o $@ server_threadpool.c $(COMMON_OBJ) $(LDFLAGS)

# 公共对象
calculate.o: calculate.c common.h
	$(CC) $(CFLAGS) -c calculate.c

clean:
	rm -f *.o $(NORMAL_APPS) $(THREAD_APPS)
