#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>

#define MAX_JOBS 5

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct jobs {
  int pid;
  char *prog;
};

struct jobs jobsTable[MAX_JOBS]; //jobs table
int numBackProc = 0; //used to manage jobs table and max jobs

//arguments to pass into thread
typedef struct {
  char *buffer;
  int status, pid, numBackProc;
} backgroundArgs;

//function to be passed into thread
void *backgroundProcess(void *arg) {
  backgroundArgs *backArgs = (backgroundArgs *)arg;
  int procNumIndex = backArgs->numBackProc - 1;

  jobsTable[procNumIndex].pid = backArgs->pid;
  jobsTable[procNumIndex].prog = backArgs->buffer;

  waitpid(backArgs->pid, &backArgs->status, 0);

  jobsTable[procNumIndex].pid = 0;
  jobsTable[procNumIndex].prog = "";

  pthread_mutex_lock(&mutex);
  numBackProc -= 1;
  pthread_mutex_unlock(&mutex);
}

//function for modifying buffer
void modifyBuffer(char *buffer, char modChar, int howMany) {
  for (int i = 0; i < 32; i++) {
    if (buffer[i] == modChar) buffer[i - howMany] = '\0';
  }
}

int main(int argc, char *argv[]) {
  char buffer[32];
  char *bufferOrig, *bufferFinal;
  int status = 0;

  while(1) {
    //accept user input
    printf("ZPShell> ");
    fgets(buffer, 32, stdin);

    //make modifications to buffer
    modifyBuffer(buffer, '\n', 0);
    bufferOrig = malloc(strlen(buffer) + 1);
    strcpy(bufferOrig, buffer);

    if (buffer[strlen(buffer) - 1] == '&') modifyBuffer(buffer, '&', 1);
    bufferFinal = malloc(strlen(buffer) + 1);
    strcpy(bufferFinal, buffer);

    //exit the shell
    if (strcmp(buffer, "exit") == 0) {
      break;
    }

    //display jobs table
    else if (strcmp(buffer, "jobs") == 0) {
      printf("PID\tProgram\n");
      for (int i = 0; i < MAX_JOBS; i++) {
        if (jobsTable[i].prog != "" && jobsTable[i].pid != 0) {
          printf("%d\t%s\n", jobsTable[i].pid, jobsTable[i].prog);
        }
      }
    }

    //process
    else {
      if (bufferOrig[strlen(bufferOrig) - 1] == '&' && numBackProc == 5) {
        printf("Too many jobs.  Try again in a little bit.\n");
      } else {
        int pid = fork();
        if (pid != 0) {
          if (bufferOrig[strlen(bufferOrig) - 1] == '&') {
            numBackProc += 1;
            backgroundArgs args = {bufferFinal, status, pid, numBackProc};
            pthread_t p1;
            pthread_create(&p1, NULL, backgroundProcess, &args);
          } else {
            waitpid(pid, &status, 0);
          }
        } else {
          char *arguments[] = {buffer, NULL};
          char *envp[] = {NULL};
          execve(buffer, arguments, envp);
        }
      }
    }
  }
  free(bufferOrig);
  free(bufferFinal);
}
