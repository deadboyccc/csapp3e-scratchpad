#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void write_demo_file(const char *path) {
    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        perror("fopen");
        exit(1);
    }

    fprintf(fp, "CSAPP\nC refresher\nSystems programming\n");
    fclose(fp);
}

int main(void) {
    const char *path = "/tmp/c_refresher_demo.txt";
    write_demo_file(path);

    // File I/O reads text as a stream.
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        perror("fopen");
        return 1;
    }

    char line[128];
    printf("file contents:\n");
    while (fgets(line, sizeof(line), fp) != NULL) {
        printf("%s", line);
    }
    fclose(fp);

    // fork() duplicates the process; child and parent run independently.
    int pipefd[2];
    if (pipe(pipefd) != 0) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();
    if (pid == 0) {
        close(pipefd[0]);
        const char *msg = "child says: hello from pipe\n";
        write(pipefd[1], msg, strlen(msg));
        close(pipefd[1]);
        _exit(0);
    }

    close(pipefd[1]);
    char buffer[128] = {0};
    ssize_t n = read(pipefd[0], buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        printf("parent received: %s", buffer);
    }
    waitpid(pid, NULL, 0);

    return 0;
}
