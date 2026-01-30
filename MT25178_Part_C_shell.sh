#!/bin/bash
# Roll Number: MT25178
# File: MT25178_Part_C_shell.sh
# Description: Automates profiling using Network Namespaces (Robust Perf Version)

# 1. Compile
echo "[INFO] Compiling..."
make clean
make
if [ $? -ne 0 ]; then
    echo "[ERROR] Compilation Failed!"
    exit 1
fi

# 2. Configuration for NAMESPACES
PORT=8080
SERVER_IP="10.0.0.1"  # IP inside server_ns
# Run tests for slightly longer to ensure we capture enough data
DURATION=5
OUTPUT_CSV="MT25178_Part_C_CSV.csv"

MSG_SIZES=(1024 32768 262144 1048576)
THREADS=(1 2 4 8)
MODELS=("A1" "A2" "A3")

echo "Model,MsgSize,Threads,Throughput_Gbps,Latency_us,CPU_Cycles,Cache_Misses,Context_Switches" > $OUTPUT_CSV

echo "[INFO] Starting Namespace Experiments..."

for MODEL in "${MODELS[@]}"; do
    SERVER_BIN="./MT25178_Part_${MODEL}_Server"
    CLIENT_BIN="./MT25178_Part_${MODEL}_Client"

    for SIZE in "${MSG_SIZES[@]}"; do
        for THREAD in "${THREADS[@]}"; do
            echo "------------------------------------------------"
            echo "[TEST] Model=$MODEL | Size=$SIZE | Threads=$THREAD"
            
            # --- START SERVER in 'server_ns' ---
            sudo ip netns exec server_ns bash -c "exec -a specific_server_proc $SERVER_BIN $PORT" > server_log.tmp 2>&1 &
            sleep 1
            
            # Find PID
            SERVER_PID=$(pgrep -f "specific_server_proc" | head -n 1)
            
            if [ -z "$SERVER_PID" ]; then
                echo "[ERROR] Server did not start!"
                continue
            fi

            # --- START PERF (Manual Mode) ---
            PERF_OUTPUT="perf_results.tmp"
            
            # Start perf in background attached to PID. NO SLEEP COMMAND.
            sudo perf stat -p $SERVER_PID -e cycles,cache-misses,cs --output $PERF_OUTPUT & 
            PERF_PID=$!

            # --- RUN CLIENT in 'client_ns' ---
            CLIENT_OUTPUT=$(sudo ip netns exec client_ns $CLIENT_BIN $SERVER_IP $PORT $SIZE $THREAD)
            
            # --- STOP PERF ---
            # Send SIGINT (Ctrl+C) to perf so it flushes data to file
            sudo kill -INT $PERF_PID
            wait $PERF_PID 2>/dev/null

            # Parse Results
            THROUGHPUT=$(echo "$CLIENT_OUTPUT" | grep "Throughput:" | awk '{print $2}')
            LATENCY=$(echo "$CLIENT_OUTPUT" | grep "Avg Latency:" | awk '{print $3}')
            CYCLES=$(grep "cycles" $PERF_OUTPUT | awk '{print $1}' | tr -d ',')
            MISSES=$(grep "cache-misses" $PERF_OUTPUT | awk '{print $1}' | tr -d ',')
            CTX_SWITCH=$(grep "cs" $PERF_OUTPUT | awk '{print $1}' | tr -d ',')

            # Validation
            if [ -z "$CYCLES" ]; then CYCLES=0; fi
            if [ -z "$MISSES" ]; then MISSES=0; fi
            if [ -z "$CTX_SWITCH" ]; then CTX_SWITCH=0; fi

            echo "$MODEL,$SIZE,$THREAD,$THROUGHPUT,$LATENCY,$CYCLES,$MISSES,$CTX_SWITCH" >> $OUTPUT_CSV
            echo "   [OK] $THROUGHPUT Gbps | $CYCLES Cycles"

            # Cleanup Server
            sudo kill -9 $SERVER_PID 2>/dev/null
            wait $SERVER_PID 2>/dev/null
            sleep 1
        done
    done
done

rm -f server_log.tmp perf_results.tmp
echo "[DONE] Data saved to $OUTPUT_CSV"
