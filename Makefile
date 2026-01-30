# Roll Number: MT25178
# Assignment: PA02
CC = gcc
CFLAGS = -Wall -pthread -O2

# Build all targets
all: server_a1 client_a1 server_a2 client_a2 server_a3 client_a3

# Part A1 (Two-Copy)
server_a1: MT25178_Part_A1_Server.c
	$(CC) $(CFLAGS) -o MT25178_Part_A1_Server MT25178_Part_A1_Server.c

client_a1: MT25178_Part_A1_Client.c
	$(CC) $(CFLAGS) -o MT25178_Part_A1_Client MT25178_Part_A1_Client.c

# Part A2 (One-Copy)
server_a2: MT25178_Part_A2_Server.c
	$(CC) $(CFLAGS) -o MT25178_Part_A2_Server MT25178_Part_A2_Server.c

client_a2: MT25178_Part_A2_Client.c
	$(CC) $(CFLAGS) -o MT25178_Part_A2_Client MT25178_Part_A2_Client.c

# Part A3 (Zero-Copy)
server_a3: MT25178_Part_A3_Server.c
	$(CC) $(CFLAGS) -o MT25178_Part_A3_Server MT25178_Part_A3_Server.c

client_a3: MT25178_Part_A3_Client.c
	$(CC) $(CFLAGS) -o MT25178_Part_A3_Client MT25178_Part_A3_Client.c

clean:
	rm -f MT25178_Part_A1_Server MT25178_Part_A1_Client
	rm -f MT25178_Part_A2_Server MT25178_Part_A2_Client
	rm -f MT25178_Part_A3_Server MT25178_Part_A3_Client
	rm -f *.o *.csv
