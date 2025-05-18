#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/limits.h>

#define USERNAME_SIZE 32
#define CLUE_SIZE 128

// structura pentru o comoara (aceeasi ca in treasure_manager)
typedef struct {
    int treasure_id;
    char username[USERNAME_SIZE];
    float latitude;
    float longitude;
    char clue[CLUE_SIZE];
    int value;
} Treasure;

// structura pentru un nod din lista de scoruri
typedef struct Node {
    char username[USERNAME_SIZE];
    int score;
    struct Node *next;
} ScoreNode;

// adauga un scor la utilizatorul dat sau creaza un nod nou
ScoreNode* add_score(ScoreNode* head, const char* username, int value) {
    ScoreNode* current = head;
    while (current) {
        if (strcmp(current->username, username) == 0) {
            current->score += value; // aduna la scorul existent
            return head;
        }
        current = current->next;
    }
    // utilizator nou, se creeaza nod nou
    ScoreNode* new_node = (ScoreNode*)malloc(sizeof(ScoreNode));
    strcpy(new_node->username, username);
    new_node->score = value;
    new_node->next = head;
    return new_node;
}

// afiseaza scorurile din lista
void print_scores(ScoreNode* head) {
    ScoreNode* current = head;
    while (current) {
        printf("%s: %d\n", current->username, current->score);
        current = current->next;
    }
}

// elibereaza memoria folosita pentru lista de scoruri
void free_scores(ScoreNode* head) {
    ScoreNode* current;
    while (head) {
        current = head;
        head = head->next;
        free(current);
    }
}

// programul principal
int main(int argc, char* argv[]) {
    if (argc != 2) {
        // mesaj de eroare daca nu se da numele vanatorii
        fprintf(stderr, "Usage: %s <hunt_id>\n", argv[0]);
        return 1;
    }

    // construieste calea catre fisierul treasures.dat
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/treasures.dat", argv[1]);

    // deschide fisierul in mod read-only
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("Nu pot deschide fisierul de comori");
        return 1;
    }

    Treasure t;
    ScoreNode* scores = NULL;

    // citeste comorile si adauga scorurile in lista
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        scores = add_score(scores, t.username, t.value);
    }
    close(fd);

    // afiseaza scorurile calculate
    printf("Scoruri pentru vanatoarea %s:\n", argv[1]);
    print_scores(scores);

    // elibereaza memoria
    free_scores(scores);

    return 0;
}
