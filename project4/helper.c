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

Job *head = NULL;
Job *tail = NULL;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond3 = PTHREAD_COND_INITIALIZER;
int paused_job_id = -1;     // (-1 we are not paused/blocked)
int changing_priority = -1; // (-1 we are not changing priority)
char schedule[20];

// Function show help fucnction for user
void show_help()
{
    printf("Available commands:\n");
    printf("  submit <file.gcode> <file.png>\n");
    printf("  list\n");
    printf("  wait <jobid>\n");
    printf("  waitany\n");
    printf("  waitall\n");
    printf("  remove <jobid>\n");
    printf("  setpriority <jobid> <priority>\n");
    printf("  schedule <fifo|sjf|priority|balanced>\n");
    printf("  quit\n");
    printf("  help\n");
}

// Swap function helper function for jobs and scheduler
void swap(Job *a, Job *b)
{
    // Swap all attributes of jobs except the next pointer
    int temp_id = a->job_id;
    char *temp_state = a->current_state;
    char *temp_file = a->file_name;
    char *temp_output = a->output_file;
    size_t temp_size = a->input_size;
    size_t temp_size2 = a->output_size;
    int temp_p = a->priority;
    char temp_time[20];
    strcpy(temp_time, a->submit_time);
    char temp_time2[20];
    strcpy(temp_time2, a->start_time);
    char temp_time3[20];
    strcpy(temp_time3, a->completion_time);

    // Copy b's data into a
    a->job_id = b->job_id;
    a->current_state = b->current_state;
    a->file_name = b->file_name;
    a->output_file = b->output_file;
    a->input_size = b->input_size;
    a->output_size = b->output_size;
    a->priority = b->priority;
    strcpy(a->submit_time, b->submit_time);
    strcpy(a->start_time, b->start_time);
    strcpy(a->completion_time, b->completion_time);

    // Copy a's old data into b
    b->job_id = temp_id;
    b->current_state = temp_state;
    b->file_name = temp_file;
    b->output_file = temp_output;
    b->input_size = temp_size;
    b->output_size = temp_size2;
    b->priority = temp_p;
    strcpy(b->submit_time, temp_time);
    strcpy(b->start_time, temp_time2);
    strcpy(a->completion_time, temp_time3);
    // strcpy(b->completion_time, temp_time3);  // FIXED: Copy to b instead of a
}

// Sort linked list useful for scheduler
void sort_linked_list()
{
    if (strncmp(schedule, "fcfs", 4) == 0) // default assign fcfs
    {
        // fcfs
        if (head == NULL)
            return;
        Job *i, *j;
        for (i = head; i != NULL; i = i->next)
        {
            for (j = i->next; j != NULL; j = j->next)
            {
                if (i->submit_time > j->submit_time)
                {
                    swap(i, j); // Swap if priority of i is greater than j
                }
            }
        }
    }
    else if (strncmp(schedule, "sjf", 3) == 0) // default assign sjf
    {
        // sjf
        if (head == NULL)
            return;
        Job *i, *j;
        for (i = head; i != NULL; i = i->next)
        {
            for (j = i->next; j != NULL; j = j->next)
            {
                if (i->input_size > j->input_size)
                {
                    swap(i, j); // Swap if duration of i is greater than j
                }
            }
        }
    }
    else if (strncmp(schedule, "balance", 3) == 0) // default balamnce
    {
        // balance
        Job *i, *j;
        time_t now = time(NULL); // Current time in seconds
        for (i = head; i != NULL; i = i->next)
        {
            for (j = i->next; j != NULL; j = j->next)
            {
                // Convert submit_time (HH:MM:SS) to seconds
                struct tm tm_i = {0};
                struct tm tm_j = {0};

                strptime(i->submit_time, "%H:%M:%S", &tm_i);
                strptime(j->submit_time, "%H:%M:%S", &tm_j);

                time_t submit_time_i = mktime(&tm_i);
                time_t submit_time_j = mktime(&tm_j);

                // Calculate wait time for each job
                int i_wait_time = (int)difftime(now, submit_time_i);
                int j_wait_time = (int)difftime(now, submit_time_j);

                // Sort based on wait time priority or SJF
                if ((i->input_size > j->input_size) ||
                    (i_wait_time >= WAIT_THRESHOLD && j_wait_time < WAIT_THRESHOLD) ||
                    (i_wait_time < j_wait_time && i_wait_time >= WAIT_THRESHOLD))
                {
                    swap(i, j);
                }
            }
        }
    }
}

// Function to add job to our queue
void add_to_list(Job *new_job)
{
    pthread_mutex_lock(&lock);
    new_job->next = NULL;
    if (tail == NULL)
    {
        head = new_job;
    }
    else
    {
        tail->next = new_job;
    }
    tail = new_job;

    sort_linked_list();
    // Wake up all threads so they can compete for jobs
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&lock);
}

// Function to sumbit job to the queue
void submit_job(char *input_name, char *output_name, int id, struct tm *local_time)
{
    // create a job object to be filled in and returned to the list
    Job *new_job = malloc(sizeof(Job));
    if (!new_job)
    {
        perror("Failed to allocate memory for new job");
        return;
    }
    // assign items
    new_job->job_id = id;
    new_job->file_name = strdup(input_name);
    new_job->output_file = strdup(output_name);
    new_job->current_state = strdup("WAITING");
    new_job->priority = 1;
    new_job->next = NULL;

    // assign size
    char path[BUFSIZ];
    snprintf(path, sizeof(path), "./examples/%s", new_job->file_name);
    printf("%s\n", path);
    struct stat file_stat;
    if (stat(path, &file_stat) == 0)
    {
        new_job->input_size = file_stat.st_size; // Set input size
    }
    else
    {
        new_job->input_size = 0; // If the file cannot be accessed, set size to 0
    }
    new_job->output_size = 0;
    // get time
    char buffer[20]; // To store formatted time as HH:MM:SS(change this from 9 to 20)
    strftime(buffer, sizeof(buffer), "%H:%M:%S", local_time);
    new_job->submit_time = strdup(buffer); // Copy the formatted time string to the job
    new_job->start_time = strdup("0");
    new_job->completion_time = strdup("0");
    // add to list
    add_to_list(new_job);
}

// Print job list with response time and competion time in an organize time
void print_job_list()
{
    pthread_mutex_lock(&lock);
    printf("\nCurrent Job List:\n");
    printf("--------------------------------------------------------------------------------------------------------------------\n");
    printf("| ID | State     | File Name       | Input Size | Output Size | Submit Time | Start Time | End Time   | Priority |\n");
    printf("--------------------------------------------------------------------------------------------------------------------\n");

    Job *curr = head;
    int completed_jobs = 0;
    double total_turnaround = 0.0;
    double total_response = 0.0;

    while (curr != NULL)
    {
        printf("| %-2d | %-9s | %-15s | %-10ld | %-11ld | %-11s | %-10s | %-10s | %-8d |\n",
               curr->job_id, curr->current_state, curr->file_name,
               curr->input_size, curr->output_size, curr->submit_time,
               curr->start_time, curr->completion_time, curr->priority);

        // Only calculate for properly completed jobs
        if (strcmp(curr->current_state, "COMPLETE") == 0 &&
            strcmp(curr->completion_time, "0") != 0 &&
            strcmp(curr->start_time, "0") != 0)
        {

            struct tm tm_submit = {0}, tm_start = {0}, tm_complete = {0};
            time_t submit, start, complete;

            if (strptime(curr->submit_time, "%H:%M:%S", &tm_submit) &&
                strptime(curr->start_time, "%H:%M:%S", &tm_start) &&
                strptime(curr->completion_time, "%H:%M:%S", &tm_complete))
            {

                submit = mktime(&tm_submit);
                start = mktime(&tm_start);
                complete = mktime(&tm_complete);

                double response = difftime(start, submit);      // get response time
                double turnaround = difftime(complete, submit); // get turnaround time

                if (response >= 0 && turnaround >= 0)
                { // Only count valid times
                    total_response += response;
                    total_turnaround += turnaround;
                    completed_jobs++;
                }
            }
        }

        curr = curr->next;
    }
    printf("--------------------------------------------------------------------------------------------------------------------\n");

    if (completed_jobs > 0)
    {
        printf("\nStatistics for completed jobs:\n");
        printf("Average Response Time: %.2f seconds\n", total_response / completed_jobs);
        printf("Average Turnaround Time: %.2f seconds\n", total_turnaround / completed_jobs);
    }
    else
    {
        printf("\nNo completed jobs with valid timestamps to calculate statistics.\n");
    }

    pthread_mutex_unlock(&lock);
}

// Get a job function

Job *get_job()
{
    pthread_mutex_lock(&lock);

    while (head == NULL)
    {
        pthread_cond_wait(&cond, &lock);
    }

    // If we are pausing for a specific job, block other jobs from running
    Job *curr = head;

    while (curr != NULL)
    {
        if (paused_job_id == -1 || curr->job_id == paused_job_id || changing_priority == -1)
        {
            if (strcmp(curr->current_state, "WAITING") == 0)
            {
                // Mark the job as "RUNNING"
                free(curr->current_state);
                curr->current_state = strdup("RUNNING");

                pthread_mutex_unlock(&lock);
                return curr;
            }
        }
        curr = curr->next;
    }

    pthread_mutex_unlock(&lock);
    return NULL; // No available job
}

// Function to print and get main
void simulate_print(Job *curr_job)
{
    pid_t pid = fork(); // fork function
    if (pid < 0)
    {
        perror("Fork Failed\n");
    }
    else if (pid == 0) // let child work --> must go back
    {
        char arg1[BUFSIZ] = "./gcode2png";
        char arg2[BUFSIZ];
        char arg3[BUFSIZ];

        snprintf(arg2, sizeof(arg2), "examples/%s", curr_job->file_name);
        snprintf(arg3, sizeof(arg3), "--output=%s", curr_job->output_file);

        char *args[] = {arg1, arg2, arg3, NULL};

        execvp(args[0], args);
        perror("execvp failed");
        exit(EXIT_FAILURE);
    }
    else // parent must pick up
    {
        int status;
        waitpid(pid, &status, 0);
    }
}

// Print job simulation function(showing the threads )
void *find_job(void *arg)
{
    while (1)
    {
        Job *curr_job = get_job();
        if (curr_job == NULL)
        {
            continue;
        }

        // Simulate printing the job
        printf("Thread %ld running job %d\n", pthread_self(), curr_job->job_id);
        // print_job_list(); // debugger

        // send the print job actually
        simulate_print(curr_job);
        curr_time = time(NULL);
        local_time = localtime(&curr_time);
        char buffer[9]; // To store formatted time as HH:MM:SS
        strftime(buffer, sizeof(buffer), "%H:%M:%S", local_time);

        // Mark job as complete
        pthread_mutex_lock(&lock);
        free(curr_job->current_state);
        free(curr_job->start_time); // added this to free memory
        curr_job->start_time = strdup(buffer);
        curr_job->current_state = strdup("COMPLETE"); // added this to free memory
        // sleep(3);
        //  grab final time
        curr_time = time(NULL);
        local_time = localtime(&curr_time);
        char buffer2[9]; // To store formatted time as HH:MM:SS
        strftime(buffer2, sizeof(buffer2), "%H:%M:%S", local_time);
        free(curr_job->completion_time);
        curr_job->completion_time = strdup(buffer2);
        // grab output fize
        char path[BUFSIZ];
        snprintf(path, sizeof(path), "./%s", curr_job->output_file);
        printf("%s\n", path);
        struct stat file_stat;
        if (stat(path, &file_stat) == 0)
        {
            curr_job->output_size = file_stat.st_size; // Set input size
        }
        else
        {
            curr_job->output_size = 0; // If the file cannot be accessed, set size to 0
        }
        pthread_cond_signal(&cond2);
        printf("Thread %ld completed job %d\n", pthread_self(), curr_job->job_id);
        pthread_mutex_unlock(&lock);
    }
}

// First wait for speific job with id
void wait_for_job(int id)
{
    pthread_mutex_lock(&lock);

    // Find the job
    Job *job = head;
    while (job && job->job_id != id)
        job = job->next;

    if (!job)
    {
        printf("Job %d not found.\n", id);
        pthread_mutex_unlock(&lock);
        return;
    }

    // Block all threads except the one running the specified job
    paused_job_id = id;

    // Wait until the job is complete
    while (strcmp(job->current_state, "COMPLETE") != 0)
    {
        pthread_cond_wait(&cond2, &lock);
    }

    // Allow other threads to resume
    paused_job_id = -1;

    // Notify the other threads that they can proceed now
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&lock);
}

// function waitany just find the first job that its in queue and waiting
void waitany()
{
    pthread_mutex_lock(&lock);

    // Find the first job that is in "WAITING" state
    Job *job = head;
    while (job && strcmp(job->current_state, "WAITING") != 0)
        job = job->next;

    if (!job)
    {
        printf("No job in WAITING state found.\n");
        pthread_mutex_unlock(&lock);
        return;
    }

    // Block all threads except the one running this job
    paused_job_id = job->job_id;

    // Wait until the job is complete
    while (strcmp(job->current_state, "COMPLETE") != 0)
    {
        pthread_cond_wait(&cond2, &lock);
    }

    // Allow other threads to resume
    paused_job_id = -1;

    // Notify the other threads that they can proceed now
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&lock);
}

// Function to go thorugh all waiting jobs and execute them
void waitall()
{
    pthread_mutex_lock(&lock);

    while (1)
    {
        Job *job = head;
        while (job && strcmp(job->current_state, "WAITING") != 0)
            job = job->next;

        // if we dont have any more job because we have gone through all the linke dlist --> we are done
        if (!job)
        {
            printf("all jobs processed, submit more\n");
            pthread_mutex_unlock(&lock);
            return;
        }
        // Block all threads except the one running this job
        paused_job_id = job->job_id;

        // Wait until the job is complete
        while (strcmp(job->current_state, "COMPLETE") != 0)
        {
            pthread_cond_wait(&cond2, &lock);
        }
        // Allow other threads to resume
        paused_job_id = -1;
    }

    pthread_mutex_unlock(&lock);
}

// Change the priority
void change_priority(int jobid, int priority)
{
    // accesing linkedlist which is our critical resource
    pthread_mutex_lock(&lock);
    // Find the job
    Job *job = head;
    while (job && job->job_id != jobid)
        job = job->next;

    if (!job)
    {
        printf("Job %d not found.\n", jobid);
        pthread_mutex_unlock(&lock);
        return;
    }

    // Block all threads except the one running the specified job
    changing_priority = jobid;
    // chnage priority
    job->priority = priority;
    // priority
    if (head == NULL)
        return;
    Job *i, *j;
    for (i = head; i != NULL; i = i->next)
    {
        for (j = i->next; j != NULL; j = j->next)
        {
            if (i->priority < j->priority)
            {
                swap(i, j); // Swap if priority of i is greater than j
            }
        }
    }
    // Allow other threads to resume
    paused_job_id = -1;
    // Notify the other threads that they can proceed now
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&lock);
}

// Function to remove jobs from the linked list
void remove_job(int job_id)
{
    pthread_mutex_lock(&lock);

    Job *job = head;
    Job *prev = NULL;

    // Find the job node
    while (job && job->job_id != job_id)
    {
        prev = job;
        job = job->next;
    }

    // If the job was not found
    if (!job)
    {
        printf("Job not found.\n");
        pthread_mutex_unlock(&lock);
        return;
    }

    // If the job is the head node, update head
    if (job == head)
    {
        head = job->next;
    }
    else
    {
        prev->next = job->next;
    }

    // Free the job node
    free(job);

    // Resort the list
    sort_linked_list();

    pthread_mutex_unlock(&lock);
}
