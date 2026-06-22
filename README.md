# 网络编程与并发编程 — 远程计算器

> **操作系统课程设计**
>
> 基于 C/S 架构的远程计算器，支持 TCP Socket 通信，涵盖**单进程、多进程、多线程、线程池**四种并发模型。

---

## 目录

- [项目概述](#项目概述)
- [文件结构](#文件结构)
- [环境要求](#环境要求)
- [快速开始](#快速开始)
- [实验说明](#实验说明)
- [性能对比](#性能对比)
- [Git 分支与提交记录](#git-分支与提交记录)
- [许可证](#许可证)

---

## 项目概述

本项目实现了基于 TCP 协议的远程计算器。客户端发送数学表达式至服务器，服务器计算后将结果返回。支持加减乘除和括号的整数四则混合运算。

### 四种并发模型

| 文件 | 模型 | 核心技术 | 适用场景 |
|------|------|----------|----------|
| `server.c` | 单进程顺序 | socket/bind/listen/accept | 学习 Socket 基础 |
| `server_process.c` | 多进程并发 | fork/waitpid/SIGCHLD | 稳定性优先 |
| `server_thread.c` | 多线程并发 | pthread_create/detach | 性能优先 |
| `server_threadpool.c` | 线程池+互斥锁 | 环形队列/pthread_mutex | 高并发生产环境 |
| `server_threadpool_cv.c` | 线程池+条件变量 | pthread_cond_wait/signal | 最优性能（零 CPU 空转）|

### 技术栈

```
C语言 · POSIX API · TCP Socket · GCC 11.4 · GDB 12.1 · GNU Make
Ubuntu 22.04 LTS · Docker · WSL 2 · Git
```

---

## 文件结构

```
work.c/
├── common.h                    # 公共头文件：端口/缓冲区/calculate 声明
├── calculate.c                 # 表达式计算器（递归下降，+-*/() 整数四则）
├── client.c                    # 客户端（实验 1/2/3 通用）
│
├── server.c                    # 实验1：单进程顺序服务器
├── server_process.c            # 实验2：多进程并发 (fork)
├── server_thread.c             # 实验2：多线程并发 (pthread)
├── server_threadpool.c         # 实验3：线程池 + 互斥锁（忙轮询）
├── server_threadpool_cv.c      # 实验3：线程池 + 条件变量（阻塞等待）
│
├── stress_test.c               # 并发压测工具（QPS/延迟统计）
├── test_env.c                  # 环境验证程序（socket/pthread/fork/mutex）
│
├── Makefile                    # 一键编译
├── oslab.sh                    # Docker 快捷命令脚本
├── README.md                   # 本文件
└── 报告内容.md                  # 课程设计报告正文
```

---

## 环境要求

| 组件 | 版本/说明 |
|------|----------|
| 操作系统 | Ubuntu 22.04 LTS（Docker 容器或虚拟机） |
| 编译器 | GCC 11.4.0+ |
| 调试器 | GDB 12.1+ |
| 构建工具 | GNU Make 4.3+ |
| 宿主机 | Windows 11 + Docker Desktop 29.4+ + WSL 2 |

**环境搭建（Docker 方式）：**

```bash
# 1. 启动 Docker Desktop，拉取镜像
docker pull ubuntu:22.04

# 2. 创建容器（挂载代码目录）
MSYS_NO_PATHCONV=1 docker run -d --name oslab -v "D:\work_c":/work -w /work ubuntu:22.04 sleep infinity

# 3. 安装编译工具
MSYS_NO_PATHCONV=1 docker exec oslab apt update
MSYS_NO_PATHCONV=1 docker exec oslab apt install -y build-essential gdb

# 4. 验证环境
MSYS_NO_PATHCONV=1 docker exec oslab gcc --version
MSYS_NO_PATHCONV=1 docker exec oslab gdb --version
```

---

## 快速开始

```bash
# 编译全部（使用快捷脚本）
./oslab.sh make

# 或直接在容器内执行
MSYS_NO_PATHCONV=1 docker exec -it -w /work oslab make

# 启动服务器（四种任选）
./oslab.sh ./server               # 单进程
./oslab.sh ./server_process       # 多进程并发
./oslab.sh ./server_thread        # 多线程并发
./oslab.sh ./server_threadpool    # 线程池

# 另开终端，启动客户端
./oslab.sh ./client
> 2+3*4
= 14
> (100-25)*8/5
= 120
> quit
```

**压测：**

```bash
# 启动目标服务器（如线程池），另开终端运行：
./oslab.sh ./stress_test 127.0.0.1 100 10
# 100个并发线程 × 每个10次请求 = 1000次请求
```

---

## 实验说明

### 实验一：基础 Socket 远程计算器

**流程：** `socket() → bind() → listen() → accept() → recv() → calculate() → send() → close()`

**关键设计：**
- `setsockopt(SO_REUSEADDR)` 避免端口 TIME_WAIT 导致 bind 失败
- `htonl/htons` 处理网络字节序（主机序→大端序）
- 表达式计算采用递归下降法，支持带括号的加减乘除

### 实验二：并发编程远程计算器

**多进程版 (server_process.c)：**
- `fork()` 创建子进程处理客户端请求
- 子进程 `close(socket_fd)` / 父进程 `close(connected_fd)` 防止 fd 泄漏
- `signal(SIGCHLD, handler)` + `waitpid(WNOHANG)` 回收僵尸进程

**多线程版 (server_thread.c)：**
- `pthread_create()` 每连接创建线程
- `malloc` 动态分配 connected_fd 避免竞态条件
- `pthread_detach()` 线程自动回收

### 实验三：线程池性能优化

**核心设计：**
- 预创建 5 个工作线程 + 容量 100 的环形任务队列
- `pthread_mutex_lock/unlock` 保护队列的入队/出队操作
- **条件变量版**：`pthread_cond_wait/signal` 替代 `usleep` 忙轮询，零 CPU 空转

**生产者-消费者模型：**
- 生产者（主线程）：accept → enqueueTask
- 消费者（工作线程）：dequeueTask → recv → calculate → send → close

---

## 性能对比

| 指标 | server.c | server_process | server_thread | threadpool(cv) |
|------|----------|---------------|---------------|----------------|
| 并发模型 | 串行 | 每连接一进程 | 每连接一线程 | 线程池+条件变量 |
| 100并发耗时 | ~23s | ~3.2s | ~0.5s | ~0.3s |
| 线程创建数 | 1 | 100进程 | 100线程 | 5（预创建）|
| CPU 空转 | 无 | 无 | 无 | 无 |
| 资源可控性 | — | 差 | 差 | **好** |

---

## Git 分支与提交记录

```
*   03965d7 (main) merge dev — 完整实验代码与性能优化
|\
| * 71c2aad fix: 修复 stress_test.c 缺少 math.h 导致的链接错误
| * 642921b feat: 添加条件变量版线程池（回答实验3信号量思考题）
| * cd1a65b feat: 添加并发压测工具 stress_test.c
| * a935fb5 fix: 改进表达式计算器错误处理（除零/非法表达式）
| * 36a4195 docs: 添加项目 README 文档
|/
* d3a06d3 移除二进制文档文件
* be50b8f 网络编程与并发编程实验代码
```

- `main` — 稳定分支，当前最新代码
- `dev` — 开发分支，功能迭代和 bug 修复

---

## 许可证

本项目仅用于教育目的（操作系统课程设计）。
