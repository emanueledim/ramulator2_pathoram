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
* ./4.run.sh: avvia il simulatore utilizzando una configurazione di default.

# Descrizione Progetto
Il repository contiene l'implementazione del componente PathORAM per l'Obliviousness della memoria, integrando il componente di verifica dell'integrità. È composto dai seguenti moduli:
* PathORAMSystem : implementato in **path_ORAM_system.cpp**;
* ORAMController : implementato in **oram_controller.h** e **oram_controller.cpp**;
* Stash : implementato in **stash.h** e **stash.cpp**;
* PositionMap : implementato in **position_map.h** e **position_map.cpp**;
* AccessLogic : implementato in **access_logic.h** e **access_logic.cpp**;
* OOBTree : implementato in **oob_tree.h** e **oob_tree.cpp**;
* Bucket : implementato in **bucket.h**;

## PathORAMSystem
Macro-componente che contiene e tempifica (clock) i componenti di memoria (ORAM Controller, DRAM Controller, DRAM, ...). Il Frontend (formato da CPU ed LLC) avanza le richieste di lettura o writeback di un blocco tramite questo componente, il quale ha il compito di **inoltrare** le richieste all'**ORAM Controller**. 

## ORAMController
Componente centrale che implementa la logica del protocollo PathORAM:
* Gestisce la **Position Map** e lo **Stash**.
* Genera le richieste di accesso alla memoria in **modo oblivious**.
* Smista e accoda le richieste verso i controller DRAM.
* Tiene traccia dello stato delle **transazioni** in corso e di quelle completate.

## OOBTree
Struttura Out of Band dell'ORAM Tree che contiene un **albero linearizzato** di nodi Bucket, necessari per mantenere le informazioni dei metadati
dei bucket e di stub data block. Per ogni regione di memoria (lineare) pari a **Z * Block_Size**, viene associato un BlockHeader che mantiene le informazioni dei blocchi in essa contenuta.
Fornisce l'implementazione dei metodi che permettono un accesso controllato alla struttura.

## Bucket
Modella i bucket. Fornisce i metodi necessari per manipolare i metadati (BlockHeader) e i blocchi (BlockData).

## BlockHeader
Modella il singolo blocco header di un blocco.
