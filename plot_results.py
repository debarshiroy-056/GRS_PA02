import matplotlib.pyplot as plt
import numpy as np

# Use Times New Roman for a professional report look
plt.rcParams['font.family'] = 'Times New Roman'
plt.rcParams['font.size'] = 12

# ==========================================
# EXPERIMENTAL DATA (Extracted from CSV)
# ==========================================

# X-Axis Labels
msg_sizes = ["1KB", "32KB", "256KB", "1MB"]
thread_counts = [1, 2, 4, 8]

# ---------------------------------------------------------
# 1. Throughput (Gbps)
# Fixed at Threads=1 to compare the baseline copy overhead
# ---------------------------------------------------------
A1_throughput = [0.6010, 4.1525, 19.6663, 30.8902]  # Two-Copy
A2_throughput = [1.5725, 19.7352, 31.9942, 40.2200]  # One-Copy
A3_throughput = [1.0738, 15.7752, 26.4820, 30.7895]  # Zero-Copy

# ---------------------------------------------------------
# 2. Latency (microseconds)
# Fixed at MsgSize=256KB to see contention effects
# ---------------------------------------------------------
A1_latency = [104.81, 103.69, 103.83, 104.52]
A2_latency = [64.26, 72.35, 72.37, 74.51]
A3_latency = [81.38, 82.79, 82.09, 83.68]

# ---------------------------------------------------------
# 3. Cache Misses (LLC)
# Fixed at Threads=1. Note: Numbers might vary based on perf interval
# ---------------------------------------------------------
A1_misses = [803, 543, 459, 541]
A2_misses = [612, 518, 834, 421]
A3_misses = [671, 788, 1262, 978]

# ---------------------------------------------------------
# 4. CPU Cycles
# Raw cycle counts captured during the transfer window
# ---------------------------------------------------------
A1_cycles = [34524, 27279, 28753, 23517]
A2_cycles = [49367, 36673, 27793, 22015]
A3_cycles = [44361, 27237, 86473, 51835]


# ==========================================
# PLOTTING FUNCTIONS
# ==========================================

def plot_throughput():
    """Generates the Throughput vs Message Size bar chart."""
    plt.figure(figsize=(10, 6))
    x = np.arange(len(msg_sizes))
    width = 0.25

    # Grouped bars for comparison
    plt.bar(x - width, A1_throughput, width, label='Two-Copy (A1)')
    plt.bar(x, A2_throughput, width, label='One-Copy (A2)')
    plt.bar(x + width, A3_throughput, width, label='Zero-Copy (A3)')

    plt.xlabel('Message Size', fontweight='bold')
    plt.ylabel('Throughput (Gbps)', fontweight='bold')
    plt.title('Throughput vs Message Size (Threads=1)', fontweight='bold')
    plt.xticks(x, msg_sizes)
    plt.legend()
    plt.grid(True, linestyle='--', alpha=0.5)
    
    filename = 'MT25178_Part_D_Plot_Throughput.png'
    plt.savefig(filename)
    print(f"Saved {filename}")

def plot_latency():
    """Generates the Latency vs Thread Count line chart."""
    plt.figure(figsize=(10, 6))
    
    plt.plot(thread_counts, A1_latency, marker='o', label='Two-Copy (A1)')
    plt.plot(thread_counts, A2_latency, marker='s', label='One-Copy (A2)')
    plt.plot(thread_counts, A3_latency, marker='^', label='Zero-Copy (A3)')

    plt.xlabel('Thread Count', fontweight='bold')
    plt.ylabel('Latency (us)', fontweight='bold')
    plt.title('Latency vs Thread Count (MsgSize=256KB)', fontweight='bold')
    plt.legend()
    plt.grid(True, linestyle='--', alpha=0.5)
    
    filename = 'MT25178_Part_D_Plot_Latency.png'
    plt.savefig(filename)
    print(f"Saved {filename}")

def plot_cache_misses():
    """Generates the Cache Misses vs Message Size plot."""
    plt.figure(figsize=(10, 6))
    
    plt.plot(msg_sizes, A1_misses, marker='o', label='Two-Copy (A1)')
    plt.plot(msg_sizes, A2_misses, marker='s', label='One-Copy (A2)')
    plt.plot(msg_sizes, A3_misses, marker='^', label='Zero-Copy (A3)')

    plt.xlabel('Message Size', fontweight='bold')
    plt.ylabel('LLC Cache Misses', fontweight='bold')
    plt.title('Cache Misses vs Message Size (Threads=1)', fontweight='bold')
    # Using log scale since misses can vary wildly
    plt.yscale('log')
    plt.legend()
    plt.grid(True, linestyle='--', alpha=0.5)
    
    filename = 'MT25178_Part_D_Plot_Cache.png'
    plt.savefig(filename)
    print(f"Saved {filename}")

def plot_cycles():
    """Generates the CPU Cycles per Byte vs Message Size plot."""
    plt.figure(figsize=(10, 6))
    
    # Calculate approximate bytes transferred: (Gbps * 10^9 / 8) * 5 seconds
    duration = 5.0
    A1_bytes = [tp * 1e9 / 8 * duration for tp in A1_throughput]
    A2_bytes = [tp * 1e9 / 8 * duration for tp in A2_throughput]
    A3_bytes = [tp * 1e9 / 8 * duration for tp in A3_throughput]

    # Cycles per Byte = Total Cycles / Total Bytes
    # Avoid division by zero if bytes is 0
    A1_cpb = [c / b if b > 0 else 0 for c, b in zip(A1_cycles, A1_bytes)]
    A2_cpb = [c / b if b > 0 else 0 for c, b in zip(A2_cycles, A2_bytes)]
    A3_cpb = [c / b if b > 0 else 0 for c, b in zip(A3_cycles, A3_bytes)]

    plt.plot(msg_sizes, A1_cpb, marker='o', label='Two-Copy (A1)')
    plt.plot(msg_sizes, A2_cpb, marker='s', label='One-Copy (A2)')
    plt.plot(msg_sizes, A3_cpb, marker='^', label='Zero-Copy (A3)')

    plt.xlabel('Message Size', fontweight='bold')
    plt.ylabel('CPU Cycles per Byte', fontweight='bold')
    plt.title('CPU Efficiency vs Message Size (Threads=1)', fontweight='bold')
    plt.legend()
    plt.grid(True, linestyle='--', alpha=0.5)
    
    filename = 'MT25178_Part_D_Plot_Cycles.png'
    plt.savefig(filename)
    print(f"Saved {filename}")

if __name__ == "__main__":
    plot_throughput()
    plot_latency()
    plot_cache_misses()
    plot_cycles()
    print("All plots generated successfully.")
