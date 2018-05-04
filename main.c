#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

#define BUF_SIZE 1024       // Dimensione dei buffer

#define KNRM  "\x1B[0m"     // Codici ASCII dei colori
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

#define PIPE_READ 0
#define PIPE_WRITE 1

#define clear() printf("\033[H\033[J")


int log_out, log_err;     // File di log
int child_out[2],         // Pipe per leggere out ed err dei comandi da eseguire
    child_err[2];


/*
    Stampa a video s con il colore indicato
    ESEMPIO: printcolor("Ciao\n", KRED);
*/
void printcolor(char *s, char *color) {
    printf("%s%s%s", color, s, KNRM);
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
    Stampa il prompt della shell come "[utente@percorso]$"
*/
void print_prompt() {
    char* user = getuser();
    char path[BUF_SIZE];

    printf("[");
    printcolor(user, KNRM);
    printf(" @ ");
    getcwd(path, BUF_SIZE);
    printcolor(path, KYEL);
    printf("]$ ");
}


/*
    Stampa l'aiuto
*/
void printhelp() {
    char *help = "QUI CI METTIAMO UN BELL'HELP!\n";
    printf("%s", help);
}


/*
    Esegui il comando passato come figlio e registra stdout e stderr.
    Parametro: array degli argomenti, il primo elemento sarà il comando
*/
int exec_cmd(char** args) {
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
        if (ret_code < 0) perror("Cannot execute command");
        close(child_out[PIPE_WRITE]);
        close(child_err[PIPE_WRITE]);
    } else {
        // Parent: chiudi i pipe che non servono e aspetta il figlio
        close(child_out[PIPE_WRITE]);
        close(child_err[PIPE_WRITE]);
        wait(NULL);
    }

    // Leggi stdout del figlio e scrivilo su stdout e logfile
    char* buf = (char*)malloc(sizeof(char)*BUF_SIZE);
    while (read(child_out[PIPE_READ], buf, BUF_SIZE) > 0) {
        fprintf(stdout, "%s", buf);
        write(log_out, buf, strlen(buf));
    }
    close(child_out[PIPE_READ]);

    // Leggi stderr del figlio e scrivilo su stderr e logfile
    while (read(child_err[PIPE_READ], buf, BUF_SIZE) > 0) {
        fprintf(stderr, "%s", buf);
        write(log_err, buf, strlen(buf));
    }
    close(child_err[PIPE_READ]);

    free(buf); // Mannaggia al cazzo che me le dimentico sempre ste free
    return ret_code;
}


/*
    Continua a leggere l'input dell'utente ed interpretalo
*/
int main() {
    // Apri i file di log
    log_out = open("log_stdout.txt", O_RDWR | O_CREAT, 0777);
    log_err = open("log_stderr.txt", O_RDWR | O_CREAT, 0777);

    // Buffer per l'input dell'utente
    char *comando;

    while(1) {
        print_prompt();

        size_t dim = 1000;
        comando = (char *) malloc(dim*sizeof(char));
        getline(&comando, &dim, stdin);

        //rimuovo caratteri che potrebbero farmi fallire strcmp
        comando[strcspn(comando, "\r\n")] = 0;

        // Controlla se è un comando interno ed interpretalo,
        // altrimenti eseguilo come figlio
        if (strcmp(comando, "clear") == 0) clear();
        else if(strcmp(comando, "exit") == 0) break;
        else if(strcmp(comando, "help") == 0) printhelp();
        else {
            // TODO fare un parse_line, ad exec_cmd va passato un array di argomenti non la stringa...
            exec_cmd((char* []){ comando, NULL });
        }

        free(comando);
    }


    // Pulisci tutto ed esci
    close(log_out);
    close(log_err);
    return 0;
}
