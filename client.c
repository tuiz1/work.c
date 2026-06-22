/**
 * client.c — 远程计算器客户端（实验 1/2/3 通用）
 *
 * 用法: ./client [服务器IP]
 * 默认连接 127.0.0.1
 *
 * 输入计算表达式，输出计算结果。
 * 输入 "quit" 退出。
 */
#include "common.h"

int main(int argc, char *argv[]) {
    const char *server_ip = (argc > 1) ? argv[1] : "127.0.0.1";

    /* ---- 1. 创建套接字 ---- */
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("socket");
        exit(1);
    }

    /* ---- 2. 连接服务器 ---- */
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(DEFAULT_PORT);
    if (inet_pton(AF_INET, server_ip, &servaddr.sin_addr) <= 0) {
        fprintf(stderr, "无效的 IP 地址: %s\n", server_ip);
        close(sock_fd);
        exit(1);
    }

    if (connect(sock_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        perror("connect");
        close(sock_fd);
        exit(1);
    }
    printf("已连接到服务器 %s:%d\n", server_ip, DEFAULT_PORT);
    printf("输入计算表达式（如 2+3*4 或 (10-2)*(8+5)），输入 quit 退出\n");

    /* ---- 3. 交互循环：发送表达式 → 接收结果 ---- */
    char sendline[MAXLINE];
    char recvline[MAXLINE];

    while (1) {
        printf("\n> ");
        fflush(stdout);

        if (fgets(sendline, MAXLINE, stdin) == NULL) break;
        // 去掉末尾换行符
        size_t len = strlen(sendline);
        if (len > 0 && sendline[len - 1] == '\n') sendline[len - 1] = '\0';
        if (len == 0 || sendline[0] == '\0') continue;

        if (strcmp(sendline, "quit") == 0) break;

        /* 发送表达式 */
        if (send(sock_fd, sendline, strlen(sendline), 0) < 0) {
            perror("send");
            break;
        }

        /* 接收结果 */
        int n = recv(sock_fd, recvline, MAXLINE - 1, 0);
        if (n <= 0) {
            if (n == 0) printf("服务器关闭了连接\n");
            else perror("recv");
            break;
        }
        recvline[n] = '\0';
        printf("= %s\n", recvline);
    }

    /* ---- 4. 关闭套接字 ---- */
    close(sock_fd);
    printf("已断开连接。\n");
    return 0;
}
