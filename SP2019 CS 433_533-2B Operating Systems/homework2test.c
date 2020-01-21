#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>


int green_cd(char **args);
int green_help(char **args);
int green_quit(char **args);
int green_environ(char **args);
int green_list(char **args);
int green_set(char **args);
int green_history(char **args);

 char *command_str[] = {
  "cd",
  "help",
  "quit",
  "environ",
  "list",
  "set",
  "history"
};

int (*command_func[]) (char **) = {
  &green_cd,
  &green_help,
  &green_quit,
  &green_environ,
  &green_list,
  &green_set,
  &green_history
};

int green_num_commands() {
  return sizeof(command_str) / sizeof(char *);
}

int green_cd(char **args) {
  if (args[1] == NULL) {
    fprintf(stderr, "greensh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("green");
    }
  }
  return 1;
}
int green_list(char **args) {
  DIR *d;
  struct dirent *dir;
  d = opendir(".");
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      printf("%s\n", dir->d_name);
    }
    closedir(d);
  }
  return 1;
}

int green_history(char **args) {
  FILE *fp;
  if ((fp = fopen("greensh.log","r")) == NULL) {
     fprintf(stderr,"greensh: error opening log file to show history\n");
  } else {
     char *line = NULL;
     size_t linecap = 0;
     ssize_t linelen;
     while ((linelen = getline(&line, &linecap, fp)) > 0)
        fwrite(line, linelen, 1, stdout);
     fclose(fp);
  }
  return 1;
}
extern char **environ;

 int green_environ(char **args) {
	int i;
	char *s = *environ;
	for (i = 1; s; i++) {
		printf("%s\n", s);
		s = *(environ + i);
	}
	return 1;
}

int green_set(char **args) {
	if (args[1] == NULL || args[2] == NULL) {
		fprintf(stderr, "Error: needs two arguments, name and value");
		return 1;
	}
	setenv(args[1], args[2], 1);
	return 1;
}
int green_help(char **args) {
  printf("environ – list all environment strings as name=value. \n");
  printf("set <NAME> <VALUE> – should set the environment variable.\n");
  printf("list – list all the files in the current directory \n");
  printf("cd<directory> – change the current directory to the <directory>. ");
  printf("help – displaythe internal commands and a short description on how to use them.  \n ");
  printf("quit – quit the shell program.\n");
  int i;
  
  for (i = 0; i < green_num_commands(); i++) {
    printf(" %s\n", command_str[i]);
  }
  printf("Use the man command for information on other programs.\n");
  return 1;
}


int green_quit(char **args) {
  return 0;
}


int green_launch(char **argv) {
  pid_t pid;
  int status;
  
  pid = fork();
  if (pid == 0)
  {
    if (execvp(argv[0], argv) == -1) {
      perror("greensh");
      exit(-1);
    }
  } else if (pid > 0) {
    wait(&status); /* wait for the child process to terminate */
  } else {
    perror("fork");
    exit(EXIT_FAILURE);
  }
  return 1;
}


int green_execute(char **args) {
  int i;
  if (args[0] == NULL) {
    
    return 1;
  }
  for (i = 0; i < green_num_commands(); i++) {
    if (strcmp(args[0], command_str[i]) == 0) {
      return (*command_func[i])(args);
    }
  }
  return green_launch(args);
}


#define green_RL_BUFSIZE 1024

char *green_read_line(void) {
  int bufsize = green_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;
  if (!buffer) {
    fprintf(stderr, "greensh: allocation error\n");
    exit(EXIT_FAILURE);
  }
  while (1) {
  
    c = getchar();
    if (c == EOF) {
      exit(EXIT_SUCCESS);
    } else if (c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;
    
    if (position >= bufsize) {
      bufsize += green_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "greensh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

#define green_TOK_BUFSIZE 64
#define green_TOK_DELIM " \t\r\n\a"

char **green_split_line(char *line) {
  int bufsize = green_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;
  if (!tokens) {
    fprintf(stderr, "greensh: allocation error\n");
    exit(EXIT_FAILURE);
  }
  token = strtok(line, green_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;
    if (position >= bufsize) {
      bufsize += green_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
		free(tokens_backup);
        fprintf(stderr, "greensh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
    token = strtok(NULL, green_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}
void green_loop(void) {
  char *line;
  char **args;
  int status;
  FILE *fp;
  do {
    if ((fp = fopen("greensh.log","a")) == NULL) {
       fprintf(stderr, "greensh: error opening log file\n");
       exit(EXIT_FAILURE);
    }
    printf("greensh> ");
    line = green_read_line();
    if (line[0] != '\n' || line[0] != '\0')
       fprintf(fp,"%s\n", line);
    fclose(fp);
    args = green_split_line(line);
    status = green_execute(args);
    free(line);
    free(args);
  } while (status);
}


int main(int argc, char **argv) {
  
  green_loop();
  return EXIT_SUCCESS;
}
