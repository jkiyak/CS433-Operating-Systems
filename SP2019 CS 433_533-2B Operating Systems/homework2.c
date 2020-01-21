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
int green__set(char **args);
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
extern char **environ; int green_environ(char **args) {
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


void pipe1() {
	if (pipe(pipefd) == 0) { /* Open a pipe */
    if ((pid = fork()) == 0) { /* I am the child process */
      close(pipefd[1]); /* close write end */
      while (read(pipefd[0], &c, 1) > 0) {
          c = toupper(c);
          write(1, &c, 1);
      }
      write(1, "\n", 1);
      close(pipefd[0]);
      exit(EXIT_SUCCESS);
    } else if (pid > 0) { /* I am the parent process */
      close(pipefd[0]); /* close read end */
      write(pipefd[1], argv[1], strlen(argv[1]));
      close(pipefd[1]);
      wait(&status); /* wait for child to terminate */
      if (WIFEXITED(status))
         printf("Child process exited with status = %d\n", WEXITSTATUS(status));
      else
         printf("Child process did not terminate normally!\n");
    } else { /* we have an error in fork */
      perror("fork");
      exit(EXIT_FAILURE);
    }
  } else {
    perror("pipe");
    exit(EXIT_FAILURE);
  }
}
void pipe2() {
	
	  pid_t pid1, pid2;
    int pipefd[2]; /* fildes[0] for read, fildes[1] for write */
    int status1, status2;
    if (argc != 3) {
        printf("Usage: %s <command1> <command2>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    if (pipe(pipefd) == 0) { /* Open a pipe */
      pid1 = fork(); /* fork first process to execute command1 */
      if (pid1 == 0) { /* this is the child process */
        /* close read end of the pipe */
	close(pipefd[0]);
	/* replace stdout with write end of pipe */
	if (dup2(pipefd[1], 1) == -1) {
           perror("dup2");
           exit(EXIT_FAILURE);
        }
        /* execute <command1> */
        execlp(argv[1], argv[1], (char *)NULL);
        printf("If you see this statement then exec failed ;-(\n");
        perror("execlp");
        exit(EXIT_FAILURE);
      } else if (pid1 < 0) { /* we have an error */
        perror("fork"); /* use perror to print the system error 
message */
        exit(EXIT_FAILURE);
      }
      pid2 = fork(); /* fork second process to execute command2 */
      if (pid2 == 0) { /* this is child process */
        /* close write end of the pipe */
	close(pipefd[1]);
        /* replace stdin with read end of pipe */
	if (dup2(pipefd[0], 0) == -1) {
           perror("dup2");
           exit(EXIT_FAILURE);
        }
        /* execute <command2> */
        execlp(argv[2], argv[2], (char *)NULL);
        printf("If you see this statement then exec failed ;-(\n");
        perror("execlp");
        exit(EXIT_FAILURE);
      } else if (pid2 < 0) { /* we have an error */
        perror("fork"); /* use perror to print the system error 
message */
        exit(EXIT_FAILURE);
      }
      /* close the pipe in the parent */
      close(pipefd[0]);
      close(pipefd[1]);
      /* wait for both child processes to terminate */
      waitpid(pid1, &status1, 0);
      waitpid(pid2, &status2, 0);
    } else {
      perror("pipe");
      exit(EXIT_FAILURE);
    }
    return 0;
}
void pipe2() {
	
pid_t pid1, pid2;
    int pipefd[2]; /* fildes[0] for read, fildes[1] for write */
    int status1, status2;
    if (argc != 3) {
        printf("Usage: %s <command1> <command2>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    if (pipe(pipefd) == 0) { /* Open a pipe */
      pid1 = fork(); /* fork first process to execute command1 */
      if (pid1 == 0) { /* this is the child process */
        /* close read end of the pipe */
	close(pipefd[0]);
	/* replace stdout with write end of pipe */
	if (dup2(pipefd[1], 1) == -1) {
           perror("dup2");
           exit(EXIT_FAILURE);
        }
        /* execute <command1> */
        execlp(argv[1], argv[1], (char *)NULL);
        printf("If you see this statement then exec failed ;-(\n");
        perror("execlp");
        exit(EXIT_FAILURE);
      } else if (pid1 < 0) { /* we have an error */
        perror("fork"); /* use perror to print the system error 
message */
        exit(EXIT_FAILURE);
      }
      pid2 = fork(); /* fork second process to execute command2 */
      if (pid2 == 0) { /* this is child process */
        /* close write end of the pipe */
	close(pipefd[1]);
        /* replace stdin with read end of pipe */
	if (dup2(pipefd[0], 0) == -1) {
           perror("dup2");
           exit(EXIT_FAILURE);
        }
        /* execute <command2> */
        execlp(argv[2], argv[2], (char *)NULL);
        printf("If you see this statement then exec failed ;-(\n");
        perror("execlp");
        exit(EXIT_FAILURE);
      } else if (pid2 < 0) { /* we have an error */
        perror("fork"); /* use perror to print the system error 
message */
        exit(EXIT_FAILURE);
      }
      /* close the pipe in the parent */
      close(pipefd[0]);
      close(pipefd[1]);
      /* wait for both child processes to terminate */
      waitpid(pid1, &status1, 0);
      waitpid(pid2, &status2, 0);
    } else {
      perror("pipe");
      exit(EXIT_FAILURE);
    }
    return 0;
	
}	

void pipe3() {
	
pid_t pid1, pid2, pid3;
    int pipefd1[2]; /* pipefd1[0] for read, pipefd1[1] for write */
    int pipefd2[2]; /* pipefd2[0] for read, pipefd2[1] for write */
    int status1, status2, status3;
    if (argc != 4) {
        printf("Usage: %s <command1> <command2> <command3>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    if (pipe(pipefd1) != 0) { /* Open pipefd1 */
      perror("pipe");
      exit(EXIT_FAILURE);
    }
    if (pipe(pipefd2) != 0) { /* Open pipefd2 */
      perror("pipe");
      exit(EXIT_FAILURE);
    }
    pid1 = fork(); /* fork first process to execute command1 */
    if (pid1 == 0) { /* this is the child process */
      /* close read end of the pipefd1 */
      close(pipefd1[0]);
      /* close both ends of pipefd2 */
      close(pipefd2[0]);
      close(pipefd2[1]);
      /* replace stdout with write end of pipefd1 */
      if (dup2(pipefd1[1], 1) == -1) {
	perror("dup2");
	exit(EXIT_FAILURE);
      }
      
      /* execute <command1> */
      execlp(argv[1], argv[1], (char *)NULL);
      perror("execlp");
      exit(EXIT_FAILURE);
      
    } else if (pid1 < 0) { /* we have an error */
      perror("fork"); /* use perror to print the system error message 
*/
      exit(EXIT_FAILURE);
    }
    
    pid2 = fork(); /* fork second process to execute command2 */
    if (pid2 == 0) { /* this is child process */
      /* close write end of the pipefd1 */
      close(pipefd1[1]);
      
      /* replace stdin with read end of pipefd2 */
      if (dup2(pipefd1[0], 0) == -1) {
	perror("dup2");
	exit(EXIT_FAILURE);
      }
      /* close read end of pipefd2 */
      close(pipefd2[0]);
      /* replace stdout with write end of pipefd2 */
      if (dup2(pipefd2[1], 1) == -1) {
	perror("dup2");
	exit(EXIT_FAILURE);
      }
      
      /* execute <command2> */
      execlp(argv[2], argv[2], (char *)NULL);
      perror("execlp");
      exit(EXIT_FAILURE);
      
    } else if (pid2 < 0) { /* we have an error */
      perror("fork"); /* use perror to print the system error message 
*/
      exit(EXIT_FAILURE);
    }
    pid3 = fork(); /* fork third process to execute command3 */
    if (pid3 == 0) { /* this is child process */
      /* close both ends of the pipefd1 */
      close(pipefd1[0]);
      close(pipefd1[1]);
      
      /* close write end of pipefd2 */
      close(pipefd2[1]);
      /* replace stdin with read end of pipefd2 */
      if (dup2(pipefd2[0], 0) == -1) {
	perror("dup2");
	exit(EXIT_FAILURE);
      }
      
      /* execute <command3> */
      execlp(argv[3], argv[3], (char *)NULL);
      perror("execlp");
      exit(EXIT_FAILURE);
      
    } else if (pid3 < 0) { /* we have an error */
      perror("fork"); /* use perror to print the system error message 
*/
      exit(EXIT_FAILURE);
    }
    
    /* close the pipes in the parent */
    close(pipefd1[0]);
    close(pipefd1[1]);
    close(pipefd2[0]);
    close(pipefd2[1]);
    
    /* wait for both child processes to terminate */
    waitpid(pid1, &status1, 0);
    waitpid(pid2, &status2, 0);
    waitpid(pid3, &status3, 0);
    return 0;
	
}
void jobs () {
	
	int pid;
	int status;
	pid = wait(NULL);
	printf("Pid %d exited.\n\a", pid);
	
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
