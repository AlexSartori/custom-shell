#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../headers/exec.h"
#include "../headers/utils.h"
#include "../headers/vector.h"
#include "../headers/internals.h"
#include "../headers/parsers.h"


char *comando, prompt[BUF_SIZE];                        // Buffer per l'input dell'utente e il prompt
struct OPTIONS opt;                                     // Opzioni con cui è stata chiamata la shell
int running;



void shell_exit(int status) {
    printf("\n");

    close(log_out);                                     // Chiudi i file di log
    close(log_err);
    vectors_destroy();
    exit(status);
}


void init_shell(int argc, char** argv) {
    running = 1;                                        // Attiva il loop principale
    opt = read_options(argc, argv);                     // Leggi gli argomenti di chiamata
    child_cmd_pid = 0;                                  // Pid del comando figlio
    save_ret_code = opt.save_ret_code;                  // Se salvare il codice di ritorno
    run_timeout = opt.timeout;                          // Timeout esecuzione figli
    cmd_id = subcmd_id = 0;                             // Id incrementale del comando e sottocomandi_pv

    rl_bind_key('\t', rl_complete);                     // Tab autocomplete
    stifle_history(opt.hist_size);                      // Controlla dimensione history

    signal(SIGINT, sigHandler);                         // Intercetta Ctrl-C e timeout
    signal(SIGALRM, sigHandler);

    log_out = open(opt.log_out_path,                    // File di log stdout
                   O_RDWR | O_CREAT | O_APPEND, 0644);
    if (log_out == -1) {
        printcolor("! Error: cannot open stdout log, quitting.", KRED);
        shell_exit(-1);
    }

    log_err = open(opt.log_err_path,                    // File di log stderr
                   O_RDWR | O_CREAT | O_APPEND, 0644);
    if (log_err == -1) {
        printcolor("! Error: cannot open stderr log, quitting.", KRED);
        shell_exit(-1);
    }

    vectors_initializer();                              // Inizializza array alias e variabili
}


void sigHandler(int sig) {
    // printf("\nParent (%d) received signal: %d\n", getpid(), sig);

    if (sig == SIGKILL || sig == SIGTERM)
        shell_exit(0);
    else if (sig == SIGALRM) {                          // Timeout esecuzione
        if(child_cmd_pid != 0) kill(child_cmd_pid, 9);
    } else if (sig == SIGINT && child_cmd_pid != 0)     // Ctrl-C per il figlio
        kill(child_cmd_pid, 9);
    else if (sig == SIGINT && child_cmd_pid == 0)       // Ctrl-C per la shell
        shell_exit(0);
    else
        fprintf(stderr, "Unhandled signal: %d\n", sig);
}


int main(int argc, char** argv) {
    init_shell(argc, argv);

    while (running) {
        child_cmd_pid = 0;                              // Nessun figlio in esecuzione
        get_prompt(prompt);                             // Crea il prompt
        comando = readline(prompt);                     // Leggi l'input dell'utente

        if (run_timeout != -1) alarm(run_timeout);      // Timeout di esecuzione figli

        if (comando == NULL) break;                     // Ctrl-D

        if (strlen(comando) == 0)                       // Linea vuota
        { free(comando); continue; }


        add_history(comando);                           // Gestisci history


        // Gestisco punto e virgola
        char** comandi_pv = split_pv(comando);

        int j;
        for (j = 0; comandi_pv[j] != NULL; j++) {
            cmd_id++; subcmd_id = 0;
            if (strcmp(comandi_pv[j], "exit") == 0) { running = 0; break; }

            // Gestisco &&
            int br = gest_and(comandi_pv[j]);
            if (br == -1) {
                printcolor("! Error: One of the command failed.\n", KRED);
                continue; // Ho incontrato break nell'&&
            } else {
                 comandi_pv[j] = comandi_pv[j] + br;
            }

            int t = strlen(comandi_pv[j]) - 1;
            if (comandi_pv[j][t] == '&') {
                comandi_pv[j][t] = '\0';

                pid_t pid = fork();
                if (pid < 0) perror("Cannot fork for background execution");
                else if (pid == 0) {
                    printf("[%d] Running in backgound: %s\n", getpid(), comandi_pv[j]);
                    close(0);

                    comandi_pv[j] = parse_alias(comandi_pv[j]);                 // Gestisci alias
                    comandi_pv[j] = expand_wildcard(comandi_pv[j]);             // Gestisci wildcards

                    exec_line(comandi_pv[j]);
                    printf("\n[%d] Done: %s\n", getpid(), comandi_pv[j]);
                    fflush(stdout);
                }
            } else {
                comandi_pv[j] = parse_alias(comandi_pv[j]);                 // Gestisci alias
                comandi_pv[j] = expand_wildcard(comandi_pv[j]);             // Gestisci wildcards
                struct PROCESS p = exec_line(comandi_pv[j]);
                if (p.status == 65280) printcolor("! Error: command not found.\n", KRED);
                else if (p.status != 0) { printcolor("Non-zero exit status: ", KMAG); printf("%d\n", p.status); }
            }
        }

        free(comando);
        free(comandi_pv);

        // Controllo dimensione dei file
        struct stat buffer;
        if(stat(opt.log_out_path, &buffer) == 0)
            if (buffer.st_size > opt.max_size)
                printcolor("! Warning: maximum stdout log file size reached\n", KRED);
        if(stat(opt.log_err_path, &buffer) == 0)
            if (buffer.st_size > opt.max_size)
                printcolor("! Warning: maximum stderr log file size reached\n", KRED);
    }

    // Pulisci tutto ed esci
    shell_exit(0);
    return 0;
}
