#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include<fcntl.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

int log_out, log_err;

int main(int argc, char** argv) {
    log_out = open("log_stdout.txt", O_RDWR | O_CREAT, 0777);
    log_err = open("log_stderr.txt", O_RDWR | O_CREAT, 0777);

    int child_out[2], child_err[2];
    pid_t pid;

    // Habemus Pipe cit.
    if (pipe(child_out) < 0) perror("Cannot create stdout pipe to child");
    if (pipe(child_err) < 0) perror("Cannot create stderr pipe to child");
    if ((pid = fork())  < 0) perror("Cannot create child");

    if (pid == 0) { // Child
        close(child_out[PIPE_READ]);
        close(child_err[PIPE_READ]);
        dup2(child_out[PIPE_WRITE], 1);
        dup2(child_err[PIPE_WRITE], 2);
        char* args[] = {"ls", "niente", NULL};
        if (execvp(args[0], args) < 0) perror("Cannot execute command");
        close(child_out[PIPE_WRITE]);
        close(child_err[PIPE_WRITE]);
    } else {
        close(child_out[PIPE_WRITE]);
        close(child_err[PIPE_WRITE]);
        wait(NULL);
    }

    printf("Child's stdout:\n");

    char* buf;
    buf = (char*)malloc(sizeof(char)+1000);
    if (read(child_out[PIPE_READ], buf, 1000) > 0) {
        printf("%s", buf);
        write(log_out, buf, strlen(buf));
    }
    close(log_out);
    close(child_out[PIPE_READ]);

    printf("Child's stderr:\n");

    if (read(child_err[PIPE_READ], buf, 1000) > 0) {
        printf("%s", buf);
        write(log_err, buf, strlen(buf));
    }
    close(log_err);
    close(child_err[PIPE_READ]);
}
