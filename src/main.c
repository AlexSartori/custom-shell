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


// Buffer per l'input dell'utente e il prompt
char *comando, prompt[BUF_SIZE];
int log_out, log_err; // File di log
int cmd_id = 0, subcmd_id = 0; // ID incrementale del comando, per il file di log
struct OPTIONS opt; // Opzioni con cui è stata chiamata la shell

void init_shell(struct OPTIONS opt) {
    child_cmd_pid = 0;
    save_ret_code = 0;
    run_timeout = -1;

    // Tab autocomplete
    rl_bind_key('\t', rl_complete);

    // Catch Ctrl-C and execution timeout
    signal(SIGINT, sigHandler);
    signal(SIGALRM, sigHandler);

    // Apri i file di log
    log_out = open(opt.log_out_path, O_RDWR | O_CREAT | O_APPEND, 0644);
    log_err = open(opt.log_err_path, O_RDWR | O_CREAT | O_APPEND, 0644);

    // Inizializza array alias e variabili
    vectors_initializer();
}


void shell_exit(int status) {
    printf("\n");
    // Libera i buffer
    free(comando);
    // Chiudi i file
    close(log_out);
    close(log_err);
    // Esci
    exit(status);
}


void sigHandler(int sig) {
    // printf("Parent (%d) received signal: %d\n", getpid(), sig);
    if (sig == SIGKILL || sig == SIGTERM)
        shell_exit(0);
    else if (sig == SIGALRM) {
        if(child_cmd_pid != 0) kill(child_cmd_pid, 9); // printf("%s\n", "Yolo");
    } else if (sig == SIGINT && child_cmd_pid != 0) // Killo il figlio bloccato
        kill(child_cmd_pid, 9);
    else // Ctrl-C e nessun processo in esecuzione -> faccio uscire la shell
        shell_exit(0);
}


int main(int argc, char** argv) {
    opt = read_options(argc, argv);
    init_shell(opt);

    save_ret_code = opt.save_ret_code;
    run_timeout = opt.timeout;

    int for_loop = 0;

    while(1) {
        get_prompt(prompt);
        comando = readline(prompt);

        child_cmd_pid = 0;
        alarm(run_timeout);

        if (comando == NULL) break; // Ctrl-D
        if (strlen(comando) == 0) continue; // Linea vuota

        // Gestisco history
        add_history(comando);
        stifle_history(opt.hist_size);

        // Gestisco alias
        comando = parse_alias(comando);

        // Gestisco wildcards
        comando = expand_wildcar(comando);


        // Gestisco punto e virgola
        int pv = 0, cont = 0, i, j;
        for(i = 0; comando[i] != '\0'; i++) if(comando[i] == ';') pv++;
        char* comandi[pv + 1];
        if (pv!=0) {
            cont = gest_pv(comandi, comando);
        } else {
            cont = 1;
            comandi[cont - 1] = comando;
        }

        for (j=0; j<cont; j++){
            cmd_id++;
            subcmd_id = 0;

            if(comandi[j] == NULL || strlen(comandi[j]) == 0 ) continue;

            // Gestisco &&
            int br;
            br = gest_and(comandi[j], &cmd_id, subcmd_id , log_out, log_err);
            if (br == -1){
                 continue; // Ho incontrato break nell'&&
            } else {
                 comandi[j]= comandi[j] + br;
            }

	        // Gestisco redirect
            if(redirect(comandi[j], &cmd_id, subcmd_id, log_out, log_err) == 1) continue;

            // Controlla se il comando è fatto di soli spazi
            int spazi = 0, k;
            for (k=0; k<strlen(comandi[j]); k++) if (comandi[j][k]== ' ') spazi++;
            if (strlen(comandi[j]) == spazi) continue;

            // Tolgo gli spazi finali
            int t = strlen(comandi[j]) - 1;
            while (comandi[j][t] == ' ') t--;
            comandi[j][t+1] = '\0';

            if (comandi[j][t] == '&') {
                comandi[j][t] = '\0';

                pid_t pid = fork();
                if (pid < 0) perror("Cannot fork for background execution");
                else if (pid == 0) {
                    printf("[%d] Running in backgound: %s\n", getpid(), comandi[j]);
                    close(0);
                    int n = open("/dev/null", O_RDWR);
                    // dup2(n, 1);
                    // dup2(n, 2);
                    exec_line(comandi[j], cmd_id, &subcmd_id, n, n);
                    printf("\n[%d] Done: %s\n", getpid(), comandi[j]);
                    fflush(stdout);
                } else
                    child_cmd_pid = pid;

                continue;
            }

            char *copy = (char*) malloc(sizeof(char) * BUF_SIZE);
            strcpy(copy, comandi[j]);
            if(strcmp(strtok(copy, " "), "for") == 0) for_loop = 1;

            if(for_loop == 1) {
                int a = 0, b = 0, c = 0, lim = 0;
                char *var = (char*) malloc(sizeof(char) * BUF_SIZE);
                char *cmd = (char*) malloc(sizeof(char) * BUF_SIZE);
                char *cmd_parsed = (char*) malloc(sizeof(char) * BUF_SIZE);

                char *tmp;
                strcpy(var, strtok(NULL, " "));
                if(var != NULL) a = 1;
                if(search_var_name(var) != NULL) azz_var(var);
                else {
                    char *make = (char*) malloc(sizeof(char) * BUF_SIZE);
                    snprintf(make, BUF_SIZE, "var \'%s\' = \'%d'\"", var, 0);
                    make_var(make);
                }
                tmp = strtok(NULL, " ");
                if(tmp != NULL && strcmp(tmp, "in") == 0) b = 1;
                char *limite = strtok(NULL, " ");
                lim = atoi(parse_vars(limite));
                tmp = strtok(NULL, " ");
                if(tmp != NULL && strcmp(tmp, "do") == 0) c = 1;

                if(a == 1 && b == 1 && c == 1) {
                    tmp = strtok(NULL, " ");
                    while(tmp != NULL && strcmp(tmp, "end") != 0) {
                        strcat(cmd, tmp);
                        tmp = strtok(NULL, " ");
                        if(tmp != NULL) strcat(cmd, " ");
                        else strcat(cmd, "\0");
                    }
                    int start = 0;
                    for(start = 0; start <= lim; start++) {
                        // Gestisco variabili
                        cmd_parsed = parse_vars(cmd);
                        
                        struct PROCESS p = exec_line(cmd_parsed, cmd_id, &subcmd_id, log_out, log_err);
                        if (p.status == 65280) printcolor("! Error: command not found.\n", KRED);
                        else if (p.status != 0) { printcolor("Non-zero exit status: ", KMAG); printf("%d\n", p.status); }
                        inc_var(var);
                    }
                } else {
                    printcolor("! Error: for loop not valid.\n", KRED);
                }

                for_loop = 0;
            } else {
                free(copy);
                // Gestisco variabili
                comandi[j] = parse_vars(comandi[j]);

                // Gestisci redirect
                if(redirect(comandi[j], &cmd_id, subcmd_id, log_out, log_err) == 1) continue;

                struct PROCESS p = exec_line(comandi[j], cmd_id, &subcmd_id, log_out, log_err);
                if (p.status == 65280) printcolor("! Error: command not found.\n", KRED);
                else if (p.status != 0) { printcolor("Non-zero exit status: ", KMAG); printf("%d\n", p.status); }
            }
        }

        // Controllo dimensione dei file
        struct stat buffer;
        if(stat(opt.log_out_path, &buffer) == 0)
            if (buffer.st_size > opt.max_size)
                printcolor("Il file di log dello stdout ha raggiunto la dimensione massima!\n", KRED);
        if(stat(opt.log_err_path, &buffer) == 0)
            if (buffer.st_size > opt.max_size)
                printcolor("Il file di log dello stderr ha raggiunto la dimensione massima!\n", KRED);
    }

    // Pulisci tutto ed esci
    shell_exit(0);
    return 0;
}
