#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/select.h>
#include <dirent.h>

#define COMMAND_FILE "monitor_command.txt"

pid_t monitor_pid = -1;     // procesul monitor (child)
int pipe_fd[2];             // descriptorii pentru pipe (citire si scriere)

// handler pentru terminarea procesului copil (monitor)
void sigchld_handler(int sig) {
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    if (pid == monitor_pid) {
        printf("Monitorul s-a terminat cu status %d.\n", status);
        monitor_pid = -1;
    }
}

// scrie comanda data de utilizator in fisierul temporar
void write_command(const char *cmd) {
    FILE *fp = fopen(COMMAND_FILE, "w");
    if (!fp) {
        perror("Nu pot scrie comanda");
        return;
    }
    fprintf(fp, "%s\n", cmd);
    fclose(fp);
}

// citeste outputul de la monitor prin pipe si il afiseaza
void read_monitor_output() {
    char buf[256];
    ssize_t n;
    fd_set set;
    struct timeval timeout;

    FD_ZERO(&set);
    FD_SET(pipe_fd[0], &set);
    timeout.tv_sec = 2;      // timeout de 2 secunde
    timeout.tv_usec = 0;

    while (1) {
        int ready = select(pipe_fd[0] + 1, &set, NULL, NULL, &timeout);
        if (ready > 0) {
            n = read(pipe_fd[0], buf, sizeof(buf) - 1);
            if (n > 0) {
                buf[n] = '\0';
                printf("%s", buf);
            } else {
                break;
            }
        } else {
            break;
        }
    }
}

// porneste procesul monitor si seteaza comunicarea prin pipe
void start_monitor() {
    if (monitor_pid > 0) {
        printf("Monitorul deja ruleaza.\n");
        return;
    }
    if (pipe(pipe_fd) < 0) {
        perror("Eroare la pipe");
        exit(1);
    }

    monitor_pid = fork();
    if (monitor_pid == 0) {
        dup2(pipe_fd[1], STDOUT_FILENO);  // redirecteaza stdout in pipe
        close(pipe_fd[0]);                // inchide capatul de citire
        execl("./treasure_monitor", "treasure_monitor", NULL); // porneste monitorul
        perror("Eroare la execl");
        exit(1);
    }
    close(pipe_fd[1]);  // inchide capatul de scriere in parinte
    printf("Monitorul a fost pornit.\n");
}

// trimite semnal de terminare monitorului
void stop_monitor() {
    if (monitor_pid > 0) {
        kill(monitor_pid, SIGTERM);
    } else {
        printf("Monitorul nu ruleaza.\n");
    }
}

// ruleaza un proces extern care calculeaza scorurile pentru un hunt
void run_score_calculator(const char* hunt_id) {
    int fd[2];
    if (pipe(fd) < 0) {
        perror("Eroare la pipe pentru scoruri");
        return;
    }
    pid_t pid = fork();
    if (pid == 0) {
        dup2(fd[1], STDOUT_FILENO); // redirecteaza output-ul catre pipe
        close(fd[0]);
        execl("./calculate_score", "calculate_score", hunt_id, NULL);
        perror("Eroare la execl calculate_score");
        exit(1);
    }
    close(fd[1]);

    // citeste scorurile din pipe
    char buf[256];
    ssize_t n;
    while ((n = read(fd[0], buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
    }
    close(fd[0]);
    waitpid(pid, NULL, 0);
}

// parcurge directoarele si apeleaza calculul scorurilor pentru fiecare hunt
void calculate_scores_all() {
    DIR *dir = opendir(".");
    if (!dir) {
        perror("Nu pot deschide directorul curent");
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR &&
            strncmp(entry->d_name, "hunt", 4) == 0) {
            run_score_calculator(entry->d_name);
        }
    }
    closedir(dir);
}

// interfata principala interactiva
int main() {
    // seteaza handler pentru SIGCHLD
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    char line[256];
    while (1) {
        printf("$ ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;

        char *cmd = strtok(line, " \n");
        if (!cmd) continue;

        // comenzi simple
        if (strcmp(cmd, "start_monitor") == 0) {
            start_monitor();
        } else if (strcmp(cmd, "stop_monitor") == 0) {
            stop_monitor();
        } else if (strcmp(cmd, "exit") == 0) {
            if (monitor_pid > 0) {
                printf("Nu poti iesi inca. Monitorul inca ruleaza.\n");
            } else {
                break;
            }

        // comanda externa pentru scoruri
        } else if (strcmp(cmd, "calculate_score") == 0) {
            calculate_scores_all();

        // comenzi trimise la monitor prin fisier si semnal
        } else if (strcmp(cmd, "list_hunts") == 0 ||
                   strcmp(cmd, "list_treasures") == 0 ||
                   strcmp(cmd, "view_treasure") == 0) {

            char *args = strtok(NULL, "\n");
            char full_cmd[256];

            if (args)
                snprintf(full_cmd, sizeof(full_cmd), "%s %s", cmd, args);
            else
                snprintf(full_cmd, sizeof(full_cmd), "%s", cmd);

            write_command(full_cmd);

            if (monitor_pid > 0) {
                kill(monitor_pid, SIGUSR1);   // trimite semnal catre monitor
                read_monitor_output();        // citeste raspunsul din pipe
            } else {
                printf("Monitorul nu ruleaza.\n");
            }

        } else {
            printf("Comanda necunoscuta.\n");
        }
    }

    return 0;
}
