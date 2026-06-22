/**
 * common.h — 网络编程与并发编程实验 公共头文件
 */
#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DEFAULT_PORT  8888
#define MAXLINE       4096
#define LISTEN_BACKLOG 10

/* 计算表达式（支持 + - * / 和括号的整数四则运算） */
int calculate(const char *expr, char *result, int maxlen);

#endif
