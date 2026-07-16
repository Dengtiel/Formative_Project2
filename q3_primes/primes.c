/*
 * primes.c
 * Question 3: Multithreaded prime counting with pthread_mutex_t
 *
 * Counts primes in [1, 200000] using 16 POSIX threads.
 * The range is split into 16 equal (or near-equal) segments;
 * each thread scans its own segment and only locks the mutex
 * to add its local count to the shared total (not per-number),
 * which minimizes lock contention.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>

#define LIMIT      200000
#define NUM_THREADS 16

static long shared_total = 0;
static pthread_mutex_t total_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int start;   /* inclusive */
    int end;     /* inclusive */
    int id;
} thread_range_t;

static int is_prime(int n) {
    if (n < 2) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;
    int limit = (int)sqrt((double)n);
    for (int i = 3; i <= limit; i += 2) {
        if (n % i == 0) return 0;
    }
    return 1;
}

static void *count_primes_worker(void *arg) {
    thread_range_t *range = (thread_range_t *)arg;
    long local_count = 0;

    for (int n = range->start; n <= range->end; n++) {
        if (is_prime(n)) local_count++;
    }

    pthread_mutex_lock(&total_mutex);
    shared_total += local_count;
    pthread_mutex_unlock(&total_mutex);

    printf("[thread %2d] range [%6d - %6d] -> %ld primes\n",
           range->id, range->start, range->end, local_count);

    return NULL;
}

int main(void) {
    pthread_t threads[NUM_THREADS];
    thread_range_t ranges[NUM_THREADS];

    int chunk = LIMIT / NUM_THREADS;
    int remainder = LIMIT % NUM_THREADS;
    int current = 1;

    for (int i = 0; i < NUM_THREADS; i++) {
        int size = chunk + (i < remainder ? 1 : 0);
        ranges[i].id = i;
        ranges[i].start = current;
        ranges[i].end = current + size - 1;
        current += size;

        if (pthread_create(&threads[i], NULL, count_primes_worker, &ranges[i]) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("\nThe synchronized total number of prime numbers between 1 and %d is %ld\n",
           LIMIT, shared_total);

    return EXIT_SUCCESS;
}
