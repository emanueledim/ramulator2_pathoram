import argparse
import os
import re
import matplotlib.pyplot as plt
from pathlib import Path
import numpy as np

# === CLI ===
parser = argparse.ArgumentParser(description="Plots of MemorySystem counters.")
parser.add_argument('-i', '--input', required=True, help='Log files directory')
parser.add_argument('-o', '--output', required=True, help='Output directory to save graphs')
args = parser.parse_args()

os.makedirs(args.output, exist_ok=True)

input_folder = args.input
output_folder = args.output

def channel_label(n):
    return f"{n} Channel" if n == 1 else f"{n} Channels"
    


def parse_memorysystem(path):
    """
    Estrae le metriche globali e conta i controller nella sezione MemorySystem.
    """
    in_mem = False
    controller_count = 0
    metrics = {}
    with open(path, 'r') as f:
        for line in f:
            line = line.strip()
            if line.startswith("MemorySystem:"):
                in_mem = True
                continue
            if in_mem:
                if line == "":
                    break
                if line.startswith("Controller:"):
                    controller_count += 1
                elif ':' in line:
                    key, val = line.split(":", 1)
                    key = key.strip()
                    val = val.strip()
                    try:
                        num = float(val) if '.' in val else int(val)
                        metrics[key] = num
                    except:
                        pass
    return controller_count, metrics, in_mem

# === Parsing dei file ===
dataset = []
    
# Controlla che la cartella esista
if not os.path.isdir(input_folder):
    raise ValueError(f"Il percorso '{input_folder}' non è una cartella valida.")
    
# Scorre tutti i file nella cartella
i = 1
for filename in os.listdir(input_folder):
    
    full_path = os.path.join(input_folder, filename)
        
    # Salta se non è un file
    if not os.path.isfile(full_path):
        continue
    
    # Chiama la tua funzione di parsing
    n, metrics, in_mem = parse_memorysystem(full_path)
    if in_mem == False:
        continue
        
    metrics['integrity_controller_percent_active'] = (float(metrics['integrity_controller_active_cycles'])/float(metrics['memory_system_cycles'])) * 100.0
    metrics['integrity_controller_percent_idle'] = (float(metrics['integrity_controller_idle_cycles'])/float(metrics['memory_system_cycles'])) * 100.0
    if(metrics['integrity_controller_num_reqs'] != 0):
        metrics['integrity_controller_avg_latency'] = float(metrics['integrity_controller_latency'])/float(metrics['integrity_controller_num_reqs'])
    else:
        metrics['integrity_controller_avg_latency'] = 0
        
    
    metrics['oram_controller_avg_latency'] = float(metrics['oram_controller_cumulative_latency'])/float(metrics['total_num_read_requests'])
        
    dataset.append({
        'file': filename,
        'channels': n,
        'metrics': metrics,
        'idx': i
    })
    i = i+1


dataset_no_hash = [d for d in dataset if d['metrics']['integrity_controller_active_cycles'] == 0]
dataset_with_hash = [d for d in dataset if d['metrics']['integrity_controller_active_cycles'] != 0]

# === Grafici ===
with open("index_configurations.txt", 'w', encoding='utf-8') as f:
    for d in dataset:
        f.write(f"{d['file']} = {d['idx']}\n")

# === Metiche comuni tra i file ===
keys_sets = [set(d['metrics'].keys()) for d in dataset]
common_metrics = set.intersection(*keys_sets)
if not common_metrics:
    print("Nessuna metrica comune tra i file.")
    exit()

#Colors https://matplotlib.org/stable/gallery/color/named_colors.html
# === Grafici ===
for metric in sorted(common_metrics):
    x = [d['idx'] for d in dataset]
    y_values = [d['metrics'][metric] for d in dataset]

    plt.figure(figsize=(8, 4))
    plt.bar(x, y_values, color='skyblue')
    title = metric.replace("_", " ").capitalize()
    plt.title(title)
    plt.xlabel("Index configuration number")
    plt.ylabel(metric)
    plt.grid(axis='y', linestyle='--', alpha=0.5)
    plt.tight_layout()
    
    fname = f"{metric.replace('/', '_')}.png"
    plt.savefig(os.path.join(output_folder, fname))
    plt.close()
    
# === Metiche comuni tra i file ===
keys_sets = [set(d['metrics'].keys()) for d in dataset_with_hash]
common_metrics = set.intersection(*keys_sets)
if not common_metrics:
    print("Nessuna metrica comune tra i file.")
    exit()

# === Grafici ===
for metric in sorted(common_metrics):
    x = [d['idx'] for d in dataset_with_hash]
    y_values = [d['metrics'][metric] for d in dataset_with_hash]

    plt.figure(figsize=(8, 4))
    plt.bar(x, y_values, color='skyblue')
    title = metric.replace("_", " ").capitalize()
    plt.title(title)
    plt.xlabel("Index configuration number")
    plt.ylabel(metric)
    plt.grid(axis='y', linestyle='--', alpha=0.5)
    plt.tight_layout()
    
    fname = f"with_hash_{metric.replace('/', '_')}.png"
    plt.savefig(os.path.join(output_folder, fname))
    plt.close()
    
# === Metiche comuni tra i file ===
keys_sets = [set(d['metrics'].keys()) for d in dataset_no_hash]
common_metrics = set.intersection(*keys_sets)
if not common_metrics:
    print("Nessuna metrica comune tra i file.")
    exit()

# === Grafici ===
for metric in sorted(common_metrics):
    x = [d['idx'] for d in dataset_no_hash]
    y_values = [d['metrics'][metric] for d in dataset_no_hash]

    plt.figure(figsize=(8, 4))
    plt.bar(x, y_values, color='red')
    title = metric.replace("_", " ").capitalize()
    plt.title(title)
    plt.xlabel("Index configuration number")
    plt.ylabel(metric)
    plt.grid(axis='y', linestyle='--', alpha=0.5)
    plt.tight_layout()
    
    fname = f"no_hash_{metric.replace('/', '_')}.png"
    plt.savefig(os.path.join(output_folder, fname))
    plt.close()

print(f"Grafici creati nella cartella '{output_folder}' per {len(common_metrics)} metriche.")




