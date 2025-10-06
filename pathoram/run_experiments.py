import argparse
import os
import yaml
import subprocess
import copy
from concurrent.futures import ProcessPoolExecutor, as_completed

# === CLI ===
parser = argparse.ArgumentParser(description="Run a pool of threads to execute the experiments")
parser.add_argument('-i', '--input', required=True, help='Base configuration file (e.g. ./config_oram_hbm2.yaml)')
parser.add_argument('-t', '--trace', required=True, help='Trace file setted in the configuration file')
args = parser.parse_args()

baseline_config_file = args.input
trace = args.trace

def run_command(cmd, log_file):
  print(f"Executing process")
  with open(log_file, "w") as f:
    subprocess.run(cmd, stdout=f)

num_lines = sum(1 for _ in open(trace))                  
max_workers = 8
commands = []

#Parameters to process
length_tree_list = [268435456, 2147483648] #2GB and 256MB
block_size_list = [512]
z_blocks_list = [1, 16]
arity_list = [2, 16]
stash_size_list = [256, 512]
encrypt_decrypt_delay_list = [1, 80]
hash_delay_list = [0, 80]
channels_list = [1, 8]
addr_mappers_list = ["BaRoPchChCo"]

base_config = None
with open(baseline_config_file, 'r') as f:
  base_config = yaml.safe_load(f)

for length_tree in length_tree_list:
  for block_size in block_size_list:
    for z_blocks in z_blocks_list:
      for arity in arity_list:
        for stash_size in stash_size_list:
          for en_de_delay in encrypt_decrypt_delay_list:
            for hash_delay in hash_delay_list:
              for ch in channels_list:
                for addr_mapper in addr_mappers_list:
                  #Copy configuration
                  config = copy.deepcopy(base_config)
                  #Prepare log file
                  #Override the parameter to change
                  config["MemorySystem"]["length_tree"] = length_tree
                  config["MemorySystem"]["block_size"] = block_size
                  config["MemorySystem"]["z_blocks"] = z_blocks
                  config["MemorySystem"]["arity"] = arity
                  config["MemorySystem"]["stash_size"] = stash_size
                  config["MemorySystem"]["encrypt_delay"] = en_de_delay
                  config["MemorySystem"]["decrypt_delay"] = en_de_delay
                  config["MemorySystem"]["hash_delay"] = hash_delay
                  config["MemorySystem"]["DRAM"]["org"]["channel"] = ch
                  config["MemorySystem"]["AddrMapper"]["impl"] = addr_mapper    
                  #config["Frontend"]["traces"] = trace
                  config["Frontend"]["num_expected_insts"] = num_lines
                  #Set the command
                  cmd = ["./ramulator2", "-c", str(config)]
                  log_filename = f"logs/hbm2_{length_tree}_{block_size}_{z_blocks}_{arity}_{stash_size}_{en_de_delay}_{hash_delay}_{ch}_{addr_mapper}.log"
                  #Add the command to the list
                  commands.append((cmd, log_filename))

with ProcessPoolExecutor(max_workers=max_workers) as executor:
  futures = [executor.submit(run_command, cmd, log_file) for cmd, log_file in commands]