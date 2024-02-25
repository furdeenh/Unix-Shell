#ifndef SH_H
#define SH_H

#include "get_path.h"

int sh(int argc, char **argv, char **envp);
char *which(char *command, struct pathelement *pathlist);
void grep(const char *pattern, const char *filename);
char *where(char *command, struct pathelement *pathlist);
void list(char *dir);
void printenv(char **envp);
void cd(char *path);
void set_alias(const char *name, const char *command);
void print_history();

#define PROMPTMAX 32
#define MAXARGS 10
#define MAX_HISTORY_SIZE 100

#endif

