#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

// Structures
typedef struct
{
    size_t start;
    size_t stop;
    int *array1;
    int *array2;
} args;

// Globals
int max_size = 0;
int n_threads = 0;
long long total_work = 0;
long long final_total = 0;
pthread_mutex_t Lock = PTHREAD_MUTEX_INITIALIZER;
long long completed_work = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Fill arrays with random numbers
void fill_arrays(int *array1, int *array2)
{
    for (int i = 0; i < max_size; i++)
    {
        array1[i] = rand() % 101;
        array2[i] = rand() % 101;
    }
}

// Fill arguments for each thread
void fill_arguments(args *args_array, int *buffer1, int *buffer2)
{
    int chunk_size = max_size / n_threads;
    int remainder = max_size % n_threads;
    int start = 0;
    for (int i = 0; i < n_threads; i++)
    {
        int extra = (i < remainder) ? 1 : 0;
        args_array[i].start = start;
        args_array[i].stop = start + chunk_size + extra;
        args_array[i].array1 = buffer1;
        args_array[i].array2 = buffer2;
        start = args_array[i].stop;
    }
}

// Tracker thread
void *tracker(void *arg)
{
    int percent = 0;
    int last_percent = -1; // Track the last printed percent
    while (1)
    {
        // calculate new percentage
        pthread_mutex_lock(&mutex);
        percent = (completed_work * 100) / total_work;
        if (completed_work >= total_work)
        {
            percent = 100;
        }
        pthread_mutex_unlock(&mutex);

        // only print (=) when theres a 5 increase
        if (percent != last_percent && (percent % 2 == 0 || percent == 100))
        {
            printf("\r%3d%% [", percent);
            for (int i = 0; i < percent / 2; i++)
            {
                printf("=");
            }
            printf("]");

            fflush(stdout);
            last_percent = percent;
        }

        if (percent >= 100)
        {
            break;
        }

        usleep(100000);
    }
    printf("\nDone!\n");
    return NULL;
}

// Worker threads
void *compute_sum(void *Args)
{
    args *p = (args *)Args;
    long long curr_total = 0;
    for (int i = p->start; i < p->stop; i++)
    {
        int local_sum = 0; // Avoid global memory access in loop
        for (int j = p->start; j < p->stop; j++)
        {
            local_sum += p->array1[i] * p->array2[j];
        }
        // Add local sum to thread total (reduces memory writes)
        curr_total += local_sum;
        // Update completed_work **once per outer iteration**
        pthread_mutex_lock(&mutex);
        completed_work += max_size;
        pthread_mutex_unlock(&mutex);
    }

    // Add final sum to global total
    pthread_mutex_lock(&Lock);
    final_total += curr_total;
    pthread_mutex_unlock(&Lock);

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Missing arguments\n");
        return 1;
    }

    max_size = atoi(argv[1]);
    n_threads = atoi(argv[2]);

    // Use dynamic memory instead of stack arrays
    int *buffer1 = malloc(max_size * sizeof(int));
    int *buffer2 = malloc(max_size * sizeof(int));

    if (!buffer1 || !buffer2)
    {
        printf("Memory allocation failed\n");
        return 1;
    }

    total_work = (long long)max_size * max_size; // Avoid overflow
    srand(42);
    fill_arrays(buffer1, buffer2);

    // start the tracking thread
    pthread_t tracking_thread;
    pthread_create(&tracking_thread, NULL, tracker, NULL);

    // start the worker threads
    pthread_t threads[n_threads];
    args Args[n_threads];
    fill_arguments(Args, buffer1, buffer2);
    clock_t start_time = clock();
    for (int i = 0; i < n_threads; i++) // assign working threads
        pthread_create(&threads[i], NULL, compute_sum, &Args[i]);

    // wait for threads
    for (int i = 0; i < n_threads; i++)
        pthread_join(threads[i], NULL);

    pthread_join(tracking_thread, NULL);

    // end and clock the time
    clock_t end_time = clock();
    double cpu_time_used = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    printf("The total sum is: %lld --> Time: %f seconds\n", final_total, cpu_time_used);

    // Free allocated memory
    free(buffer1);
    free(buffer2);

    return 0;
}
