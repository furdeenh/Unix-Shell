#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "sh.h"

#define PROMPTMAX 32
#define MAXARGS 16
#define MAX_CANON 200

char *which(char *command, struct pathelement *pathlist);
void grep(const char *pattern, const char *filename);
char *where(char *command, struct pathelement *pathlist);
void list(char *dir);
char *substitute_environment_variables(const char *line, char **envp);
void update_ACC_environment_variable(char *value);
void exit_shell(const char *arg);
void execute_commands_from_file(const char *filename, char **envp);
int execute_command(const char *command, char *args[], char **envp);

int sh(int argc, char **argv, char **envp) {
    char *prompt = calloc(PROMPTMAX, sizeof(char));
    char *commandline = calloc(MAX_CANON, sizeof(char));
    char *command, *arg, *commandpath, *p, *pwd, *owd;
    char **args = calloc(MAXARGS, sizeof(char*));
    int uid, status, go = 1;
    struct passwd *password_entry;
    char *homedir;
    struct pathelement *pathlist;

    uid = getuid();
    password_entry = getpwuid(uid);   /* get passwd info */
    homedir = password_entry->pw_dir;  /* Home directory to start out with*/

    if ((pwd = getcwd(NULL, PATH_MAX + 1)) == NULL)
    {
        perror("getcwd");
        exit(2);
    }
    owd = calloc(strlen(pwd) + 1, sizeof(char));
    memcpy(owd, pwd, strlen(pwd));
    prompt[0] = ' ';
    prompt[1] = '\0';

    /* Put PATH into a linked list */
    pathlist = get_path();

    if (argc > 1) {
        execute_commands_from_file(argv[1], envp);
        go = 0;
    }

    while (go)
    {
        /* print your prompt */
        printf("%s[/usa/furdeenh/cisc361-project-2]> ", prompt);

        /* get command line and process */
        fgets(commandline, MAX_CANON, stdin);

        /* Remove the newline character from commandline */
        commandline[strlen(commandline) - 1] = '\0';

        /* Split the commandline into command and arguments */
        command = strtok(commandline, " ");
        arg = strtok(NULL, " ");

        if (command != NULL) {
            /* check for each built-in command and implement */
            if (strcmp(command, "exit") == 0) {
                exit_shell(arg);
                go = 0;
                continue;
            } else if (strcmp(command, "list") == 0) {
                list(pwd);
                continue;
            } else if (strcmp(command, "cd") == 0) {
                if (arg != NULL) {
                    chdir(arg);
                } else {
                    chdir(homedir);
                }
                continue;
            } else if (strcmp(command, "cd~") == 0) {
                chdir(homedir);
                continue;
            } else if (strstr(commandline, "grep ") == commandline) {
                if (arg != NULL) {
                    char *pattern = strtok(arg, " ");
                    char *file = strtok(NULL, "\n");
                    if (pattern != NULL && file != NULL) {
                        grep(pattern, file);
                    } else {
                        printf("Usage: grep <pattern> <file>\n");
                    }
                } else {
                    printf("Usage: grep <pattern> <file>\n");
                }
                continue;
            } else if (strstr(commandline, "alias ") == commandline) {
                printf("alias not implemented\n");
                continue;
            } else if (strstr(command, "history") == commandline) {
                printf("history not implemented\n");
                continue;
            } else if (strcmp(command, "setenv") == 0) {
                char *name = arg;
                char *value = strtok(NULL, "\n");
                if (name != NULL && value != NULL) {
                    setenv(name, value, 1);
                    printf("%s=%s\n", name, value);
                } else {
                    printf("Usage: setenv <name> <value>\n");
                }
                continue;
            } else if (strcmp(command, "addacc") == 0) {
                char *value = arg;
                if (value != NULL) {
                    char *current_acc = getenv("ACC");
                    if (current_acc == NULL) {
                        setenv("ACC", "0", 1);
                        current_acc = "0";
                    }
                    int new_acc = atoi(current_acc) + atoi(value);
                    char new_acc_str[16];
                    sprintf(new_acc_str, "%d", new_acc);
                    setenv("ACC", new_acc_str, 1);
                } else {
                    printf("Usage: addacc <value>\n");
                }
                continue;
            } else if (strstr(commandline, "?") == commandline) {
                int exit_status = WEXITSTATUS(status);
                if (exit_status == 0) {
                    if (arg != NULL) {
                        execute_commands_from_file(arg, envp);
                    } else {
                        printf("Usage: ?<filename>\n");
                    }
                }
                continue;
            }

            /* else program to exec */
            {
                /* find it */
                char *path = which(command, pathlist);

                if (path != NULL) {
                    if (access(path, X_OK) == 0) {
                        pid_t child_pid = fork();
                        if (child_pid == 0) {
                            /* Child process */
                            if (arg != NULL) {
                                /* Execute the command with the argument */
                                char *new_args[] = {command, arg, NULL};
                                execute_command(path, new_args, envp);
                            } else {
                                /* Execute the command without an argument */
                                char *new_args[] = {command, NULL};
                                execute_command(path, new_args, envp);
                            }
                            /* execve only returns if there's an error */
                            perror("execve");
                            exit(1);
                        } else {
                            /* Parent process */
                            waitpid(child_pid, &status, 0);
                        }
                    } else {
                        fprintf(stderr, "%s: Command not found or not executable.\n", command);
                    }
                } else {
                    fprintf(stderr, "%s: Command not found.\n", command);
                }
            }
        }
    }

    return 0;
}

char *which(char *command, struct pathelement *pathlist) {
    while (pathlist) {
        char *path = malloc(PATH_MAX + 1);
        snprintf(path, PATH_MAX, "%s/%s", pathlist->element, command);
        if (access(path, F_OK) == 0) {
            return path;
        }
        free(path);
        pathlist = pathlist->next;
    }
    return NULL;
}

void list(char *dir) {
    struct dirent *entry;
    DIR *dp = opendir(dir);

    if (!dp) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dp))) {
        printf("%s\n", entry->d_name);
    }

    closedir(dp);
}

void grep(const char *pattern, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("fopen");
        return;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, file)) != -1) {
        if (strstr(line, pattern) != NULL) {
            printf("%s", line);
        }
    }

    free(line);
    fclose(file);
}

char *substitute_environment_variables(const char *line, char **envp) {
    char *result = calloc(MAX_CANON, sizeof(char));
    const char *p = line;
    char *r = result;
    char *var_start;
    char *var_end;

    while (*p) {
        if (*p == '$' && *(p + 1) == '$') {
            p++;
            int pid = getpid();
            sprintf(r, "%d", pid);
            r += strlen(r);
            p++;
        } else if (*p == '$' && *(p + 1) == '?') {
            p++;
            int status = 0;
            if (WIFEXITED(status)) {
                status = WEXITSTATUS(status);
            }
            sprintf(r, "%d", status);
            r += strlen(r);
            p++;
        } else if (*p == '$') {
            var_start = (char *)(p + 1);
            var_end = var_start;

            while (*var_end && isalnum(*var_end)) {
                var_end++;
            }

            if (var_start != var_end) {
                char var_name[64];
                strncpy(var_name, var_start, var_end - var_start);
                var_name[var_end - var_start] = '\0';
                char *env_var = getenv(var_name);
                if (env_var != NULL) {
                    strcpy(r, env_var);
                    r += strlen(env_var);
                }
                p = var_end;
            } else {
                *r++ = *p++;
            }
        } else {
            *r++ = *p++;
        }
    }

    *r = '\0';
    return result;
}

void update_ACC_environment_variable(char *value) {
    char *current_acc = getenv("ACC");
    if (current_acc == NULL) {
        setenv("ACC", "0", 1);
        current_acc = "0";
    }
    int new_acc = atoi(current_acc) + atoi(value);
    char new_acc_str[16];
    sprintf(new_acc_str, "%d", new_acc);
    setenv("ACC", new_acc_str, 1);
}

void exit_shell(const char *arg) {
    int exit_code = 0;
    if (arg != NULL) {
        if (isdigit(*arg)) {
            exit_code = atoi(arg);
        }
    }
    exit(exit_code);
}

int execute_command(const char *command, char *args[], char **envp) {
    execve(command, args, envp);
    perror("execve");
    exit(1);
}

void execute_commands_from_file(const char *filename, char **envp) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("fopen");
        return;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    while ((read = getline(&line, &len, file)) != -1) {
        line[strlen(line) - 1] = '\0'; // Remove the newline character

        if (strcmp(line, "exit") == 0) {
            break;
        }

        char *substituted_line = substitute_environment_variables(line, envp);
        char *args[MAXARGS];
        char *token = strtok(substituted_line, " ");
        int i = 0;
        while (token != NULL && i < MAXARGS) {
            args[i] = token;
            token = strtok(NULL, " ");
            i++;
        }
        args[i] = NULL;

        int status = 0;
        int pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            int result = execute_command(args[0], args, envp);
            exit(result);
        } else {
            waitpid(pid, &status, 0);
        }
    }

    free(line);
    fclose(file);
}

void print_prompt(const char *prompt) {
    printf("%s> ", prompt);
}

void setenv_var(char *name, char *value) {
    if (setenv(name, value, 1) == 0) {
        printf("%s=%s\n", name, value);
    } else {
        perror("setenv");
    }
}

