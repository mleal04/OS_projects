/*
Skeleton code for solving the readers-writers problem using a monitor.
Complete the solution by adding any necessary global variables,
and implement the functions read_lock, write_lock, etc.
Carefully examine the output generated to be sure that you are only
admitting multiple readers at once or a single writer at once.
*/

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define NFORKS 5
int sleeptime = 1000000; /* microseconds */

// globals to take care of
int forks[NFORKS] = {0};

// Create monitor
typedef struct
{
    pthread_mutex_t lock;
    pthread_cond_t can_use_forks;
} monitor;

// Start monitor
monitor my_monitor = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};

void pickup(int id)
{
    pthread_mutex_lock(&my_monitor.lock);
    int left = id;                 // pick up left fork (itself)
    int right = (id + 1) % NFORKS; // pick up right fork (edge case for last one)

    // fix deadlock: last philosopher (left = next one) (right = itself)
    if (id == NFORKS - 1)
    {
        int temp = left;
        left = right;
        right = temp;
    }

    while (forks[left] || forks[right])
    {
        pthread_cond_wait(&my_monitor.can_use_forks, &my_monitor.lock);
    }

    forks[left] = 1;
    forks[right] = 1;

    pthread_mutex_unlock(&my_monitor.lock);
}

void putdown(int id)
{
    pthread_mutex_lock(&my_monitor.lock);

    int left = id;
    int right = (id + 1) % NFORKS;

    // Make sure to reverse fork order for last philosopher
    if (id == NFORKS - 1)
    {
        int temp = left;
        left = right;
        right = temp;
    }

    forks[left] = 0;
    forks[right] = 0;

    pthread_cond_broadcast(&my_monitor.can_use_forks);
    pthread_mutex_unlock(&my_monitor.lock);
}

void *philosopher_thread(void *arg)
{
    int *id_pointer = (int *)arg;
    int id = *id_pointer;

    while (1)
    {
        printf("thread %d sleeping\n", id);
        usleep(rand() % sleeptime);
        printf("thread %d hungry\n", id);
        // philospher is ready to eat
        pickup(id);
        // philosopher is eating
        printf("thread %d eating\n", id);
        usleep(rand() % sleeptime);
        // philosopher done eating
        putdown(id);
    }
}

int main(int argc, char *argv[])
{
    pthread_t threadid[NFORKS];
    // philosopher ids
    int id[NFORKS];
    int i;

    for (i = 0; i < NFORKS; i++)
    {
        id[i] = i;
        pthread_create(&threadid[i], NULL, philosopher_thread, &id[i]);
    }

    for (int i = 0; i < NFORKS; i++)
    {
        pthread_join(threadid[i], NULL);
    }

    return 0;
}
