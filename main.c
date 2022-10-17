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

typedef struct ShellProcess {
    int pid;
    int timeX;
    char *command[200];
    char *params[30];
    int paramPtr;
    int backgroundProcess;
} shell_process;

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

/* return 1 if error occured, 0 if no error occured during parsing */
int read_commands(shell_process shell_cmd_array[], int *cmd_ptr){
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
    
    shell_process newcmd;
    memset(&newcmd, 0, sizeof(shell_process));
    // printf("Doing processing shit\n");
    for(int pt = 0; pt < i; pt ++){
        if (strcmp(arr[pt], "|") == 0){
            shell_cmd_array[(*cmd_ptr)++] = newcmd;
            memset(&newcmd, 0, sizeof(shell_process));
        }
        else{
            newcmd.params[newcmd.paramPtr] = arr[pt];
            newcmd.paramPtr++;
        }
    }
    shell_cmd_array[(*cmd_ptr)++] = newcmd;
    for(int k = 0; k < *cmd_ptr ; k++){
        if (strcmp(shell_cmd_array[k].params[0], "timeX") == 0){
            if (shell_cmd_array[k].params[1] == NULL){
                fprintf(stderr, "[ERROR] \"timeX\" cannot be a standalone command\n");
                return 1;
            }
            strcpy(shell_cmd_array[k].command, shell_cmd_array[k].params[1]);
            pointer_shift_to_left_by_one(shell_cmd_array[k].params);
            shell_cmd_array[k].timeX = 1;
        }
        else if (strcmp(shell_cmd_array[k].params[0], "exit") == 0){
            // timeX variable keeps track of whether timeX was entered
            if (shell_cmd_array[k].params[1] != NULL){
                fprintf(stderr, "[ERROR] \"exit\" must be a standalone command\n");
                return 1;
            }
            printf("3200shell: Terminating\n");
            exit(0);
        }
        else{
            strcpy(shell_cmd_array[k].command, shell_cmd_array[k].params[0]);
        }

    }

    // printf("Printing struct command and timeX details\n");
    // for(int k = 0; k < *cmd_ptr ; k++){
    //     printf("Struct %d command = %s timeX = %d\n", k, shell_cmd_array[k].command, shell_cmd_array[k].timeX);
    // }
    return 0;
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

    shell_process shell_process_array[1024];
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
        int timeX = 0;
        int shell_cmd_len = 0;
        waiting_for_input = 1;
        display_prompt();
        int err = read_commands(shell_process_array, &shell_cmd_len);
        if (err){
            continue;
        }
        waiting_for_input = 0;

        // set mask for SIGUSR1. Placed before fork for synchoronization purposes
        sigset_t sigset;
        int sig;
        sigemptyset(&sigset);
        sigaddset(&sigset, SIGUSR1);
        sigprocmask(SIG_BLOCK, &sigset, NULL);
        printf("Number of commands entered %d\n", shell_cmd_len);

        // 2D array of pipes for child processes. Number of pipes = children - 1
        int pfd[shell_cmd_len - 1][2];
        int result = -1;
        
        printf("Creating %d pipes\n", shell_cmd_len -1);
        // create pipes
        for (int s = 0; s < shell_cmd_len - 1; s++) {

            result = pipe(pfd[s]);
            if (result == -1) {
                fprintf(stderr, "Error occured during pipe creation of child %d\n", s);
                exit(1);
            }
        }
        // Array to store PID of children for parent to handle after
        int child_pid_array[shell_cmd_len];

        for (int ch = 0; ch < shell_cmd_len; ch++){
            int child = fork();
            // fork fail
            if(child == -1){
                fprintf(stderr, "[ERROR] fork() Failed! %s\n", strerror(errno));
                continue;
            }
            child_pid_array[ch] = child;

            // only child executes this logic
            if (child == 0){
                // child logic
                printf("[CHILD] Child %d waiting for SIGUSR1\n", ch);
                int res = sigwait(&sigset, &sig);
                printf("[CHILD] Child %d received SIGUSR1\n", ch);
                if (res != 0){
                    fprintf(stderr, "Error occured while waiting for signal %s", strerror(errno));
                    exit(1);
                }

                // If we are not the first program in the pipe
                if (ch > 0) {
                    // Use the output from the previous program in the pipe as our input
                    // Check the read end of the pipe and STDIN are different descriptors
                    if (pfd[ch - 1][0] != STDIN_FILENO) {
                        // Send the read end of the pipe to STDIN
                        if (dup2(pfd[ch - 1][0], STDIN_FILENO) == -1) {
                            fprintf(stderr, "Error occured during pipe input end management of child %d\n", ch);
                            exit(1);                    
                        }
                    }
                    printf("[CHILD] %d rerouted stdin\n");
                }

                // Before we execute a process, bind the write end of the pipe to STDOUT
                // Don't do this to the last process in the pipe, just send output to STDOUT as normal
                if (ch < shell_cmd_len - 1) {
                    // Check the write end of the pipe and STDOUT are different descriptors
                    if (pfd[ch][1] != STDOUT_FILENO) {
                        // Send the write end of the pipe to STDOUT
                        if (dup2(pfd[ch][1], STDOUT_FILENO) == -1) {
                            fprintf(stderr, "Error occured during pipe output end management of child %d\n", ch);
                            exit(1);   
                        }
                    }
                    printf("[CHILD] %d rerouted stdout\n");
                }

                for (int j = 0; j < shell_cmd_len - 1; j++) {
                    printf("[CHILD] %d Closing ends of pipe %d\n", child , j);
                    if(close(pfd[j][0]) == -1){
                        fprintf(stderr, "Error occured during pipe closure of child %n\n", j);
                        exit(1);   
                    }
                    if(close(pfd[j][1]) == -1){
                        fprintf(stderr, "Error occured during pipe closure of child %n\n", j);
                        exit(1);   
                    }
                }

                //child process. Start new process
                if (execvp(shell_process_array[ch].command, shell_process_array[ch].params) == -1){
                    fprintf(stderr, "[ERROR] '%s' %s %d\n", command, strerror(errno), (int) getpid());
                    exit(errno);
                    }
            }
        }
        
        int n = 0;
        int status;
        int child_pid;
        // parent logic

        for (int j = 0; j < shell_cmd_len - 1; j++) {
            printf("[PARENT] %d Closing ends of pipe %d\n" , 0);
            if(close(pfd[j][0]) == -1){
                fprintf(stderr, "Error occured during pipe closure of child %n\n", j);
                exit(1);   
            }
            if(close(pfd[j][1]) == -1){
                fprintf(stderr, "Error occured during pipe closure of child %n\n", j);
                exit(1);   
            }
        }

        while (n < shell_cmd_len){

            printf("[PARENT] Sending SIGUSR1 to child %d\n", n);

            //send SIGUSR1 to child[n]. Synchronization purpose
            kill(child_pid_array[n], SIGUSR1);
            
            // wait for that child to terminate
            child_pid = wait(&status);  // wait for child to terminate
            printf("Process %d exited\n", child_pid);
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

            if (shell_process_array[0].timeX) {
                printf("3200 shell: (PID)%d  (CMD)%s    (user)%f s  (sys)%f s\n",child_pid, command, convert_to_seconds(usage.ru_utime.tv_sec, usage.ru_utime.tv_usec), convert_to_seconds(usage.ru_stime.tv_sec, usage.ru_stime.tv_usec));
            }
            n++;
        }
    }
}