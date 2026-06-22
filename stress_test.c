/**
 * stress_test.c — 并发压测客户端
 * 用法: ./stress_test [服务器IP] [并发数] [每客户端请求数]
 * 示例: ./stress_test 127.0.0.1 100 10
 */
#include "common.h"
#include <pthread.h>
#include <sys/time.h>

typedef struct {
    const char *server_ip;
    int         thread_id;
    int         num_requests;
    double      total_time_ms;
    int         success;
    int         fail;
} ThreadArg;

void *stress_thread(void *arg) {
    ThreadArg *a = (ThreadArg *)arg;
    struct timeval start, end;

    for (int i = 0; i < a->num_requests; i++) {
        int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd < 0) { a->fail++; continue; }

        struct sockaddr_in servaddr;
        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port   = htons(DEFAULT_PORT);
        inet_pton(AF_INET, a->server_ip, &servaddr.sin_addr);

        gettimeofday(&start, NULL);

        if (connect(sock_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
            close(sock_fd); a->fail++; continue;
        }

        char expr[64];
        snprintf(expr, sizeof(expr), "%d+%d*%d", a->thread_id, i, 3);
        send(sock_fd, expr, strlen(expr), 0);

        char buf[MAXLINE];
        recv(sock_fd, buf, MAXLINE - 1, 0);

        gettimeofday(&end, NULL);
        a->total_time_ms += (end.tv_sec - start.tv_sec) * 1000.0 +
                            (end.tv_usec - start.tv_usec) / 1000.0;

        close(sock_fd);
        a->success++;
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    const char *server_ip = (argc > 1) ? argv[1] : "127.0.0.1";
    int concurrency       = (argc > 2) ? atoi(argv[2]) : 10;
    int req_per_client    = (argc > 3) ? atoi(argv[3]) : 10;

    printf("压测配置: 服务器=%s, 并发=%d, 每连接请求=%d\n",
           server_ip, concurrency, req_per_client);
    printf("总请求数=%d\n\n", concurrency * req_per_client);

    pthread_t *threads = malloc(sizeof(pthread_t) * concurrency);
    ThreadArg *args    = malloc(sizeof(ThreadArg) * concurrency);

    struct timeval total_start, total_end;
    gettimeofday(&total_start, NULL);

    for (int i = 0; i < concurrency; i++) {
        args[i].server_ip    = server_ip;
        args[i].thread_id    = i;
        args[i].num_requests = req_per_client;
        args[i].total_time_ms = 0;
        args[i].success      = 0;
        args[i].fail         = 0;
        pthread_create(&threads[i], NULL, stress_thread, &args[i]);
    }

    int total_success = 0, total_fail = 0;
    double total_time_ms = 0;

    for (int i = 0; i < concurrency; i++) {
        pthread_join(threads[i], NULL);
        total_success += args[i].success;
        total_fail   += args[i].fail;
        total_time_ms = fmax(total_time_ms, args[i].total_time_ms);
    }

    gettimeofday(&total_end, NULL);
    double wall_time = (total_end.tv_sec - total_start.tv_sec) * 1000.0 +
                       (total_end.tv_usec - total_start.tv_usec) / 1000.0;

    printf("========== 压测结果 ==========\n");
    printf("成功: %d, 失败: %d\n", total_success, total_fail);
    printf("总耗时: %.0f ms\n", wall_time);
    printf("QPS: %.1f 请求/秒\n", total_success / (wall_time / 1000.0));
    printf("平均延迟: %.1f ms\n", total_time_ms / (concurrency * req_per_client));

    free(threads);
    free(args);
    return 0;
}
