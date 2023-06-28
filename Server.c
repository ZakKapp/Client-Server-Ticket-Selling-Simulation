#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <math.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define PORT_NUMBER 5436
#define MAX_CLIENTS 5

int rows = 10;
int cols = 10;
int*  seats;
int seatsLeft;

typedef struct threadInfo_ {
	int sockfd;
} threadInfo;

int queue[10];
int clientCount = 0;
pthread_t handlerThreads[MAX_CLIENTS];
pthread_t listener;
pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condQueue = PTHREAD_COND_INITIALIZER;

//helper function for finding out how many seats are still available
int seatsAvailable() {
	int numAvailable = 0;
	for(int i = 0; i < (rows * cols); i++) {
		if(seats[i] == 0)
			numAvailable++;
	}
	seatsLeft = numAvailable;
	return numAvailable;
} // end seatsAvailable

//helper function for checking if a seat ticket can be bought
int buyTicket(int index) {
	int success = 0;
	if(seats[index] == 0) {
		pthread_mutex_lock(&myMutex);
		seats[index] = 1;
		success = 1;
		pthread_mutex_unlock(&myMutex);
	}
	return success;
} // end buyTicker

//function to mangage and handle the requests of clients
void manager(int connfd) {
	char sendBuff[BUFFER_SIZE];
	memset(sendBuff, 0, sizeof(sendBuff));

	pid_t userPid;
	int pidFlag = read(connfd, &userPid, sizeof(userPid));

	if(seatsLeft == 0)
		return;

	//error checking
	if(pidFlag < 0) {
		printf("ERROR: Failed to retrieve the user's PID\n");
	}

	printf("New Client is PID[%d]\n\n", userPid);

	//startup message
	sprintf(sendBuff, "This plane has %d rows and %d seats per row\nThere are %d seats remaining\n", rows, cols, seatsAvailable());
	write(connfd, sendBuff, strlen(sendBuff));

	int handShake = 0;
	read(connfd, &handShake, sizeof(handShake));
	if(handShake != 1)
		printf("ERROR: Did not receive acknowledgment\n");


	//sending seat information
	int seatStats[3] = {seatsAvailable(), rows, cols};

	write(connfd, seatStats, sizeof(seatStats));

	while(1) {
		char message[BUFFER_SIZE];
		int seatRequest[2];

		read(connfd, seatRequest, sizeof(seatRequest));
		int r = seatRequest[0];
		int c = seatRequest[1];
		int index = c + (r * cols);

		if(buyTicket(index) == 0) {
			printf("Failed to reserve seat at [%d, %d]\n", r, c);
			sprintf(message, "Seat at [%d, %d] is taken\n", r, c);
		}
		else {
			printf("Success in reserving seat at [%d, %d]\n", r, c);
			sprintf(message, "You now own seat [%d, %d]\n", r, c);
		}
		write(connfd, message, strlen(message));

		//checking to see if client received message
		read(connfd, &handShake, sizeof(handShake));
		if(handShake != 1)
			printf("ERROR: Did not receive acknowledgment\n");

		int availability = seatsAvailable();
		write(connfd, &availability, sizeof(availability));

		//printing updated seat map
		for(int i = 0; i < (rows * cols); i++) {
			printf("%d  ", seats[i]);
			if((i + 1) % cols == 0)
				printf("\n");
		}
		printf("\n");
		if(availability == 0)
			break;
	} // manager loop
	return;
} // end manager

//function to add incoming clients to the queue
void submitClient(int clientConnfd) {
	pthread_mutex_lock(&myMutex);
	queue[clientCount] = clientConnfd;
	clientCount++;
	pthread_mutex_unlock(&myMutex);
	pthread_cond_signal(&condQueue);
} // end submitClient

//thread function for accepting new clients and adding them to the thread queue
void * listenerThread(void* param) {
	threadInfo * myParam = (threadInfo *)param;
	int connfd;
	int sockfd = myParam->sockfd;
	while(1) {
		connfd = accept(sockfd, (struct sockaddr*)NULL, NULL);
		if(connfd < 0) {
			printf("ERROR: Failed to connect to user\n");
			continue;
		}
		submitClient(connfd);
	}
	return (void*)0;
}

//thread function for handling incoming clients
void * clientThread(void * param) {
	while(1) {
		int clientConnfd = 0;
		if(seatsAvailable() == 0)
			break;

		pthread_mutex_lock(&myMutex);
		while(clientCount == 0) {
			pthread_cond_wait(&condQueue, &myMutex);
		}

		clientConnfd = queue[0];

		//moving the clients forward in the queue
		for(int i = 0; i < clientCount - 1; i++) {
			queue[i] = queue[i+1];
		}
		clientCount--;

		pthread_mutex_unlock(&myMutex);

		manager(clientConnfd);
		close(clientConnfd);

	}
	return (void*)0;
} // end clientThread

//helper function for checking if command line arguments are valid numbers
int isValidNumber(char num[]) {
        for(int i = 0; num[i] != '\0' ; i++) {
                if(num[i] == '-')
                        return 1;
                if(num[i] < '0' || num[i] > '9')
                        return 1;
        }
        return 0;
} // end isValidNumber


int main(int argc, char *argv[]) {
	if(argc != 1 && argc != 3) {
		printf("ERROR: Program called incorrectly\n");
                printf("Correct Usage: <%s> <number of rows> <number of columns>\n", argv[0]);
		printf("If no numbers are given, defaults will be used\n");
		return 1;
	}

	//check if input contains valid numbers
	if(argc == 3) {
		for(int i = 1; i < argc; i++) {
                	if(isValidNumber(argv[i]) != 0) {
                	        printf("ERROR: Invalid Input for Seat Matrix\n");
                	        printf("Please make sure inputs are positive, non-zero integers\n");
                	        return -1;
                	}
        	}
		rows = atoi(argv[1]);
		cols = atoi(argv[2]);
	}

	//allocating memory for the seat layout
	seats = malloc((rows * cols) * sizeof(int));
	for(int i = 0; i < (rows * cols); i++) {
		seats[i] = 0;
	}

	int sockfd;

	struct sockaddr_in serv_addr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(PORT_NUMBER);

	bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	listen(sockfd, 10);

	//creating object to pass to listener thread
	threadInfo * x = malloc(sizeof(threadInfo));
	x->sockfd = sockfd;


	//server successfully started
	printf("Server successfully started\n");

	//printing initial seat map
	for(int i = 0; i < (rows * cols); i++) {
		printf("%d  ", seats[i]);
		if((i + 1) % cols == 0)
			printf("\n");
	}
	printf("\n");

	printf("Listening...\n");

	//creating the threads
	for(int i = 0; i < MAX_CLIENTS; i++) {
		pthread_create(&handlerThreads[i], NULL, clientThread, NULL);
	}
	pthread_create(&listener, NULL, listenerThread, (void*)x);

	//server loop
	while(seatsLeft > 0) {
		//keep the server running until all seats are sold
	} // end server loop

	//ensuring that all automatic clients have finished
	sleep(8);
	printf("There are no more seats available on the plane\n");
	printf("Shutting down Server...\n");

	free(seats);
	return 0;
} // end main
