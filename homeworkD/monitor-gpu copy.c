/*
A skeleton for solving the GPU Dataset problem.
Ensure that (up to) four threads may use the GPU at once,
but only if they are operating on the same dataset.
Add whatever global variables are needed, and then implement
gpu_enter() and gpu_exit().
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int nthreads = 100;
int sleeptime = 1000000;

// globals to take care of
int curr_dataset = -1;
int curr_active_threads = 0;

// create the monitor
typedef struct
{
    pthread_mutex_t lock;        // lock criticals
    pthread_cond_t can_goto_gpu; // check if we can use gpu
} monitor;

// start the monitor
monitor my_monitor = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};

void gpu_enter(int dataset)
{
    pthread_mutex_lock(&my_monitor.lock);
    if (curr_dataset == -1 || curr_active_threads == 0)
    {
        curr_dataset = dataset;
    }
    while (curr_active_threads >= 4 || curr_dataset != dataset)
    {
        pthread_cond_wait(&my_monitor.can_goto_gpu, &my_monitor.lock);
    }
    curr_active_threads++;
    pthread_mutex_unlock(&my_monitor.lock);
}

void gpu_exit()
{
    pthread_mutex_lock(&my_monitor.lock);
    curr_active_threads--;
    pthread_cond_broadcast(&my_monitor.can_goto_gpu);
    pthread_mutex_unlock(&my_monitor.lock);
}

void *user_thread(void *arg)
{
    while (1)
    {
        int d = rand() % 10;
        printf("thread %lu wants to use gpu with dataset %d\n", (unsigned long)pthread_self(), d);
        gpu_enter(d);
        printf("thread %lu using gpu with dataset %d\n", (unsigned long)pthread_self(), d);
        usleep(500000);
        printf("thread %lu done using gpu with dataset %d\n", (unsigned long)pthread_self(), d);
        gpu_exit();
    }
}

int main(int argc, char *argv[])
{
    pthread_t tid[nthreads];
    int i;

    srand(time(0));

    for (i = 0; i < nthreads; i++)
    {
        pthread_create(&tid[i], 0, user_thread, 0);
    }

    for (i = 0; i < nthreads; i++)
    {
        void *result;
        pthread_join(tid[i], &result);
    }

    printf("main: all threads done.\n");

    return 0;
}