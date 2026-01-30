#!/bin/bash
# Description: Creates two network namespaces (server_ns, client_ns) linked by a veth pair.

echo "[INFO] Setting up Network Namespaces..."

# 1. Delete old namespaces if they exist (cleanup)
sudo ip netns del server_ns 2>/dev/null
sudo ip netns del client_ns 2>/dev/null

# 2. Add Namespaces
sudo ip netns add server_ns
sudo ip netns add client_ns

# 3. Create veth link (virtual cable)
sudo ip link add veth_server type veth peer name veth_client

# 4. Plug the cable ends into the namespaces
sudo ip link set veth_server netns server_ns
sudo ip link set veth_client netns client_ns

# 5. Configure Server Side (IP: 10.0.0.1)
sudo ip netns exec server_ns ip addr add 10.0.0.1/24 dev veth_server
sudo ip netns exec server_ns ip link set veth_server up
sudo ip netns exec server_ns ip link set lo up

# 6. Configure Client Side (IP: 10.0.0.2)
sudo ip netns exec client_ns ip addr add 10.0.0.2/24 dev veth_client
sudo ip netns exec client_ns ip link set veth_client up
sudo ip netns exec client_ns ip link set lo up

echo "[INFO] Done! Server IP: 10.0.0.1 | Client IP: 10.0.0.2"
