import re
import os
import argparse
import matplotlib.pyplot as plt
from collections import defaultdict

# === CLI ARGUMENTS ===
parser = argparse.ArgumentParser(description="Estrae metriche per canale e genera grafici PNG.")
parser.add_argument('--input', '-i', required=True, help='Path del file di input (log)')
parser.add_argument('--output', '-o', required=True, help='Cartella di output per i grafici')
args = parser.parse_args()

logfile_path = args.input
output_dir = args.output
os.makedirs(output_dir, exist_ok=True)

# === DATI ===
channel_data = defaultdict(lambda: defaultdict(float))
all_metrics = set()
channels_set = set()
pattern = re.compile(r"(\w+)_([0-9]+)$")
selected_metrics = ["avg_read_latency"]

# === PARSING ===
with open(logfile_path, "r") as f:
    for line in f:
        line = line.strip()
        if ':' not in line:
            continue
        key, val = line.split(":", 1)
        key = key.strip()
        val = val.strip()

        m = pattern.match(key)
        if not m:
            continue  # ignora chiavi senza _N

        metric, ch_str = m.group(1), m.group(2)
        try:
            num = float(val) if '.' in val else int(val)
        except ValueError:
            continue

        ch = int(ch_str)
        channel_data[ch][metric] = num
        all_metrics.add(metric)
        channels_set.add(ch)

channels = sorted(channels_set)

# === PLOT & SAVE ===
def plot_and_save(metric):
    values = [channel_data[ch].get(metric, 0) for ch in channels]
    if all(v == 0 for v in values):
        return

    plt.figure(figsize=(6, 4))
    plt.bar([f"Channel {ch}" for ch in channels], values, color='teal')
    plt.title(metric)
    plt.ylabel(metric)
    plt.grid(axis='y', linestyle='--', alpha=0.6)
    plt.tight_layout()

    filename = f"{metric}.png"
    plt.savefig(os.path.join(output_dir, filename))
    plt.close()

filtered_metrics = [m for m in all_metrics if m in selected_metrics]

for metric in sorted(all_metrics):
    plot_and_save(metric)

print(f"Grafici salvati in '{output_dir}/' per {len(all_metrics)} metriche.")
