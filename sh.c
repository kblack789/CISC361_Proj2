#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <glob.h>
#include "sh.h"
#include "mylib.h"
#define max_buf_size 1024
#define tok_buf_size 64
#define MAX_SIZE 2048
#define command_list_size 13

struct history_list *history_head = NULL; //initialize head node for history list
struct history_list *history_tail = NULL; //initialize tail node for history list

int sh( int argc, char **argv, char **envp )
{
  char *prompt = calloc(PROMPTMAX, sizeof(MAX_SIZE));
  char *commandline = calloc(MAX_CANON, sizeof(char));
  char *command, *arg, *commandpath, *p, *pwd, *owd;
  char **args = calloc(MAXARGS, sizeof(char*));
  char **args_wild = calloc(max_buf_size, sizeof(char*));
  int uid, i, status, argsct, go = 1;
  struct passwd *password_entry;
  char *homedir;
  struct pathelement *pathlist;
  char * prev_enviornment = malloc(sizeof(prev_enviornment)); 
  
  //list of built in commands for the shell
  const char *command_list[command_list_size] = {"which", "where", "exit", "cd", "kill", "pid", "printenv", "setenv", "prompt", "history", "list", "alias", "pwd"};

  uid = getuid();
  password_entry = getpwuid(uid); //get passwd info 
  homedir = password_entry->pw_dir;	//Home directory to start out with

  if ( (pwd = getcwd(NULL, PATH_MAX+1)) == NULL )
  {
    perror("getcwd");
    exit(2);
  }
  owd = calloc(strlen(pwd) + 1, sizeof(char));
  memcpy(owd, pwd, strlen(pwd));
  prompt[0] = ' '; prompt[1] = '\0';

  // Put PATH into a linked list
  pathlist = get_path();

  //initialize struct for alias linked list
  struct alias_list *alias = NULL; 

  while ( go ){
    //handle ctrl Z and C signals
    signal(SIGINT, sigintHandler);
    signal(SIGTSTP, signalSTPHandler);

    //initialize command_line varible
    char *command_line = calloc(MAX_CANON, sizeof(char));
    char *command_line_temp = calloc(MAX_CANON, sizeof(char));

    //initialize and get current working directoy
    char cwd1[max_buf_size];
    getcwd(cwd1, sizeof(cwd1));
    printf("%s [%s]>", prompt, cwd1); //print the updating current working directory and prompt for shell

    //fgets below handles input into shell. The if statmatent tells the user that ctrl-D is not allowed to be used to exit.
    if (fgets(command_line, MAX_CANON, stdin) == NULL) {
      printf("\nTo quit, please use the exit command: exit.\n");
    }

    rmv_new_line(command_line); //format command line
    add_history(command_line); //add command to history
    int arg_count = parse(command_line, args); //parse command line into args

/* Start of the checks for build in commands.*/

    //Checks if command is an alias and replaces the command line with that alias if it is
    if(alias != NULL){
      struct alias_list *temp = alias;
      while(temp!=NULL){
        if(strcmp(args[0], temp->alias) == 0){
          strcpy(command_line, temp->full);
          arg_count = parse(command_line, args);
          arg_count = arg_count - 1;
          break;
        }
        temp=temp->next;
      }
    }
    //checks if the argmument is which. If it is, run which accordingly
    if (strcmp(args[0], "which") == 0){
      if(arg_count == 1){
        fprintf(stderr, "Incorrect number of arguments for %s function.\n", args[0]);
      }
      else{
        int i = 1;
        for (i; i < arg_count; i++){      //execute which for all arguments given
          printf("Executing built-in %s\n", args[0]);
          char *cmd = malloc(sizeof(cmd));
          cmd = strdup(args[i]);
          char* c_path = which(cmd, pathlist);
          printf("%s\n", c_path);
        }
      }
    }

    //checks if the argmument is where. If it is, run where accordingly
    else if (strcmp(args[0], "where") == 0){
      if(arg_count == 1){
        fprintf(stderr, "Incorrect number of arguments for %s function.\n", args[0]);
      }
      else{
        int i = 1;
        for (i; i < arg_count; i++){    //execute which for all arguments given
          if(args[i] == " ")
          printf("Executing built-in %s\n", args[0]);
          char *cmd = malloc(sizeof(cmd));
          cmd = strdup(args[i]);
          char* c_path = which(cmd, pathlist);
          printf("%s\n", c_path);
        }
      }
    }

    //checks if the argmument is exit. If it is, exit from the shell
    else if (strcmp(args[0], "exit") == 0){
      if (arg_count > 1){
        fprintf(stderr, "Incorrect number of arguments for %s function.\n", args[0]);
      }
      else{
        printf("Executing built-in %s\n", args[0]);
        shell_exit();
      }
    }

    //checks if the argmument is cd. If it is, change the directory. 
    else if (strcmp(args[0], "cd") == 0){
      char cwd[max_buf_size];
      //get current directory to add args to
      getcwd(cwd,sizeof(cwd));
      if (arg_count == 1){
        printf("Executing built-in %s\n", args[0]);
        strcpy(prev_enviornment, cwd);
        cd(getenv("HOME"), prev_enviornment);
      }
      else if(arg_count == 2){
        printf("Executing built-in %s\n", args[0]);
        char *temp = malloc(1024);
        strcpy(temp, prev_enviornment);
        if (chdir(args[1]) != -1 || args[1] == "-"){
          strcpy(prev_enviornment, cwd);
        }
        cd(args[1], temp);
      }
      else{
        fprintf(stderr, "Incorrect number of arguments for %s function.\n", args[0]);
      }
    }

    //checks if the argmument is alias. If it is alias, either print alias, ask for more arguments, or add to alias list
    else if (strcmp(args[0], "alias") == 0) {
      if(arg_count == 1){ // if not arguemnts print list of aliases
        printf("Executing built-in %s\n", args[0]);
        get_alias(alias);
      }
      else if (arg_count >= 3){ //if all args. add them to alias list
        printf("Executing built-in %s\n", args[0]);
        int i = 0;
        int check = 0;
        for (i; i<command_list_size; i++){
          if (strcmp(args[1], command_list[i]) == 0){ //if the command already exits
            fprintf(stderr, "Cannot alias. Command already exits\n");
            check = 1;
          }
        }
        if(check == 0){
          alias = add_alias(alias, args);
        }
      }
      else{
        fprintf(stderr, "Incorrect number of arguments for %s function.\n", args[0]);
      }
    }

    //checks if the argmument is kill. If it is, kill the current working process.
    else if (strcmp(args[0], "kill") == 0){
      printf("Executing built-in %s\n", args[0]);
      if (arg_count == 1){
        fprintf(stderr,"Please enter what to kill.\n");
      }
      else {
        kill_process(args, arg_count);
      }
    }

    //checks if the argmument is pid. If it is, prints the pid number.
    else if (strcmp(args[0], "pid") == 0){
      if(arg_count > 1){
        fprintf(stderr, "Incorrect number of arguments for %s function.\n", args[0]);
      }
      else{
        printf("Executing built-in %s\n", args[0]);
        printpid();
      }
    }

    //checks if the argmument is printev. If it is, print the enviornment depending on the arugments
    else if (strcmp(args[0], "printenv") == 0){
      if (arg_count == 1){
        printenv(envp);
      }
      else if (arg_count == 2){
          print_env_variable(args[1]);
      }
      else{
        fprintf(stderr, "Incorrect number of arguments for %s function.\n", args[0]);
      }
    }

    //checks if the argmument is history. If it is, print the history given the ammount of arguments.
    else if (strcmp(args[0], "history") == 0){
      if (arg_count == 1){
        printf("Executing built-in %s\n", args[0]);
        get_history(10, history_tail);
      }
      else if (arg_count == 2){
        printf("Executing built-in %s\n", args[0]);
        get_history(atoi(args[1]), history_tail);
      }
      else{
        fprintf(stderr, "Incorrect number of arguments for %s function.\n", args[0]);
      }
    }

    //checks if the argmument is prompt. If it is allow the user to change the shell's prompt.
    else if (strcmp(args[0], "prompt") == 0){
      if(arg_count == 1){
        printf("Executing built-in %s\n", args[0]);
        char *prom = malloc(sizeof(char));
        printf("%s", "input prompt prefix: ");
        fgets(prom, MAX_CANON, stdin);
        strtok(prom, "\n");
        strcpy(prompt,prom);
      }
      else if(arg_count == 2){
        printf("Executing built-in %s\n", args[0]);
        strcpy(prompt, args[1]);
      }
      else{
        fprintf(stderr, "Incorrect number of arguments for %s function.\n", args[0]);
      }
    }

    //checks if the argmument is list. If it is, list the files in the directory, or a given directory.
    else if (strcmp(args[0], "list") == 0){
      printf("Executing built-in %s\n", args[0]);
      if (arg_count == 1){
        char cwd[max_buf_size];
        getcwd(cwd, sizeof(cwd));
        printf("%s: \n", cwd);
        list(cwd);
      }
      else{
        printf("Executing built-in %s\n", args[0]);
        int i;
        for(i = 1; i < arg_count; i++){
          printf("%s: \n", args[i]);
          list(args[i]);
        }
      }
    }

    //checks if the argmument is setenv. If it is, allow the user to set the enviornment or clear the environment.
    else if(strcmp(args[0], "setenv") == 0){
      printf("Executing built-in %s\n", args[0]);
      if (arg_count == 1){
        printenv(envp);
      }
      else if (arg_count == 2){   //handles the special case for PATH
        set_env(args[1], " ");
        if (strcmp(args[1], "PATH") == 0){
          free_pathlist(pathlist);
          pathlist = get_path();
        }
      }
      else if (arg_count == 3){   //handles the special case for PATH
        set_env(args[1], args[2]);
        if (strcmp(args[1], "PATH") == 0){
          free_pathlist(pathlist);
          pathlist = get_path();
        }
      }
      else{
        errno = E2BIG;
        perror("setenv");
      }
    }

    //checks if the argmument is pwd. If it is, print the working directory.
    else if(strcmp("pwd", args[0]) == 0){
      if (arg_count == 1){
        printf("Executing built-in %s\n", args[0]);
        printwd();
      }
      else{
        fprintf(stderr, "Incorrect number of arguments for %s function.\n", args[0]);
      }
    }

    //Run when the argument is not a built in command.
    //This forks the process and creates a child. This allows the process to be run on the terminal through the shell.
    else{
      pid_t pid;
      char* p = args[0];
      if (access(args[0], F_OK) == -1){
        p = which(args[0], pathlist);
      }
      if (p == ""){   //doesn't fork if process isn't found by which
        printf("\n");
      }
      else if (p != ""){
        pid = fork();
        if (pid == 0){
          int status;
          if (strcmp("ls", args[0]) == 0 && wildcard_check(command_to_string(args, arg_count)) == 1){ //AG
            int l=1;
            int counter;
            char *ex_args = malloc(512);
            while(l <= arg_count){
              //check if arg has wild card 
              if(wildcard_check(args[l]) == 1){
                char cwd[max_buf_size];
                getcwd(cwd, sizeof(cwd));
                char *wild_list = malloc(512);
                strcpy(wild_list, list_wildcard(cwd, args[l]));

                char *xx = malloc(strlen(wild_list) + strlen(ex_args) + 1);
                strcat(xx, ex_args);
                strcat(xx, " ");
                strcat(xx, wild_list);
                parse(xx, args_wild);
                status = execvp(p, args_wild);
              }
              else {
                strcat(ex_args, " ");
                strcat(ex_args, args[l]);
                counter++;
              }
              l++;
            }
            parse(ex_args, args_wild);
            execvp(p, args_wild);
          }

          //Below is an overwrite for file. This allows file to be run and file * to be run
          else if (strcmp("file", args[0]) == 0 && wildcard_check(command_to_string(args, arg_count)) == 1){
            int l=1;
            int counter;
            char *ex_args = malloc(512);
            while(l <= arg_count){
              //check if arg has wild card 
              if(wildcard_check(args[l]) == 1){
                char cwd[max_buf_size];
                getcwd(cwd, sizeof(cwd));
                //char *wild_list = malloc(strlen(list_wildcard(cwd, args[l])));
                char *wild_list = malloc(512);
                strcpy(wild_list, list_wildcard(cwd, args[l]));

                char *xx = malloc(strlen(wild_list) + strlen(ex_args) + 1);
                strcat(xx, ex_args);
                strcat(xx, " ");
                strcat(xx, wild_list);
                parse(xx, args_wild);
                status = execvp(p, args_wild);
              }
              l++;
            }
          }

          else {
            status = execvp(p, args);
          }
          if (status != -1){
            printf("Executing %s\n", args[0]);
          }
          if (status == -1){
            exit(EXIT_FAILURE);
          }
        }
        else if (pid > 0){
          int childStat;
          waitpid(pid, &childStat, 0);
        }
        else{
          perror("Command not found.");
        }
      }
    }    
  } 
}

//Function for which. 
char *which(char *command, struct pathelement *pathlist ){
  while(pathlist->next != NULL){
    char* loc_com = (char *) malloc(max_buf_size);
    strcat(loc_com, pathlist->element);
    strcat(loc_com, "/");   //puts path in form that can be accessed
    strcat(loc_com, command);
    if (access(loc_com, F_OK) != -1){
      return loc_com;
    }
    else{
      pathlist = pathlist->next;
    }
  }
  if(pathlist->next == NULL){   //handles last element in pathlist
    char* loc_com = (char *) malloc(max_buf_size);
    strcpy(loc_com, pathlist->element);
    strcat(loc_com, "/");
    strcat(loc_com, command);
    if(access(loc_com, F_OK) != -1){
      return loc_com;  
    }
    else{
      printf("Command not found.");
      return("");
    }
  }
} 

//Function for where
char *where(char *command, struct pathelement *pathlist ){
  char* total_loc = malloc(sizeof(max_buf_size));
  while (pathlist != NULL){
    char* loc_com = malloc(sizeof(max_buf_size));
    strcpy(loc_com, pathlist->element);
    strcat(loc_com, "/");   //puts path in form that can be accessed
    strcat(loc_com, command);
    if (access(loc_com, F_OK) != -1){
      strcat(total_loc, loc_com);
      pathlist = pathlist->next;
    }
    else{
      pathlist = pathlist->next;
    }
  }
  if(strcmp(total_loc, "") != 0){
    return(total_loc);
  }
  else{
    return("Command not found.");
  }
}

//functions for cd
void cd(char *pth, char *prev){
  //append our path to current path
  char *path = malloc(sizeof(*path));
  strcpy(path,pth);
  char cwd[max_buf_size];
  //get current directory to add args to
  getcwd(cwd,sizeof(cwd));
  if (strcmp(path, "..") == 0){
    chdir(cwd);
    return;
  }
  //create string to check
  strcat(cwd,"/");
  if(strcmp(path, "-")==0){
    chdir(prev);
    return;
  }
  //if directory is passed in as a whole path form
  if(strstr(path,"/")){
    if(chdir(path)!=0){
      chdir(path);
    }
    return;
  }
  //if not passed in whole path form
  strcat(cwd, path);
  int exists = chdir(cwd);
  if(exists != 0){
    perror("cd");
  }
  else{
    chdir(cwd);
  }
}

//function for printwd
void printwd(){
  char cwd[max_buf_size];
  getcwd(cwd,sizeof(cwd));
  printf("%s\n", cwd);
}

//function for printpid
void printpid(){
  printf("Process ID: %d\n", getpid());
}

//function for list
void list (char *dir){
  DIR* direct;
  struct dirent* entryp;
  direct = opendir(dir);
  if(direct == NULL){
    printf("Can't open directory %s\n", dir);
  }
  else{
    while((entryp = readdir(direct)) != NULL){
      printf("%s\n", entryp->d_name);
    }
    closedir(direct);
  }
} 

//function for shell exit
void shell_exit(){
  exit(0);
}

//function for pint environment variable
void print_env_variable(char *env_var){
  char *env = getenv(env_var);
  if(env != NULL){
    printf("%s\n", env);
  }
  else {
    printf("Enviornment does not exist.\n");
  }
}

//function for print environment
void printenv(char **envp){
  char **env;
  env = envp;
  while(*env){
    printf("%s\n", *env);
    env++;
  }
}

//function for kill process
void kill_process(char **args, int count){
  if (count == 2){
    kill(atoi(args[1]), SIGKILL);
  }
  else if(strstr(args[1], "-") != NULL){
    kill(atoi(args[2]), atoi(++args[1]));
  }
}

//function for remove new line when commands entered
void rmv_new_line(char* string){
  int len;
  if((len = strlen(string))>0){
    if(string[len-1] == '\n'){
      string[len-1] = '\0';
    }
  }
}

//function for parse
int parse(char* command, char** parameters){
  int i;
  for(i=0; i<MAX_CANON; i++){
    parameters[i] = strsep(&command, " ");
    if(parameters[i] == NULL){
      return i;
      break;
    }
  }
}


//function for add_history. This adds a history command to the doubly linked list
void add_history(char* command){
  struct history_list *new_node = malloc(sizeof(struct history_list));
  new_node->node = malloc(1024);
  strcpy(new_node->node, command);
  new_node->next = NULL;
  new_node->previous = history_tail;
  if(history_tail){
    history_tail->next = new_node;
  }
  history_tail = new_node;
  if(!history_head){
    history_head = new_node;
  }
}

//function for getting history. This prints out the history of the commands given the amount you want to print
void get_history(int i, struct history_list *tail){
  int j = 1;
  while(j<=i && tail!=NULL){
    printf("%s\n",tail->node);
    tail=tail->previous;
    j++;
  }
}

//adds to the doublely linked list that stores all the aliases for the shell.
struct alias_list* add_alias(struct alias_list *head, char** args){
  //takes in char** of args. args[0] is command, args[1] is the alias, args[2+] are the full command
  struct alias_list *temp;
  struct alias_list *new_node = malloc(sizeof(struct alias_list));
  temp = head;
  new_node->alias = malloc(MAX_SIZE);
  new_node->full = malloc(MAX_SIZE);
  strcpy(new_node->alias, args[1]);
  int i = 2;
  char* full_temp = malloc(sizeof(MAX_SIZE));
  while (args[i] != NULL){
    strcat(full_temp, args[i]);
    strcat(full_temp, " ");
    i++;
  }
  strcpy(new_node->full, full_temp);
  new_node->next = NULL;
  if(temp == NULL){
    head = new_node;
  }
  else if(temp->next == NULL){
    temp->next = new_node;
  }
  else{
    while(temp->next != NULL){
      temp = temp->next;
    }
    temp->next = new_node;
  }
  return head;
}

//prints out the list of aliases. This allow them to print each of the aliases side by side with their full commands
void get_alias(struct alias_list *head) {
  struct alias_list *temp = head;
  while(temp!=NULL){
    printf("full command: %s\n", temp->full);
    printf("alias: %s\n", temp->alias);
    temp=temp->next;
  }
}

//a match functions which takes in a char* with a wildcard character (*, ?) and takes in another char * which it compares to. Returns 1 if it is not true. and 0 if it is.
int match(char *first, char * second){
    if (*first == '\0' && *second == '\0')
        return 1;

    if (*first == '*' && *(first+1) != '\0' && *second == '\0')
        return 0;
 
    if (*first == '?' || *first == *second)
        return match(first+1, second+1);
 
    if (*first == '*')
        return match(first+1, second) || match(first, second+1);
    return 0;
}

//Checks if the string inputed has a wildcard variable.
int wildcard_check(char *check_string){
  int i = 0;
  const char *invalid_characters = "*?";
  char *c = check_string;
  while (*c){
    if (strchr(invalid_characters, *c)){
          i = 1;
       }
       c++;
  }
  if (i == 1){
    return 1;
  }
  else{
    return 0;
  }
}

//function for setting the enviornment 
void set_env(char* envname, char* envval){
  setenv(envname, envval, 1); 
}

//functions that frees the pathlist when the env of path is changed
void free_pathlist(struct pathelement *head){
  struct pathelement *current = head;
  struct pathelement *temp;
  while(current->next != NULL){
    temp = current;
    current = current->next;
    free(temp);
  }
  free(current);
}

/* Signal Handler for SIGINT */
void sigintHandler(int sig_num){
  signal(SIGCHLD, sigintHandler);
  fflush(stdout);
  return;
}

void signalSTPHandler(int sig_num){
  signal(SIGTSTP, signalSTPHandler);
  //printf("Can't terminate process with Ctrl+Z %d \n", waitpid(getpid(),NULL,0));
  fflush(stdout);
  return;
}

//runs wildcard and lists all files applicable
char *list_wildcard (char *dir, char* wildcard){
  int wild_size=0;
  int space_cnt = 0;
  char *wild_list;
  DIR* direct;
  struct dirent* entryp;
  direct = opendir(dir);
  while((entryp = readdir(direct)) != NULL){
    if (match(wildcard, entryp->d_name) == 1){
      if (strcmp(entryp->d_name, "..") != 0 && strcmp(entryp->d_name, ".") != 0 && strcmp(entryp->d_name, ".git") != 0){
        wild_size += strlen(entryp->d_name);  //increment the size of the wildcard list
        space_cnt++;
        if (opendir(entryp->d_name) != NULL){
          list_wildcard(entryp->d_name, wildcard);
        }
      }
    }
  }
  //closedir(direct);
  wild_size += space_cnt +1;
  wild_list = malloc(wild_size);

  DIR* direct1;
  struct dirent* entryp1;
  direct1 = opendir(dir);
  while((entryp1 = readdir(direct1)) != NULL){
    if (match(wildcard, entryp1->d_name) == 1){
      if (strcmp(entryp1->d_name, "..") != 0 && strcmp(entryp1->d_name, ".") != 0 && strcmp(entryp1->d_name, ".git") != 0){
        strcat(wild_list, entryp1->d_name); //add to wildcard list
        strcat(wild_list, " ");
        if (opendir(entryp1->d_name) != NULL){
          list_wildcard(entryp1->d_name, wildcard);
        }
      }
    }
  }
  //closedir(direct1);
  return wild_list;
}

//used in checking to see whether or not wildcard is called in the command line
char * command_to_string(char ** args, int arg_count){
  char *command = malloc(512);
  int i = 0;
  while(args[i] != NULL){
    strcat(command, args[i]);
    strcat(command, " ");
    i = i + 1;
  }
  return command;
}