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

#define MAX_PID_LENGTH 10 //de mutat in constants

int daemon_pid;

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
        return errno;
    }

    char buf[MAX_PID_LENGTH];
    ssize_t bytesRead = read(fd, buf, MAX_PID_LENGTH);
    close(fd);

    if (bytesRead <= 0) {
        perror("Error reading daemon pid");
        return -1;
    }

    daemon_pid=atoi(buf);
}

void read_resoults_from_daemon(int signo) {
    char* res = malloc(1000000);
    daemon_message("Dupa primeste marime fisier\n");
    FILE* file = fopen(output_file_path, "r");
    if (file == NULL) {
        perror("Couldn't open daemon output file");
        return;
    }
    daemon_message("Inainte de malloc\n");

    daemon_message("Dupa malloc\n");
    fread(res, 1, 1000000, file);
    fclose(file);
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
    signal(SIGUSR2, read_resoults_from_daemon);
    write_da_pid();
    read_daemon_pid();

    char instruction[INSTR_LENGTH];
    if (argc == 1) {
        printf("No arguments found. Please introduce an analyze-option and a desired path.\n");
        printf("Use --help command for more information.\n");
        return EXIT_FAILURE;
    } else if (check_valid_option(argv[1]) == false) {
        printf("No such option, please choose a valid option.\nUse --help command for more information.\n");
        return EXIT_FAILURE;
    } else if (argc == 2 && is_list_help_command(argv[1]) == false) {
        printf("No task-id found. Please select an existing task-id.\n");
        return EXIT_FAILURE;
    } else if (argc >= 4 && (is_add_command(argv[1]) && (!is_priority_option(argv[3])))) {
        printf("Cannot set priority for a nonexistent analysis task. Please use the -a or --add option.\n");
        return EXIT_FAILURE;
    } else {
        if (is_add_command(argv[1])) {
            if (argc != 5 || (argc == 5 && !is_priority_option(argv[3]))) {
                printf("Invalid arguments for --add command.\nUse --help command for more information.\n");
                return EXIT_FAILURE;
            } else {
                char priority[7];
                if (validate_priority_option(argv[4], priority)) {
                    sprintf(instruction, "%s\n%s\n%s\n", ADD, argv[2], priority);
                } else {
                    printf("Invalid arguments for --add command.\nUse --help command for more information.\n");
                    return EXIT_FAILURE;
                }
            }
        } else if (is_list_command(argv[1])) {
            if (argc > 2) {
                printf("Invalid arguments for --list command.\nUse --help command for more information.\n");
                return EXIT_FAILURE;
            } else {
                sprintf(instruction, "%s\n", LIST);
            }
        } else if (is_help_command(argv[1])) {
            if (argc > 2) {
                printf("Invalid arguments for --help command.\n");
                return EXIT_FAILURE;
            } else {
                sprintf(instruction, "%s\n", HELP);
            }
        } else if (argc != 3) {
            printf("Invalid number of arguments.\nUse --help command for more information.\n");
            return EXIT_FAILURE;
        } else if (is_suspend_command(argv[1])) {
            if (!is_valid_task_id(argv[2])) {
                printf("Invalid task ID.\n");
                return EXIT_FAILURE;
            }
            sprintf(instruction, "%s\n%s\n", SUSPEND, argv[2]);
        } else if (is_resume_command(argv[1])) {
            if (!is_valid_task_id(argv[2])) {
                printf("Invalid task ID.\n");
                return EXIT_FAILURE;
            }
            sprintf(instruction, "%s\n%s\n", RESUME, argv[2]);
        } else if (is_remove_command(argv[1])) {
            if (!is_valid_task_id(argv[2])) {
                printf("Invalid task ID.\n");
                return EXIT_FAILURE;
            }
            sprintf(instruction, "%s\n%s\n", REMOVE, argv[2]);
        } else if (is_info_command(argv[1])) {
            if (!is_valid_task_id(argv[2])) {
                printf("Invalid task ID.\n");
                return EXIT_FAILURE;
            }
            sprintf(instruction, "%s\n%s\n", INFO, argv[2]);
        } else if (is_print_command(argv[1])) {
            if (!is_valid_task_id(argv[2])) {
                printf("Invalid task ID.\n");
                return EXIT_FAILURE;
            }
            sprintf(instruction, "%s\n%s\n", PRINT, argv[2]);
        }
    }

    write_to_daemon(instruction);

    return EXIT_SUCCESS;
}

