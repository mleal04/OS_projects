#define _XOPEN_SOURCE 700
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <time.h>
#include "helper.h"
#define WAIT_THRESHOLD 2 // 10 seconds before a job gets priority

// Main Function
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("usage: ./printsched #\n");
        return 1;
    }

    // update schedule default
    strcpy(schedule, "fcfs");

    int job_id = 0;
    int num_printers = atoi(argv[1]);
    pthread_t printers[num_printers];

    for (int i = 0; i < num_printers; i++)
    {
        pthread_create(&printers[i], NULL, find_job, NULL);
    }

    char input[BUFSIZ];
    while (1)
    {
        printf("> ");   // Show prompt
        fflush(stdout); // Ensure prompt is displayed immediately

        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            break; // Exit on EOF (Ctrl+D)
        }
        // sleep(4);

        input[strcspn(input, "\n")] = 0; // Remove newline

        if (strlen(input) == 0)
        {
            continue; // Skip empty input
        }

        char *token = strtok(input, " ");
        if (token == NULL)
        {
            printf("Invalid command. Type 'help' for available commands.\n");
            continue;
        }

        if (strcmp(token, "submit") == 0)
        {
            char input_name[BUFSIZ] = {0};
            char output_name[BUFSIZ] = {0};

            token = strtok(NULL, " ");
            if (token)
            {
                strcpy(input_name, token);
            }
            else
            {
                printf("Invalid usage. Correct usage: submit <file.gcode> <file.png>\n");
                continue;
            }

            token = strtok(NULL, " ");
            if (token)
            {
                strcpy(output_name, token);
            }
            else
            {
                printf("Invalid usage. Correct usage: submit <file.gcode> <file.png>\n");
                continue;
            }

            job_id++;
            curr_time = time(NULL);
            local_time = localtime(&curr_time);
            submit_job(input_name, output_name, job_id, local_time);
        }
        else if (strcmp(token, "wait") == 0) // wait function
        {
            token = strtok(NULL, " ");
            if (token)
            {
                wait_for_job(atoi(token));
            }
            else
            {
                printf("Invalid usage. Correct usage: wait <jobid>\n");
            }
        }
        else if (strcmp(token, "waitany") == 0) // waitany function
        {
            waitany();
        }
        else if (strcmp(token, "waitall") == 0) // waitall function
        {
            waitall();
        }
        else if (strcmp(token, "schedule") == 0)
        {
            token = strtok(NULL, " ");
            if (token == NULL)
            {
                printf("Invalid usage. Correct usage: schedule <fcfs|sjf|priority|balance>\n");
                continue;
            }

            if (strcmp(token, "fcfs") == 0)
            {
                strcpy(schedule, "fcfs");
                printf("New scheduling policy: %s\n", schedule);
            }
            else if (strcmp(token, "sjf") == 0)
            {
                strcpy(schedule, "sjf");
                printf("New scheduling policy: %s\n", schedule);
            }
            else if (strcmp(token, "priority") == 0)
            {
                strcpy(schedule, "priority");
                printf("New scheduling policy: %s\n", schedule);
            }
            else if (strcmp(token, "balance") == 0)
            {
                strcpy(schedule, "balance");
                printf("New scheduling policy: %s\n", schedule);
            }
            else
            {
                printf("Invalid scheduling policy. Available options: fcfs, sjf, priority, balance\n");
            }
        }
        else if (strcmp(token, "setpriority") == 0)
        {
            token = strtok(NULL, " ");
            if (token == NULL)
            {
                printf("Invalid usage. Correct usage: setpriority <jobid> <priority>\n");
                continue;
            }
            int jobid = atoi(token);

            token = strtok(NULL, " ");
            if (token == NULL)
            {
                printf("Invalid usage. Correct usage: setpriority <jobid> <priority>\n");
                continue;
            }
            int priority = atoi(token);

            change_priority(jobid, priority);
        }
        else if (strcmp(token, "remove") == 0)
        {
            token = strtok(NULL, " ");
            if (token)
            {
                int jobid = atoi(token);
                remove_job(jobid);
            }
            else
            {
                printf("Invalid usage. Correct usage: remove <jobid>\n");
            }
        }
        else if (strcmp(token, "list") == 0)
        {
            print_job_list();
        }
        else if (strcmp(token, "quit") == 0)
        {
            break;
        }
        else if (strcmp(token, "help") == 0)
        {
            show_help();
        }
        else
        {
            printf("Invalid command '%s'. Type 'help' for available commands.\n", token);
        }
    }

    return 0;
}