#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

// structures
typedef struct
{
    size_t start;
    size_t stop;
    int *array1;
    int *array2;
} args;

// globals
int max_size = 0;
int n_threads = 0;

// global for threads to take care of
int final_total = 0;
pthread_mutex_t Lock = PTHREAD_MUTEX_INITIALIZER;

// fill arrays with random numbers
void fill_arrays(int array1[], int array2[])
{
    for (int i = 0; i < max_size; i++)
    {
        array1[i] = rand() % 101; // Generates numbers between 0-100
        array2[i] = rand() % 101;
    }
}

// fill the start and stop for each thread
void fill_arguments(args args[], int buffer1[], int buffer2[])
{
    int chunk_size = max_size / n_threads;
    int remainder = max_size % n_threads; // Extra elements that don’t fit evenly
    int start = 0;
    for (int i = 0; i < n_threads; i++)
    {
        int extra = (i < remainder) ? 1 : 0; // First 'remainder' threads get 1 extra element
        args[i].start = start;
        args[i].stop = start + chunk_size + extra;
        args[i].array1 = buffer1;
        args[i].array2 = buffer2;
        start = args[i].stop;
        printf("start=%zu, stop=%zu\n", args[i].start, args[i].stop);
    }
}

// THREADS
// each task will come here and begin calculating its own task
void *compute_sum(void *Args)
{
    args *p = (args *)Args;
    for (int i = p->start; i < p->stop; i++)
    {
        for (int j = p->start; j < p->stop; j++)
        {
            int pair_prod = p->array1[i] * p->array2[j];
            pthread_mutex_lock(&Lock);
            final_total += pair_prod;
            pthread_mutex_unlock(&Lock);
        }
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Missing arguments\n");
        return 1;
    }
    // create the arrays
    max_size = atoi(argv[1]);
    int buffer1[max_size], buffer2[max_size];
    srand(42);
    fill_arrays(buffer1, buffer2);

    // start threads and arguments
    n_threads = atoi(argv[2]);
    pthread_t threads[n_threads];           // array of threads
    args Args[n_threads];                   // arguments for those threads
    fill_arguments(Args, buffer1, buffer2); // fill up the argumentss + place buffers

    // time start
    clock_t start_time, end_time;
    double cpu_time_used;
    start_time = clock();
    // start  and assign threads
    for (int i = 0; i < n_threads; i++)
    {
        // create every thread and assign task
        pthread_create(&threads[i], NULL, compute_sum, &Args[i]);
    }

    for (int i = 0; i < n_threads; i++)
    { // Wait for threads
        pthread_join(threads[i], NULL);
    }
    end_time = clock();
    cpu_time_used = ((double)(end_time - start_time)) / CLOCKS_PER_SEC; // Calculate the elapsed time in seconds
    printf("the total sum is: %d --> time: %f seconds\n", final_total, cpu_time_used);
    return 0;
}