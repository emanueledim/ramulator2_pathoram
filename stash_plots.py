import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import csv
import os

# === CLI ===
parser = argparse.ArgumentParser(description="Plots of stash occupancy over time.")
parser.add_argument('-i', '--input', required=True, help='Log files directory')
parser.add_argument('-o', '--output', required=True, help='Output directory to save graphs')
args = parser.parse_args()

os.makedirs(args.output, exist_ok=True)

directory = args.input

for prefix_name in os.listdir(directory):
    x = []
    y = []
    print(prefix_name)
        
    with open(os.path.join(directory, prefix_name), 'r') as csvfile:
        reader = csv.reader(csvfile)
        for row in reader:
            if len(row) == 2:
                x.append(float(row[0]))
                y.append(float(row[1]))

    fig, ax = plt.subplots()
    ax.plot(x, y, linestyle='-', color='b', drawstyle='default')

    # Formattazione asse X con separatori delle migliaia
    plt.title('Stash occupancy during simulation')
    plt.xlabel('Clock cycle')
    plt.ylabel('% Stash occupancy')
    plt.ylim(0, 100)  # Forza il range da 0 a 100
    plt.grid(True)

    plt.savefig(f'grafico_{prefix_name}.png', dpi=300)
    plt.close()
