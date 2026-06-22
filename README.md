# 网络编程与并发编程 — 操作系统课程设计

基于 C/S 架构的远程计算器，支持 TCP Socket 通信，涵盖多进程、多线程、线程池三种并发模型。

## 项目结构

```
├── common.h              # 公共头文件（端口/缓冲区/声明）
├── calculate.c           # 表达式计算器（递归下降，+-*/() 整数四则）
├── client.c              # 客户端（实验 1/2/3 通用）
├── server.c              # 实验1：单进程顺序服务器
├── server_process.c      # 实验2：多进程并发 (fork)
├── server_thread.c       # 实验2：多线程并发 (pthread)
├── server_threadpool.c   # 实验3：线程池 + 互斥锁
├── Makefile              # 一键编译
└── README.md
```

## 环境要求

- Ubuntu 22.04 LTS
- GCC 11.4+
- GDB 12.1+
- GNU Make

## 编译与运行

```bash
# 编译全部
make

# 启动服务器（任选）
./server               # 单进程
./server_process       # 多进程并发
./server_thread        # 多线程并发
./server_threadpool    # 线程池

# 另开终端，启动客户端
./client [服务器IP]
> 2+3*4
= 14
```

## 实验概要

| 实验 | 文件 | 核心技术 |
|------|------|----------|
| 实验1 | server.c + client.c | TCP Socket、bind/listen/accept/connect |
| 实验2 | server_process.c | fork()、waitpid()、僵尸进程处理 |
| 实验2 | server_thread.c | pthread_create/join/detach、线程安全 |
| 实验3 | server_threadpool.c | 线程池、任务队列、互斥锁(mutex) |

## 许可证

教育用途
