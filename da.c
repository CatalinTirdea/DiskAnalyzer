#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include "utils.h"

void write_to_daemon(const char* instruction) {
    create_directory(instruction_file_path);

    int fd = open(instruction_file_path, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU | S_IRWXG | S_IRWXO);
    int len = strlen(instruction);
    write(fd, instruction, len);
    close(fd);

    if (daemon_pid > 0) {
        kill(daemon_pid, SIGUSR1);
        sleep(2);
    } else {
        fprintf(stderr, "Couldn't send instruction to daemon because it is not running\n");
        exit(EXIT_FAILURE);
    }
}

void read_daemon_pid() {
    int fd = open(daemon_pid_file_path, O_RDONLY);
    if (fd < 0) {
        perror("Daemon hasn't started");
        exit(EXIT_FAILURE);
    }

    char buf[MAX_PID_LENGTH];
    ssize_t bytesRead = read(fd, buf, MAX_PID_LENGTH);
    close(fd);

    if (bytesRead <= 0) {
        perror("Error reading daemon pid");
        exit(EXIT_FAILURE);
    }

    daemon_pid=atoi(buf);
}

void read_results_from_daemon(int signo) {
    char* res = malloc(1000000);
    if (res == NULL) {
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
    }
    daemon_message("Dupa primeste marime fisier\n");
    FILE* file = fopen(output_file_path, "r");
    if (file == NULL) {
        perror("Couldn't open daemon output file");
        free(res);
        return;
    }
    daemon_message("Inainte de malloc\n");

    daemon_message("Dupa malloc\n");
    syze_t bytesRead = fread(res, 1, 1000000, file);
    fclose(file);

    if (bytesRead <= 0) {
        perror("Error reading from daemon output file");
        free(res);  // Free allocated memory before exiting
        exit(EXIT_FAILURE);
    }
    daemon_message("Dupa read\n");
    printf("%s\n", res);
    free(res);
}

void write_da_pid() {
    create_directory(da_pid_file_path);

    char pidstring[MAX_PID_LENGTH];
    sprintf(pidstring, "%d", getpid());

    int fd = open(da_pid_file_path, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU | S_IRWXG | S_IRWXO);
    if (fd < 0) {
        perror("Could not create da pid file");
        exit(EXIT_FAILURE);
    }

    write(fd, pidstring, strlen(pidstring));
    close(fd);
}
int main(int argc, char** argv) {
    signal(SIGUSR2, read_results_from_daemon);
    write_da_pid();
    read_daemon_pid();
    
    char* instruction = malloc(INSTR_LENGTH);
    if (argc == 1) {
        printf("No arguments found. Please introduce an analyze-option and a desired path.\n");
        printf("Use --help command for more information.\n");
        return 0;
    } else if (
        strcmp(argv[1], "-a") && strcmp(argv[1], "--add") && strcmp(argv[1], "-p") && strcmp(argv[1], "--priority")
        && strcmp(argv[1], "-S") && strcmp(argv[1], "--suspend") && strcmp(argv[1], "-R") && strcmp(argv[1], "--resume")
        && strcmp(argv[1], "-r") && strcmp(argv[1], "--remove") && strcmp(argv[1], "-i") && strcmp(argv[1], "--info")
        && strcmp(argv[1], "-l") && strcmp(argv[1], "--list") && strcmp(argv[1], "-h") && strcmp(argv[1], "--help")
    ) {
        printf("No such option, please choose a valid option.\nUse --help command for more information.\n");
        return 0;
    } else if (argc == 2 && strcmp(argv[1], "-l") && strcmp(argv[1], "--list") && strcmp(argv[1], "-h") && strcmp(argv[1], "--help")){
        printf("No task-id found. Please select an existing task-id.\n");
        return 0;
    } else if (argc >= 4 && (strcmp(argv[1], "-a") && strcmp(argv[1], "--add") && (!strcmp(argv[3], "-p") || !strcmp(argv[3], "--priority")))){
        printf("Cannot set priority for a nonexistent analysis task. Please use the -a or --add option.\n");
        return 0;
    } else {
        if (!strcmp(argv[1], "-a") || !strcmp(argv[1], "--add")){
            if (argc != 5 || (argc == 5 && strcmp(argv[3], "-p") && strcmp(argv[3], "--priority"))){
                printf("Invalid arguments for --add command.\nUse --help command for more information.\n");
                return 0;
            } else { // ADD command
                char* priority = malloc(7);
                // TODO check is valid path 
                if (!strcmp(argv[4], "1"))
                    strcpy(priority, "1");
                else if (!strcmp(argv[4], "2"))
                    strcpy(priority, "2");
                else if (!strcmp(argv[4], "3"))
                    strcpy(priority, "3");
                else printf("Invalid arguments for --add command.\nUse --help command for more information.\n");
                sprintf(instruction, "%s\n%s\n%s\n", ADD, argv[2], priority);
            }
        } else if (!strcmp(argv[1], "-l") || !strcmp(argv[1], "--list")){ 
            if (argc > 2){
                printf("Invalid arguments for --list command.\nUse --help command for more information.\n");
                return 0;
            } else { // LIST command
                sprintf(instruction, "%s\n", LIST);
            }
        } else if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")){ 
            if (argc > 2){
                printf("Invalid arguments for --help command.\n");
                return 0;
            } else { // HELP command
                sprintf(instruction, "%s\n", HELP);
            }
        } else if (argc != 3){
            printf("Invalid number of arguments.\nUse --help command for more information.\n");
            return 0;
        } else if (!strcmp(argv[1], "-S") || !strcmp(argv[1], "--suspend")){
            if(!atoi(argv[2]))
                printf ("Invalid task ID.");
            // SUSPEND command
            sprintf(instruction, "%s\n%s\n", SUSPEND, argv[2]);
        } else if (!strcmp(argv[1], "-R") || !strcmp(argv[1], "--resume")){
            if(!atoi(argv[2]))
                printf ("Invalid task ID.");
            // RESUME command
            sprintf(instruction, "%s\n%s\n", RESUME, argv[2]);
        } else if (!strcmp(argv[1], "-r") || !strcmp(argv[1], "--remove")){
            if(!atoi(argv[2]))
                printf ("Invalid task ID.");
            // REMOVE command
            sprintf(instruction, "%s\n%s\n", REMOVE, argv[2]);
        } else if (!strcmp(argv[1], "-i") || !strcmp(argv[1], "--info")){
            if(!atoi(argv[2]))
                printf ("Invalid task ID.");
            // INFO command
            sprintf(instruction, "%s\n%s\n", INFO, argv[2]);
        } else if (!strcmp(argv[1], "-p") || !strcmp(argv[1], "--print")){
            if(!atoi(argv[2]))
                printf ("Invalid task ID.");
            // PRINT command
            sprintf(instruction, "%s\n%s\n", PRINT, argv[2]);
        }
    }
    write_to_daemon(instruction);

    return 0;
}
