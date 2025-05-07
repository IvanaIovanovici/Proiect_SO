#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>  

void list_hunts() 
{
    DIR *dir = opendir(".");  
    if (dir == NULL) 
    {
        perror("Nu am putut deschide directorul");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) 
    {
        // verificam daca intrarea este un subdirector si nu "." sau ".."
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) 
        {
            printf("Vanatoare: %s\n", entry->d_name);
        }
    }

    closedir(dir); 
}

void list_treasures(const char *hunt_name) 
{
    char path[256];
    snprintf(path, sizeof(path), "%s/treasures.dat", hunt_name);
    
    FILE *file = fopen(path, "r");
    if (file == NULL) 
    {
        perror("Nu am putut deschide fisierul de comori");
        return;
    }

    char treasure_name[256];
    while (fgets(treasure_name, sizeof(treasure_name), file)) 
    {
        printf("Comora: %s", treasure_name);
    }

    fclose(file);
}

void view_treasure(const char *hunt_name, const char *treasure_name) 
{
    char path[256];
    snprintf(path, sizeof(path), "%s/treasures.dat", hunt_name);
    
    FILE *file = fopen(path, "r");
    if (file == NULL) 
    {
        perror("Nu am putut deschide fisierul de comori");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) 
    {
        if (strstr(line, treasure_name)) 
        {
            printf("Detalii comora: %s", line);
        }
    }

    fclose(file);
}

void start_monitor() 
{
    pid_t pid = fork();
    if (pid == 0) 
    {
        // procesul copil va actiona ca monitor
        while (1) 
        {
            sleep(1);  // asteaptya un semnal pentru a răspunde
        }
        exit(0);  
    } else if (pid > 0) 
    {
        // procesul parinte va returna imediat
        printf("Monitorul a fost pornit.\n");
    } else 
    {
        perror("Eroare la fork()");
    }
}

void stop_monitor(pid_t monitor_pid) 
{
    if (monitor_pid > 0) 
    {
        kill(monitor_pid, SIGTERM);  // trimit semnalul pentru a opri monitorul
        waitpid(monitor_pid, NULL, 0);  // astept terminarea monitorului
        printf("Monitorul a fost oprit.\n");
    } else 
    {
        printf("Monitorul nu rulează.\n");
    }
}

int main()
 {
    pid_t monitor_pid = -1;

    while (1) 
    {
        printf("\nComenzi disponibile:\n");
        printf("start_monitor - Porneste monitorul\n");
        printf("list_hunts - Listeaza vanatoarele\n");
        printf("list_treasures <hunt_name> - Listeaza comorile dintr-o vanatoare\n");
        printf("view_treasure <hunt_name> <treasure_name> - Vezi detalii despre o comoara\n");
        printf("stop_monitor - Opreste monitorul\n");
        printf("exit - Iese din program\n");
        
        char command[256];
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = '\0';  // sterge caracterul '\n' de la final

        if (strncmp(command, "start_monitor", 13) == 0) 
        {
            if (monitor_pid == -1) {

                start_monitor();
                monitor_pid = getpid();  // salveaza PID-ul monitorului
            } else {
                printf("Monitorul este deja pornit.\n");
            }
        } else if (strncmp(command, "list_hunts", 11) == 0) 
        {
            list_hunts();
        } else if (strncmp(command, "list_treasures", 14) == 0) 
        {
            char hunt_name[256];
            sscanf(command + 15, "%s", hunt_name);  // extrag numele vanatorii
            list_treasures(hunt_name);
        } else if (strncmp(command, "view_treasure", 13) == 0) 
        {
            char hunt_name[256], treasure_name[256];
            sscanf(command + 14, "%s %s", hunt_name, treasure_name);  // extrag numele vanatorii si al comorii
            view_treasure(hunt_name, treasure_name);
        } else if (strncmp(command, "stop_monitor", 12) == 0) 
        {
            if (monitor_pid != -1) 
            {
                stop_monitor(monitor_pid);
                monitor_pid = -1;
            } else 
            {
                printf("Monitorul nu este pornit.\n");
            }
        } else if (strncmp(command, "exit", 4) == 0) 
        {
            if (monitor_pid != -1) 
            {
                printf("Monitorul ruleaza inca. Opriti-l mai intai.\n");
            } else 
            {
                break;  // iese din program
            }
        } else 
        {
            printf("Comanda invalida. Incercati din nou.\n");
        }
    }

    return 0;
}
