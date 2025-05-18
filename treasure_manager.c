#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>

#define USERNAME_SIZE 32
#define CLUE_SIZE 128
#define BUFFER_SIZE 256

// structura comoara - corespunde formatului binar din fisier
typedef struct {
    int treasure_id;
    char username[USERNAME_SIZE];
    float latitude;
    float longitude;
    char clue[CLUE_SIZE];
    int value;
} Treasure;

// logheaza o actiune intr-un fisier text + creaza symlink pentru hunt
void log_action(const char *hunt_id, const char *action) {
    char log_path[BUFFER_SIZE];
    snprintf(log_path, sizeof(log_path), "%s/logged_hunt", hunt_id);

    int fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        perror("! eroare la deschiderea fisierului de log !");
        return;
    }

    // obtine timpul curent si scrie logul
    time_t now = time(NULL);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));
    dprintf(fd, "[%s] %s\n", time_str, action);
    close(fd);

    // creaza symlink catre fisierul de log
    char symlink_name[BUFFER_SIZE];
    snprintf(symlink_name, sizeof(symlink_name), "logged_hunt-%s", hunt_id);
    unlink(symlink_name);  // sterge symlink-ul anterior daca exista
    symlink(log_path, symlink_name);
}

// adauga o comoara intr-un fisier binar corespunzator unui hunt
void add_treasure(const char *hunt_id) {
    mkdir(hunt_id, 0755);  // creaza directorul hunt daca nu exista

    char file_path[BUFFER_SIZE];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);

    Treasure t;
    char input[BUFFER_SIZE];

    // citeste campurile comorii din stdin
    printf("ID Comoara: ");
    fgets(input, sizeof(input), stdin);
    sscanf(input, "%d", &t.treasure_id);

    printf("Nume utilizator: ");
    fgets(t.username, sizeof(t.username), stdin);
    t.username[strcspn(t.username, "\n")] = 0;

    printf("Latitudine: ");
    fgets(input, sizeof(input), stdin);
    sscanf(input, "%f", &t.latitude);

    printf("Longitudine: ");
    fgets(input, sizeof(input), stdin);
    sscanf(input, "%f", &t.longitude);

    printf("Indiciu: ");
    fgets(t.clue, sizeof(t.clue), stdin);
    t.clue[strcspn(t.clue, "\n")] = 0;

    printf("Valoare: ");
    fgets(input, sizeof(input), stdin);
    sscanf(input, "%d", &t.value);

    // scrie comoara in fisier binar
    int fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        perror("! eroare la deschiderea fisierului pentru comoara !");
        return;
    }
    write(fd, &t, sizeof(Treasure));
    close(fd);

    log_action(hunt_id, "A fost adaugata o comoara");
}

// afiseaza toate comorile dintr-un hunt
void list_treasures(const char *hunt_id) {
    char file_path[BUFFER_SIZE];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);

    struct stat st;
    if (stat(file_path, &st) < 0) {
        perror("eroare la obtinerea informatiilor despre fisier");
        return;
    }

    printf("Vanatoare: %s\nDimensiune fisier: %ld bytes\nUltima modificare: %s",
           hunt_id, st.st_size, ctime(&st.st_mtime));

    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        perror("! eroare la deschiderea fisierului de comori !");
        return;
    }

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("ID: %d, Utilizator: %s, Lat: %.2f, Long: %.2f, Valoare: %d\n",
               t.treasure_id, t.username, t.latitude, t.longitude, t.value);
    }
    close(fd);

    log_action(hunt_id, "Au fost listate comorile");
}

// afiseaza o comoara dupa id
void view_treasure(const char *hunt_id, int id) {
    char file_path[BUFFER_SIZE];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);

    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        perror("eroare la deschiderea fisierului");
        return;
    }

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.treasure_id == id) {
            printf("--- Comoara %d ---\nUtilizator: %s\nLatitudine: %.2f\nLongitudine: %.2f\nIndiciu: %s\nValoare: %d\n",
                   t.treasure_id, t.username, t.latitude, t.longitude, t.clue, t.value);
            log_action(hunt_id, "Vizualizare comoara");
            close(fd);
            return;
        }
    }
    printf("Comoara cu ID %d nu a fost gasita.\n", id);
    close(fd);
}

// sterge o comoara dupa id (rescrie fisierul fara ea)
void remove_treasure(const char *hunt_id, int id) {
    char file_path[BUFFER_SIZE];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);

    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        perror("! eroare la deschiderea fisierului original !");
        return;
    }

    int temp_fd = open("temp.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (temp_fd < 0) {
        perror("! eroare la crearea fisierului temporar !");
        close(fd);
        return;
    }

    Treasure t;
    int found = 0;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.treasure_id != id) {
            write(temp_fd, &t, sizeof(Treasure));
        } else {
            found = 1;
        }
    }

    close(fd);
    close(temp_fd);

    if (found) {
        rename("temp.dat", file_path);
        log_action(hunt_id, "A fost eliminata o comoara");
    } else {
        remove("temp.dat");
        printf("Comoara cu ID %d nu a fost gasita.\n", id);
    }
}

// sterge complet un hunt si fisierele asociate
void remove_hunt(const char *hunt_id) {
    char file_path[BUFFER_SIZE];

    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);
    unlink(file_path);

    snprintf(file_path, sizeof(file_path), "%s/logged_hunt", hunt_id);
    unlink(file_path);

    char symlink_name[BUFFER_SIZE];
    snprintf(symlink_name, sizeof(symlink_name), "logged_hunt-%s", hunt_id);
    unlink(symlink_name);

    rmdir(hunt_id);
    printf("Vanatoarea %s a fost stearsa.\n", hunt_id);
}

// functie principala - interpreteaza comenzi din linia de comanda
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Utilizare: %s <comanda> <hunt_id> [<id>]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "add") == 0) {
        add_treasure(argv[2]);
    } else if (strcmp(argv[1], "list") == 0) {
        list_treasures(argv[2]);
    } else if (strcmp(argv[1], "view") == 0 && argc == 4) {
        view_treasure(argv[2], atoi(argv[3]));
    } else if (strcmp(argv[1], "remove_treasure") == 0 && argc == 4) {
        remove_treasure(argv[2], atoi(argv[3]));
    } else if (strcmp(argv[1], "remove_hunt") == 0) {
        remove_hunt(argv[2]);
    } else {
        fprintf(stderr, "Comanda sau parametrii invalizi.\n");
        return 1;
    }

    return 0;
}
