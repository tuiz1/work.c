/**
 * 环境验证程序 —— 测试 socket / pthread / fork
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* ---- 测试 1: socket 编程 ---- */
void test_socket() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        exit(1);
    }
    printf("  [OK] socket() 创建成功, fd=%d\n", fd);
    close(fd);
}

/* ---- 测试 2: pthread 多线程 ---- */
void *thread_func(void *arg) {
    int *n = (int *)arg;
    printf("  线程 %d: 运行中...\n", *n);
    return NULL;
}

void test_pthread() {
    pthread_t t1, t2;
    int a1 = 1, a2 = 2;
    if (pthread_create(&t1, NULL, thread_func, &a1) != 0) {
        perror("pthread_create");
        exit(1);
    }
    if (pthread_create(&t2, NULL, thread_func, &a2) != 0) {
        perror("pthread_create");
        exit(1);
    }
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("  [OK] pthread_create/join 成功\n");
}

/* ---- 测试 3: fork 多进程 ---- */
void test_fork() {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        // 子进程
        printf("  子进程 (PID=%d): fork 成功\n", getpid());
        _exit(0);
    } else {
        // 父进程
        int status;
        waitpid(pid, &status, 0);
        printf("  父进程 (PID=%d): 子进程已回收, [OK] fork/waitpid 成功\n", getpid());
    }
}

/* ---- 测试 4: 互斥锁 ---- */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int shared = 0;

void *mutex_thread(void *arg) {
    pthread_mutex_lock(&mutex);
    shared++;
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void test_mutex() {
    pthread_t threads[5];
    for (int i = 0; i < 5; i++)
        pthread_create(&threads[i], NULL, mutex_thread, NULL);
    for (int i = 0; i < 5; i++)
        pthread_join(threads[i], NULL);
    if (shared == 5)
        printf("  [OK] pthread_mutex_lock/unlock 成功, shared=%d\n", shared);
    else
        printf("  [FAIL] 互斥锁异常, shared=%d (期望 5)\n", shared);
}

/* ---- 主函数 ---- */
int main() {
    printf("\n========== 环境验证 ==========\n");
    printf("编译器: GCC %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
    printf("\n--- 测试 1: socket 编程 ---\n");
    test_socket();

    printf("\n--- 测试 2: pthread 多线程 ---\n");
    test_pthread();

    printf("\n--- 测试 3: fork 多进程 ---\n");
    test_fork();

    printf("\n--- 测试 4: 互斥锁 ---\n");
    test_mutex();

    printf("\n========== 全部通过! ==========\n\n");
    return 0;
}
