#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <signal.h>

// parent's process ID
int parent_pid;
int waiting_for_input = 1;

typedef struct ShellCommand {
    char *command;
    char *params[30];
    int paramPtr;
    int timeX;
    int backgroundProcess;
} shell_command;


void pointer_shift_to_left_by_one(char *a[]) {
   int i;
   for (i = 0; *(a+i) != NULL; i++) {
      *(a+i) = *(a+i+1);
   }
}


float convert_to_seconds(int seconds, long int microseconds){
    return (seconds + (1e-6*microseconds));
}

void display_prompt(){
    fseek(stdin, 0, SEEK_END);
    printf("3230shell ## ");

}


void read_commands(char cmd[], char *par[]){
    char input[1024];
    int ct = 0, i = 0, j = 0;
    char *arr[100], *words;
    // Read line
    for (;;){
        int c = fgetc(stdin);
        if (c == -1){
            continue;
        }
        input[ct++] = (char) c;
        if (c == '\n'){
            break;
        }
    }
    input[ct] = '\0';
    printf("[INFO]: User entered command: %s", input);
    // TODO: FIGURE OUT WHY SPACES BREAK THIS!
    words = strtok(input, "| \n");

    while (words != NULL){
        arr[i++] = strdup(words);
        words = strtok(NULL, " \n");
    }
    printf("[INFO]: Tokenized command is: ");
    for(int j = 0; j < i; j ++){
        printf("%s, ", arr[j]);
    }
    printf("\n");
    
    int structPtr = 0;
    shell_command newcmd;
    shell_command shell_cmd_array[1024];
    printf("Doing processing shit\n");
    for(int pt = 0; pt < i; pt ++){
        if (strcmp(arr[pt], "|") == 0){
            printf("Break found!\n");
            shell_cmd_array[structPtr++] = newcmd;
            memset(&newcmd, 0, sizeof(shell_command));
        }
        else{
            newcmd.params[newcmd.paramPtr] = arr[pt];
            newcmd.paramPtr++;
        }
    }

    shell_cmd_array[structPtr] = newcmd;

    printf("Printing struct arr details\n");
    for(int k=0;k<=structPtr;k++){
        printf("Printing details of struct %d\n", k);
        for(int l=0;l<shell_cmd_array[k].paramPtr;l++){
            printf("%s ", shell_cmd_array[k].params[l]);
        }
        printf("\n");
    }
    exit(0);
    if (strcmp(arr[0], "timeX") == 0){
        strcpy(cmd, arr[1]);
    }
    else{
        strcpy(cmd, arr[0]);
    }

    for(j = 0;j < i; j++){
        par[j] = arr[j];
    }
    par[j] = NULL;
}

void sigHandler(int signum, siginfo_t *sig, void *v){
    if(signum == SIGINT){
        if (!waiting_for_input){
            return;
        }
        printf("\n");
        display_prompt();
    }
}

int main(){
    char command[200], *params[30];
    struct rusage usage;
    int timeX;
    struct sigaction sa;
    parent_pid = (int) getpid();

    // register sigaction for SIGINT
    sigaction(SIGINT, NULL, &sa);
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = sigHandler;
	sigaction(SIGINT, &sa, NULL);

    while(1){
        waiting_for_input = 1;
        display_prompt();
        read_commands(command, params);
        waiting_for_input = 0;
        timeX = 0;
        if (strcmp(params[0], "exit") == 0){
            if(params[1] != NULL){
                fprintf(stderr, "exit must be a standalone command!\n");
                continue;
            }
            printf("3200shell: Terminated\n");
            exit(0);
        }
        // timeX variable keeps track of whether timeX was entered
        if (strcmp(params[0], "timeX") == 0){
            timeX = 1;
            pointer_shift_to_left_by_one(params);
            if (params[0] == NULL){
                fprintf(stderr, "[ERROR] \"timeX\" cannot be a standalone command\n");
            }
        }

        // set mask for SIGUSR1. Placed before fork for synchoronization purposes
        sigset_t sigset;
        int sig;
        sigemptyset(&sigset);
        sigaddset(&sigset, SIGUSR1);
        sigprocmask(SIG_BLOCK, &sigset, NULL);

        int child = fork();
        if(child == -1){
            fprintf(stderr, "[ERROR] fork() Failed! %s", strerror(errno));
            continue;
        }
        if (child == 0){
            // child logic
            int res = sigwait(&sigset, &sig);
            if (res != 0){
                fprintf(stderr, "Error occured while waiting for signal %s", strerror(errno));
                exit(1);
            }
            //child process. Start new process
            if (execvp(command, params) == -1){
                fprintf(stderr, "[ERROR] '%s' %s %d\n", command, strerror(errno), (int) getpid());
                exit(errno);
            }
        }
        else{
            // parent logic

            //send SIGUSR1 to child
            kill(child, SIGUSR1);
            int status;
            waitpid(child, &status, 0);  // wait for child to terminate
            getrusage(RUSAGE_CHILDREN, &usage);
            // TODO: FIGURE OUT WTF IS GOING ON???
            if(WIFSIGNALED(status)){
                int terminating_signal = (int) WTERMSIG(status);
                printf("Child exited cause of signal! %d\n", terminating_signal);
                if (terminating_signal == 124){
                    printf("Interrupt\n");
                }
                else if(terminating_signal == SIGKILL){
                    printf("Killed\n");
                }
            }

            if (timeX) {
            printf("3200 shell: (PID)%d  (CMD)%s    (user)%f s  (sys)%f s\n",child, command, convert_to_seconds(usage.ru_utime.tv_sec, usage.ru_utime.tv_usec), convert_to_seconds(usage.ru_stime.tv_sec, usage.ru_stime.tv_usec));
            }
        }
    }
}