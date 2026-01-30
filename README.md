# PA02: Analysis of Network I/O Primitives

# Author: Debarshi Roy  
# Roll Number: MT25178  
# Course: Graduate Systems (CSE638)  
**Department:** Computational Biology, IIIT Delhi  

## Project Overview
This project analyzes the cost of data movement in network I/O by implementing and profiling three socket communication strategies:
1.  # Part A1 (Two-Copy): Standard `send()` / `recv()` (Baseline).
2.  # Part A2 (One-Copy): Optimized `sendmsg()` with Scatter-Gather I/O (`struct iovec`).
3.  # Part A3 (Zero-Copy): Linux `MSG_ZEROCOPY` API with completion notification polling.

The project utilizes **Linux Network Namespaces** to simulate a realistic network environment (two distinct nodes) on a single machine, ensuring accurate latency and throughput profiling.

##  Repository Structure
* `MT25178_Part_A1_Server.c` / `_Client.c`: Two-Copy Implementation.
* `MT25178_Part_A2_Server.c` / `_Client.c`: One-Copy Implementation (Scatter-Gather).
* `MT25178_Part_A3_Server.c` / `_Client.c`: Zero-Copy Implementation (`MSG_ZEROCOPY`).
* `MT25178_Part_C_shell.sh`: Automation script using Network Namespaces for profiling.
* `plot_results.py`: Python script to visualize the collected data.
* `Makefile`: Build automation.
* `MT25178_Part_C_CSV.csv`: Raw experimental data.
* `MT25178_Report.pdf`: Final analysis report.

## How to Build and Run

### 1. Compilation
Use the provided Makefile to compile all client/server binaries:
```bash
make clean
make

# 2. Automated Experimentation

Run the shell script to execute all experiments (A1, A2, A3) across various message sizes and thread counts. 
Note: sudo access may be required for perf to read hardware counters.

# Unlock perf counters (if needed)
sudo sysctl -w kernel.perf_event_paranoid=-1

# Run experiments
./MT25178_Part_C_shell.sh

# 3. Generate Plots

Once the MT25178_Part_C_CSV.csv is generated, use the Python script to create graphs:

python3 plot_results.py

# Observations

Throughput: One-Copy (A2) generally outperformed Two-Copy and Zero-Copy for messages < 1MB due to lower system call overhead compared to A1 and lower page-pinning overhead compared to A3.

Latency: One-Copy (A2) demonstrated the lowest latency.

Efficiency: CPU Cycles per Byte decreased as message size increased across all models.

# AI Declaration

Generative AI (Gemini 3 Pro) was used for:

Generating boilerplate code for struct iovec and msghdr setup.

Formatting awk commands for parsing perf output in the shell script.

Debugging perf_event_paranoid permissions. All core logic and analysis were implemented and verified manually.

### Step 2: The Critical `.gitignore`
The assignment strictly says no presence of Binary Files. We must prevent `git` from accidentally tracking your executables.

Create a `.gitignore` file:
```bash
nano .gitignore

*.o
*.out
*.tmp
MT25178_Part_A1_Server
MT25178_Part_A1_Client
MT25178_Part_A2_Server
MT25178_Part_A2_Client
MT25178_Part_A3_Server
MT25178_Part_A3_Client
__pycache__/
.DS_Store