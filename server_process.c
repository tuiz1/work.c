/**
 * server_process.c — 实验2：基于多进程并发的远程计算器服务器
 *
 * fork() 为每个客户端创建子进程，父进程继续监听。
 * 用法: ./server_process
 */
#include "common.h"
#include <sys/wait.h>
#include <signal.h>

/* 回收僵尸子进程的信号处理函数 */
void sigchld_handler(int sig) {
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(void) {
    int socket_fd, connected_fd;
    struct sockaddr_in servaddr;
    char buff[MAXLINE], result[MAXLINE];

    /* 注册 SIGCHLD 处理，防止僵尸进程 */
    signal(SIGCHLD, sigchld_handler);

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
    printf("[多进程] 服务器启动，监听端口 %d ...\n", DEFAULT_PORT);

    /* ---- 4. 主循环：accept → fork → 子进程处理 ---- */
    while (1) {
        /* 阻塞直到有客户端连接 */
        connected_fd = accept(socket_fd, (struct sockaddr *)NULL, NULL);
        if (connected_fd == -1) {
            printf("accept socket error: %s (errno: %d)\n", strerror(errno), errno);
            continue;
        }

        /* 创建子进程 */
        int pid = fork();
        if (pid < 0) {
            perror("fork");
            close(connected_fd);
            continue;
        }

        if (pid == 0) {
            /* ======== 子进程 ======== */
            close(socket_fd);  // 子进程关闭监听套接字

            /* 接收客户端计算表达式 */
            int n = recv(connected_fd, buff, MAXLINE, 0);
            if (n > 0) {
                buff[n] = '\0';
                printf("[子进程 %d] 收到: %s\n", getpid(), buff);

                /* 执行计算 */
                calculate(buff, result, MAXLINE);

                /* 向客户端发送计算结果 */
                send(connected_fd, result, strlen(result), 0);
                printf("[子进程 %d] 发送结果: %s\n", getpid(), result);
            }

            close(connected_fd);  // 子进程关闭已连接套接字
            exit(0);

        } else {
            /* ======== 父进程 ======== */
            close(connected_fd);  // 父进程关闭已连接套接字
            /* 父进程继续循环，接收下一个连接 */
        }
    }

    close(socket_fd);
    return 0;
}
