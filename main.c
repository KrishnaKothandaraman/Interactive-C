#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>


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
    printf("3230shell ## ");

}

void read_commands(char cmd[], char *par[]){
    char input[1024];
    int ct = 0, i = 0, j = 0;
    char *arr[100], *words;

    // Read line
    for (;;){
        int c = fgetc(stdin);
        input[ct++] = (char) c;
        if (c == '\n'){
            break;
        }
    }
    input[ct] = '\0';
    printf("[INFO]: User entered command: %s", input);

    words = strtok(input, " \n");

    while (words != NULL){
        arr[i++] = strdup(words);
        words = strtok(NULL, " \n");
    }
    printf("[INFO]: Tokenized command is: ");
    for(int j = 0; j < i; j ++){
        printf("%s, ", arr[j]);
    }
    printf("\n");
    
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

int main(){
    char command[200], *params[30];
    struct rusage usage;
    int timeX;
    while(1){
        display_prompt();
        read_commands(command, params);
        timeX = 0;
        // timeX variable keeps track of whether timeX was entered
        if (strcmp(params[0], "timeX") == 0){
            timeX = 1;
            pointer_shift_to_left_by_one(params);
            if (params[0] == NULL){
                fprintf(stderr, "[ERROR] \"timeX\" cannot be a standalone command\n");
            }
        }

        int child = fork();
        if(child == -1){
            fprintf(stderr, "[ERROR] fork() Failed! %s", strerror(errno));
            continue;
        }
        if (child == 0){
            //child process. Start new process
            printf("[INFO]: Child successfully created\n");
            if ( execvp(command, params) == -1){
                fprintf(stderr, "[ERROR] '%s' %s\n", command, strerror(errno));
                continue;
            }
        }
        else{
            int status;
            waitpid(child, &status, 0);  // wait for child to terminate
            getrusage(RUSAGE_CHILDREN, &usage);
            if (timeX) {
            printf("(PID)%d\t(CMD)%s\t(user)%fs\t(sys)%fs\n",child, command, convert_to_seconds(usage.ru_utime.tv_sec, usage.ru_utime.tv_usec), convert_to_seconds(usage.ru_stime.tv_sec, usage.ru_stime.tv_usec));
            }
        }
        break;
    }
}