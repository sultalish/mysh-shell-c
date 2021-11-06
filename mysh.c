// Alisher Sultangazin

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>

// Custom struct for holding commands.
struct command {
  char cmd[257];
  struct command *next;
};

// Custom struct for holding PIDs.
struct process {
  pid_t pid;
  struct process *next;
};

// Initialize all functions
struct process* delete_PID(pid_t key);
struct process* delete_first_PID();
struct command* addToHistory(char* cmd);
void addToHistoryFile();
void clear_history();
void runCommand(char** args, struct command *head, struct command *history_head);
void printHistory(struct command *head, int counter);
bool isDirectoryExists(const char *path);
void move_to_dir(char* dir);
int is_file(char* path);
void run_program(char **args);
void run_background_program(char **args);
char** replay_number(struct command *head, struct command *history_head, int renumber);
char *concat_stringsArray(char** args);
void runCommand(char** args, struct command *head, struct command *history_head);
char *read_line();
char **tokenize_str(char *cmd);
void addToHistoryFromFile();
void printHelp();

// Global variables to make data transfer easy.
struct command *history_head = NULL;
struct command *history_curr = NULL;
int command_counter = 1;
char* currentdir;
char* checkdir;
char* tempdir;
struct process *head_PID = NULL;
struct process *current_PID = NULL;

// print the list of commands
void printHelp()
{
  printf("---------------------------------------------------------------------\n");
  printf("# Here's the list of commands:\n");
  printf("# hist                            |   show the history of commands\n");
  printf("# hist -c                         |   clean the history of commands\n");
  printf("# changedir                       |   change the current directory\n");
  printf("# currentdir                      |   show the current directory\n");
  printf("# run program [parameters]        |   run the program with given parameters\n");
  printf("# background program [parameters] |   run the program with given parameters in the background\n");
  printf("# terminate PID                   |   terminate the given PID\n");
  printf("# terminateall                    |   terminate all existing PIDs\n");
  printf("# replay num                      |   replay the command with the given number in history\n");
  printf("# repeat num command              |   run the command num numbers of time\n");
  printf("--------------------------------------------------------------------\n");
}

// Deletes given PID
struct process* delete_PID(pid_t key)
{
  struct process* current = head_PID;
  struct process* previous = NULL;

  if (head_PID == NULL)
  {
    return NULL;
  }

  while (current->pid != key)
  {
    if (current->next == NULL)
    {
      return NULL;
    } else
    {
      previous = current;
      current = current->next;
    }
  }

  // found a match
  if (current == head_PID)
  {
    // change first to point to next
    head_PID = head_PID->next;
    current_PID = head_PID;
  }
  else
  {
    // relink
    previous->next = current->next;
    current_PID = previous;
  }

  return current;
}

// Deletes first PID
struct process* delete_first_PID()
{
  // save reference to the fisrt link
  struct process *tempLink = head_PID;

  // mark next to first link as first
  head_PID = head_PID->next;

  //return deleted link
  return tempLink;
}


// Adds a command to the history linked list.
struct command* addToHistory(char* cmd)
{
  struct command *new_command = malloc(sizeof(struct command));
  memset(new_command->cmd, '\0', sizeof(new_command->cmd));
  strcpy(new_command->cmd, cmd);
  new_command->next = NULL;
  history_curr = new_command;
  return history_curr;
}

// Reverses the history linked list
void reverseHistory(struct command** history_head)
{
  struct command* prev = NULL;
  struct command* current = *history_head;
  struct command* next = NULL;

  history_curr = current;
  while (current != NULL)
  {
    next = current->next;
    current->next = prev;
    prev = current;
    current = next;
  }
  *history_head = prev;
}

// Adds current history to history.txt (or creates history.txt and adds history there)
void addToHistoryFile()
{
  // Create a FILE instance
  FILE *fp;
  fp = fopen("history.txt", "w");
  int buffer_length = 1024;
  int buffer_count = 0;
  char *buffer = malloc(buffer_length * sizeof(char));
  char c;

  // concat buffer with recent history
  reverseHistory(&history_head);
  while (history_head != NULL)
  {
    if (buffer_count >= buffer_length)
    {
      buffer_length += 1024;
      buffer = realloc(buffer, buffer_length * sizeof(char));
    }

    strcat(buffer, history_head->cmd);
    buffer_count += strlen(history_head->cmd);
    strcat(buffer, "\n");
    buffer_count++;
    history_head = history_head->next;
  }

  fputs(buffer, fp);
  fclose(fp);
}

// Function to clear the history
void clear_history()
{
  free(history_curr);
  free(history_head);

  history_head = NULL;
  history_curr = NULL;

  command_counter = 0;

  history_curr = addToHistory("history -c");
  command_counter = command_counter + 1;
  history_head = history_curr;
}

// This printHistory
void printHistory(struct command *history_curr, int counter)
{
   if (history_curr == NULL)
	   return;

   counter = counter - 1;

   if (counter < 0)
	   return;
   printHistory(history_curr->next, counter);
   printf("%d %s\n", counter, history_curr->cmd);
}

// Checks if directory exists
bool isDirectoryExists(const char *path)
{
  struct stat stats;
  strcpy(tempdir, currentdir);
  if(path[0] != '/')
  {
    strcat(tempdir, "/");
    strcat(tempdir, path);
    stat(tempdir, &stats);
    if(S_ISDIR(stats.st_mode))
      return true;
    return false;
  }
  else
  {
  stat(path, &stats);
  if (S_ISDIR(stats.st_mode))
    return true;
  return false;
  }
}


void move_to_dir(char* dir)
{
	if(isDirectoryExists(dir) == true)	//directory exists
	{
    if(dir[0] == '/')
      {
      currentdir = dir;
      return;
      }
    else
      {
      strcat(currentdir, dir);
      strcat(currentdir, "/");
      return;
      }
	}
	else
	{
		printf("\nDirectory does not exist.\n");
		return;
	}
}

int is_file(char* path)
{
  struct stat buf;
  stat(path, &buf);
  return S_ISREG(buf.st_mode);
}

void run_program(char **args)
{
  // If file is not executable return and print that program cannot be found
  // We check it before the fork() and execv() so that there won't be any errors
  if (!(is_file(args[1]) && !(access(args[1], F_OK) || access(args[1], X_OK))))
  {
    fprintf(stderr, "# Program cannot be found or executed!\n");
    return;
  }

  // fork a child process
  pid_t pid = fork();
  int status;

  pid_t temp_pid;

  if (pid < 0)
  {
    // ERROR
    exit(1);
  }
  // child process
  else if (pid == 0)
  {
    execv(args[1], args);
  }
  else
  {
    // This is ran by parent. We wait for the child to terminate
    do
    {
      temp_pid = wait(&status);
    } while (temp_pid != pid);
    // we returned from the child process!!!
    // Now, let's save the PID in the PID List
    if (head_PID == NULL)
    {
      struct process *new_pid = malloc(sizeof(struct process));
      new_pid->pid = pid;
      new_pid->next = NULL;
      current_PID = new_pid;
      head_PID = current_PID;
    }
    else
    {
      struct process *new_pid = malloc(sizeof(struct process));
      new_pid->pid = pid;
      new_pid->next = NULL;
      current_PID->next = new_pid;
      current_PID = new_pid;
    }
    return;
  }



  return;
}

void run_background_program(char **args)
{
  // If file is not executable return and print that program cannot be found
  // We check it before the fork() and execv() so that there won't be any errors
  if (!(is_file(args[1]) && !(access(args[1], F_OK) || access(args[1], X_OK))))
  {
    fprintf(stderr, "# Program cannot be found or executed!\n");
    return;
  }

  // fork a child process
  pid_t pid = fork();
  int status;

  pid_t temp_pid;

  if (pid < 0)
  {
    // ERROR
    exit(1);
  }
  // child process
  else if (pid == 0)
  {
    execv(args[1], args);
  }
  else
  {
    // This is ran by parent. We wait for the child to terminate
    printf("The PID of the current program is %d.\n", pid);
    do
    {
      temp_pid = wait(&status);
    } while (temp_pid != pid);

    // we returned from the child process!!!
    // Now, let's save the PID in the PID List

    if (head_PID == NULL)
    {
      struct process *new_pid = malloc(sizeof(struct process));
      new_pid->pid = pid;
      new_pid->next = NULL;
      current_PID = new_pid;
      head_PID = current_PID;
    }
    else
    {
      struct process *new_pid = malloc(sizeof(struct process));
      new_pid->pid = pid;
      new_pid->next = NULL;
      current_PID->next = new_pid;
      current_PID = new_pid;
    }
    return;
  }

  if (head_PID == NULL)
  {
    struct process *new_pid = malloc(sizeof(struct process));
    new_pid->pid = pid;
    new_pid->next = NULL;
    current_PID = new_pid;
    head_PID = current_PID;
  }
  else
  {
    struct process *new_pid = malloc(sizeof(struct process));
    new_pid->pid = pid;
    new_pid->next = NULL;
    current_PID->next = new_pid;
    current_PID = new_pid;
  }
  return;
}


char** replay_number(struct command *head, struct command *history_head, int renumber)
{
	if(history_curr == NULL)
		return NULL;

	int i = 0;
	char t[20];
	struct command *temp = history_head;

	//printf("checking for %d\n", renumber);

  renumber = command_counter - renumber - 2;

  // lol 1
  // history 2
  // replay0 3


	while(i != renumber)
	{
		temp = temp->next;
		i++;

		if(temp == NULL)
		{
			printf("Replay not found");
			return NULL;
		}

	}

  char** args = malloc(2 * sizeof(char*));
  args = tokenize_str(temp->cmd);

	//printf("Replaying: %s\n", args[0]);
	//runCommand(args, head, history_head);
	return args;

}

char *concat_stringsArray(char** args)
{
  // get the full command to add to the history
  char* full_command = malloc(257 * sizeof(char));
  int i = 0;
  while (args[i] != NULL)
  {
    strcat(full_command, args[i]);
    if (args[i] != NULL)
    {
      strcat(full_command, " ");
    }
    i++;
  }

  return full_command;
}

// This adds a command to the history and then runs the command.
// Returns true if command is valid, else returns false.
void runCommand(char** args, struct command *history_curr, struct command *history_head)
{
  history_curr->next = addToHistory(concat_stringsArray(args));

  // EXECUTE
  if (!strcmp(args[0], "hist"))
  {
    // if no parameters, print history
    if (args[1] == NULL)
    {
      printHistory(history_head, command_counter);
    }
    // if there's a parameter check if the parameter is valid
    else
    {
      if (strcmp(args[1], "-c") == 0)
      {
        printf("Cleaning history!\n");
        clear_history();
      }
    }

    return;
  }
  // changedir directory
  else if (!strcmp(args[0], "changedir"))
  {
    move_to_dir(args[1]);
    return;
  }
  // run program [parameters]
  else if (!strcmp(args[0], "run"))
  {
    // check if param exists
    if (args[1] != NULL)
    {
      // check if user wants to start a program, else just return
      run_program(args);
    }

    return;
  }
  // background program [parameters]
  else if (!strcmp(args[0], "background"))
  {
    if (args[1] != NULL)
    {
      run_background_program(args);
    }

    return;
  }
  // terminate PID
  else if (!strcmp(args[0], "terminate"))
  {
    char* dump;
    pid_t pid_to_delete;

    if (args[1] != NULL)
    {
      pid_to_delete = strtol(args[1], &dump, 10);
    }

    if (pid_to_delete > 0 && !kill(pid_to_delete, 0))
    {
      kill(pid_to_delete, SIGKILL);
    }

    printf("Deleting %d\n", pid_to_delete);
    delete_PID(pid_to_delete);

    return;
  }
  // terminateall
  else if (!strcmp(args[0], "terminateall"))
  {
    int count = 0;
    struct process *temp = head_PID;
    // Delete and kill all PIDs from the list
    while (temp != NULL)
    {
      if (head_PID->pid > 0 && !kill(head_PID->pid, 0))
      {
        kill(head_PID->pid, SIGKILL);
      }
      count++;
      temp = temp->next;
    }

    // Print all exterminated processes
    if (head_PID == NULL)
    {
      printf("No processes to exterminate!\n");
      return;
    }
    else
    {
      printf("Exterminating %d processes: ", count);
    }
    while (head_PID != NULL)
    {
      printf("%d", head_PID->pid);
      delete_PID(head_PID->pid);

      if (head_PID == NULL)
      {
        printf("\n");
      }
      else
      {
        printf(" ");
      }
    }
    head_PID = NULL;
    current_PID = NULL;

    return;
  }
  // currentdir DONE
  else if (!strcmp(args[0], "currentdir"))
  {
    printf("%s\n", currentdir);
	  return;
  }

  //Check for replay
  else if(strstr(args[0], "replay") != NULL)
  {
    if(args[1] == NULL)
    {
      printf("Error\n");
      return;
    }

    char* dump;
	  int tem = strtol(args[1], &dump, 10);

    char** temp = replay_number(history_curr, history_head, tem);

    if(temp == NULL)
		 return;

    runCommand(temp, history_curr, history_head);

    return;
  }

  //Repeat a command n times
  else if(strstr(args[0], "repeat") != NULL)
  {
	  char** newargs = NULL;
	  char* newcommand = malloc(257*sizeof(char));
	  //printf("Activating repeat\n");

	  if(args[1] == NULL && args[2] == NULL)
	  {
		  printf("Error\n");
	  }

	  char* dump;
	  int tem = strtol(args[1], &dump, 10);

    // If the command needs to be called <= 0 time, return
    if (tem <= 0)
    {
      return;
    }
    // If there's no command to call, return
    if (args[2] == NULL)
    {
      return;
    }
	  //printf("Tem is: %d\n", tem);
	  //printf("Dumping: %s\n", dump);


	  int z = 2;
	  int i = 0;

	  //args = repeat 2 movetodir
	  //newargs = movetodir

	  //repeat command line
	  while(args[z] != NULL)
	  {
		  //printf("Showing: %s\n", args[z]);
		  strcat(newcommand, args[z]);
		  if (args[z] != NULL)
		  {
			     strcat(newcommand, " ");
		  }

		  //printf("New command: %s\n", newcommand);
		  z++;
	  }

	  newargs = tokenize_str(newcommand);

	  //input the command line
	  for(int j = 0; j < tem; j++)
	  {
		  runCommand(newargs, history_curr, history_head);
	  }

	  return;
  }
  // help
  else if (!strcmp(args[0], "help"))
  {
    printHelp();
    return;
  }
  // quit
  else if (!strcmp(args[0], "quit"))
  {
    addToHistoryFile();
    exit(0);
  }

  return;
}

// Read the line for the shell
char *read_line()
{
  int pos = 0;
  // Let's assume max size of line is 100
  char* buffer = malloc(sizeof(char) * 100);
  int ch;

  if (!buffer)
  {
    exit(EXIT_FAILURE);
  }

  while (1)
  {
    // Read a char
    ch = getchar();

    // Once we hit EOF, we return
    if (ch == EOF || ch == '\n')
    {
      buffer[pos] = '\0';
      return buffer;
    }
    else
    {
      buffer[pos++] = ch;
    }
  }
}

// Function to tokenize the line and return an array of words
char **tokenize_str(char *cmd)
{
  // I'd set the default size to be 4, but could get bigger
  int size = 4;
  // keep track of the current token position
  int position = 0;
  // allocate an array of strings
  char **args = malloc(size * sizeof(char*));
  // get token
  char *token = strtok(cmd, " ");

  while (token != NULL)
  {
    args[position++] = token;

    if (position >= size)
    {
      // expand the array of strings
      size += 4;
      args = realloc(args, size * sizeof(char*));
    }

    // get token
    token = strtok(NULL, " ");
  }
  // end the array
  args[position] = NULL;

  return args;
}

void addToHistoryFromFile()
{
  // Create a FILE instance
  FILE *fp;
  if (access("history.txt", F_OK) == 0)
  {
    // file exists
    fp = fopen("history.txt", "r");
    fseek(fp, 0, SEEK_END); // goto end of file
    if (ftell(fp) == 0)
    {
      return;
    }
    fseek(fp, 0, SEEK_SET); // goto begin of file
  }
  else
  {
    // file doesn't exist
    return;
  }

  int buffer_length = 1024;
  int buffer_count = 0;
  char *buffer = malloc(buffer_length * sizeof(char));
  char c;
  struct command *hist_head = NULL;
  struct command *prev = NULL;

  // concat buffer with new history
  while ((c = getc(fp)) != EOF)
  {
    // extend buffer length if needed
    if (buffer_count >= buffer_length)
    {
      buffer_length += 1024;
      buffer = realloc(buffer, buffer_length * sizeof(char));
    }

    // once encountered newline add command to recent history
    if (c == '\n')
    {
      struct command *new_command = malloc(sizeof(struct command));
      memset(new_command->cmd, '\0', sizeof(new_command->cmd));
      strcpy(new_command->cmd, buffer);
      new_command->next = NULL;

      prev = hist_head;

      if (hist_head == NULL)
      {
        hist_head = new_command;
      }
      else
      {
        while (prev->next != NULL)
        {
          prev = prev->next;
        }
        prev->next = new_command;
      }
      command_counter++;

      buffer = NULL;
      buffer = malloc(buffer_length * sizeof(char));
      buffer_count = 0;
    }
    else
    {
      buffer[buffer_count++] = c;
    }
  }

  history_head = hist_head;
  history_curr = prev->next;

  reverseHistory(&history_head);

  fclose(fp);
}

// The classic "int main(void)". Need I say more?
int main(void)
{
  char* cmd = malloc(257);
  currentdir = malloc(100 * sizeof(char));
  tempdir = malloc(100 * sizeof(char));
  checkdir = malloc(100 * sizeof(char));
  size_t buffer = 0;

  addToHistoryFromFile();

  // array of strings
  char **args;
  currentdir = getcwd(currentdir, 200);

  do
  {
    printf("# ");
    // read the line and save it in the cmd string
    cmd = read_line();
    // if the input is a newline, we don't do anything
    // but if it is not, we do the following
    if (strcmp(cmd, ""))
    {
      // tokenize the array of strings
      args = tokenize_str(cmd);

      if (history_head == NULL)
      {
        history_head = addToHistory(cmd);
        runCommand(args, history_curr, history_head);
        command_counter = command_counter + 1;
        history_head = history_curr;
      }
      else
      {
        runCommand(args, history_curr, history_head);
        command_counter = command_counter + 1;
      }
    }
  } while (strcmp(cmd, "") == 0);

  do
  {
    printf("# ");
    if (strcmp((cmd = read_line()), "") == 0)
      continue;

    // tokenize the array of strings
    args = tokenize_str(cmd);
    runCommand(args, history_curr, history_head);
    command_counter = command_counter + 1;
  } while (1);


  return 0;
}
