#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include "list.h"
#include <pthread.h>

//Structure to maintain socket parameters
struct socketParams{
    int port;
    int socketNum;
    struct sockaddr_in address;
};

int statementBeginsWith(const char *, const char *); //function to check inputs of statements
int checkSTalkInput(const char *);
int initializeSTalkFromLocal(char *);
void* inputRunner();
void* outputRunner();
void * receiveMessageRunner(void *);
void * sendMessageRunner(void *);

//List ADT to maintain the order of operations
List* threadList;

//Global Variables
int originPort;
int destinationPort;
char *destinationMachine;
char buffer[1024];
char message[1024];
int messageFlag = 0;
int exitFlag = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_t inputThread, outputThread, datagramThread, sendDataThread;

int main() {
    char input[100];
    threadList = List_create();
    //Figure out how to get message here

    while (1) {
        printf("\nEnter your command or type \"exit\" to exit the process:\n");
        fgets(input, 100, stdin);
        if (statementBeginsWith(input, "exit")) {
            break;
        } else {
            if (statementBeginsWith(input, "s-talk")) {
                if (checkSTalkInput(input) == 1) {
                    printf("\nValid S-talk statement found\n");
                    initializeSTalkFromLocal(input);

                } else {
                    printf("\nInvalid s-talk statement, please check your paremeters");
                }
            } else {
                printf("\nThe format to start an s-talk session is: \ns-talk [my port number] [remote machine name] [remote port number]\n");
            }
        }
    }
    return 0;
}

int initializeSTalkFromLocal(char *input) {

    char *tokenizedInput;
    tokenizedInput = strtok(input, " ");

    //Getting the Origin Port using delimiters
    tokenizedInput = strtok(NULL, " ");
    originPort = atoi(tokenizedInput);

    //Getting the Destination Machine using delimiters
    tokenizedInput = strtok(NULL, " ");
    destinationMachine = tokenizedInput;

    //Getting the Destination Port using delimiters
    tokenizedInput = strtok(NULL, " ");
    destinationPort = atoi(tokenizedInput);

//    printf("Origin port is: %d\n", originPort);
//    printf("Destination machine is: %s\n", destinationMachine);
//    printf("Destination port is: %d\n", destinationPort);

    //Setting up the local Socket Address structure
    struct socketParams *localSocketParams;
    localSocketParams = malloc(sizeof(struct socketParams));
    localSocketParams->socketNum=socket(AF_INET, SOCK_DGRAM, 0);
    localSocketParams->address.sin_family=AF_INET;
    localSocketParams->address.sin_port = htons(originPort);
    localSocketParams->address.sin_addr.s_addr = INADDR_ANY;
    memset(&localSocketParams->address.sin_zero, '\0', 8);
    bind(localSocketParams->socketNum, (struct sockaddr *)&localSocketParams->address, sizeof(struct sockaddr_in));

    struct socketParams *destinationSocketParams;
    destinationSocketParams = malloc(sizeof(struct socketParams));
    destinationSocketParams->socketNum=socket(AF_INET, SOCK_DGRAM, 0);
    destinationSocketParams->address.sin_family=AF_INET;
    destinationSocketParams->address.sin_port = htons(destinationPort);
    destinationSocketParams->address.sin_addr.s_addr = INADDR_ANY;
    memset(&destinationSocketParams->address.sin_zero, '\0', 8);
    bind(destinationSocketParams->socketNum, (struct sockaddr *)&destinationSocketParams->address, sizeof(struct sockaddr_in));

    //s-talk 6060 localhost 6001
    printf("=======Welcome to Chatroom with machine %s on PORT %d=======\n", destinationMachine, destinationPort);

    //This is where the thread should start looking for input
    if(pthread_create(&inputThread, NULL, (void*) inputRunner, NULL)!=0){
        printf("There was an error creating the INPUT THREAD");
        return 0;
    }

    if(pthread_create(&sendDataThread, NULL, sendMessageRunner, (void *) localSocketParams)!=0){
        printf("There was an error creating the SEND DATA THREAD");
        return 0;
    }

    if(pthread_create(&datagramThread, NULL, receiveMessageRunner, (void *) destinationSocketParams)!=0){
        printf("There was an error creating the RECEIVE DATAGRAM THREAD");
        return 0;
    }

    pthread_join(datagramThread, NULL);
    pthread_join(sendDataThread, NULL);
    pthread_join(inputThread, NULL);

    //pthread_join(inputThread, NULL);
    //pthread_create(&outputThread, NULL, outputRunner, NULL);
    //pthread_create(&datagramThread, NULL, receiveMessageRunner, (void *) destinationSocketParams);

//    while(strcmp(buffer,"!")!=0) {
//
//        pthread_join(inputThread, NULL);
//        //pthread_join(datagramThread, NULL);
//        //pthread_join(outputThread, NULL);
//        //pthread_join(sendDataThread, NULL);
//        printf("Back in main while %s\n", buffer);
//        break;
//    }

    if(strcmp(buffer,"!")==0) {
        close(localSocketParams->socketNum);
        close(destinationSocketParams->socketNum);
    }
}

void* inputRunner(){
    while(read(STDIN_FILENO, &buffer, sizeof(buffer))>0)
    {
        fflush(stdout);
        printf("You:\t");
        pthread_mutex_lock(&mutex);
        char *tokenizedInput;
        tokenizedInput = strtok(buffer, "\n");
        //if(buffer[0]=='!' && strlen(buffer)==1) {
        if(strcmp(tokenizedInput,"!")==0){
            printf("EXIT STATEMENT FOUND.");
            messageFlag=0;
            pthread_mutex_unlock(&mutex);
            pthread_exit(0);
        }
        buffer[strlen(buffer)+1]='\n';
        messageFlag=1;
        pthread_mutex_unlock(&mutex);
    }
    //Once invoked, add to list
    pthread_exit(0);
}

void * sendMessageRunner(void *localSocket){
    while(1) {
        if (messageFlag == 1) {
            pthread_mutex_lock(&mutex);
            if(strcmp(buffer,"!")==0){
                pthread_exit(0);
            }
            struct socketParams *destinationParams = (struct socketParams *) localSocket;
            sendto(destinationParams->socketNum, &buffer, 1024, 0,
                   (struct sockaddr *) &destinationParams->address,
                   sizeof(struct sockaddr_in));
            messageFlag = 0;
            pthread_mutex_unlock(&mutex);
        }
    }
    //Once invoked, add to list
}

void * receiveMessageRunner(void *destinationSocket){
    while(1) {
        if(exitFlag==1){
            pthread_exit(0);
        }
        struct socketParams *destinationParams = (struct socketParams *) destinationSocket;
        recvfrom(destinationParams->socketNum, (char *) message, 1024, MSG_WAITALL,
                 (struct sockaddr *) &destinationParams->address, sizeof(struct sockaddr_in));
        printf("\nReceived message: %s", &message);
    }
    //Once invoked, add to list
}

void* outputRunner(){
    int size = strlen(buffer);
    while(size > 0)
    {
        write(STDIN_FILENO, buffer, size);
        //pthread_exit(0);
    }
    //Once invoked, add to list
}


int checkSTalkInput(const char *input) {
    //Really bad way of checking if inputs are valid sorry (literally checking if there's 4 things in the input LOL)
    int count = 0, i;
    for (i = 0; input[i] != '\0'; i++) {
        if (input[i] == ' ' && input[i + 1] != ' ')
            count++;
    }
    if (count == 3)
        return 1;
    else
        return 0;
}

int statementBeginsWith(const char *a, const char *b) {
    if (strncmp(a, b, strlen(b)) == 0) return 1;
    return 0;
}
