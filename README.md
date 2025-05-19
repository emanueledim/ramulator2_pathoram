# PathORAM
Repository che contiene l'implementazione di PathORAM per il simulatore Ramulator2

# Requisiti
Architettura macchina host: x86

Ubuntu 22.04 nativo oppure WSL 2 con Ubuntu 22.04 (se su Windows) 

# Istruzioni
Basterà eseguire gli script secondo l'ordine enumerato:
* ./1.install_requirements.sh: installa i pacchetti richiesti per buildare il simulatore;
* ./2.install_project.sh: clona la repository di ramulator2 e installa i sorgenti del componente PathORAM;
* ./3.build.sh: genera l'eseguibile di ramulator2
* ./4.run.sh: avvia il simulatore utilizzando la configurazione con PathORAM: config_example_oram.yaml

# Descrizione Progetto
Il repository contiene l'implementazione del componente PathORAM per l'Obliviousness della memoria, integrando il componente di verifica dell'integrità. È composto dai seguenti moduli:
* PathORAMSystem : implementato in **path_ORAM_system.cpp**
* ORAMController : implementato in **oram_controller.h** e **oram_controller.cpp**
* ORAMTree : implementato in **oram_tree.h**
* BucketHeader : implementato in **bucket_header.h**

## PathORAMSystem
Macro-componente che contiene e tempifica (clock) i componenti di memoria (ORAM Controller, DRAM Controller, DRAM, ...). Il Frontend (formato da CPU ed LLC) avanza le richieste di lettura o writeback di un blocco tramite questo componente, il quale ha il compito di **inizializzare** un percorso nell'ORAM Tree, preparando le strutture utili per l'**ORAM Controller**. Inoltre, alloca le entry nella position map. 

## ORAMController
Componente centrale che implementa la logica del protocollo PathORAM:
* Gestisce la **Position Map** e lo **Stash**.
* Genera le richieste di accesso alla memoria in **modo oblivious**.
* Smista e accoda le richieste verso i controller DRAM.
* Tiene traccia dello stato delle **transazioni** in corso e di quelle completate.

## ORAMTree
Struttura Out of Band che contiene un **albero linearizzato** di nodi BucketHeader, necessari per mantenere le informazioni dei metadati
dei bucket. Per ogni regione di memoria (lineare) pari a **Z * Block_Size**, viene associato un BucketHeader che mantiene le informazioni dei blocchi in essa contenuta.
Fornisce l'implementazione dei metodi che permettono un accesso controllato alla struttura.

## BucketHeader
Modella l'header del bucket. Ad ogni Bucket associa un bucket_id univoco e contiene le informazioni 
e i metodi necessari per identificare (blocco vuoto, dummy o data) e manipolare i metadati. A ciascun blocco, associa un block_id.

