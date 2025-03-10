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
#include <time.h>

int nthreads = 10;
int sleeptime = 10000; /* microseconds */
int value = 0;
int readcount = 0;

// create the monitor
typedef struct
{
    pthread_mutex_t lock;     // to lock and unlock
    pthread_cond_t can_write; // to know if we can write
    pthread_cond_t can_read;  // to know if we can read
} monitor;

// start the monitor
monitor my_monitor = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER};

void read_lock_fn()
{
    pthread_mutex_lock(&my_monitor.lock);
    if (readcount == 0) // check and see if reader is done
    {
        pthread_cond_wait(&my_monitor.can_read, &my_monitor.lock);
    }
    readcount++;
    pthread_mutex_unlock(&my_monitor.lock);
}

void read_unlock()
{
    pthread_mutex_lock(&my_monitor.lock);
    readcount--;
    if (readcount == 0) // tell the writer he can go
    {
        pthread_cond_broadcast(&my_monitor.can_write);
    }
    pthread_mutex_unlock(&my_monitor.lock);
}

void write_lock_fn()
{
    pthread_mutex_lock(&my_monitor.lock);
    while (readcount > 0)
    { // wait until signaled by last reader
        pthread_cond_wait(&my_monitor.can_write, &my_monitor.lock);
    }
}

void write_unlock_fn()
{
    pthread_cond_broadcast(&my_monitor.can_read); // Notify readers that the writer is done
    pthread_mutex_unlock(&my_monitor.lock);
}

// Assign the threads to run continuously
void *reader_writer_thread(void *arg)
{
    while (1)
    {
        if (rand() % 10 < 8)
        {
            // Assign job of reader
            printf("Thread %p wants to read\n", (void *)pthread_self());
            read_lock_fn();
            printf("Thread %p reading (%d readers)\n", (void *)pthread_self(), readcount);
            int v = value;
            usleep(rand() % sleeptime);
            printf("Thread %p done reading value %d\n", (void *)pthread_self(), v);
            read_unlock();
        }
        else
        {
            // Writer
            int v = rand() % 100;
            printf("Thread %p wants to write\n", (void *)pthread_self());
            write_lock_fn();
            printf("Thread %p writing value %d\n", (void *)pthread_self(), v);
            value = v;
            usleep(rand() % sleeptime);
            printf("Thread %p done writing value\n", (void *)pthread_self());
            write_unlock_fn();
        }
    }
}

int main(int argc, char *argv[])
{
    pthread_t tid[nthreads];

    srand(time(0));

    // Create threads
    for (int i = 0; i < nthreads; i++)
    {
        pthread_create(&tid[i], NULL, reader_writer_thread, NULL);
    }

    // Wait for the threads to complete
    for (int i = 0; i < nthreads; i++)
    {
        pthread_join(tid[i], NULL);
    }

    printf("Main: all threads done.\n");

    return 0;
}
