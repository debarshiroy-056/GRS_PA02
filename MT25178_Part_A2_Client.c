/*
 * Roll Number: MT25178
 * File: MT25178_Part_A1_Client.c
 * Description: Multi-threaded TCP Client (Two-Copy / Standard recv)
 * Assignment: PA02
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>

// Configuration sent to Server
typedef struct {
    int msg_size;
    int duration_sec;
} client_config_t;

// Arguments passed to each thread
typedef struct {
    char *server_ip;
    int port;
    int msg_size;
    int duration;
} thread_args_t;

// Stats collected by each thread
typedef struct {
    long long total_bytes;
    double total_latency_us;
    long msg_count;
} thread_stats_t;

void *worker_thread(void *args) {
    thread_args_t *t_args = (thread_args_t *)args;
    thread_stats_t *stats = malloc(sizeof(thread_stats_t));
    stats->total_bytes = 0;
    stats->total_latency_us = 0;
    stats->msg_count = 0;

    int sock;
    struct sockaddr_in serv_addr;
    char *buffer = malloc(t_args->msg_size);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return NULL;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(t_args->port);

    if (inet_pton(AF_INET, t_args->server_ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return NULL;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return NULL;
    }

    // 1. Send Config to Server
    client_config_t config;
    config.msg_size = t_args->msg_size;
    config.duration_sec = t_args->duration;
    send(sock, &config, sizeof(config), 0);

    // 2. Receive Loop
    struct timeval start, end;
    ssize_t valread;
    ssize_t received_in_msg = 0;
    
    // We measure the time to receive ONE full message (Application Latency)
    gettimeofday(&start, NULL);

    while (1) {
        valread = recv(sock, buffer, t_args->msg_size, 0);
        if (valread <= 0) break; // Server closed connection or done

        stats->total_bytes += valread;
        received_in_msg += valread;

        // If we received a full "message" worth of bytes, calculate latency
        if (received_in_msg >= t_args->msg_size) {
            gettimeofday(&end, NULL);
            double latency = (end.tv_sec - start.tv_sec) * 1000000.0 + (end.tv_usec - start.tv_usec);
            stats->total_latency_us += latency;
            stats->msg_count++;
            
            // Reset for next message latency
            received_in_msg = 0;
            gettimeofday(&start, NULL); 
        }
    }

    close(sock);
    free(buffer);
    return stats;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Usage: %s <SERVER_IP> <PORT> <MSG_SIZE> <THREAD_COUNT>\n", argv[0]);
        return 1;
    }

    char *server_ip = argv[1];
    int port = atoi(argv[2]);
    int msg_size = atoi(argv[3]);
    int thread_count = atoi(argv[4]);
    int duration = 5; // Run each test for 5 seconds

    pthread_t threads[thread_count];
    thread_args_t args = {server_ip, port, msg_size, duration};
    thread_stats_t *results[thread_count];

    printf("🚀 Starting Client: %d threads, %d bytes msg, %ds duration\n", thread_count, msg_size, duration);

    // Spawn Threads
    for (int i = 0; i < thread_count; i++) {
        pthread_create(&threads[i], NULL, worker_thread, (void *)&args);
    }

    // Join Threads & Aggregate
    long long global_bytes = 0;
    double global_latency_sum = 0;
    long global_msgs = 0;

    for (int i = 0; i < thread_count; i++) {
        void *ret;
        pthread_join(threads[i], &ret);
        if (ret) {
            results[i] = (thread_stats_t *)ret;
            global_bytes += results[i]->total_bytes;
            global_latency_sum += results[i]->total_latency_us;
            global_msgs += results[i]->msg_count;
            free(results[i]);
        }
    }

    // Calculate Final Metrics
    double throughput_mbps = (global_bytes * 8.0) / (duration * 1000000.0 * 1000.0); // Gbps
    double avg_latency = (global_msgs > 0) ? (global_latency_sum / global_msgs) : 0.0;

    printf("------------------------------------------------\n");
    printf(" Results (Two-Copy):\n");
    printf("   Throughput: %.4f Gbps\n", throughput_mbps);
    printf("   Avg Latency: %.2f us\n", avg_latency);
    printf("------------------------------------------------\n");

    return 0;
}
