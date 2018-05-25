#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <readline/history.h>

#include "../headers/internals.h"
#include "../headers/utils.h"
#include "../headers/exec.h"
#include "../headers/vector.h"
#include "../headers/parsers.h"

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


void signal_child(int sig) {
    printf("Process %d received signal %d\n", getpid(), sig);
    exit(1);
}

/*
    Elabora la linea di input dell'utente.
    Se c'è un |, chiama exec_line ricorsivamente, altrimenti chiama
    exec_cmd con il comando. La variabile LOG_CMD indica se trasferire
    i buffer di stdout e stderr sui file di log o se roitornre la struct intatta.

    Returns:
        Struct con le info sul processo
*/
struct PROCESS exec_line(char* line) {
    static int LOG_CMD = 1;
    struct PROCESS p, pre_pipe;
    // printf("----    Exec line: %s\n", line);
    // printf("Running task: %d\n", child_cmd_pid);


    // Finché l'ultimo carattere è un pipe o uno spazio, lo tolgo
    while (line[strlen(line)-1] == '|' || line[strlen(line)-1] == ' ') line[strlen(line)-1] = '\0';

    // Cerco dal fondo se c'è un pipe
    int i; for (i = strlen(line); i >= 0; i--) if (line[i] == '|') break;

    if (i >= 0) { // C'è un | nella posizione i, splitto su i
        line[i] = '\0';

        // Disabilito il logging dell'output mentre eseguo ciò che sta a sinistra,
        // perché mi serve un processo con lo stdout ancora tutto nel buffer.
        int old_log_cmd = LOG_CMD;
        LOG_CMD = 0;
        pre_pipe = exec_line(line);
        LOG_CMD = old_log_cmd;
        wait(&pre_pipe.status);
        close(pre_pipe.stdin);
    }

    // Seconda parte del pipe o l'unico comando ricevuto
    subcmd_id++;
    if (redirect(line+(i+1), &p) == 1) { // C'era un redirect ed è stato gestito nella funzione
        ;
    } else
        p = exec_cmd(line+(i+1));

    if (i >= 0) { // Piping
        log_process(pre_pipe, line, cmd_id, subcmd_id-1, (int[]){ log_out, log_err, p.stdin, 2 });
        close(pre_pipe.stdout);
        close(pre_pipe.stderr);
        close(p.stdin);
    }

    if (LOG_CMD) {
        wait(&p.status);
        log_process(p, line+(i+1), cmd_id, subcmd_id, (int[]){ log_out, log_err, 1, 2 });
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
struct PROCESS exec_cmd(char* l) {
    // Sostituisci variabili (se non è un for, altrimenti se ne occupa lui)
    char* line;
    if (l[0] != 'f' && l[1] != 'o' && l[2] != 'r')
        line = parse_vars(l);
    else
        { line = malloc(sizeof(char)*(strlen(l)+1)); strcpy(line, l); }

    char* line_copy = malloc(sizeof(char)*(strlen(line)+1));
    strcpy(line_copy, line);

    struct PROCESS dummy;
    dummy.stdout = dummy.stderr = dummy.stdin = open("/dev/null", O_RDWR);
    // printf("----    exec_cmd: %s\n", line);


    // Separo comando e argomenti
    int spazi = 0, i = 0, c;
    for (c = 0; line[c] != '\0'; c++) if (line[c] == ' ') spazi++;
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
    if (strcmp(args[0], "clear") == 0)
        dummy.status = clear();
    else if (strcmp(args[0], "exit") == 0)
        shell_exit(0);
    else if (strcmp(args[0], "help") == 0)
        dummy = exec_internal(print_help, args[1]);
    else if (strcmp(args[0], "alias") == 0) {
        char *tmp = args[1];
        if(tmp == NULL) {
            dummy = exec_internal(list_alias, NULL);
        } else {
            dummy.status = make_alias(line_copy);
        }
    } else if (strcmp(args[0], "var") == 0) {
        char *tmp = args[1];
        if(tmp == NULL) {
            dummy = exec_internal(list_vars, NULL);
        } else {
            dummy.status = make_var(line_copy);
        }
    } else if (strcmp(args[0], "cd") == 0) {
        dummy.status = chdir(args[1]);
    } else if (strcmp(args[0], "history") == 0) {
        dummy = exec_internal(print_history, args[1]);
    } else if (strcmp(args[0], "for") == 0) {
        dummy.status = do_for(args);
    } else {
        // È un comando shell
        free(line_copy);
        return fork_cmd(args);
    }

    free(line);
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
        // signal(SIGINT, signal_child);
        // signal(SIGALRM, signal_child);
        // alarm(run_timeout);

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
        child_cmd_pid = pid;
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
        // signal(SIGINT, signal_child);
        // signal(SIGALRM, signal_child);
        // alarm(run_timeout);

        // Chiudi pipe-end che non servono
        close(child_in[PIPE_WRITE]);
        close(child_out[PIPE_READ]);
        close(child_err[PIPE_READ]);

        // Redirigi in/out/err ed esegui il comando
        dup2(child_in[PIPE_READ],   0);
        dup2(child_out[PIPE_WRITE], 1);
        dup2(child_err[PIPE_WRITE], 2);

        int ret_code = execvp(args[0], args);
        free(args);
        // if (ret_code < 0) perror("Cannot execute command");

        // Chiudi i pipe
        close(child_in[PIPE_READ]);
        close(child_out[PIPE_WRITE]);
        close(child_err[PIPE_WRITE]);

        exit(ret_code);
    } else { // Parent
        child_cmd_pid = pid;
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
