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
#define VALID_OPTIONS_COUNT 6
const char *VALID_OPTIONS[VALID_OPTIONS_COUNT] = {"--add", "--list", "--help", "--suspend", "--resume"};
int daemon_pid;

bool is_add_command(const char *command) {
    return (strcmp(command, "--add") == 0);
}

bool is_priority_option(const char *option) {
    // Assuming a valid priority option is a non-empty string containing only digits
    if (option == NULL || option[0] == '\0') {
        return false;  // Empty string is not a valid priority option
    }

    for (size_t i = 0; option[i] != '\0'; ++i) {
        if (!isdigit(option[i])) {
            return false;  // Non-digit character found
        }
    }

    return true;  // All characters are digits, indicating a valid priority option
}

bool check_valid_option(const char *option) {
    for (int i = 0; i < VALID_OPTIONS_COUNT; ++i) {
        if (strcmp(option, VALID_OPTIONS[i]) == 0) {
            return true;  // Valid option found
        }
    }
    return false;  // Option not found in the valid options
}

bool is_list_help_command(const char *command) {
    return (strcmp(command, "--list") == 0 || strcmp(command, "--help") == 0);
}

bool validate_priority_option(const char *option, char *priority) {
    // Assuming valid priorities are integers in the range 1-5
    int priorityValue = atoi(option);

    if (priorityValue >= 1 && priorityValue <= 5) {
        // Valid priority, copy it to the output buffer (e.g., priority)
        snprintf(priority, 7, "%d", priorityValue);
        return true;
    } else {
        // Invalid priority
        return false;
    }
}

bool is_list_command(const char *command) {
    return (strcmp(command, "--list") == 0);
}

bool is_help_command(const char *command) {
    return (strcmp(command, "--help") == 0);
}

bool is_suspend_command(const char *command) {
    return (strcmp(command, "--suspend") == 0);
}

bool is_remove_command(const char *command) {
    return (strcmp(command, "--remove") == 0);
}

bool is_info_command(const char *command) {
    return (strcmp(command, "--info") == 0);
}

bool is_print_command(const char *command) {
    return (strcmp(command, "--print") == 0);
}

bool is_valid_task_id(const char *task_id) {
    // Assuming a valid task ID is a non-empty string containing only digits
    if (task_id == NULL || task_id[0] == '\0') {
        return false;  // Empty string is not a valid task ID
    }

    for (size_t i = 0; task_id[i] != '\0'; ++i) {
        if (!isdigit(task_id[i])) {
            return false;  // Non-digit character found
        }
    }

    return true;  // All characters are digits, indicating a valid task ID
}

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

void read_resoults_from_daemon(int signo) {
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

