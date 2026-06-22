/**
 * server_threadpool.c — 实验3：基于线程池的远程计算器服务器
 *
 * 预创建线程池，使用任务队列 + 互斥锁管理客户端请求。
 * 用法: ./server_threadpool
 */
#include "common.h"
#include <pthread.h>

/* ---- 线程池配置 ---- */
#define THREAD_COUNT  5
#define QUEUE_CAPACITY 100

/* ---- 任务结构体 ---- */
typedef struct {
    int connected_fd;  // 已连接套接字描述符
} Task;

/* ---- 任务队列结构体 ---- */
typedef struct {
    Task *tasks;        // 任务数组指针
    int   capacity;     // 任务队列容量
    int   size;         // 任务队列当前大小
    int   front;        // 队头索引
    int   rear;         // 队尾索引
} TaskQueue;

/* ---- 线程池结构体 ---- */
typedef struct {
    pthread_t *threads;       // 线程数组指针
    int        thread_count;  // 线程数量
    TaskQueue  task_queue;    // 任务队列
} ThreadPool;

/* ---- 全局互斥锁，保护任务队列 ---- */
static pthread_mutex_t task_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ---- 任务队列操作 ---- */

/* 初始化任务队列 */
void initTaskQueue(TaskQueue *queue, int capacity) {
    queue->tasks    = (Task *)malloc(sizeof(Task) * capacity);
    queue->capacity = capacity;
    queue->size     = 0;
    queue->front    = 0;
    queue->rear     = -1;
}

/* 添加任务到任务队列 */
void enqueueTask(TaskQueue *queue, Task task) {
    pthread_mutex_lock(&task_mutex);
    if (queue->size < queue->capacity) {
        queue->rear = (queue->rear + 1) % queue->capacity;
        queue->tasks[queue->rear] = task;
        queue->size++;
    } else {
        // 队列已满，丢弃或关闭连接
        close(task.connected_fd);
        fprintf(stderr, "[警告] 任务队列已满，丢弃一个连接\n");
    }
    pthread_mutex_unlock(&task_mutex);
}

/* 从任务队列中取出任务（阻塞轮询） */
Task dequeueTask(TaskQueue *queue) {
    Task task = { .connected_fd = -1 };  // 默认值 -1 表示无效任务

    while (1) {
        pthread_mutex_lock(&task_mutex);
        if (queue->size > 0) {
            task = queue->tasks[queue->front];
            queue->front = (queue->front + 1) % queue->capacity;
            queue->size--;
            pthread_mutex_unlock(&task_mutex);
            return task;
        }
        pthread_mutex_unlock(&task_mutex);
        // 队列为空，短暂休眠后重试
        usleep(10000);  // 10ms
    }
}

/* ---- 线程执行函数 ---- */
void *threadFunc(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;
    char buff[MAXLINE], result[MAXLINE];

    while (1) {
        /* 从任务队列取出任务 */
        Task task = dequeueTask(&(pool->task_queue));

        if (task.connected_fd != -1) {
            /* 接收客户端计算表达式 */
            int n = recv(task.connected_fd, buff, MAXLINE, 0);
            if (n > 0) {
                buff[n] = '\0';
                printf("[线程 %lu] 收到: %s\n", pthread_self(), buff);

                /* 执行计算 */
                calculate(buff, result, MAXLINE);

                /* 向客户端发送计算结果 */
                send(task.connected_fd, result, strlen(result), 0);
                printf("[线程 %lu] 发送结果: %s\n", pthread_self(), result);
            }

            close(task.connected_fd);
        }
    }
    return NULL;
}

/* ---- 主函数 ---- */
int main(void) {
    int socket_fd;
    struct sockaddr_in servaddr;

    /* ---- 1. 创建套接字 ---- */
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        printf("create socket error: %s (errno: %d)\n", strerror(errno), errno);
        exit(0);
    }

    /* 设置 SO_REUSEADDR，避免 TIME_WAIT 导致 bind 失败 */
    int opt = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* ---- 2. 绑定地址 ---- */
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(DEFAULT_PORT);

    if (bind(socket_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        printf("bind socket error: %s (errno: %d)\n", strerror(errno), errno);
        exit(0);
    }

    /* ---- 3. 开启监听 ---- */
    if (listen(socket_fd, LISTEN_BACKLOG) == -1) {
        printf("listen socket error: %s (errno: %d)\n", strerror(errno), errno);
        exit(0);
    }

    /* ---- 4. 创建线程池 ---- */
    ThreadPool thread_pool;
    thread_pool.thread_count = THREAD_COUNT;
    thread_pool.threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_pool.thread_count);
    initTaskQueue(&(thread_pool.task_queue), QUEUE_CAPACITY);

    for (int i = 0; i < thread_pool.thread_count; i++) {
        pthread_create(&(thread_pool.threads[i]), NULL, threadFunc, &thread_pool);
    }
    printf("[线程池] 服务器启动，%d 个工作线程，监听端口 %d ...\n",
           thread_pool.thread_count, DEFAULT_PORT);

    /* ---- 5. 主循环：接收连接，放入任务队列 ---- */
    while (1) {
        Task task;
        task.connected_fd = accept(socket_fd, (struct sockaddr *)NULL, NULL);
        if (task.connected_fd == -1) {
            printf("accept socket error: %s (errno: %d)\n", strerror(errno), errno);
            continue;
        }

        enqueueTask(&(thread_pool.task_queue), task);
    }

    close(socket_fd);
    return 0;
}
