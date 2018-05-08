#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include "utils.h"
#include "queue.h"
#include <readline/readline.h>
#include <readline/history.h>

int main(int argc, char** argv) {

    int log_out, log_err; // File di log
    int child_out[2], child_err[2]; // Pipe per leggere out ed err dei comandi da eseguire

    //printf("Parsing argv...\n");
    read_options(argc, argv);
    //printf("\n");
    
    // Apri i file di log
    log_out = open("log_stdout.txt", O_RDWR | O_CREAT, 0777);
    log_err = open("log_stderr.txt", O_RDWR | O_CREAT, 0777);

    // Buffer per l'input dell'utente
    char *comando, prompt[100];

    //history queue
    Queue *history = ConstructQueue(50);

    //tab autocomplete
    rl_bind_key('\t', rl_complete);

    while(1) {

        snprintf(prompt, sizeof(prompt), "[%s%s%s @ %s%s%s] -> ", KCYN, getuser(), KNRM, KYEL, getcwd(NULL, 1024), KNRM);

        size_t DIM_BUFFER_COMANDO = 1000;
        comando = (char *) malloc(DIM_BUFFER_COMANDO*sizeof(char));
        comando = readline(prompt);

        //rimuovo caratteri che potrebbero farmi fallire strcmp
        comando[strcspn(comando, "\r\n")] = 0;

        //gestisco history
        NODE *pN = (NODE*) malloc(sizeof (NODE));
        char *cp = (char*) malloc(strlen(comando));
        strcpy(cp, comando);
        pN -> info = cp;
        Enqueue(history, pN);
        add_history(comando);

        comando = strtok (comando," ");
        char *ok;

        if (strcmp(comando, "clear") == 0) clear();
        else if(strcmp(comando, "exit") == 0) break;
        else if(strcmp(comando, "help") == 0) printhelp();
        else if(strcmp(comando, "history") == 0) {
            int lim = 20;
            ok = strtok (NULL," ");
            if(ok != NULL) lim = atoi(ok);
            if(lim < 1 || lim > 50) {
                printf("! ERRORE, LIMITE = 20\n");
                lim = 20;
            }
            lim = history->size - lim;
            if(lim < 0) lim = 0;
            NODE *tmp = history -> head;
            int cont = 0;
            while (tmp != NULL) {
                if(cont >= lim) printf("%s\n", tmp->info);
                tmp = tmp -> prev;
                cont++;
            }
        }
        else if(strcmp(comando, "cd") == 0) {
            int status = chdir(strtok (NULL, " "));
            if(status == -1) {
                printcolor("! Errore: cartella inesistente\n", KRED);
            }
        }
        else { 
            // TODO fare un parse_line, ad exec_cmd va passato un array di argomenti non la stringa...

            /*
                Francesco:
                va bene così? una roba tipo "comando" spazio "argomenti"
                possiamo anche leggere più argomenti tipo "comando -a1 -a2 -a3" ma va estesa questa parte
                stavo pensando di leggere tutti i tokens in una volta e metterli in una coda,
                durante la lettura voglio catturare le variabili d'ambiente, vedere se matchano con quelle dichiarate
                e mettere direttamente nella coda il contenuto della variabile
                poi la coda dovrebbe diventare l'array per i parametri sotto
            */
            Queue *par = ConstructQueue(10);
            char *tmp = strtok (NULL, " ");
            while(tmp != NULL) {
                //printf("%s\n", tmp);
                NODE *pN = (NODE*) malloc(sizeof (NODE));
                char *cp = (char*) malloc(strlen(tmp));
                strcpy(cp, tmp);
                pN -> info = cp;
                Enqueue(par, pN);
                tmp = strtok (NULL, " ");
            }
            char **arg = malloc((par->size + 2) * sizeof(char*));
            arg[0] = (char*) malloc(strlen(comando));
            strcpy(arg[0], comando);

            NODE *n = par->head;
            int i = 1;
            while (n != NULL) {
                arg[i] = (char*) malloc(strlen(n->info));
                strcpy(arg[i], n->info);
                n = n -> prev;
                i++;
            }
            arg[i] = NULL;

            exec_cmd(arg, log_out, log_err, child_out, child_err);
            for(i=0;i<par->size+1;i++) free(arg[i]);
            DestructQueue(par);
        }

        free(comando);
    }


    // Pulisci tutto ed esci
    DestructQueue(history);
    close(log_out);
    close(log_err);
    return 0;
}
