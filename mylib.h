struct history_list
{
  char *node;			/* a dir in the path */
  struct history_list *next;	/* pointer to next node */
  struct history_list *previous; /*pointer to previous node*/
};
void add_history(char* command);
void get_history(int i, struct history_list *tail);

struct alias_list
{
  //char *node;			/* a dir in the path */
  struct alias_list *next;		/* pointer to next node */
  char *alias;
  char *full;
};
struct alias_list* add_alias(struct alias_list *head, char**args);
void get_alias(struct alias_list *head);