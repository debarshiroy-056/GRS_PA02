/*
 * Roll Number: MT25178
 * File: MT25178_Part_A3_Server.c
 * Description: Multi-threaded TCP Server (Zero-Copy / MSG_ZEROCOPY)
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
#include <sys/uio.h>
#include <linux/errqueue.h> // Required for MSG_ZEROCOPY notifications
#include <errno.h>

#define MAX_FIELDS 8

typedef struct {
    int msg_size;
    int duration_sec;
} client_config_t;

// Helper to clear the kernel notifications from the Error Queue
void read_completions(int sock) {
    char buffer[256];
    struct msghdr msg = {0};
    struct iovec iov;
    char control[100];
    struct cmsghdr *cm;
    struct sock_extended_err *serr;

    iov.iov_base = buffer;
    iov.iov_len = sizeof(buffer);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control;
    msg.msg_controllen = sizeof(control);

    // Non-blocking read from the Error Queue
    while (recvmsg(sock, &msg, MSG_ERRQUEUE | MSG_DONTWAIT) > 0) {
        // Iterate over control messages to find ZEROCOPY completion
        for (cm = CMSG_FIRSTHDR(&msg); cm; cm = CMSG_NXTHDR(&msg, cm)) {
            if (cm->cmsg_level == SOL_IP && cm->cmsg_type == IP_RECVERR) {
                serr = (struct sock_extended_err *)CMSG_DATA(cm);
                if (serr->ee_errno == 0 && serr->ee_origin == SO_EE_ORIGIN_ZEROCOPY) {
                    // Successfully notified completion
                }
            }
        }
    }
}

void *handle_client(void *socket_desc) {
    int sock = *(int *)socket_desc;
    free(socket_desc);

    client_config_t config;
    if (recv(sock, &config, sizeof(config), 0) <= 0) {
        close(sock);
        return NULL;
    }

    // Enable ZEROCOPY on this socket
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_ZEROCOPY, &opt, sizeof(opt))) {
        perror("Setsockopt SO_ZEROCOPY failed (Kernel might not support it)");
        // Continue anyway; standard sendmsg will just fallback silently in some kernels
    }

    int field_size = config.msg_size / MAX_FIELDS;
    char *buffers[MAX_FIELDS];
    struct iovec iov[MAX_FIELDS];
    struct msghdr msg = {0};

    // Allocation
    for (int i = 0; i < MAX_FIELDS; i++) {
        // calloc helps ensure zero-initialized clean pages
        buffers[i] = (char *)calloc(1, field_size);
        iov[i].iov_base = buffers[i];
        iov[i].iov_len = field_size;
    }

    msg.msg_iov = iov;
    msg.msg_iovlen = MAX_FIELDS;

    time_t start_time = time(NULL);
    unsigned long packets_sent = 0;

    while (time(NULL) - start_time < config.duration_sec) {
        
        // Send with MSG_ZEROCOPY flag
        if (sendmsg(sock, &msg, MSG_ZEROCOPY) < 0) {
            if (errno == ENOBUFS) {
                // Socket buffer full of pinned pages? Clear completions.
                read_completions(sock);
                continue; 
            }
            perror("sendmsg zero-copy failed");
            goto cleanup;
        }

        packets_sent++;

        // Periodically check for completions to unpin pages
        if (packets_sent % 20 == 0) {
             read_completions(sock);
        }
    }
    
    // Final flush of completions
    read_completions(sock);

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

    // Reuse Address
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

    printf("Server listening on port %d (Zero-Copy / MSG_ZEROCOPY)...\n", port);

    while (1) {
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        pthread_t tid;
        new_sock = malloc(sizeof(int)); // Correct 4-byte allocation
        *new_sock = client_fd;
        
        if (pthread_create(&tid, NULL, handle_client, (void *)new_sock) < 0) {
            perror("Could not create thread");
            free(new_sock);
        }
        pthread_detach(tid);
    }

    return 0;
}
