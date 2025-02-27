/**************************************************************
* Class::  CSC-415-03 Spring 2025
* Name:: Manci Raj
* Student ID:: 922823535
* GitHub-Name:: manciraj
* Project:: Assignment 3 – Simple Shell with Pipes
*
* File:: Raj_Manci_HW3_main.c
*
* Description:: A simple shell that executes commands with argument parsing
* and supports multiple pipes using fork(), execvp(), and wait().
*
**************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_INPUT 103  // Max input length (as per Required)
#define MAX_ARGS 10     // Max arguments per command
#define MAX_PIPES 5     // Max supported pipes

// Function prototypes
void execute_command(char *input);
void execute_piped_commands(char *commands[], int num_pipes);
void parse_input(char *input, char *commands[], int *num_pipes);

int main(int argc, char *argv[]) {
    char input[MAX_INPUT];  // Input buffer
    char *prompt = (argc > 1) ? argv[1] : "Prompt> ";  // Default/custom prompt

    // Main shell loop
    while (1) {
        printf("%s ", prompt);
        fflush(stdout);

        // Read user input
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\nExiting shell...\n");
            break;
        }

        // Remove trailing newline
        input[strcspn(input, "\n")] = 0;

        // Ignore empty lines
        if (strlen(input) == 0) {
            continue;
        }

        // Check if user wants to exit or not
        if (strcmp(input, "exit") == 0) {
            printf("Exiting shell...\n");
            break;
        }

        // Execute command it will handles both single and piped commands
        execute_command(input);
    }

    return 0;
}

//Function to parse input and determine if pipes exist or not.
 
void parse_input(char *input, char *commands[], int *num_pipes) {
    char *token = strtok(input, "|");
    while (token != NULL) {
        commands[*num_pipes] = token;
        (*num_pipes)++;
        token = strtok(NULL, "|");
    }
    commands[*num_pipes] = NULL;  // NULL terminate the array
}

//Function to execute a single command or handle piped execution.

void execute_command(char *input) {
    char *commands[MAX_PIPES + 1];  // To store individual commands
    int num_pipes = 0;

    parse_input(input, commands, &num_pipes);

    if (num_pipes == 1) {
        // No pipes, execute normally
        char *args[MAX_ARGS + 1];
        int arg_count = 0;

        args[arg_count] = strtok(commands[0], " ");
        while (args[arg_count] != NULL) {
            arg_count++;
            if (arg_count >= MAX_ARGS) break;
            args[arg_count] = strtok(NULL, " ");
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
        } else if (pid == 0) {
            // Child process executes command
            execvp(args[0], args);
            perror("Execution failed");
            exit(1);
        } else {
            // Parent waits for child process
            int status;
            waitpid(pid, &status, 0);
            printf("Child %d exited with %d\n", pid, WEXITSTATUS(status));
        }
    } else {
        // Handle piped commands
        execute_piped_commands(commands, num_pipes);
    }
}

//Function to execute commands with pipes.
void execute_piped_commands(char *commands[], int num_pipes) {
    int pipes[MAX_PIPES][2];
    pid_t pids[MAX_PIPES + 1];

    // Create pipes
    for (int i = 0; i < num_pipes - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("Pipe creation failed");
            return;
        }
    }

    // Create child processes
    for (int i = 0; i < num_pipes; i++) {
        char *args[MAX_ARGS + 1];
        int arg_count = 0;
        args[arg_count] = strtok(commands[i], " ");

        while (args[arg_count] != NULL) {
            arg_count++;
            if (arg_count >= MAX_ARGS) break;
            args[arg_count] = strtok(NULL, " ");
        }

        pids[i] = fork();
        if (pids[i] < 0) {
            perror("Fork failed");
            return;
        } else if (pids[i] == 0) {
            // Child process

            // If not the first command, set up input redirection
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }

            // If not the last command, set up output redirection
            if (i < num_pipes - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            // Close all pipes in child process
            for (int j = 0; j < num_pipes - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Execute the command
            execvp(args[0], args);
            perror("Execution failed");
            exit(1);
        }
    }

    // Close all pipes in the parent
    for (int i = 0; i < num_pipes - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all child processes
    for (int i = 0; i < num_pipes; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        printf("Child %d exited with %d\n", pids[i], WEXITSTATUS(status));
    }
}

