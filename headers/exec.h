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

pid_t child_cmd_pid;                // PID figlio in esecuzione
int run_timeout;                    // Timeout esecuzione comandi
int log_out, log_err;               // File di log
int cmd_id, subcmd_id;              // ID incrementale del comando, per il file di log


struct PROCESS exec_line(char* line);
struct PROCESS exec_cmd(char* line);
struct PROCESS exec_internal(int (*f)(char*), char* args);
struct PROCESS fork_cmd(char** args);

void shell_exit(int);

#endif
