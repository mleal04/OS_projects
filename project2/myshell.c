#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>    // For perror() and errno
#include <sys/wait.h> // For wait()
#include "dirwatch.h"
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>

// printing errors
void errors(int status, pid_t child_pid)
{
    // we have terminated this child
    printf("myshell: process %d exited ", child_pid);
    if (WIFEXITED(status))
    {
        // normal exit
        printf("normally with status %d\n", WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status))
    {
        // abnormal termination
        int signal_num = WTERMSIG(status);
        printf("abnormally with signal %d (%s)\n", signal_num, strsignal(signal_num));
    }
    else
    {
        printf("Unknown exit status.\n");
    }
}

// pass in arguments + return pid
int start_command(char *args[BUFSIZ], int flag)
{
    // Create child process
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork failed");
        return 1;
    }

    if (pid == 0)
    {
        // child process begin to execute
        if (execvp(args[0], args) < 0)
        {
            printf("myshell: unknown command: %s\n", args[0]);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        // parent process begin to execute
        if (flag == 1)
        {
            printf("myshell: process %d started\n", pid);
        }
        else if (flag == 5)
        {
            return pid;
        }

        fflush(stdout); // to let parent print the child stuff
        usleep(50000);  // make the paren process wait a bit until start again
    }
    return pid;
}

// pass in pid
int wait_for_command(pid_t process)
{
    int status;
    pid_t child_pid = waitpid(process, &status, 0); // Wait for any child with -1
    // that one doesnt exits
    if (child_pid == -1)
    {
        if (errno == ECHILD)
        {
            return -1;
        }
        else
        {
            perror("waitpid failed");
        }
    }
    errors(status, child_pid);
    return 0;
}

int main(int argc, char *argv[])
{
    // allow stdin to be from --> FILE or user-input
    FILE *user_input = stdin;
    // check if we have a file
    if (argc > 1)
    {
        user_input = fopen(argv[1], "r");
        if (user_input == NULL)
        {
            printf("myshell: file cannot be open\n");
            return 1;
        }
    }
    // if we dont have file --> still just stdin
    char given_d[BUFSIZ] = "./"; // Make it a mutable array for directory path
    while (1)
    {
        printf("myshell> ");
        char input_p[BUFSIZ];
        // open file or stdin --> check if we are done
        if (fgets(input_p, sizeof(input_p), user_input) == NULL)
        {
            if (user_input != stdin)
            { // we read a file
                fclose(user_input);
            }
            break;
        }
        // remove null from the input
        input_p[strlen(input_p) - 1] = 0;
        // get the program
        char *token;
        token = strtok(input_p, " ");

        if (token == NULL)
        {
            continue;
        }

        // check programs --> list
        if (strcmp(token, "list") == 0)
        {
            token = strtok(NULL, " "); // check to see if we have to change the directory
            if (token == NULL)
            {
                dirwatch_main_call(given_d); // Use current directory
            }
            else // we have to change directory
            {
                dirwatch_main_call(token); // Use new directory
            }
        }
        // check programs --> chdir
        else if (strcmp(token, "chdir") == 0)
        {
            token = strtok(NULL, " ");
            if (token == NULL)
            {
                printf("myshell: no directory entered\n");
                continue;
            }
            else
            {
                if (chdir(token) == 0) // Change the directory
                {
                    if (getcwd(given_d, sizeof(given_d)) == NULL)
                    {
                        perror("getcwd failed");
                    }
                    printf("Changed directory to: %s\n", given_d);
                }
                else
                {
                    perror("chdir failed");
                }
            }
        }
        // check programs --> pwd
        else if (strcmp(token, "pwd") == 0)
        {
            char cwd[BUFSIZ];
            if (getcwd(cwd, sizeof(cwd)) != NULL)
            {
                printf("%s\n", cwd);
            }
            else
            {
                perror("getwd failed");
            }
        } // check programs --> start (new prog)
        else if (strcmp(token, "start") == 0)
        {
            char *args[BUFSIZ];
            int index = 0;
            // extract the first token --> new program
            char *token = strtok(NULL, " ");
            if (token == NULL)
            { // no new program entered
                printf("myshell: no new program entered\n");
                continue;
            }
            args[index++] = token;
            // extract additional arguments
            while ((token = strtok(NULL, " ")) != NULL)
            {
                args[index++] = token;
            }
            // NULL-terminate the argument list
            args[index] = NULL;
            start_command(args, 1);

        } // check programs --> wait (The wait command causes the shell to wait for any child process to exit)
        else if (strcmp(token, "wait") == 0)
        {
            int status;
            pid_t child_pid = waitpid(-1, &status, 0); // Wait for any child with -1
            // no child process exist (At all)
            if (child_pid == -1)
            {
                if (errno == ECHILD)
                {
                    printf("myshell: No children\n");
                    continue;
                }
                else
                {
                    perror("waitpid failed");
                }
            }
            errors(status, child_pid);
        } // check programs --> waitfor ()
        else if (strcmp(token, "waitfor") == 0)
        {
            token = strtok(NULL, " ");
            if (token == NULL)
            {
                printf("myshell: no process entered\n");
                continue;
            }
            pid_t process = atoi(token); // pid
            if (wait_for_command(process) == -1)
            {
                printf("myshell: No such process\n");
                continue;
            }

        } // check programs --> kill
        else if (strcmp(token, "kill") == 0)
        {
            char *token = strtok(NULL, " "); // this should be the process pid as a string
            if (token == NULL)
            {
                printf("myshell: did not enter a process PID\n");
                continue;
            }
            pid_t pid = atoi(token); // process pid as an int
            if (kill(pid, SIGKILL) < 0)
            {
                printf("myshell: No such process.\n");
                continue;
            }
            else
            {
                wait_for_command(pid);
            }
        }
        else if (strcmp(token, "run") == 0) // needs to run start + need to run waitfor
        {
            char *args[BUFSIZ];
            int index = 0;
            // extract the first token --> new program
            char *token = strtok(NULL, " ");
            if (token == NULL)
            { // no new program entered
                printf("myshell: no new program entered\n");
                continue;
            }
            args[index++] = token;
            // extract additional arguments
            while ((token = strtok(NULL, " ")) != NULL)
            {
                args[index++] = token;
            }
            // NULL-terminate the argument list
            args[index] = NULL;
            pid_t process = start_command(args, 2);
            if (wait_for_command(process) == -1)
            {
                printf("myshell: No such process\n");
                continue;
            }
        }
        else if (strcmp(token, "array") == 0)
        {
            // get the number count
            char *token = strtok(NULL, " ");
            if (token == NULL)
            {
                printf("myshell: incorrect input\n");
                continue;
            }
            int counts = atoi(token);
            char *args[BUFSIZ]; // to store commands
            int index = 0;      // to index the arguments
            int flag = 0;

            // extract command
            while ((token = strtok(NULL, " ")) != NULL)
            {
                if (strcmp(token, "@") == 0)
                {
                    flag = 5;
                    continue;
                }
                args[index++] = token;
            }
            args[index] = NULL; // NULL-terminate the argument list

            if (index == 0) // need at least ONE command
            {
                printf("arguments are missing\n");
                continue;
            }

            // adjust the end command if we have a @
            for (int i = 0; i < counts; i++)
            {
                if (flag == 5) // change the very last command
                {
                    args[index] = malloc(20);
                    if (args[index] == NULL)
                    {
                        perror("malloc failed");
                        exit(1);
                    }
                    // clear the buffer
                    memset(args[index], 0, 20);
                    // concat string
                    sprintf(args[index], "%s %d", args[index], i + 1);
                }

                start_command(args, 5); // send to exec to start new program
            }
            while (1)
            {
                int status;
                pid_t child_pid = waitpid(-1, &status, 0);
                // no child process exist (At all)
                if (child_pid == -1)
                {
                    if (errno == ECHILD)
                    {
                        // printf("myshell: No children\n");
                        break;
                    }
                    else
                    {
                        perror("waitpid failed");
                    }
                }
                errors(status, child_pid);
            }
        }
        else if ((strcmp(token, "exit") == 0) || (strcmp(token, "quit") == 0))
        {
            return 0;
        }
        else
        {
            printf("myshell: unknown command: %s\n", token);
            continue;
        }
    }
    return 0;
}
