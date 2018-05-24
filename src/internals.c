#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <readline/history.h>

#include "../headers/utils.h"
#include "../headers/vector.h"
#include "../headers/internals.h"


vector vector_alias;
vector vector_vars;

void vectors_initializer() {
    vector_init(&vector_alias);
    vector_init(&vector_vars);
}

//***************** GESTIONE ALIAS

char* parse_alias(char* comando) {
    char *init = (char*)malloc(sizeof(char)*(strlen(comando)));
    strcpy(init, comando);
    char* token = strtok(comando, " ");
    char *ns;
    int ok = 0, i;
    for(i=0;i<vector_total(&vector_alias);i++) {
        elemento* tmp;
        tmp = (elemento*)vector_get(&vector_alias, i);

        if(strcmp(token, tmp->name) == 0) {
            ok = 1;
            ns = (char*)malloc(sizeof(char)*(strlen(comando) + strlen(tmp->data)));
            strcpy(ns, tmp->data);
            token = strtok(NULL, " ");
            while(token) {
                strcat(ns, " ");
                strcat(ns, token);
                token = strtok(NULL, " ");
            }
            strcat(ns, "\0");
            break;
        }

    }

    if(ok == 0) return init;
    else return ns;
}

int list_alias() {
    elemento* tmp;
    int i;
    for(i=0; i<vector_total(&vector_alias); i++) {
        tmp = (elemento*)vector_get(&vector_alias, i);
        printf("%s = %s\n", tmp->name, tmp->data);
    }
    return 0;
}

int search_alias(elemento* el) {
    int i;
    for(i=0; i<vector_total(&vector_alias); i++) {
        elemento* tmp;
        tmp = (elemento*)vector_get(&vector_alias, i);

        if(strcmp(el->name, tmp->name) == 0) {
            tmp -> data = el -> data;
            return 1;
        }

    }

    return 0;
}


int make_alias(char *copy_line) {
	char *alias = (char*)malloc(sizeof(char) * strlen(copy_line));
    char *content = (char*)malloc(sizeof(char) * strlen(copy_line));
    int active = 0, first = 1, alias_index = 0, content_index = 0, i;

    for(i=0; copy_line[i]!='\0'; i++) {
        if(active == 1 && first == 1) alias[alias_index++] = copy_line[i];
        if(active == 1 && first == 0) content[content_index++] = copy_line[i];
        if(copy_line[i] == '\'' && active == 0) active = 1;
        else if(copy_line[i] == '\'' && active == 1) {
            active = 0;
            first = 0;
        }
    }
    alias[alias_index-1] = '\0';
    content[content_index-1] = '\0';

    if(strlen(alias) == 0 || strlen(content) == 0) {
        printcolor("! Error: format \'alias\'=\'command\'\n", KRED);
    } else {
        elemento *insert = (elemento*)malloc(sizeof(elemento));
        insert -> name = alias;
        insert -> data = content;
        if (search_alias(insert) == 1) {
            printf("Warning: alias \'%s\' has been overwritten\n", alias);
        } else {
            vector_add(&vector_alias, insert);
        }
    }

    return 0;
}

//***************** GESTIONE VARIABILI

int list_vars() {
    elemento* tmp;
    int i;
    for(i=0; i<vector_total(&vector_vars); i++) {
        tmp = (elemento*)vector_get(&vector_vars, i);
        printf("%s = %s\n", tmp->name, tmp->data);
    }
    return 0;
}

int search_var_elemento(elemento* el) {
    int i;
    for(i=0; i<vector_total(&vector_vars); i++) {
        elemento* tmp;
        tmp = (elemento*)vector_get(&vector_vars, i);

        if(strcmp(el->name, tmp->name) == 0) {
            tmp -> data = el -> data;
            return 1;
        }

    }

    return 0;
}

char* search_var_name(char *name) {
    int i;
    for(i=0; i<vector_total(&vector_vars); i++) {
        elemento* tmp;
        tmp = (elemento*)vector_get(&vector_vars, i);

        if(strcmp(name, tmp->name) == 0) {
            return tmp -> data;
        }

    }
    return NULL;
}

char* parse_vars(char *comando) {
    char *c = (char* )malloc(sizeof(char) * strlen(comando)*2);
    char *var = (char* )malloc(sizeof(char) * strlen(comando));
    int j = 0;
    int i = 0;
    int z = 0;
    char* data;
    int k = 0;

    for(i = 0; comando[i]; i++) {
        if(comando[i] != '$') {
            c[j] = comando[i];
            j++;
        } else {
            z = 0;
            i++;
            while(comando[i] != '\0' && comando[i] != ' ' && comando[i] != '$') {
                var[z] = comando[i];
                i++;
                z++;
            }
            var[z] = '\0';
            //printf("FOUND VAR: %s\n", var);
            data = search_var_name(var);

            if(data == NULL) {
                printf("Error: variable not found\n");
                return "";
            }
            //printf("FOUND DATA: %s\n", data);
            for(k=0; data[k]; k++) {
                c[j] = data[k];
                j++;
            }
            i--;
        }
    }
    c[j] = '\0';
    free(var);
    //printf("%s\n", c);
    return c;
}

int make_var(char *copy_line) {
    char *var = (char*)malloc(sizeof(char) * strlen(copy_line));
    char *content = (char*)malloc(sizeof(char) * strlen(copy_line));
    int active = 0, first = 1, var_index = 0, content_index = 0, i;

    for(i=0; copy_line[i]!='\0'; i++) {
        if(active == 1 && first == 1) var[var_index++] = copy_line[i];
        if(active == 1 && first == 0) content[content_index++] = copy_line[i];
        if(copy_line[i] == '\'' && active == 0) active = 1;
        else if(copy_line[i] == '\'' && active == 1) {
            active = 0;
            first = 0;
        }
    }
    var[var_index-1] = '\0';
    content[content_index-1] = '\0';

    if(strlen(var) == 0 || strlen(content) == 0) {
        printcolor("! Error: format \'var\'=\'content\'\n", KRED);
    } else {
        elemento *insert = (elemento*)malloc(sizeof(elemento));
        insert -> name = var;
        insert -> data = content;
        if (search_var_elemento(insert) == 1) {
            printf("Warning: var \'%s\' has been overwritten\n", var);
        } else {
            vector_add(&vector_vars, insert);
        }
    }

    return 0;
}

void inc_var(char *name) {
    int i;
    for(i=0; i<vector_total(&vector_vars); i++) {
        elemento* tmp;
        tmp = (elemento*)vector_get(&vector_vars, i);

        if(strcmp(name, tmp->name) == 0) {
            snprintf(tmp->data, 100, "%d", atoi(tmp -> data) + 1);
        }

    }
}

void azz_var(char *name) {
    int i;
    for(i=0; i<vector_total(&vector_vars); i++) {
        elemento* tmp;
        tmp = (elemento*)vector_get(&vector_vars, i);

        if(strcmp(name, tmp->name) == 0) {
            snprintf(tmp->data, 100, "%d", 0);
        }
    }
}

//***************** GESTIONE HISTORY

int print_history(char *hist_arg) {
    /*
	HIST_ENTRY** hist = history_list();
    int n; // Quanti elementi della cronologia mostrare

    // Se non è specificato li mostro tutti
    if (hist_arg == NULL) n = history_length;
    else n = min(atoi(hist_arg), history_length);

    int i;
    for (i = history_length - n; i < history_length; i++)
        printf("  %d\t%s\n", i + history_base, hist[i]->line);

    return 0;
    */

}

//***************** GESTIONE WILDCARDS

char *expand_wildcar(char *s) {
    char *ns = (char* ) malloc(sizeof(char) * 1024);
    char *ns1 = (char* ) malloc(sizeof(char) * 1024);
    char *search = (char* ) malloc(sizeof(char) * 1024);
    int tmp = 0, found = 0, letters_before = 0, letters_after = 0, i = 0, pos = 0;

    for(i=0;i<1024;i++) ns[i] = '\0';


    for(i=0; s[i]; i++) {
        if(s[i] == ' ') tmp = 0;
        else tmp++;

        if(found > 0) letters_after++;

        if(s[i] == '*' && found == 0) {
            letters_before = tmp;
            found++;
        } else if(s[i] == '*' && found > 0) {
            found++;
        }
    }



    if(found > 2) {
        ns[0] = '\0';
        printf("ERROR\n");
        return ns;
    }

    if(found == 0) {
        free(ns);
        return s;
    }

    //printf("found: %d letters_before: %d\n", found, letters_before);

    if(found == 1 && letters_before == 1) {
        //int i;
        for(i=0; s[i]; i++) {
            //printf("%c\n", s[i]);
            if(s[i] != '*') ns[i] = s[i];
            else {
                i++;
                pos = 0;
                while(s[i] != ' ' && s[i] != '\0') {
                    search[pos] = s[i];
                    i++;
                    pos++;
                }
                search[pos] = '\0';
                break;
            }
        }
        //printf("%s\n", ns);
        //ns[i-1] = '\0';
        snprintf(ns1, 1024, "%s | grep %s$", ns, search);
        //printf("%s\n", ns1);
    }

    if(found == 1 && letters_before != 1 && letters_after == 0) {
        //int i;
        for (i=0; s[i]; i++) {
            if(s[i] != '*') ns[i] = s[i];
            else {
                int tmp = i;
                while(ns[tmp] != ' ') tmp--;
                ns[tmp] = '\0';
                i--;
                int pos = letters_before - 2;
                while(s[i] != ' ') {
                    search[pos] = s[i];
                    i--;
                    pos--;
                }
                search[letters_before - 1] = '\0';
                break;
            }
        }
        snprintf(ns1, 1024, "%s | grep ^%s", ns, search);
        //printf("%s\n", ns);
    }

    if(found == 2) {
        //int i;
        for(i=0; s[i]; i++) {
            if(s[i] != '*') ns[i] = s[i];
            else {
                i++;
                int pos = 0;
                while(s[i] != ' ' && s[i] != '\0' && s[i] != '*') {
                    search[pos] = s[i];
                    i++;
                    pos++;
                }
                search[pos] = '\0';
                break;
            }
        }
        snprintf(ns1, 1024, "%s | grep %s", ns, search);
        //printf("%s\n", ns);
    }

    if(found == 1 && letters_before != 1 && letters_after != 0) {
        char *search1 = (char* ) malloc(sizeof(char) * 1024);
        //int i;
        for(i=0; s[i]; i++) {
            if(s[i] != '*') ns[i] = s[i];
            else {
                int tmp = i;
                int start = i+1;
                while(ns[tmp] != ' ') tmp--;
                ns[tmp] = '\0';
                i--;
                int pos = letters_before - 2;
                while(s[i] != ' ') {
                    search[pos] = s[i];
                    i--;
                    pos--;
                }
                i = start;
                int pos2 = 0;
                while(s[i] != ' ' && s[i] != '\0') {
                    search1[pos2] = s[i];
                    i++;
                    pos2++;
                }
                search1[pos2] = '\0';
                search[letters_before - 1] = '\0';
                break;
            }
        }
        snprintf(ns1, 1024, "%s | grep ^%s | grep %s$", ns, search, search1);
        //printf("%s\n", ns);
    }
    //printf("%s\n", ns);
    //printf("%s\n", ns1);
    return ns1;
}
