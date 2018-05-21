#ifndef EXEC_H
#define EXEC_H

#define PIPE_READ 0
#define PIPE_WRITE 1

struct PROCESS {
    int stdout,
        stderr,
        stdin;
    int status;
};

pid_t child_cmd_pid;
int run_timeout;

struct PROCESS exec_line(char* line, int cmd_id, int* subcmd_id, int log_out, int log_err);
struct PROCESS exec_cmd(char* line);
struct PROCESS exec_internal(int (*f)(char*), char* args);
struct PROCESS fork_cmd(char** args);

#endif
