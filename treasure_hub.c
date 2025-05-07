#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define COMMAND_FILE "monitor_command.txt"

pid_t monitor_pid = -1;
int monitor_stopped = 0;

void sigchld_handler(int sig) 
{
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    if (pid == monitor_pid) 
    {
        printf("Monitorul s-a terminat cu status %d.\n", status);
        monitor_stopped = 1;
    }
}

void write_command(const char *cmd) 
{
    FILE *fp = fopen(COMMAND_FILE, "w");
    if (!fp) 
    {
        perror("Nu pot scrie comanda");
        return;
    }
    fprintf(fp, "%s\n", cmd);
    fclose(fp);
}

int main() 
{
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    char line[256];
    while (1) 
    {
        printf("$ ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;

        char *cmd = strtok(line, " \n");
        if (!cmd) continue;

        if (strcmp(cmd, "start_monitor") == 0) 
        {
            if (monitor_pid > 0 && !monitor_stopped) 
            {
                printf("Monitorul deja ruleaza.\n");
                continue;
            }
            monitor_pid = fork();
            if (monitor_pid == 0) 
            {
                execl("./treasure_monitor", "treasure_monitor", NULL);
                perror("Eroare la execl");
                exit(1);
            }
        } else if (strcmp(cmd, "list_hunts") == 0) 
        {
            if (monitor_pid < 0 || monitor_stopped) 
            {
                printf("Monitorul nu ruleaza.\n");
                continue;
            }
            write_command("list_hunts");
            kill(monitor_pid, SIGUSR1);
        } else if (strcmp(cmd, "list_treasures") == 0) 
        {
            char *hunt_id = strtok(NULL, " \n");
            if (!hunt_id) continue;
            write_command((char[]) { 0 });
            snprintf(line, sizeof(line), "list_treasures %s", hunt_id);
            write_command(line);
            kill(monitor_pid, SIGUSR1);
        } else if (strcmp(cmd, "view_treasure") == 0) 
        {
            char *hunt_id = strtok(NULL, " \n");
            char *id = strtok(NULL, " \n");
            if (!hunt_id || !id) continue;
            snprintf(line, sizeof(line), "view_treasure %s %s", hunt_id, id);
            write_command(line);
            kill(monitor_pid, SIGUSR1);
        } else if (strcmp(cmd, "stop_monitor") == 0) 
        {
            if (monitor_pid > 0 && !monitor_stopped) 
            {
                kill(monitor_pid, SIGTERM);
            } else 
            {
                printf("Monitorul nu ruleaza.\n");
            }
        } else if (strcmp(cmd, "exit") == 0) 
        {
            if (monitor_pid > 0 && !monitor_stopped) 
            {
                printf("Nu poti iesi inca. Monitorul inca ruleaza.\n");
            } else 
            {
                break;
            }
        } else {
            printf("Comanda necunoscuta.\n");
        }
    }

    return 0;
}
