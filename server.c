/**
 * server.c — 实验1：单进程远程计算器服务器
 *
 * 一次只处理一个客户端连接（顺序服务）。
 * 用法: ./server
 */
#include "common.h"

int main(void) {
    int socket_fd, connected_fd;
    struct sockaddr_in servaddr;
    char buff[MAXLINE], result[MAXLINE];

    /* ---- 1. 创建服务器端套接字 ---- */
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        printf("create socket error: %s (errno: %d)\n", strerror(errno), errno);
        exit(0);
    }

    /* 设置 SO_REUSEADDR，避免 TIME_WAIT 导致 bind 失败 */
    int opt = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* ---- 2. 绑定套接字地址 ---- */
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
    printf("服务器启动，监听端口 %d ...\n", DEFAULT_PORT);

    /* ---- 4. 循环接收客户端连接并处理 ---- */
    while (1) {
        /* 接收客户端连接 */
        connected_fd = accept(socket_fd, (struct sockaddr *)NULL, NULL);
        if (connected_fd == -1) {
            printf("accept socket error: %s (errno: %d)\n", strerror(errno), errno);
            continue;
        }

        /* 接收客户端发送的计算表达式 */
        int n = recv(connected_fd, buff, MAXLINE, 0);
        if (n <= 0) {
            if (n == 0) printf("客户端断开\n");
            else printf("recv error: %s (errno: %d)\n", strerror(errno), errno);
            close(connected_fd);
            continue;
        }
        buff[n] = '\0';
        printf("收到来自客户端的消息: %s\n", buff);

        /* 执行计算 */
        calculate(buff, result, MAXLINE);

        /* 向客户端发送计算结果 */
        send(connected_fd, result, strlen(result), 0);
        printf("发送结果: %s\n", result);

        /* 关闭已连接套接字 */
        close(connected_fd);
    }

    /* 关闭监听套接字（实际不会执行到此处） */
    close(socket_fd);
    return 0;
}
