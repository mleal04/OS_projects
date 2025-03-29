#ifndef HELPER_H
#define HELPER_H

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
#define WAIT_THRESHOLD 2 // 10 seconds before a job gets priority

// Job structure definition
typedef struct Job
{
    int job_id;
    char *file_name;
    char *output_file;
    size_t input_size;
    size_t output_size;
    char *submit_time;
    char *start_time;
    char *completion_time;
    char *current_state; // "WAITING", "RUNNING", "COMPLETE"
    int priority;
    struct Job *next;
} Job;

// monitor
extern Job *head;
extern Job *tail;
extern pthread_mutex_t lock;
extern pthread_cond_t cond;
extern pthread_cond_t cond2;
extern pthread_cond_t cond3;
extern int paused_job_id;
extern int changing_priority;
extern char schedule[20];

// globals
time_t curr_time;
struct tm *local_time;

// Function prototypes
void show_help();
void swap(Job *a, Job *b);
void sort_linked_list();
void add_to_list(Job *new_job);
void submit_job(char *input_name, char *output_name, int id, struct tm *local_time);
void print_job_list();
Job *get_job();
void simulate_print(Job *curr_job);
void *find_job(void *arg);
void wait_for_job(int id);
void waitany();
void waitall();
void change_priority(int jobid, int priority);
void remove_job(int job_id);

// Helper functions
void free_job(Job *job); // Add a function to free job memory

#endif