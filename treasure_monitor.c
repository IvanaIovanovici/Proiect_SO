#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/limits.h>

#define COMMAND_FILE "monitor_command.txt"
#define USERNAME_SIZE 32
#define CLUE_SIZE 128

// structura care reprezinta o comoara
typedef struct {
    int treasure_id;
    char username[USERNAME_SIZE];
    float latitude;
    float longitude;
    char clue[CLUE_SIZE];
    int value;
} Treasure;

// variabile globale care controleaza comportamentul monitorului
volatile sig_atomic_t should_terminate = 0;  // semnal pentru terminare
volatile sig_atomic_t got_command = 0;       // semnal ca a fost primita o comanda

// handler pentru semnalul SIGUSR1 - marcheaza ca a fost primita o comanda
void handle_sigusr1(int sig) {
    got_command = 1;
}

// handler pentru semnalul SIGTERM - marcheaza ca monitorul trebuie sa se opreasca
void handle_sigterm(int sig) {
    should_terminate = 1;
}

// afiseaza toate vanatorile disponibile si numarul de comori din fiecare
void list_hunts() {
    DIR *dir = opendir(".");
    if (!dir) {
        perror("Nu am putut deschide directorul curent");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // verifica daca este director si are fisier treasures.dat
        if (entry->d_type == DT_DIR &&
            strcmp(entry->d_name, ".") != 0 &&
            strcmp(entry->d_name, "..") != 0) {
            char treasure_path[PATH_MAX];
            snprintf(treasure_path, sizeof(treasure_path), "%s/treasures.dat", entry->d_name);
            if (access(treasure_path, F_OK) == 0) {
                int count = 0;
                int fd = open(treasure_path, O_RDONLY);
                if (fd >= 0) {
                    Treasure t;
                    // numara toate comorile din fisier
                    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
                        count++;
                    }
                    close(fd);
                }
                printf("Vanatoare: %s | Comori: %d\n", entry->d_name, count);
            }
        }
    }
    closedir(dir);
    fflush(stdout);  // fortam afisarea in stdout
}

// afiseaza toate comorile dintr-o vanatoare
void list_treasures(const char *hunt_id) {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/treasures.dat", hunt_id);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("Nu am putut deschide fisierul de comori");
        return;
    }

    Treasure t;
    // citeste si afiseaza fiecare comoara
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("ID: %d | Utilizator: %s | Lat: %.2f | Long: %.2f | Valoare: %d\n",
               t.treasure_id, t.username, t.latitude, t.longitude, t.value);
    }
    close(fd);
    fflush(stdout);
}

// afiseaza detaliile unei comori specifice dupa id
void view_treasure(const char *hunt_id, int id) {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/treasures.dat", hunt_id);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("Nu am putut deschide fisierul de comori");
        return;
    }

    Treasure t;
    int found = 0;
    // cauta comoara cu id-ul dat
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.treasure_id == id) {
            printf("ID: %d\nUtilizator: %s\nLatitudine: %.2f\nLongitudine: %.2f\nIndiciu: %s\nValoare: %d\n",
                   t.treasure_id, t.username, t.latitude, t.longitude, t.clue, t.value);
            found = 1;
            break;
        }
    }
    if (!found) {
        printf("Comoara cu ID %d nu a fost gasita.\n", id);
    }
    close(fd);
    fflush(stdout);
}

// citeste comanda din fisier si o executa
void process_command() {
    FILE *fp = fopen(COMMAND_FILE, "r");
    if (!fp) {
        perror("Nu pot citi comanda");
        return;
    }

    char line[256];
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return;
    }
    fclose(fp);

    // sparge linia in comanda si argumente
    char *cmd = strtok(line, " \n");
    if (!cmd) return;

    // executa comanda primita
    if (strcmp(cmd, "list_hunts") == 0) {
        list_hunts();
    } else if (strcmp(cmd, "list_treasures") == 0) {
        char *hunt_id = strtok(NULL, " \n");
        if (hunt_id) list_treasures(hunt_id);
    } else if (strcmp(cmd, "view_treasure") == 0) {
        char *hunt_id = strtok(NULL, " \n");
        char *id_str = strtok(NULL, " \n");
        if (hunt_id && id_str) view_treasure(hunt_id, atoi(id_str));
    }
}

// programul principal - asteapta semnale si executa comenzi
int main() {
    struct sigaction sa_usr1, sa_term;
    sigemptyset(&sa_usr1.sa_mask);
    sigemptyset(&sa_term.sa_mask);

    sa_usr1.sa_handler = handle_sigusr1;
    sa_usr1.sa_flags = SA_RESTART;

    sa_term.sa_handler = handle_sigterm;
    sa_term.sa_flags = SA_RESTART;

    // seteaza handlerii pentru semnalele asteptate
    sigaction(SIGUSR1, &sa_usr1, NULL);
    sigaction(SIGTERM, &sa_term, NULL);

    // bucla principala care asteapta comenzi prin semnal
    while (!should_terminate) {
        pause();  // asteapta semnal
        if (got_command) {
            got_command = 0;
            process_command();  // executa comanda primita
        }
    }

    // dupa terminare, se mai afiseaza un mesaj de confirmare
    usleep(300000);
    printf("[monitor] Oprire monitor finalizata.\n");
    fflush(stdout);
    return 0;
}
