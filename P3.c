#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

/*
  Function Declarations for builtin shell commands:
 */
int cd(char **args);
int help(char **args);
int myquit(char **args);
int launch(char **args);
int myclear(char **args);
int myenviron(char **args);
int mydir();


/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
    "cd",
    "help",
    "myquit",
    "myclear",
    "myenviron",
    "mydir"};

int (*builtin_func[])(char **) = {
    &cd,
    &help,
    &myquit,
    &myclear,
    &myenviron,
    &mydir};

int num_builtins()
{
  return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin function implementations.
*/
int myclear(char **args)
{
  if (args[1] == NULL)
  {
    system("clear");
  }
  else
  {
    if (chdir(args[1]) != 0)
    {
      perror("lsh");
    }
  }
  return 1;
}
int mydir(char **args)
{
  return 1;
}
int cd(char **args)
{
  if (args[1] == NULL)
  {
    fprintf(stderr, ": expected argument to \"cd\"\n");
  }
  else
  {
    if (chdir(args[1]) != 0)
    {
      perror("lsh");
    }
  }
  return 1;
}

int help(char **args)
{
  int i;
  printf("Aaron Davidge\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < num_builtins(); i++)
  {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

extern char **environ;

int myenviron(char **args)
{
  char **s = environ;

  for (; *s; s++)
  {
    printf("%s\n", *s);
  }

  return 1;
}

int myquit(char **args)
{
  exit(0);
  return 0;
}

int execute(char **args)
{
  int i;

  if (args[0] == NULL)
  {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < num_builtins(); i++)
  {
    if (strcmp(args[0], builtin_str[i]) == 0)
    {
      return (*builtin_func[i])(args);
    }
  }

  return launch(args);
}

int launch(char **args)
{
  pid_t pid, wpid;
  int status;

  pid = fork();
  if (pid == 0)
  {
    // Child process
    if (execvp(args[0], args) == -1)
    {
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  }
  else if (pid < 0)
  {
    // Error forking
    perror("lsh");
  }
  else
  {
    // Parent process
    do
    {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

char destination[100];

char **split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char *));
  char *token;

  if (!tokens)
  {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);

  while (token != NULL)
  {
    tokens[position] = token;
    position++;

    if (position >= bufsize)
    {
      bufsize += LSH_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char *));
      if (!tokens)
      {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  if(strcmp(tokens[0],"mydir") == 0){
    strcat(destination,"sudo ls -al ");
    strcat(destination,tokens[1]);
    system(destination);
    
  }
  

  tokens[position] = NULL;

  return tokens;
}

char *read_line(void)
{
  char *line = NULL;
  size_t bufsize = 0; // have getline allocate a buffer for us

  if (getline(&line, &bufsize, stdin) == -1)
  {
    if (feof(stdin))
    {
      exit(EXIT_SUCCESS); // We recieved an EOF
    }
    else
    {
      perror("readline");
      exit(EXIT_FAILURE);
    }
  }

  return line;
}

void main_loop(void)
{
  char *line;
  char **args;
  int status;

  do
  {
    printf("> ");
    line = read_line();
    args = split_line(line);
    status = execute(args);

    free(line);
    free(args);
  } while (status);
}

int main(int argc, char **argv)
{
  // Load config files, if any.

  // Run command loop.
  main_loop();

  // Perform any shutdown/cleanup.

  return EXIT_SUCCESS;
}