#include<stdio.h>
#include<assert.h>
#include<string.h>
#include<errno.h>
#include<stdlib.h>
#include<unistd.h>

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
    printf("User entered command: %s", input);

    words = strtok(input, " \n");

    while (words != NULL){
        arr[i++] = strdup(words);
        words = strtok(NULL, " \n");
    }
    printf("Tokenized command is: ");
    for(int j = 0; j < i; j ++){
        printf("%s, ", arr[j]);
    }
    printf("\n");
    
    strcpy(cmd, arr[0]);

    for(j = 0;j < i; j++){
        par[j] = arr[j];
    }
    par[j] = NULL;
}

int main(){
    char command[200], *params[20];
    while(1){
        display_prompt();
        read_commands(command, params);
        int child = fork();
        if(child == -1){
            printf("fork() Failed! %s", strerror(errno));
            continue;
        }
        if (child != 0){
            //child process. Start new process
            printf("Child successfully created. Now running command\n");
            if ( execvp(command, params) == -1){
                printf("Error occur occured = %s\n", strerror(errno));
                continue;
            }
        }
        else{
            wait(NULL);
        }
        break;
    }
}