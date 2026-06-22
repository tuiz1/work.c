/**
 * server_threadpool_cv.c — 实验3思考题扩展：条件变量版线程池
 *
 * 使用 pthread_cond_t 替代忙轮询，工作线程在队列为空时阻塞等待，
 * 主线程入队后发送信号唤醒。
 * 用法: ./server_threadpool_cv
 */
#include "common.h"
#include <pthread.h>

#define THREAD_COUNT  5
#define QUEUE_CAPACITY 100

typedef struct { int connected_fd; } Task;

typedef struct {
    Task *tasks;
    int   capacity, size, front, rear;
    pthread_mutex_t mutex;
    pthread_cond_t  not_empty;  // 队列非空条件变量
    pthread_cond_t  not_full;   // 队列非满条件变量
} TaskQueue;

typedef struct {
    pthread_t *threads;
    int        thread_count;
    TaskQueue  task_queue;
} ThreadPool;

/* 初始化任务队列 */
void initTaskQueue(TaskQueue *q, int cap) {
    q->tasks    = malloc(sizeof(Task) * cap);
    q->capacity = cap;
    q->size = q->front = 0;
    q->rear  = -1;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
}

/* 添加任务（生产者） */
void enqueueTask(TaskQueue *q, Task task) {
    pthread_mutex_lock(&q->mutex);
    while (q->size >= q->capacity)
        pthread_cond_wait(&q->not_full, &q->mutex);  // 等待队列非满
    q->rear = (q->rear + 1) % q->capacity;
    q->tasks[q->rear] = task;
    q->size++;
    pthread_cond_signal(&q->not_empty);  // 唤醒一个等待的消费者
    pthread_mutex_unlock(&q->mutex);
}

/* 取出任务（消费者） */
Task dequeueTask(TaskQueue *q) {
    pthread_mutex_lock(&q->mutex);
    while (q->size == 0)
        pthread_cond_wait(&q->not_empty, &q->mutex);  // 等待队列非空
    Task task = q->tasks[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;
    pthread_cond_signal(&q->not_full);  // 唤醒可能阻塞的生产者
    pthread_mutex_unlock(&q->mutex);
    return task;
}

/* 线程执行函数 */
void *threadFunc(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;
    char buff[MAXLINE], result[MAXLINE];

    while (1) {
        Task task = dequeueTask(&pool->task_queue);  // 阻塞等待，不占 CPU
        int n = recv(task.connected_fd, buff, MAXLINE, 0);
        if (n > 0) {
            buff[n] = '\0';
            calculate(buff, result, MAXLINE);
            send(task.connected_fd, result, strlen(result), 0);
        }
        close(task.connected_fd);
    }
    return NULL;
}

int main(void) {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) { perror("socket"); exit(1); }

    int opt = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(DEFAULT_PORT);
    bind(socket_fd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    listen(socket_fd, LISTEN_BACKLOG);

    ThreadPool pool;
    pool.thread_count = THREAD_COUNT;
    pool.threads = malloc(sizeof(pthread_t) * pool.thread_count);
    initTaskQueue(&pool.task_queue, QUEUE_CAPACITY);

    for (int i = 0; i < pool.thread_count; i++)
        pthread_create(&pool.threads[i], NULL, threadFunc, &pool);

    printf("[条件变量线程池] 启动，%d 线程，端口 %d\n",
           pool.thread_count, DEFAULT_PORT);

    while (1) {
        Task task;
        task.connected_fd = accept(socket_fd, NULL, NULL);
        if (task.connected_fd != -1)
            enqueueTask(&pool.task_queue, task);
    }

    close(socket_fd);
    return 0;
}
