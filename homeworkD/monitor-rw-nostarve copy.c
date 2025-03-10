/* note to self:
    - In the previous code file, we only had a reader count, which gave priority to the readers... how?
    - Readers could only read if there was no writer.
    - Once a reader was ready, we could increment the reader count over and over.
    - Writers, on the other hand, were not accumulated; there was just one writer waiting for all the readers to be done.
    - This was unfair because it created the idea of only one writer at a time.
    - Next up here --> I introduced a writer counter.

    - How did I fix this?
    - Now readers could only be incremented and allowed to read if `writecount == 0` and no writer was writing.
    - Writers could write only if...
    - Once the writer passes this check, we decrease the number of waiting writers by one and set the priority for this writer to write.
    - Once this writer is done, it will send two signals (to other writers waiting and readers waiting).
    - However, because there are fewer writers than readers, the readers will have to wait until the writer count gets to 0, even if they get signaled, because we want as many writers to write before all the readers go read.
    - So, readers can only read until no writer is writing and the writer count is 0.
 */
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

int nthreads = 10;
int sleeptime = 10000; /* microseconds */
int value = 0;

// globals we need to keep track of
int readcount = 0;  // keep track of readers
int writecount = 0; // writers waiting
int write_priority = 0;

// create the monitor
typedef struct
{
    pthread_mutex_t lock;     // to lock and unlock critical sections
    pthread_cond_t can_write; // to know if we can write
    pthread_cond_t can_read;  // to know if we can read
} monitor;

// start the monitor
monitor my_monitor = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER};

void read_lock()
{
    pthread_mutex_lock(&my_monitor.lock);
    while (writecount > 0 || write_priority == 1) // read based on writers count and priority
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
        pthread_cond_signal(&my_monitor.can_write); // signal the writer if no readers
    }
    pthread_mutex_unlock(&my_monitor.lock);
}

void write_lock()
{
    pthread_mutex_lock(&my_monitor.lock);
    writecount++;
    while (readcount > 0 || write_priority == 1) // writer waits if there are readers or another writer with priority
    {
        pthread_cond_wait(&my_monitor.can_write, &my_monitor.lock);
    }
    writecount--;
    write_priority = 1; // grant priority to the current writer
    pthread_mutex_unlock(&my_monitor.lock);
}

void write_unlock()
{
    pthread_mutex_lock(&my_monitor.lock);
    write_priority = 0; // reset writer priority after the writer is done
    // send signals to readers and writers
    pthread_cond_broadcast(&my_monitor.can_read); // wake up all readers
    pthread_cond_signal(&my_monitor.can_write);   // wake up one writer (if any)
    pthread_mutex_unlock(&my_monitor.lock);
}

void *reader_writer_thread(void *arg)
{
    while (1)
    {
        if (rand() % 10 < 8) // 80% chance to read
        {
            printf("Thread %p wants to read\n", (void *)pthread_self());
            read_lock();
            printf("Thread %p reading (%d readers)\n", (void *)pthread_self(), readcount);
            int v = value;
            usleep(rand() % sleeptime);
            printf("Thread %p done reading value %d\n", (void *)pthread_self(), v);
            read_unlock();
        }
        else // 20% chance to write
        {
            int v = rand() % 100;
            printf("Thread %p wants to write\n", (void *)pthread_self());
            write_lock();
            printf("Thread %p writing value %d\n", (void *)pthread_self(), v);
            value = v;
            usleep(rand() % sleeptime);
            printf("Thread %p done writing value\n", (void *)pthread_self());
            write_unlock();
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
