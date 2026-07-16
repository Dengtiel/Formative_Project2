/*
 * pipeline.c
 * Question 1: Process Creation, Pipes, and I/O Redirection
 *
 * Emulates: ps aux | grep <keyword>   (default keyword = "root")
 *
 * - Two child processes are created with fork()
 * - Child 1 runs "ps aux" and writes its stdout into a pipe
 * - Child 2 runs "grep <keyword>", reads stdin from the pipe,
 *   and writes its stdout into a results file
 * - The parent process owns the pipe, waits for both children,
 *   then opens the results file, reads it back, and prints the
 *   first N lines to the terminal.
 *
 * Usage: ./pipeline [keyword] [output_file]
 * Default keyword = "root", default output_file = "pipeline_output.txt"
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

#define READ_END  0
#define WRITE_END 1
#define PREVIEW_LINES 10

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    const char *keyword = (argc > 1) ? argv[1] : "root";
    const char *outfile = (argc > 2) ? argv[2] : "pipeline_output.txt";

    int fd[2];
    if (pipe(fd) == -1) die("pipe");

    pid_t pid1 = fork();
    if (pid1 < 0) die("fork (child 1: ps aux)");

    if (pid1 == 0) {
        /* Child 1: "ps aux" -> write end of pipe */
        close(fd[READ_END]);
        if (dup2(fd[WRITE_END], STDOUT_FILENO) == -1) die("dup2 child1");
        close(fd[WRITE_END]);

        char *args[] = {"ps", "aux", NULL};
        execvp(args[0], args);
        die("execvp ps aux"); /* only reached on failure */
    }

    pid_t pid2 = fork();
    if (pid2 < 0) die("fork (child 2: grep)");

    if (pid2 == 0) {
        /* Child 2: read end of pipe -> "grep <keyword>" -> output file */
        close(fd[WRITE_END]);
        if (dup2(fd[READ_END], STDIN_FILENO) == -1) die("dup2 child2 stdin");
        close(fd[READ_END]);

        int out_fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out_fd == -1) die("open output file");
        if (dup2(out_fd, STDOUT_FILENO) == -1) die("dup2 child2 stdout");
        close(out_fd);

        char *args[] = {"grep", (char *)keyword, NULL};
        execvp(args[0], args);
        die("execvp grep"); /* only reached on failure */
    }

    /* Parent: close both ends, it does not use the pipe directly */
    close(fd[READ_END]);
    close(fd[WRITE_END]);

    int status1, status2;
    waitpid(pid1, &status1, 0);
    waitpid(pid2, &status2, 0);

    printf("[parent] ps aux exited with status %d\n", WEXITSTATUS(status1));
    printf("[parent] grep %s exited with status %d\n", keyword, WEXITSTATUS(status2));
    printf("[parent] results captured in: %s\n\n", outfile);

    /* Parent reads back the file and displays a preview */
    FILE *fp = fopen(outfile, "r");
    if (!fp) die("fopen output file for reading");

    char line[1024];
    int count = 0;
    printf("---- Preview of first %d line(s) ----\n", PREVIEW_LINES);
    while (count < PREVIEW_LINES && fgets(line, sizeof(line), fp)) {
        fputs(line, stdout);
        count++;
    }
    if (count == 0) {
        printf("(no matching lines found for keyword \"%s\")\n", keyword);
    }
    fclose(fp);

    return EXIT_SUCCESS;
}
