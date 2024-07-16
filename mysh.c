#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define MODE (S_IRUSR | S_IWUSR | S_IRGRP)
#define EXPANSION_SIZE 1024
#define INITIAL_CAPACITY 1024



int count = 0;
int numTokens;

// Change directories
void directoryChange(char **args) {
    const char *target_dir;
    if (args[1]) {
        target_dir = args[1];
    } else {
        target_dir = getenv("HOME");
    }

    char *path = NULL;
    if (target_dir[0] == '~' && (target_dir[1] == '/' || target_dir[1] == '\0')) {
        size_t home_len = strlen(getenv("HOME"));
        size_t extra_path_len = strlen(target_dir);
        path = malloc(home_len + extra_path_len);
        if (path == NULL) {
            perror("cd: malloc");
            return;
        }
        strcpy(path, getenv("HOME"));
        if (target_dir[1] != '\0') {
            strcat(path, &target_dir[1]);
        }
        target_dir = path;
    }

    if (chdir(target_dir) != 0) {
        perror("cd");
        printf("ERROR->");
    }

    free(path);
}

void exitFunc(char **args) {
  printf("%s\n", "mysh: exiting");
  exit(0);
}

struct builtin {
  char *name;
  void (*func)(char **args);
};


// Array of built in commands.
struct builtin builtins[] = {
    {"exit", exitFunc}, {"cd", directoryChange},
};


int mysh_builtins() { return sizeof(builtins) / sizeof(struct builtin); }

char *expandWildcard(char *pattern) {
    DIR *dir;
    struct dirent *entry; 
    size_t filenamesLength = 0;
    char *filenames = NULL; 
    int matchCount = 0; 

    char *slashPos = strrchr(pattern, '/');
    char directory[PATH_MAX] = ".";
    if (slashPos != NULL) {
        size_t dirLength = slashPos - pattern + 1;
        strncpy(directory, pattern, dirLength);
        directory[dirLength] = '\0';
    }

    dir = opendir(directory);
    if (dir == NULL) {
        return NULL;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (fnmatch(pattern, entry->d_name, FNM_PATHNAME) == 0) {
            matchCount++;
            size_t nameLen = strlen(entry->d_name);
            filenamesLength += nameLen + 1; 
            filenames = realloc(filenames, filenamesLength);
            if (!filenames) {
                closedir(dir);
                return NULL;
            }
            strcat(filenames, entry->d_name); 
            strcat(filenames, " "); 
        }
    }

    if (matchCount == 0) {
        closedir(dir);
        return strdup(pattern);
    }

    if (filenamesLength > 0) filenames[filenamesLength - 1] = '\0';

    closedir(dir);
    return filenames;
}



int executor(char **args) {
    int fd_in = STDIN_FILENO;
    char *input = NULL;
    int inputRedirects = 0;
    int outputRedirects = 0;
    int output_fds[100];
    int pipe_pos = -1;
    pid_t pid;
    int status;
    char *path;

    if (args[0] == NULL) {
        return -1;
    }
    
    if (args[0] != NULL) {
    for (int i = 0; i < mysh_builtins(); i++) {
        if (strcmp(args[0], builtins[i].name) == 0) {
            builtins[i].func(args);
            return 0;
        }
    }
}
    for (int i = 0; i < mysh_builtins(); i++) {
        if (strcmp(args[0], builtins[i].name) == 0) {
            builtins[i].func(args);
            return 0;
        }
    }

        for (int i = 0; args[i] != NULL; i++) {
        char *arg = args[i]; 
        if (arg == NULL) {
            break;
        }
    }


    int i;
    char *j = NULL;
    for (i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "then") == 0) {
            if (WEXITSTATUS(status) == 0) {
                for (int j = i; args[j] != NULL; j++) {
                    args[j] = args[j + 1];
                }
                return executor(args + i);
            }
            return 0;
        } else if (strcmp(args[i], "else") == 0) {
            if (WEXITSTATUS(status) != 0) {
                for (int j = i; args[j] != NULL; j++) {
                    args[j] = args[j + 1];
                }
                return executor(args + i);
            }
            return 0;
        }
    }

    if (j != NULL) {
        pid = fork();
        if (pid == 0) {
            if (j != NULL) {
                execv(j, args);
                fprintf(stderr, "mysh: %s: could not execute\n", j);
                exit(EXIT_FAILURE);
            }
        } else if (pid > 0) {
            status = 0;
            while (waitpid(pid, &status, 0) != pid) {
            }
            return WEXITSTATUS(status);
        } else {
            perror("mysh: fork failed");
            return 1;
        }
    }


    if (strchr(args[0], '/') != NULL) {
        path = args[0];

        pid = fork();
        if (pid == 0) {
            // Child process
            execv(path, args);
            // If we get here, execv failed
            fprintf(stderr, "mysh: %s: could not execute\n", args[0]);
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            // Fork failed
            fprintf(stderr, "mysh: fork failed\n");
            return 1;
        } else {
            // Parent process
            do {
                if (waitpid(pid, &status, WUNTRACED) == -1) {
                    perror("waitpid");
                    return 1;
                }
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
            return WEXITSTATUS(status);
        }
    }

    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) {
            pipe_pos = i;
            break;
        }
    }

    if (pipe_pos >= 0) {
        int pipe_fds[2];
        if (pipe(pipe_fds) == -1) {
            perror("pipe");
            return 1;
        }
        output_fds[outputRedirects] = pipe_fds[1];
        outputRedirects++;
        fd_in = pipe_fds[0];
        for (int i = pipe_pos; args[i] != NULL; i++) {
            args[i] = args[i + 1];
        }
    }

        if (args && args[0]) {
        for (int i = 0; i < mysh_builtins(); i++) {
            if (strcmp(args[0], builtins[i].name) == 0) {
                builtins[i].func(args);
                return 0;
            }
        }
    }

    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {
            input = args[i + 1];
            fd_in = open(input, O_RDONLY);
            if (fd_in < 0) {
                fprintf(stderr, "mysh: failed to open input file '%s'\n", input);
                return 1;
            }
            inputRedirects = 1;
            args[i] = NULL;
            args[i + 1] = NULL;
        } else if (strcmp(args[i], ">") == 0) {
            char *output = args[i + 1];
            int fd_out;
            if (outputRedirects == 0) {
                fd_out = open(output, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP);
            } else {
                fd_out = open(output, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP);
            }
            if (fd_out < 0) {
                fprintf(stderr, "mysh: failed to open output file '%s'\n", output);
                return 1;
            }
            output_fds[outputRedirects] = fd_out;
            outputRedirects++;
            for (int j = i; args[j] != NULL; j++) {
                args[j] = args[j + 2];
            }
            i--;
        }
    }

    pid_t child_pid = fork();
    if (child_pid == -1) {
        perror("fork");
        return 1;
    } else if (child_pid == 0) {
        // Child process
        if (inputRedirects) {
            if (dup2(fd_in, STDIN_FILENO) != STDIN_FILENO) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
            close(fd_in);
        }

        if (outputRedirects > 0) {
            int last_fd_out = output_fds[outputRedirects - 1];
            if (dup2(last_fd_out, STDOUT_FILENO) != STDOUT_FILENO) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
            close(last_fd_out);
        }

        for (int i = 0; i < outputRedirects - 1; i++) {
            if (dup2(output_fds[i], STDOUT_FILENO) != STDOUT_FILENO) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
            close(output_fds[i]);
        }

        if (execvp(args[0], args) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process
        for (int i = 0; i < outputRedirects; i++) {
            close(output_fds[i]);
        }

        if (pipe_pos >= 0) {
            char **next_args = args + pipe_pos + 1;
            return executor(next_args);
        }

        // Wait for the child process to finish
        if (waitpid(child_pid, &status, 0) == -1) {
            perror("waitpid");
            return 1;
        }

        // Check the status of the child process
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            return 1;
        }
    }

    return 0;
}


char **myshSplitter(char *line) {
  int length = 0;
  int capacity = 16;

  char **tokens = malloc(capacity * sizeof(char *));
  if (!tokens) {
    perror("mysh");
    exit(1);
  }

  char *temp = malloc(10 + strlen(line) * sizeof(char));
  char *stringChange = malloc(1000 + strlen(line) * sizeof(char));
  strcpy(stringChange, "");

  for (int i = 0; i < strlen(line); i++) {
    if (line[i] == '<') {
      if (i > 0 && line[i - 1] != ' ') {
        strcat(stringChange, " ");
      }
      strcat(stringChange, "< ");
    } else if(line[i] == '>'){
        if(i > 0 && line[i - 1] != ' '){
            strcat(stringChange, " ");
        }
        strcat(stringChange, "> ");
    }
    else {
      temp[0] = line[i];
      temp[1] = '\0';
      strcat(stringChange, temp);
    }
  }
  stringChange[strlen(stringChange)] = '\0';

    strcpy(line, stringChange);

    free(temp);

    //printf("%s\n", line);
    char *delimiters = " \t\r\n";
    char *token = strtok(line, delimiters);


    while (token != NULL) {
        tokens[length] = token;
        length++;

    if (length >= capacity) {
      capacity = (int)(capacity * 1.5);
      tokens = realloc(tokens, capacity * sizeof(char *));
      if (!tokens) {
        //printf("HERE");
        perror("mysh");
        exit(1);
      }
    }
    token = strtok(NULL, delimiters);
  }


  tokens[length] = NULL;
    char *fileList = NULL;
    for (int i = 0; i < strlen(line); i++) {
        if(line[i] == '*'){
            fileList = expandWildcard(line);
            if (fileList) {
              printf("%s\n", fileList);
              line = fileList;
            }
        }
    }

    free(fileList);

  free(stringChange);
  return tokens;
}

char *allocateMemory(size_t size) {
    char *ptr = malloc(size);
    if (!ptr) {
        perror("mysh");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void handleReadError() {
    if (errno == EINTR) {
        return;
    } else {
        perror("mysh");
        exit(EXIT_FAILURE);
    }
}

char *readLine() {
    size_t capacity = INITIAL_CAPACITY;
    char *line = allocateMemory(capacity);
    size_t position = 0;
    char c;

    while (1) {
        if (read(STDIN_FILENO, &c, 1) == -1) {
            handleReadError();
        }

        if (c == '\n' || c == EOF) {
            line[position] = '\0';
            return line;
        } else {
            line[position] = c;
        }

        position++;

        if (position >= capacity) {
            capacity += EXPANSION_SIZE;
            char *temp = realloc(line, capacity);
            if (!temp) {
                perror("mysh");
                exit(EXIT_FAILURE);
            }
            line = temp;
        }
    }
}




void executeLine(char* line) {
    if (line == NULL) {
        return;
    }
    char **tokens = myshSplitter(line);
    if (tokens[0] != NULL) {
        executor(tokens);
    }
    free(tokens);
    free(line);
}

void interactiveMode() {
    printf("Welcome to my shell!\n");
    char *line = NULL;
    size_t len = 0;

    while (true) {
        printf("mysh> ");
        fflush(stdout);
        if (getline(&line, &len, stdin) == -1) {
            if (feof(stdin)) {
                clearerr(stdin);
                free(line);
                break;
            }
        } else {
            if (strcmp(line, "exit\n") == 0) {
                free(line);
                break;
            }
            executeLine(line);
            line = NULL;
        }
    }
    printf("Exiting my shell.\n");
}

void nonInteractiveMode() {
    char *line = NULL;
    size_t len = 0;
    while (getline(&line, &len, stdin) != -1) {
        executeLine(line);
        line = NULL; 
    }
    free(line);
}

void batchMode(const char* scriptPath) {
    FILE *scriptFile = fopen(scriptPath, "r");
    if (!scriptFile) {
        perror("Error opening script file");
        exit(EXIT_FAILURE);
    }

    char *line = NULL;
    size_t len = 0;
    while (getline(&line, &len, scriptFile) != -1) {
        executeLine(line);
        line = NULL; // getline will allocate a new buffer
    }

    free(line);
    fclose(scriptFile);
}

int main(int argc, char *argv[]) {
    if (isatty(STDIN_FILENO) && argc == 1) {
        // Interactive mode
        interactiveMode();
    } else if (argc == 1) {
        // Non-interactive mode, likely piped input
        nonInteractiveMode();
    } else if (argc > 1) {
        // Batch mode or direct command execution
        batchMode(argv[1]);
    }
    return 0;
}