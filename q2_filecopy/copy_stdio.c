/*
 * copy_stdio.c
 * Question 2, Version 2: file copy using standard I/O functions
 * (fopen, fread, fwrite, fclose). These calls are buffered internally
 * by glibc, so fewer underlying read()/write() syscalls are typically
 * issued compared to the raw syscall version.
 *
 * Usage: ./copy_stdio <source_file> <destination_file>
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BUF_SIZE 65536 /* 64 KB buffer, matches copy_syscall.c for fairness */

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_file> <destination_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    FILE *src = fopen(argv[1], "rb");
    if (!src) die("fopen source");

    FILE *dst = fopen(argv[2], "wb");
    if (!dst) die("fopen destination");

    char buf[BUF_SIZE];
    size_t bytes_read;
    long total_bytes = 0;
    long fread_calls = 0, fwrite_calls = 0;

    while ((bytes_read = fread(buf, 1, BUF_SIZE, src)) > 0) {
        fread_calls++;
        size_t written = fwrite(buf, 1, bytes_read, dst);
        fwrite_calls++;
        if (written != bytes_read) die("fwrite");
        total_bytes += (long)bytes_read;
    }
    if (ferror(src)) die("fread");

    fclose(src);
    fclose(dst);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) +
                      (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("[copy_stdio] Bytes copied  : %ld\n", total_bytes);
    printf("[copy_stdio] fread() calls : %ld\n", fread_calls);
    printf("[copy_stdio] fwrite() calls: %ld\n", fwrite_calls);
    printf("[copy_stdio] Elapsed time  : %.6f seconds\n", elapsed);

    return EXIT_SUCCESS;
}
