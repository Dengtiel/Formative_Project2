/*
 * copy_syscall.c
 * Question 2, Version 1: file copy using low-level system calls
 * (open, read, write, close) with a fixed-size buffer.
 *
 * Usage: ./copy_syscall <source_file> <destination_file>
 * Prints execution time and total number of read()/write() calls made.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

#define BUF_SIZE 65536 /* 64 KB buffer */

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

    int src_fd = open(argv[1], O_RDONLY);
    if (src_fd == -1) die("open source");

    int dst_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst_fd == -1) die("open destination");

    char buf[BUF_SIZE];
    ssize_t bytes_read;
    long total_bytes = 0;
    long read_calls = 0, write_calls = 0;

    while ((bytes_read = read(src_fd, buf, BUF_SIZE)) > 0) {
        read_calls++;
        ssize_t bytes_written = 0;
        while (bytes_written < bytes_read) {
            ssize_t w = write(dst_fd, buf + bytes_written, bytes_read - bytes_written);
            if (w == -1) die("write");
            write_calls++;
            bytes_written += w;
        }
        total_bytes += bytes_read;
    }
    if (bytes_read == -1) die("read");

    close(src_fd);
    close(dst_fd);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) +
                      (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("[copy_syscall] Bytes copied : %ld\n", total_bytes);
    printf("[copy_syscall] read() calls : %ld\n", read_calls);
    printf("[copy_syscall] write() calls: %ld\n", write_calls);
    printf("[copy_syscall] Elapsed time : %.6f seconds\n", elapsed);

    return EXIT_SUCCESS;
}
