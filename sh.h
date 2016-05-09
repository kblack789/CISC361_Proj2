#include "get_path.h"

int pid;
int sh( int argc, char **argv, char **envp);
char *which(char *command, struct pathelement *pathlist);
char *where(char *command, struct pathelement *pathlist);
void list ( char *dir );
void printenv(char **envp);
void cd(char *pth, char *prev);
void printwd();
void printpid();
void shell_exit();
void print_env_variable(char *env_var);
void printenv(char **envp);
void kill_process(char **args, int count);
void rmv_new_line(char* string);
int parse(char* command, char** parameters);
int wildcard_check(char *check_string);
void set_env(char* envname, char* envval);
void free_pathlist(struct pathelement *head);
void sigintHandler(int sig_num);
void signalSTPHandler(int sig_num);
char *list_wildcard (char *dir, char* wildcard);
char * command_to_string(char ** args, int arg_count);

#define PROMPTMAX 32
#define MAXARGS 10
