#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include "list.h"
#include <pthread.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAX_MESSAGE_LENGTH 1024

//Structure to maintain socket parameters
struct socketParams {
    int port;
    int socketNum;
    struct sockaddr_in address;
};

//Function prototypes
int initializeSTalkFromLocal(char* , char*, char* );
void *inputRunner();
void *outputRunner();
void *receiveMessageRunner(void *);
void *sendMessageRunner(void *);
void overWriteStdout();

//List ADT and sendMutex to maintain the input+send
List *sendMessageList;
pthread_mutex_t sendMutex = PTHREAD_MUTEX_INITIALIZER;

//List ADT and sendMutex to maintain the reception+output
List *receiveMessageList;
pthread_mutex_t receiveMutex = PTHREAD_MUTEX_INITIALIZER;

//Global Variables
char buffer[MAX_MESSAGE_LENGTH];
char message[MAX_MESSAGE_LENGTH];
int exitFlag = 0;

//Threads for each individual process we need to run concurrently
pthread_t inputThread, outputThread, datagramThread, sendDataThread;

int main( int argc, char *argv[] ) {
    sendMessageList = List_create();
    receiveMessageList = List_create();

    if( argc < 4 ) {
        printf("Too few arguments supplied.\n");
        exit(0);
    }
    else if (argc > 4) {
        printf("Too many arguments supplied.\n");
        exit(0);
    }

    initializeSTalkFromLocal(argv[1], argv[2], argv[3]);
    return 0;
}

int initializeSTalkFromLocal(char*  originPort, char* destinationMachine, char*  destinationPort) {

    int convertedOriginPort = atoi(originPort);
    int convertedDestinationPort = atoi(destinationPort);

    printf("Origin port is: %d\n", convertedOriginPort);
    printf("Destination machine is: %s\n", destinationMachine);
    printf("Destination port is: %d\n", convertedDestinationPort);

    //Setting up the local Socket Address structure
    struct socketParams *localSocketParams;
    localSocketParams = malloc(sizeof(struct socketParams));
    localSocketParams->socketNum = socket(AF_INET, SOCK_DGRAM, 0);
    localSocketParams->address.sin_family = AF_INET;
    localSocketParams->address.sin_port = htons(convertedOriginPort);
    localSocketParams->address.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(&localSocketParams->address.sin_zero, '\0', 8);
    if(bind(localSocketParams->socketNum, (struct sockaddr *) &localSocketParams->address, sizeof(struct sockaddr_in))!=0){
        printf("UNABLE TO BIND LOCAL, exiting.");
        exit(0);
    }

    struct socketParams *destinationSocketParams;
    destinationSocketParams = malloc(sizeof(struct socketParams));
    destinationSocketParams->socketNum = socket(AF_INET, SOCK_DGRAM, 0);
    destinationSocketParams->address.sin_family = AF_INET;
    destinationSocketParams->address.sin_port = htons(convertedDestinationPort);
    struct hostent *destinationMachineIP;
    if ((destinationMachineIP = gethostbyname(destinationMachine) ) == NULL ) {
        printf("\nINVALID MACHINE NAME (Please check the name of the host machine you're trying to connect to)\n");
        exit(0);
    }
    memcpy(&destinationSocketParams->address.sin_addr, destinationMachineIP->h_addr_list[0], destinationMachineIP->h_length);

    memset(&destinationSocketParams->address.sin_zero, '\0', 8);
    bind(localSocketParams->socketNum, (struct sockaddr *) &destinationSocketParams->address,
         sizeof(struct sockaddr_in));

    //s-talk 6060 localhost 6001
    printf("=======Welcome to Chatroom with machine %s on PORT %d=======\n", destinationMachine, convertedDestinationPort);

    //This is where the thread should start looking for input
    if (pthread_create(&inputThread, NULL, (void *) inputRunner, NULL) != 0) {
        printf("There was an error creating the INPUT THREAD");
        return 0;
    }

    if (pthread_create(&sendDataThread, NULL, sendMessageRunner, (void *) destinationSocketParams) != 0) {
        printf("There was an error creating the SEND DATA THREAD");
        return 0;
    }

    if (pthread_create(&datagramThread, NULL, receiveMessageRunner, (void *) localSocketParams) != 0) {
        printf("There was an error creating the RECEIVE DATAGRAM THREAD");
        return 0;
    }

    if (pthread_create(&outputThread, NULL, outputRunner, NULL) != 0) {
        printf("There was an error creating the OUTPUT THREAD");
        return 0;
    }

    pthread_join(sendDataThread, NULL);
    pthread_join(inputThread, NULL);
    pthread_join(datagramThread, NULL);
    pthread_join(inputThread, NULL);

    if (strcmp(buffer, "!") == 0) {
        close(localSocketParams->socketNum);
        close(destinationSocketParams->socketNum);
        exit(0);
    }
}

void *inputRunner() {
    while (read(STDIN_FILENO, &buffer, sizeof(buffer)) > 0) {
        overWriteStdout();
        pthread_mutex_lock(&sendMutex);
        char *tokenizedInput;
        tokenizedInput = strtok(buffer, "\n");
        List_append(sendMessageList, (void*)buffer);
        pthread_mutex_unlock(&sendMutex);
        //printf("\n");
    }
    pthread_exit(0);
}

void *sendMessageRunner(void *dest) {
    while (1) {
        if (sendMessageList->size>0) {
            pthread_mutex_lock(&sendMutex);
            List_first(sendMessageList);
            char* messageToSend = (char*) sendMessageList->current->data;
            struct socketParams *destinationParams = (struct socketParams *) dest;
            sendto(destinationParams->socketNum, messageToSend, MAX_MESSAGE_LENGTH, 0,
                   (struct sockaddr *) &destinationParams->address,
                   sizeof(struct sockaddr_in));
            List_remove(sendMessageList);
            if (strcmp(messageToSend, "!") == 0) {
                printf("=======ENDING LOCAL CHATROOM SESSION=======\n");
                pthread_mutex_unlock(&sendMutex);
                exit(0);
            }
            pthread_mutex_unlock(&sendMutex);
        }
    }
}

void *receiveMessageRunner(void *recvSocket) {
    while (1) {
        if (exitFlag == 1) {
            printf("=======ENDING LOCAL CHATROOM SESSION=======\n");
            pthread_mutex_unlock(&sendMutex);
            exit(0);
        }
        struct socketParams *destinationParams = (struct socketParams *) recvSocket;
        recvfrom(destinationParams->socketNum, (char *) message, MAX_MESSAGE_LENGTH, MSG_WAITALL,
                 (struct sockaddr *) &destinationParams->address, sizeof(struct sockaddr_in));
        if (strcmp(message, "!") == 0) {
                printf("=======ENDING LOCAL CHATROOM SESSION=======\n");
                pthread_mutex_unlock(&sendMutex);
                exit(0);
        }
        pthread_mutex_lock(&receiveMutex);
        List_append(receiveMessageList, (void*)message);
        pthread_mutex_unlock(&receiveMutex);
    }
}

void *outputRunner() {
    while (1) {
        if (exitFlag == 1) {
            pthread_exit(0);
        }
        if (receiveMessageList->size>0) {
            pthread_mutex_lock(&receiveMutex);
            List_first(receiveMessageList);
            printf("Message Received: ");
            overWriteStdout();
            char* outputMessage = (char*) receiveMessageList->current->data;
            int size = strlen(outputMessage);
            while (size > 0) {
                write(STDIN_FILENO, outputMessage, size);
                size = 0;
            }
            List_remove(receiveMessageList);
            pthread_mutex_unlock(&receiveMutex);
            printf("\n");
        }
    }
}

void overWriteStdout() {
    fflush(stdout);
}
