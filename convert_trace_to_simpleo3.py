import argparse
# === CLI ===
parser = argparse.ArgumentParser(description="Converte il formato da simple_trace a SimpleO3 di ramulator2.")
parser.add_argument('-i', '--input', required=True, help='File di input')
parser.add_argument('-o', '--output', required=True, help='File di output')
args = parser.parse_args()

input_filename = args.input
output_filename = args.output

def convert_memory_accesses(input_file, output_file):
    """
    Converte accessi a memoria nel formato:
        LD 0x39484          →   0 234884
        ST 0x27483 0x28273  →   0 160643 164467
    Dove il primo '0' è il numero di bubble (fissato a 0).
    Gli indirizzi vengono convertiti da esadecimale a decimale.
    """
    with open(input_file, 'r') as infile, open(output_file, 'w') as outfile:
        for line in infile:
            parts = line.strip().split()
            if not parts:
                continue  # Salta righe vuote
            
            instr = parts[0]
            args = parts[1:]

            # Converti ogni indirizzo da esadecimale a decimale
            converted_args = []
            for arg in args:
                if arg.startswith('0x') or arg.startswith('0X'):
                    converted_args.append(str(int(arg, 16)))
                else:
                    converted_args.append(arg)  # in caso ci sia altro (non hex)

            bubble_count = 0  # default
            outfile.write(f"{bubble_count} {' '.join(converted_args)}\n")

# Esecuzione base
if __name__ == '__main__':
    convert_memory_accesses(input_filename, output_filename)
