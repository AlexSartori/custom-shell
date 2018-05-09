#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <readline/history.h>

#include "utils.h"
#include "exec.h"


/*
    Elabora una linea (caratteri di pipe) ed esegue i comandi con gli argomenti

    Returns:
        Struct PROCESS con info sul figlio
*/
struct PROCESS exec_line(char* line) {
    // printf("----    Exec line: %s\n", line);

    // Se c'è un pipe, chiamo due exec_line ed esco
    for (int i = 0; line[i] != '\0'; i++) {
        if (line[i] == '|') {
            struct PROCESS child1, child2;
            line[i] = '\0';
            child1 = exec_line(line);
            child2 = exec_line(line+(i+1));

            // Leggi stdout di child1 e scrivilo su stdin di child2
            char* buf = (char*)malloc(sizeof(char)*BUF_SIZE);
            int r = 0;
            while ((r = read(child1.stdout, buf, BUF_SIZE)) > 0) {
                buf[r] = '\0';
                write(child2.stdin, buf, r);
            }
            free(buf);

            close(child1.stdout);
            close(child1.stdin);
            close(child1.stderr);
            close(child2.stdin);

            return child2;
        }
    }

    // Se sono arrivato qui non c'erano pipe, separo comando e argomenti
    int spazi = 0;
    for (int i = 0; line[i] != '\0'; i++) if (line[i] == ' ') spazi++;
    char* args[spazi+1]; // L'ultimo elemento dev'essere NULL
    char* a = strtok(line, " ");
    int i = 0;
    while (a) {
        args[i] = a;
        a = strtok(NULL, " ");
        i++;
    }
    args[i] = NULL;

    // Controllo se è un comando che voglio gestire internamente
    // TODO anche questi devono avere I/O su file
    if (strcmp(args[0], "clear") == 0)
        clear();
    else if (strcmp(args[0], "exit") == 0)
        shell_exit(0);
    else if (strcmp(args[0], "help") == 0)
        print_help();
    else if (strcmp(args[0], "cd") == 0)
    {
        int status = chdir(args[1]);
        if(status == -1) {
            printcolor("! Errore: cartella inesistente\n", KRED);
        }
    }
    else if (strcmp(args[0], "history") == 0)
    {
        HIST_ENTRY** hist = history_list();
        char* hist_arg = strtok(NULL, " ");
        int n; // Quanti elementi della cronologia mostrare

        // Se non è specificato li mostro tutti
        if (hist_arg == NULL) n = history_length;
        else n = min(atoi(hist_arg), history_length);

        for (int i = history_length - n; i < history_length; i++)
            printf("  %d\t%s\n", i + history_base, hist[i]->line);
    }
    else {
        return fork_cmd(args);
    }
}



/*
    Esegui il comando passato come figlio e registra stdout e stderr.

    Params:
        Array degli argomenti, il primo elemento sarà il comando

    Returns:
        Struct PROCESS con info sul figlio
*/
struct PROCESS fork_cmd(char** args) {
    // Pipe per comunicare col figlio
    int child_in[2], child_out[2], child_err[2];

    // Codice di ritorno del processo
    int ret_code = -1;
    pid_t pid;

    // Habemus Pipe cit.
    if (pipe(child_in)  < 0) perror("Cannot create stdin pipe to child");
    if (pipe(child_out) < 0) perror("Cannot create stdout pipe to child");
    if (pipe(child_err) < 0) perror("Cannot create stderr pipe to child");
    if ((pid = fork())  < 0) perror("Cannot create child");

    if (pid == 0) {
        // Child:
        //   - Chiudi pipe-end che non servono
        //   - Redirigi in/out/err ed esegui il comando
        //   - Esegui e chiudi i pipe

        close(child_in[PIPE_WRITE]);
        close(child_out[PIPE_READ]);
        close(child_err[PIPE_READ]);

        dup2(child_in[PIPE_READ], 0);
        dup2(child_out[PIPE_WRITE], 1);
        dup2(child_err[PIPE_WRITE], 2);

        ret_code = execvp(args[0], args);

        if (ret_code < 0) perror("Cannot execute command");
        close(child_in[PIPE_READ]);
        close(child_out[PIPE_WRITE]);
        close(child_err[PIPE_WRITE]);

        exit(ret_code);
    } else {
        // Parent: chiudi i pipe che non servono e aspetta il figlio
        int status;
        //wait(&status);

        //if(WIFEXITED(status)) printf("child exited with = %d\n",WEXITSTATUS(status));
        //if(WIFEXITED(status) && WEXITSTATUS(status) != 0) printcolor("! Comando non esistente\n",KRED);

        close(child_in[PIPE_READ]);
        close(child_out[PIPE_WRITE]);
        close(child_err[PIPE_WRITE]);
        // wait(NULL);

        struct PROCESS p;
        p.stdin = child_in[PIPE_WRITE];
        p.stdout = child_out[PIPE_READ];
        p.stderr = child_err[PIPE_READ];
        p.status = ret_code;
        return p;
    }
}
