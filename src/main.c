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

#include "exec.h"
#include "utils.h"
#include "vector.h"
#include "internals.h"


// Buffer per l'input dell'utente e il prompt
char *comando, prompt[BUF_SIZE];
int log_out, log_err; // File di log
int cmd_id = 0, subcmd_id = 0; // ID incrementale del comando, per il file di log

void init_shell(struct OPTIONS opt) {
    comando = (char *)malloc(BUF_SIZE*sizeof(char)),
    //tab autocomplete
    rl_bind_key('\t', rl_complete);
    // Catch Ctrl-C
    signal(SIGINT, sigHandler);
    // Apri i file di log
    log_out = open(opt.log_out_path, O_RDWR | O_CREAT, 0644);
    log_err = open(opt.log_err_path, O_RDWR | O_CREAT, 0644);
}

void shell_exit(int status) {
    printf("\nExiting normally...\n");
    // Libera i buffer
    free(comando);
    // Chiudi i file
    close(log_out);
    close(log_err);
    // Esci
    exit(status);
}

void sigHandler(int sig) {
    shell_exit(0);
}

int main(int argc, char** argv) {
    struct OPTIONS opt = read_options(argc, argv);
    init_shell(opt);
    vector_alias_initializer();

    while(1) {
        get_prompt(prompt);
        comando = readline(prompt);

        if (comando == NULL) break; // Era una linea vuota
        if (strlen(comando) == 0) continue; // Invio

        // Gestisco history
        add_history(comando);

        // Controlla se ci sono punto e virgola
        int pv = 0;
        int cont = 0;
        for(int i = 0; comando[i] != '\0'; i++) if(comando[i] == ';') pv++;
        char* comandi[pv + 1];
        if (pv!=0) {
            cont = gest_pv(comandi, comando);
        } else {
            cont = 1;
            comandi[cont - 1] = comando;
        }

        for (int j=0; j<cont; j++){
            cmd_id++;
            subcmd_id = 0;

            if(comandi[j] == NULL || strlen(comandi[j]) == 0 ) continue;
            //char tmp[BUF_SIZE]; strcpy(tmp, comandi[j]);

            //Gestici &&
            int br;
            br = gest_and(comandi[j], &cmd_id, subcmd_id , log_out, log_err);
            if (br == -1){
                 continue; // Ho incontrato break nell'&&
            } else {
                 comandi[j]= comandi[j] + br;
            }

            // Controlla se il comando è fatto di soli spazi
            int spazi = 0;
            for (int k=0; k<strlen(comandi[j]); k++) if (comandi[j][k]== ' ') spazi++;
            if (strlen(comandi[j]) == spazi) continue;

            // Tolgo gli spazi finali
            int t = strlen(comandi[j]) - 1;
            while (comandi[j][t] == ' ') t--;
            comandi[j][t+1] = '\0';
            // Se l'ultimo carattere è un pipe lo tolgo
            if (comandi[j][t] == '|') comandi[j][t] = '\0';

            struct PROCESS p = exec_line(comandi[j], cmd_id, &subcmd_id, log_out, log_err);
            if (p.status != 0) printcolor("! Error: Cannot execute command.\n", KRED);
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
