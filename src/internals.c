#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <readline/history.h>

#include "utils.h"
#include "vector.h"
#include "internals.h"


vector vector_alias;

void vector_alias_initializer() {
    vector_init(&vector_alias);
}

char* parse_alias(char* comando) {
    char *init = (char*)malloc(sizeof(char)*(strlen(comando)));
    strcpy(init, comando);
    char* token = strtok(comando, " ");
    char *ns;
    int ok = 0;
    for(int i=0;i<vector_total(&vector_alias);i++) {
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
    for(int i=0;i<vector_total(&vector_alias);i++) {
        tmp = (elemento*)vector_get(&vector_alias, i);
        printf("%s = %s\n", tmp->name, tmp->data);
    }
    return 0;
}

int search_alias(elemento* el) {
    for(int i=0;i<vector_total(&vector_alias);i++) {
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
    int active = 0;
    int first = 1;
    int alias_index = 0;
    int content_index = 0;
    for(int i=0;copy_line[i]!='\0';i++) {
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

int print_history(char *hist_arg) {
    
	HIST_ENTRY** hist = history_list();
    int n; // Quanti elementi della cronologia mostrare

    // Se non è specificato li mostro tutti
    if (hist_arg == NULL) n = history_length;
    else n = min(atoi(hist_arg), history_length);

    for (int i = history_length - n; i < history_length; i++)
        printf("  %d\t%s\n", i + history_base, hist[i]->line);

    return 0;
    
}

char *expand_wildcar(char *s) {
    char *ns = (char* ) malloc(sizeof(char) * 1024);
    char *search = (char* ) malloc(sizeof(char) * 1024);
    int tmp = 0;
    int found = 0;
    int letters_before = 0;
    int letters_after = 0;
    for(int i=0; s[i]; i++) {
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

    if(found == 1 && letters_before == 1) {
        for(int i=0; s[i]; i++) {
            if(s[i] != '*') ns[i] = s[i];
            else {
                i++;
                int pos = 0;
                while(s[i] != ' ' && s[i] != '\0') {
                    search[pos] = s[i];
                    i++;
                    pos++;
                }
                search[pos] = '\0';
                break;
            }
        }
        snprintf(ns, 1024, "%s | grep %s$", ns, search);
        //printf("%s\n", ns);
    }

    if(found == 1 && letters_before != 1 && letters_after == 0) {
        for(int i=0; s[i]; i++) {
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
        snprintf(ns, 1024, "%s | grep ^%s", ns, search);
        //printf("%s\n", ns);
    }

    if(found == 2) {
        for(int i=0; s[i]; i++) {
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
        snprintf(ns, 1024, "%s | grep %s", ns, search);
        //printf("%s\n", ns);
    }

    if(found == 1 && letters_before != 1 && letters_after != 0) {
        char *search1 = (char* ) malloc(sizeof(char) * 1024);
        for(int i=0; s[i]; i++) {
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
        snprintf(ns, 1024, "%s | grep ^%s | grep %s$", ns, search, search1);
        //printf("%s\n", ns);
    }


    return ns;
}
