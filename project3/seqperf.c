#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int final_total = 0;
int max_size = 0;

void fill_arrays(int array1[], int array2[])
{
    for (int i = 0; i < max_size; i++)
    {
        array1[i] = rand() % 101; // Generates numbers between 0-100
        array2[i] = rand() % 101;
    }
}

void compute_sum(int array1[], int array2[])
{
    for (int i = 0; i < max_size; i++)
    {
        for (int j = 0; j < max_size; j++)
        {
            int pair_prod = array1[i] * array2[j];
            final_total += pair_prod;
        }
    }
    printf("the total sum is: %d\n", final_total);
}
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Missing arguments\n");
        return 1;
    }

    max_size = atoi(argv[1]);
    if (max_size <= 0)
    {
        printf("Please enter a positive integer for array size.\n");
        return 1;
    }

    int buffer1[max_size], buffer2[max_size];
    srand(42); // fixed seed for consistency
    fill_arrays(buffer1, buffer2);

    // time start
    clock_t start_time, end_time;
    double cpu_time_used;
    start_time = clock();
    // now computer the summ
    compute_sum(buffer1, buffer2);
    end_time = clock();
    cpu_time_used = ((double)(end_time - start_time)) / CLOCKS_PER_SEC; // Calculate the elapsed time in seconds
    printf("time: %f seconds\n", cpu_time_used);
    return 0;
}
