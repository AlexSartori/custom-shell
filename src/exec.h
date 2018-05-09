#ifndef EXEC_H
#define EXEC_H

#define PIPE_READ 0
#define PIPE_WRITE 1

struct PROCESS {
    int stdout,
        stderr,
        stdin,
        status;
};

struct PROCESS exec_line(char* line);
struct PROCESS fork_cmd(char** args);

#endif