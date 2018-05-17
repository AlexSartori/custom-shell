#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <readline/history.h>

#include "internals.h"
#include "utils.h"
#include "exec.h"
#include "vector.h"

/*
    Funzionamento:

    exec_line:
        Riceve la linea di comando scritta dall'utente e separa sottocomandi e pipe.
        Una volta estratti le singole "righe di comando" chiama exec_cmd (vedi sotto).
        Es: exec_line("ls | wc") --> exec_line("ls") e exec_line("wc")

    exec_cmd:
        Separa la linea di comando in comando e argomenti, se il comando è una funzione
        interna la gestisce, altrimeni chiama fork_cmd (vedi sotto).
        Es: exec_cmd("ls -l -a") --> fork_cmd(["ls", "-l", "-a"])

    fork_cmd:
        Crea un figlio che esegue il comando con gli argomenti passati.
*/


/*
    Elabora la linea di input dell'utente.
    Se c'è un |, chiama exec_line ricorsivamente, altrimenti chiama
    exec_cmd con il comando. La variabile LOG_CMD indica se trasferire
    i buffer di stdout e stderr sui file di log o se roitornre la struct intatta.

    Returns:
        Struct con le info sul processo
*/
struct PROCESS exec_line(char* line, int cmd_id, int* subcmd_id, int log_out, int log_err) {
    static int LOG_CMD = 1;
    struct PROCESS p, pre_pipe;
    // printf("----    Exec line: %s\n", line);

    // Cerco dal fondo se c'è un pipe
    int i; for (i = strlen(line); i >= 0; i--) if (line[i] == '|') break;

    if (i >= 0) { // C'è un | nella posizione i, splitto su i
        line[i] = '\0';

        // Disabilito il logging dell'output mentre eseguo ciò che sta a sinistra,
        // perché mi serve un processo con lo stdout ancora tutto nel buffer.
        int old_log_cmd = LOG_CMD;
        LOG_CMD = 0;
        pre_pipe = exec_line(line, cmd_id, subcmd_id, log_out, log_err);
        LOG_CMD = old_log_cmd;
        wait(&pre_pipe.status);
        close(pre_pipe.stdin);
    }

    (*subcmd_id)++;
    // Seconda parte del pipe o l'unico comando ricevuto
    if (line[strlen(line+i+1)-1] == '&') {
        // L'ultimo carattere è un &, lo eseguo ma non aspetto e ritorno
        line[strlen(line+i+1)-1] = '\0';
        printf("Running in backgound: %s\n", line+i+1);
        return exec_cmd(line+(i+1));
    } else {
        p = exec_cmd(line+(i+1));
    }


    if (i >= 0) {
        // Piping
        log_process(pre_pipe, line, cmd_id, (*subcmd_id)-1, (int[]){ log_out, log_err, p.stdin, 2 });
        close(pre_pipe.stdout);
        close(pre_pipe.stderr);
        close(p.stdin);
    }

    if (LOG_CMD) {
        wait(&p.status);
        log_process(p, line+(i+1), cmd_id, *subcmd_id, (int[]){ log_out, log_err, 1, 2 });
        close(p.stdout);
        close(p.stderr);
    }

    return p;
}


/*
    Separa la linea di comando in un array di argomenti. Controlla se il
    comando è una funzione interna.
    TODO magari espandendo variabili e percorsi
*/
struct PROCESS exec_cmd(char* line) {
    struct PROCESS dummy; // = fork_cmd((char*[]) {";", NULL});
    // printf("----    exec_cmd: %s\n", line);

    // Separo comando e argomenti
    char *copy_line = (char*) malloc(sizeof(char) * strlen(line));
    strcpy(copy_line, line);
    int spazi = 0, i = 0;
    for (int c = 0; line[c] != '\0'; c++) if (line[c] == ' ') spazi++;
    char* args[spazi+1]; // L'ultimo elemento dev'essere NULL
    char* token = strtok(line, " ");
    while (token) {
        args[i] = token;
        token = strtok(NULL, " ");
        i++;
    }
    args[i] = NULL;

    // Case insensitive
    string_tolower(args[0]);

    // Controllo se è un comando che voglio gestire internamente
    // TODO anche questi devono avere I/O su file?
    if (strcmp(args[0], "clear") == 0)
        return exec_internal(clear, NULL);
    else if (strcmp(args[0], "exit") == 0)
        exit(0); // shell_exit(0);
    else if (strcmp(args[0], "help") == 0)
        return exec_internal(print_help, args[1]);
    else if (strcmp(args[0], "alias") == 0) {
        char *tmp = args[1];
        if(tmp == NULL) {
            return exec_internal(list_alias, NULL);
        } else {
            dummy.status = make_alias(copy_line);
        }
    } else if (strcmp(args[0], "cd") == 0) {
        dummy.status = chdir(args[1]);
    } else if (strcmp(args[0], "history") == 0) {
        return exec_internal(print_history, args[1]);
    } else {
        // È un comando shell
        return fork_cmd(args);
    }

    return dummy;
}


/*
    Esegui la funzione passata come processo separato,
    simulando un processo come per exec_cmd.
*/
struct PROCESS exec_internal(int (*f)(char*), char* arg) {
    // Pipe per comunicare col figlio
    int child_in[2], child_out[2], child_err[2];
    pid_t pid;

    // Habemus Pipe cit.
    if (pipe(child_in)  < 0) perror("Cannot create stdin pipe to child");
    if (pipe(child_out) < 0) perror("Cannot create stdout pipe to child");
    if (pipe(child_err) < 0) perror("Cannot create stderr pipe to child");
    if ((pid = fork())  < 0) perror("Cannot create child");

    if (pid == 0) { // Child
        // Chiudi pipe-end che non servono
        close(child_in[PIPE_WRITE]);
        close(child_out[PIPE_READ]);
        close(child_err[PIPE_READ]);

        // Redirigi in/out/err ed esegui il comando
        dup2(child_in[PIPE_READ],   0);
        dup2(child_out[PIPE_WRITE], 1);
        dup2(child_err[PIPE_WRITE], 2);

        int ret_code = f(arg);

        // Chiudi i pipe
        close(child_in[PIPE_READ]);
        close(child_out[PIPE_WRITE]);
        close(child_err[PIPE_WRITE]);

        exit(ret_code);
    } else { // Parent
        // Chiudi i pipe che non servono
        close(child_in[PIPE_READ]);
        close(child_out[PIPE_WRITE]);
        close(child_err[PIPE_WRITE]);

        struct PROCESS p;
        p.stdin  = child_in[PIPE_WRITE];
        p.stdout = child_out[PIPE_READ];
        p.stderr = child_err[PIPE_READ];
        //wait(&p.status);
        return p;
    }
}

/*
    Esegui il vettore di argomenti come comando in un figlio
        Params: Array degli argomenti, il primo elemento sarà il comando
        Returns: Struct PROCESS con info sul figlio
*/
struct PROCESS fork_cmd(char** args) {
    // printf("----    Fork cmd [0] -> %s\n", args[0]);
    // Pipe per comunicare col figlio
    int child_in[2], child_out[2], child_err[2];
    pid_t pid;

    // Habemus Pipe cit.
    if (pipe(child_in)  < 0) perror("Cannot create stdin pipe to child");
    if (pipe(child_out) < 0) perror("Cannot create stdout pipe to child");
    if (pipe(child_err) < 0) perror("Cannot create stderr pipe to child");
    if ((pid = fork())  < 0) perror("Cannot create child");

    if (pid == 0) { // Child
        // Chiudi pipe-end che non servono
        close(child_in[PIPE_WRITE]);
        close(child_out[PIPE_READ]);
        close(child_err[PIPE_READ]);

        // Redirigi in/out/err ed esegui il comando
        dup2(child_in[PIPE_READ],   0);
        dup2(child_out[PIPE_WRITE], 1);
        dup2(child_err[PIPE_WRITE], 2);

        int ret_code = execvp(args[0], args);
        // if (ret_code < 0) perror("Cannot execute command");

        // Chiudi i pipe
        close(child_in[PIPE_READ]);
        close(child_out[PIPE_WRITE]);
        close(child_err[PIPE_WRITE]);

        exit(ret_code);
    } else { // Parent
        // Chiudi i pipe che non servono
        close(child_in[PIPE_READ]);
        close(child_out[PIPE_WRITE]);
        close(child_err[PIPE_WRITE]);

        //if(WIFEXITED(status) && WEXITSTATUS(status) != 0) printcolor("! Comando non esistente\n",KRED);
        // wait(NULL);

        struct PROCESS p;
        p.stdin  = child_in[PIPE_WRITE];
        p.stdout = child_out[PIPE_READ];
        p.stderr = child_err[PIPE_READ];
        return p;
    }
}
