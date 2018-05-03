#include<stdio.h>
#include<unistd.h>
#include<pwd.h>
#include<stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

#define DIM_PATH 256

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

#define clear() printf("\033[H\033[J")

#define PIPE_READ 0
#define PIPE_WRITE 1

int log_out, log_err;
int child_out[2], child_err[2];


//EXAMPLE: printcolor("Ciao\n", KRED);
void printcolor(char *s, char *color) {
    printf("%s%s%s", color, s, KNRM);
}

void getuser() {
  register struct passwd *pw;
  register uid_t uid;
  int c;

  uid = geteuid ();
  pw = getpwuid (uid);
  if (pw) {
    printcolor(pw->pw_name, KNRM);
    //printf("%s", pw->pw_name);
  } else {
    printcolor("user", KNRM);
  }
}


void printcommand(char *path) {
    printf("[");
    getuser();
    //printcolor(user, KNRM);
    printf(" @ ");
    printcolor(path, KYEL);
    printf("]$ ");
}

void printhelp() {
    char *help = "QUI CI METTIAMO UN BELL'HELP!\n";
    printf("%s", help);
}

int exec_cmd(char** args) {
    pid_t pid;

    // Habemus Pipe cit.
    if (pipe(child_out) < 0) perror("Cannot create stdout pipe to child");
    if (pipe(child_err) < 0) perror("Cannot create stderr pipe to child");
    if ((pid = fork())  < 0) perror("Cannot create child");

    if (pid == 0) { // Child
        close(child_out[PIPE_READ]);
        close(child_err[PIPE_READ]);
        dup2(child_out[PIPE_WRITE], 1);
        dup2(child_err[PIPE_WRITE], 2);
        if (execvp(args[0], args) < 0) perror("Cannot execute command");
        close(child_out[PIPE_WRITE]);
        close(child_err[PIPE_WRITE]);
    } else {
        close(child_out[PIPE_WRITE]);
        close(child_err[PIPE_WRITE]);
        wait(NULL);
    }
}

int main() {
    // Apri i file di log
    log_out = open("log_stdout.txt", O_RDWR | O_CREAT, 0777);
    log_err = open("log_stderr.txt", O_RDWR | O_CREAT, 0777);

    char path[DIM_PATH];
    getcwd(path, DIM_PATH);
    while(1) {
        printcommand(path);
        //TEMPORANEO
        int n;
        scanf("%d", &n);
        if(n==1) clear();
        if(n==2) printhelp();
        if(n==3) {
            exec_cmd((char* [] ){"ls", "-al", NULL});

            printf("Child's stdout:\n");
            char* buf;
            buf = (char*)malloc(sizeof(char)+1000);
            if (read(child_out[PIPE_READ], buf, 1000) > 0) {
                printf("%s", buf);
                write(log_out, buf, strlen(buf));
            }
            close(child_out[PIPE_READ]);

            printf("Child's stderr:\n");
            if (read(child_err[PIPE_READ], buf, 1000) > 0) {
                printf("%s", buf);
                write(log_err, buf, strlen(buf));
            }
            close(child_err[PIPE_READ]);
        }
    }



    close(log_out);
    close(log_err);
    return 0;
}
