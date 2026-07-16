/*
 * search.c
 * Question 4: Concurrent keyword search across multiple text files
 *
 * Each file is processed by its own thread. Each thread counts how
 * many times the keyword occurs in its file, then writes its result
 * as one line into a shared output file. A mutex protects the shared
 * output file so lines from different threads never interleave.
 *
 * Usage:
 *   ./search keyword output.txt file1.txt file2.txt ... <number_of_threads>
 *
 * Note: <number_of_threads> is the LAST argument. It caps how many
 * files are processed concurrently; if there are more files than
 * threads, work is handed out in batches of that size.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#define MAX_LINE 4096

static pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;
static const char *g_keyword;
static FILE *g_output_fp;

typedef struct {
    const char *filepath;
} task_t;

static long count_occurrences_in_file(const char *filepath, const char *keyword) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        fprintf(stderr, "warning: could not open %s: %s\n", filepath, strerror(errno));
        return -1;
    }

    long count = 0;
    char line[MAX_LINE];
    size_t keyword_len = strlen(keyword);

    while (fgets(line, sizeof(line), fp)) {
        char *pos = line;
        while ((pos = strstr(pos, keyword)) != NULL) {
            count++;
            pos += keyword_len;
        }
    }

    fclose(fp);
    return count;
}

static void *search_worker(void *arg) {
    task_t *task = (task_t *)arg;
    long occurrences = count_occurrences_in_file(task->filepath, g_keyword);

    pthread_mutex_lock(&output_mutex);
    if (occurrences >= 0) {
        fprintf(g_output_fp, "%s: %ld occurrence(s) of \"%s\"\n",
                task->filepath, occurrences, g_keyword);
    } else {
        fprintf(g_output_fp, "%s: ERROR (could not open file)\n", task->filepath);
    }
    fflush(g_output_fp);
    pthread_mutex_unlock(&output_mutex);

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr,
            "Usage: %s keyword output.txt file1.txt [file2.txt ...] <number_of_threads>\n",
            argv[0]);
        return EXIT_FAILURE;
    }

    g_keyword = argv[1];
    const char *outfile = argv[2];
    int num_threads = atoi(argv[argc - 1]);
    if (num_threads <= 0) {
        fprintf(stderr, "Error: number_of_threads must be a positive integer\n");
        return EXIT_FAILURE;
    }

    /* files are everything between argv[3] and argv[argc-2] inclusive */
    int num_files = argc - 4; /* argv[0], keyword, output.txt, ..., thread_count */
    if (num_files <= 0) {
        fprintf(stderr, "Error: no input files given\n");
        return EXIT_FAILURE;
    }

    g_output_fp = fopen(outfile, "w");
    if (!g_output_fp) {
        perror("fopen output file");
        return EXIT_FAILURE;
    }

    task_t *tasks = malloc(sizeof(task_t) * num_files);
    for (int i = 0; i < num_files; i++) {
        tasks[i].filepath = argv[3 + i];
    }

    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    int launched = 0;
    while (launched < num_files) {
        int batch = (num_files - launched < num_threads) ? (num_files - launched) : num_threads;

        for (int i = 0; i < batch; i++) {
            if (pthread_create(&threads[i], NULL, search_worker, &tasks[launched + i]) != 0) {
                perror("pthread_create");
                return EXIT_FAILURE;
            }
        }
        for (int i = 0; i < batch; i++) {
            pthread_join(threads[i], NULL);
        }
        launched += batch;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    fclose(g_output_fp);
    free(tasks);
    free(threads);

    printf("Processed %d file(s) with keyword \"%s\" using up to %d thread(s)\n",
           num_files, g_keyword, num_threads);
    printf("Results written to: %s\n", outfile);
    printf("Elapsed time: %.6f seconds\n", elapsed);

    return EXIT_SUCCESS;
}
