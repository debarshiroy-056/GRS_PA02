/*
 * Roll Number: MT25178
 * File: MT25178_Part_A2_Server.c
 * Description: Multi-threaded TCP Server (One-Copy / sendmsg with iovec)
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
#include <sys/uio.h> // Required for struct iovec, sendmsg

#define MAX_FIELDS 8

typedef struct {
    int msg_size;
    int duration_sec;
} client_config_t;

void *handle_client(void *socket_desc) {
    int sock = *(int *)socket_desc;
    free(socket_desc);

    client_config_t config;
    
    // Receive Configuration
    if (recv(sock, &config, sizeof(config), 0) <= 0) {
        perror("Failed to receive config");
        close(sock);
        return NULL;
    }

    // Prepare Data
    int field_size = config.msg_size / MAX_FIELDS;
    char *buffers[MAX_FIELDS];
    struct iovec iov[MAX_FIELDS]; // Scatter-Gather Array
    struct msghdr msg = {0};

    // Allocate and setup iovec
    for (int i = 0; i < MAX_FIELDS; i++) {
        buffers[i] = (char *)malloc(field_size);
        if (buffers[i]) {
            memset(buffers[i], 'B', field_size); // Fill with dummy data
        }
        
        // Point iovec directly to the buffer
        iov[i].iov_base = buffers[i];
        iov[i].iov_len = field_size;
    }

    // Setup Message Header
    msg.msg_iov = iov;
    msg.msg_iovlen = MAX_FIELDS;

    // Send Data Loop
    time_t start_time = time(NULL);
    while (time(NULL) - start_time < config.duration_sec) {
        // ONE-COPY OPTIMIZATION:
        // We make ONE system call (sendmsg). 
        // The Kernel gathers data from the 8 buffers and sends it.
        if (sendmsg(sock, &msg, 0) < 0) {
            perror("sendmsg failed");
            goto cleanup;
        }
    }

cleanup:
    for (int i = 0; i < MAX_FIELDS; i++) {
        free(buffers[i]);
    }
    close(sock);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <PORT>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int server_fd, client_fd, *new_sock;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d (One-Copy / sendmsg)...\n", port);

    while (1) {
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        pthread_t tid;
        // FIX: Allocating correct size for integer pointer
        new_sock = malloc(sizeof(int)); 
        *new_sock = client_fd;
        
        if (pthread_create(&tid, NULL, handle_client, (void *)new_sock) < 0) {
            perror("Could not create thread");
            free(new_sock);
        }
        pthread_detach(tid);
    }

    return 0;
}
