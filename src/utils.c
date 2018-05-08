#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>

#include "utils.h"


/*
    Legge le opzioni con cui la shell è stata chiamata e setta le variabili]
    Opzioni attese:
    -o <file>, --outfile=<file>
    -e <file>, --errfile=<file>
    -m <int>, --maxsize=<int>
    -r, --retcode
    
    Params:
        argc, argv: quelli del main da leggere
    
    See:
        https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Options.html#Getopt-Long-Options
*/
void read_options(int argc, char** argv) {
    /*
        Array di struct OPTION che descrivono le opzioni aspettate
        
        Params:
            name:    nome dell'opzione
            has_arg: se l'opzione richiede un argomento obbligatorio
            flag:    quando questa opzione verrà trovata, val verrà messo dove punta flag
            val:     vedi sopra, (se flag == NULL, viene ritornato val)
    */
    struct option long_opts[] = {
        { "outfile", required_argument, 0, 'o' },
        { "errfile", required_argument, 0, 'e' },
        { "maxsize", required_argument, 0, 'm' },
        { "retcode", no_argument,       0, 'r' },
        { "help",    no_argument,       0, 'h' },
        { 0, 0, 0, 0 } // Serve come delimitatore finale dell'array
    };
    
    char c;
    int indexptr;
    opterr = 0; // Non stampare messaggi
    
    while ((c = getopt_long(argc, argv, ":o:e:m:rh", long_opts, &indexptr)) != -1) {
        switch (c) {
            case 'h':
                printhelp();
                break;
            case 'o':
                printf("  Outfile:\t%s\n", optarg);
                break;
            case 'e':
                printf("  Errfile:\t%s\n", optarg);
                break;
            case 'm':
                printf("  Max Size:\t%s [bytes]\n", optarg);
                break;
            case 'r':
                printf("  Record process return code: yes\n");
                break;
            
            case ':':
                fprintf(stderr, "Argument required for -%c\n", optopt);
                break;
            case '?':
                fprintf(stderr, "Unknown option: %s\n", argv[optind-1]);
                break;
            default:
                fprintf(stderr, "Errore nella lettura delle opzioni (codice opzione: %c)\n", c);
                break;
        }
    }
    
    // Se sono rimasti argomenti non riconosciuti avverti l'utente
    for (int i = optind; i < argc; i++)
        fprintf(stderr, "Unrecognized argument: %s\n", argv[i]);
    
    return;
}



/*
    Stampa l'aiuto
*/
void printhelp() {
    printf("\n\n\
                     C u s t o m   S h e l l\n\
                   Progetto Sistemi Operativi 1\n\n\n\
  Usage:\n\
      ./shell <options>\n\n\
  Options:\n\
      -h, --help           Mostra questo messaggio\n\
      -o, --outfile        File di log dello stdout dei comandi\n\
      -e, --errfile        File di log dello stderr dei comandi\n\
      -m, --maxsize        Dimensione massima in bytes dei file di log\n\
      -r, --retcode        Registra anche i codici di ritorno dei comandi\n\n\n");
}


/*
    Ritorna il nome dell'utente attivo (altrimenti la stringa "user")
*/
char* getuser() {
  register struct passwd *pw;
  register uid_t uid;
  int c;

  uid = geteuid ();
  pw = getpwuid (uid);
  return pw ? pw->pw_name : "user";
}


/*
    Stampa a video s con il colore indicato
    ESEMPIO: printcolor("Ciao\n", KRED);
*/
void printcolor(char *s, char *color) {
    printf("%s%s%s", color, s, KNRM);
}


/*
    Esegui il comando passato come figlio e registra stdout e stderr.
    Parametro: array degli argomenti, il primo elemento sarà il comando
*/
int exec_cmd(char** args, int log_out, int log_err, int *child_out, int *child_err) {
    // Codice di ritorno del processo

    int ret_code = -1;
    pid_t pid;

    // Habemus Pipe cit.
    if (pipe(child_out) < 0) perror("Cannot create stdout pipe to child");
    if (pipe(child_err) < 0) perror("Cannot create stderr pipe to child");
    if ((pid = fork())  < 0) perror("Cannot create child");

    if (pid == 0) {
        // Child:
        //   - Chiudi pipe-end che non servono
        //   - Redirigi out/err ed esegui il comando
        //   - Chiudi i pipe

        close(child_out[PIPE_READ]);
        close(child_err[PIPE_READ]);
        dup2(child_out[PIPE_WRITE], 1);
        dup2(child_err[PIPE_WRITE], 2);
        ret_code = execvp(args[0], args);
        exit(errno);
        if (ret_code < 0) perror("Cannot execute command");
        close(child_out[PIPE_WRITE]);
        close(child_err[PIPE_WRITE]);
    } else {
        // Parent: chiudi i pipe che non servono e aspetta il figlio
        int status;

        wait(&status);       /*you made a exit call in child you 
                           need to wait on exit status of child*/

        //if(WIFEXITED(status)) print("child exited with = %d\n",WEXITSTATUS(status));
        if(WIFEXITED(status) && WEXITSTATUS(status) != 0) printcolor("! Comando non esistente\n",KRED);

        close(child_out[PIPE_WRITE]);
        close(child_err[PIPE_WRITE]);
        //wait(NULL);
    }

    // Leggi stdout del figlio e scrivilo su stdout e logfile
    char* buf = (char*)malloc(sizeof(char)*BUF_SIZE);
    int r = 0;
    while ((r = read(child_out[PIPE_READ], buf, BUF_SIZE)) > 0) {
        buf[r] = '\0';
        fprintf(stdout, "%s", buf);
        write(log_out, buf, r);
    } 
    close(child_out[PIPE_READ]);
    

    // Leggi stderr del figlio e scrivilo su stderr e logfile
    while ((r = read(child_err[PIPE_READ], buf, BUF_SIZE)) > 0) {
        buf[r] = '\0';
        fprintf(stderr, "%s", buf);
        write(log_err, buf, r);
    }
    close(child_err[PIPE_READ]);
    free(buf);
    return ret_code;
}
