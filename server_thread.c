/**
 * server_thread.c — 实验2：基于多线程并发的远程计算器服务器
 *
 * pthread_create() 为每个客户端创建线程处理请求。
 * 用法: ./server_thread
 */
#include "common.h"
#include <pthread.h>

/**
 * 客户端处理函数 — 每个线程的执行体
 * @param client_socket_ptr  指向已连接套接字描述符的指针（动态分配）
 */
void *handle_client(void *client_socket_ptr) {
    int  client_fd = *(int *)client_socket_ptr;
    char buff[MAXLINE];
    char result[MAXLINE];

    /* 接收客户端消息 */
    int n = recv(client_fd, buff, MAXLINE, 0);
    if (n > 0) {
        buff[n] = '\0';
        printf("[线程 %lu] 收到: %s\n", pthread_self(), buff);

        /* 执行计算 */
        calculate(buff, result, MAXLINE);

        /* 向客户端发送计算结果 */
        send(client_fd, result, strlen(result), 0);
        printf("[线程 %lu] 发送结果: %s\n", pthread_self(), result);
    }

    /* 关闭客户端已连接套接字 */
    close(client_fd);

    /* 释放保存已连接套接字描述符的内存 */
    free(client_socket_ptr);

    /* 线程结束 */
    pthread_exit(NULL);
}

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
    printf("[多线程] 服务器启动，监听端口 %d ...\n", DEFAULT_PORT);

    /* ---- 4. 主循环：accept → 创建线程处理 ---- */
    while (1) {
        /* 为每个客户端动态分配存储已连接套接字描述符的空间
         * 避免多线程竞争导致描述符被覆盖 */
        int *connected_fdp = (int *)malloc(sizeof(int));
        if (connected_fdp == NULL) {
            perror("malloc");
            continue;
        }

        *connected_fdp = accept(socket_fd, (struct sockaddr *)NULL, NULL);
        if (*connected_fdp == -1) {
            printf("accept socket error: %s (errno: %d)\n", strerror(errno), errno);
            free(connected_fdp);
            continue;
        }

        /* 创建新线程，处理客户端请求 */
        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_client, (void *)connected_fdp) != 0) {
            perror("pthread_create");
            close(*connected_fdp);
            free(connected_fdp);
        }

        /* 分离线程，线程结束后自动回收资源 */
        pthread_detach(client_thread);
    }

    close(socket_fd);
    return 0;
}
