# 🏴‍☠️ Treasure Hunt Manager - Linux System Programming

Acest proiect este un sistem modular de gestiune pentru activități de tip "Treasure Hunt", dezvoltat în **C** pentru sisteme de operare de tip Unix/Linux. Aplicația utilizează o arhitectură multi-proces și mecanisme de comunicare inter-proces (IPC).

## 🏗️ Arhitectura Sistemului

Proiectul este împărțit în patru componente principale, fiecare având un rol specific:

* **Treasure Hub (`treasure_hub.c`)**: Nucleul aplicației. Acesta orchestrează întregul sistem, lansează procesul de monitorizare și oferă interfața interactivă cu utilizatorul. Comunică cu monitorul prin **pipe-uri** și **semnale**.
* **Treasure Monitor (`treasure_monitor.c`)**: Un proces de tip "daemon" care rulează în fundal. Acesta așteaptă semnale (**SIGUSR1**) pentru a procesa comenzi asincrone și raportează starea vânătorilor de comori.
* **Treasure Manager (`treasure_manager.c`)**: Modulul responsabil pentru persistența datelor. Gestionează fișierele binare (`.dat`), creează **symlink-uri** pentru organizare și menține un jurnal (log) al acțiunilor.
* **Calculate Score (`calculate_score.c`)**: Un utilitar care procesează datele binare și calculează clasamentul utilizatorilor folosind **liste simplu înlănțuite** alocate dinamic.

## 🛠️ Tehnologii și Concepte Utilizate

* **Gestiunea Proceselor**: `fork()`, `exec()`, `waitpid()`, manipularea PID-urilor.
* **Comunicare Inter-Proces (IPC)**: Pipe-uri anonime, semnale POSIX (`sigaction`, `kill`).
* **Sistem de Fișiere**: Manipulare fișiere binare (`open`, `read`, `write`), directoare (`opendir`), legături simbolice (`symlink`).
* **Sincronizare**: Utilizarea tipurilor `sig_atomic_t` pentru handleri de semnale.
* **Structuri de Date**: Liste înlănțuite pentru agregarea scorurilor în timp real.

## 🚀 Cum se rulează
```bash
# Compilare
gcc -o hub treasure_hub.c
gcc -o monitor treasure_monitor.c
gcc -o manager treasure_manager.c
gcc -o score calculate_score.c

# Pornire sistem
./hub
