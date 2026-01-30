/*
 * Roll Number: MT25178
 * File: MT25178_Part_A1_Server.c
 * Description: Multi-threaded TCP Server (Two-Copy / Standard send)
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

#define MAX_FIELDS 8

// Structure to define the configuration sent by the client
typedef struct {
    int msg_size;
    int duration_sec;
} client_config_t;

// Structure to be sent (Dynamic fields)
typedef struct {
    char *fields[MAX_FIELDS];
    int total_size;
} message_t;

void *handle_client(void *socket_desc) {
    int sock = *(int *)socket_desc;
    free(socket_desc);

    client_config_t config;
    
    // 1. Receive Configuration from Client
    if (recv(sock, &config, sizeof(config), 0) <= 0) {
        perror("Failed to receive config");
        close(sock);
        return NULL;
    }

    // 2. Prepare the Data (Heap Allocation)
    // We split the requested msg_size into 8 separate strings
    int field_size = config.msg_size / MAX_FIELDS;
    char *buffers[MAX_FIELDS];
    
    for (int i = 0; i < MAX_FIELDS; i++) {
        buffers[i] = (char *)malloc(field_size);
        if (buffers[i]) {
            memset(buffers[i], 'A' + i, field_size); // Fill with dummy data
        }
    }

    // 3. Send Data continuously for fixed duration
    time_t start_time = time(NULL);
    while (time(NULL) - start_time < config.duration_sec) {
        // In "Two-Copy", we use standard send(). 
        // We send the 8 fields sequentially to simulate the structure transmission.
        for (int i = 0; i < MAX_FIELDS; i++) {
            if (send(sock, buffers[i], field_size, 0) < 0) {
                perror("Send failed");
                goto cleanup;
            }
        }
    }

cleanup:
    // 4. Cleanup
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

    // Create Socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set Socket Options (Reuse Addr)
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Bind
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d (Standard Two-Copy)...\n", port);

    while (1) {
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        pthread_t tid;
        new_sock = malloc(sizeof(int)); // Correctly allocates 4 bytes
        *new_sock = client_fd;
        
        if (pthread_create(&tid, NULL, handle_client, (void *)new_sock) < 0) {
            perror("Could not create thread");
            free(new_sock);
        }
        
        // Detach thread so resources are released on exit
        pthread_detach(tid);
    }

    return 0;
}
