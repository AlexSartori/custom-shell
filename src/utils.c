#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/wait.h>

#include "utils.h"


/*
    Copia source nel file di log e nella destinazione (stdout, stdin di un altro processo, ecc.).
*/
void write_to(int source, int log_file, int destination) {
    char* buf = (char*)malloc(sizeof(char)*BUF_SIZE);
    int r = 0;
    while ((r = read(source, buf, BUF_SIZE)) > 0) {
        write(log_file, buf, r);
        write(destination, buf, r);
    }
    free(buf);
}


/*
    Logga e stampa le info sul processo

    Streams:
        0 = log_out
        1 = log_err
        2 = stdout
        3 = stderr
*/
void log_process(struct PROCESS p, char* cmd, int cmd_id, int subcmd_id, int* streams) {
    printf("Log process: %s\n", cmd);
    char* buf = malloc(sizeof(char)*BUF_SIZE);
    time_t curtime; time(&curtime);

    //////////////////////   STDOUT   //////////////////////
    // Scrivi intestazioni
    write(streams[0], "\n\n=============================================\n", 48);
    snprintf(buf, BUF_SIZE, "\
        ID:        #%d.%d\n\
        COMMAND:   %s\n\
        DATE:      %s\
        RET CODE:  %d\n\
        OUTPUT:\n\n", cmd_id, subcmd_id, cmd, ctime(&curtime), p.status);
    write(streams[0], buf, strlen(buf));

    // Scrivi lo stdout del proc sugli output richiesti (stdout o stdin di un altro proc)
    write_to(p.stdout, streams[0], streams[2]);


    //////////////////////   STDERR   //////////////////////
    // Scrivi intestazioni
    write(streams[1], "\n\n=============================================\n", 48);
    snprintf(buf, BUF_SIZE, "\
        ID:        #%d.%d\n\
        COMMAND:   %s\n\
        DATE:      %s\
        RET CODE:  %d\n\
        STDERR:\n\n", cmd_id, subcmd_id, cmd, ctime(&curtime), p.status);
    write(streams[1], buf, strlen(buf));

    // Scrivi lo stdout del proc sugli output richiesti (stdout o stdin di un altro proc)
    write_to(p.stderr, streams[1], streams[3]);
    free(buf);
}


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
struct OPTIONS read_options(int argc, char** argv) {
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

    struct OPTIONS ret; // Contenitore delle opzioni da ritornare, lo riempio coi deafult
    strcpy(ret.log_out_path, "log_stdout.txt");
    strcpy(ret.log_err_path, "log_stderr.txt");

    char c;
    int indexptr;
    opterr = 0; // Non stampare messaggi

    while ((c = getopt_long(argc, argv, ":o:e:m:rh", long_opts, &indexptr)) != -1) {
        switch (c) {
            case 'h':
                print_help();
                break;
            case 'o':
                printf("  Outfile:\t%s\n", optarg);
                strcpy(ret.log_out_path, optarg);
                break;
            case 'e':
                printf("  Errfile:\t%s\n", optarg);
                strcpy(ret.log_err_path, optarg);
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

    return ret;
}


/*
    Se cmd è NULL, stampa l'aiuto sull'uso della shell, altrimenti un aiuto su cmd.
    Se cmd è un comando interno stampa l'aiuto scritto, altrimenti chiama "man cmd".
*/
int print_help(char* cmd) {
    if (cmd == NULL)
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
    else {
        if (strcmp(cmd, "clear") == 0)
            printf("clear: Clear the screen.");
        else if (strcmp(cmd, "exit") == 0)
            printf("exit: Terminate the session and exit the shell.");
        else if (strcmp(cmd, "alias") == 0)
            printf("alias <'a'='b'>: List aliases or register a=b.");
        else if (strcmp(cmd, "cd") == 0)
            printf("cd <dir>: Change the working directory.");
        else if (strcmp(cmd, "history") == 0)
            printf("history <n>: Show the full command history or the last N entries.");
        else {
            int pid = fork(), status;
            if (pid < 0)  perror("Cannot call MAN");
            if (pid == 0) exit(execvp("man", (char*[]) {"man", cmd, NULL}));
            else wait(&status);
            return status;
        }
    }
    return 0;
}


/*
    Ritorna il nome dell'utente attivo (altrimenti la stringa "user")
*/
char* getuser() {
  register struct passwd *pw;
  register uid_t uid;

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
    Stampa il prompt della shell come "[utente@percorso]$"
*/
char* get_prompt(char* prompt) {
    char path[BUF_SIZE];

    if (getcwd(path, BUF_SIZE) == NULL) perror("Path cannot fit in the buffer");
    snprintf(prompt, BUF_SIZE, "[%s%s%s @ %s%s%s] -> ", KCYN, getuser(), KNRM, KYEL, path, KNRM);

    return prompt;
}
