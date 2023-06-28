#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <time.h>

#define BUFFER_SIZE 1024

//default values if ini file is not given
char ip[16] = "127.0.0.1";
int port = 5436;
int timeout = 5;

//Function for if the client is run in manual mode
void manual(int sockfd, int userPid, int seatsLeft, int rows, int cols) {
	int r;
	int c;
	while(seatsLeft != 0) {
		char confirmation[BUFFER_SIZE];

		//asking the user which seat they would like to purchase
		printf("Which row would you like to sit in?\n");
		scanf("%d", &r);
		if(r < 0 || r >= rows) {
			printf("ERROR: Invalid Request\n");
			printf("Row input must be between 0 and %d\n\n", (rows-1));
			continue;
		}

		printf("Which seat would like in that row?\n");
		scanf("%d", &c);
		if(c < 0 || c >= cols) {
			printf("ERROR: Invalid Request\n");
			printf("The seat in the row must between 0 and %d\n\n", (cols-1));
			continue;
		}

		//sending seat choice to server
		int seatChoice[2] = {r, c};
		write(sockfd, seatChoice, sizeof(seatChoice));

		//receiving whether or not seatChoice was taken or accepted
		read(sockfd, confirmation, sizeof(confirmation));
		printf("%s\n", confirmation);

		//sending acknowledgement to server
		int handShake = 1;
		write(sockfd, &handShake, sizeof(handShake));

		int availability = 0;
		read(sockfd, &availability, sizeof(availability));
		seatsLeft = availability;
	}
	printf("There are no more seats available for purchase.\n");
	printf("Shutting down ticket window\n");
	return;
}

//Function for if the client is run in automatic mode
void automatic(int sockfd, int userPid, int seatsLeft, int rows, int cols) {
	srand(time(0));

	while(seatsLeft != 0) {
		char confirmation[BUFFER_SIZE];
		int randRow = rand() % rows;
		int randCol = rand() % cols;;

		//sending seat choice to server
		int seatChoice[2] = {randRow, randCol};
		write(sockfd, seatChoice, sizeof(seatChoice));

		//receiving whether or not seatChoice was taken or accepted
		read(sockfd, confirmation, sizeof(confirmation));
		printf("%s\n", confirmation);

		//sending acknowledgement to server
		int handShake = 1;
		write(sockfd, &handShake, sizeof(handShake));

		//updating how many seats are left
		int availability = 0;
		read(sockfd, &availability, sizeof(availability));
		seatsLeft = availability;

		if(seatsLeft == 0)
			break;

		//sleep a random time before trying to purchase another seat
		int sleepSeed = rand() % 3;
		if(sleepSeed == 0)
			sleep(3);
		else if(sleepSeed == 1)
			sleep(5);
		else
			sleep(7);
	}
	printf("There are no more seats available for purchase.\n");
	printf("Shutting down ticket window\n");
}

int main(int argc, char *argv[]) {
	if(argc != 2 && argc != 3) {
		printf("ERROR: Program called incorrectly\n");
		printf("Correct Usage: <%s> <MODE> <INIFILENAME>\n", argv[0]);
		printf("MODE being manual or automatic\n");
		printf("If INIFILENAME is not provided, default values will be used\n");
		return 1;
	}

	//updating client values with those in the ini file
	if(argc == 3) {
		FILE* fp = fopen(argv[2], "r");
		if(fp == NULL) {
			printf("ERROR: Unable to open ini file\n");
			return 1;
		}
		char fileBuff[50];

		//getting the ip
		fgets(fileBuff, sizeof(fileBuff), fp);
		fgets(fileBuff, sizeof(fileBuff), fp);
		char * ipTok = strtok(fileBuff, "= \n");
		ipTok = strtok(NULL, ".= \n");
		int ip1 = atoi(ipTok);
		ipTok = strtok(NULL, ".= \n");
		int ip2 = atoi(ipTok);
		ipTok = strtok(NULL, ".= \n");
		int ip3 = atoi(ipTok);
		ipTok = strtok(NULL, ".= \n");
		int ip4 = atoi(ipTok);
		snprintf(ip, sizeof(ip), "%d.%d.%d.%d", ip1, ip2, ip3, ip4);

		//getting the port
		fgets(fileBuff, sizeof(fileBuff), fp);
		char * portTok = strtok(fileBuff, "= \n");
		portTok = strtok(NULL, "= \n");
		port = atoi(portTok);

		//getting the timeout
		fgets(fileBuff, sizeof(fileBuff), fp);
		char * timeoutTok = strtok(fileBuff, "= \n");
		timeoutTok = strtok(NULL, "= \n");
		timeout = atoi(timeoutTok);

		fclose(fp);
	}

	//initializing variables
	pid_t userPid = getpid();
	int sockfd = 0;
	int n = 0;
	char recvBuff[BUFFER_SIZE];
	struct sockaddr_in serv_addr;
	int mode;

	//setting the mode of the client to manual or automatic
	if(strcmp(argv[1], "manual") == 0) {
		mode = 1;
	}
	else if(strcmp(argv[1], "automatic") == 0) {
		mode = 0;
	}
	else {
		printf("ERROR: inputted mode was not manual or automatic\n");
		return 1;
	}
	if(mode == 1)
		printf("Running client in manual mode\n");
	else
		printf("Running client in automatic mode\n");

	//creating an array to store seat information sent from the server
	//seatStats[0] = seatsLeft
	//seatStats[1] = rows
	//seatStats[2] = cols
	int seatStats[3] = {0};

	memset(recvBuff, 0, sizeof(recvBuff));
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\n Error : Could not create socket \n");
		return 1;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);

	if(inet_pton(AF_INET, ip, &serv_addr.sin_addr)<=0)
	{
		printf("\n inet_pton error occured\n");
		return 1;
	}

	//attempting to connect to the server
	for(int i = 0; i < timeout; i++) {
		if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
			printf("\nERROR: Connection Failed\n");
			if(i == (timeout-1)) {
				printf("%d Attempts to connect to the server failed\n", timeout);
				printf("Please make sure that the server is online\n\n");
				return 1;
			}
			printf("Attempting to retry connection in 4 seconds...\n\n");
			sleep(4);
		}
		else {
			printf("Successfully connected to the server\n");
			break;
		}
	}

	printf("Client PID is [%d]\n", userPid);

	//sending userPID to server
	write(sockfd, &userPid, sizeof(userPid));

	read(sockfd, recvBuff, sizeof(recvBuff));
	printf("%s\n", recvBuff);

	//sending acknowledgement to server that the message was received
	int handShake = 1;
	write(sockfd, &handShake, sizeof(handShake));

	n = read(sockfd, seatStats, sizeof(seatStats));
	if(n < 0) {
		printf("ERROR: could not read seatStats\n");
	}

	//running the client in either manual or automatic mode
	if(mode == 1)
		manual(sockfd, userPid, seatStats[0], seatStats[1], seatStats[2]);
	else
		automatic(sockfd, userPid, seatStats[0], seatStats[1], seatStats[2]);

	close(sockfd);
	return 0;
}
