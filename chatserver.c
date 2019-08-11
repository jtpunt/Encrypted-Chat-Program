

// Name: Jonathan Perry
#include <errno.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <stdbool.h> 
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/wait.h>
#include "chat_common.h"
#define MAX_NAME_LENGTH 10
#define KEY_SIZE 256
struct sigaction SIGINT_action;

void catchSIGINT(int signum){
	char* message = "Caught SIGINT, exiting";
	write(STDOUT_FILENO, message, 38);
	// raise(SIGUSR2);
	// sleep(5);
	exit(0);
}
int main(int argc, char **argv){
	int listenSocketFD, establishedConnectionFD, portNumber;	
	char client_name[MAX_NAME_LENGTH], 
		 server_name[MAX_NAME_LENGTH], 
		 client_msg[BUFFER_SIZE], 
		 server_input[BUFFER_SIZE],
		 server_msg[BUFFER_SIZE],
		 enc_client_msg[BUFFER_SIZE], 
		 enc_server_msg[BUFFER_SIZE],
		 key[KEY_SIZE];	
	socklen_t sizeofClientInfo;		
	struct sockaddr_in serverAddress, clientAddress;
	pid_t spawnPid;		
	bool isClient, isServer = false;
	size_t ciphertextSize, keySize;

	
	SIGINT_action.sa_handler = catchSIGINT;
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = 0;
	sigaction(SIGINT, &SIGINT_action, NULL);

	memset(client_name, '\0', MAX_NAME_LENGTH * sizeof(char)); // null terminate the string
	memset(server_name, '\0', MAX_NAME_LENGTH * sizeof(char)); 
	memset(client_msg, '\0', BUFFER_SIZE * sizeof(char));
	memset(server_input, '\0', BUFFER_SIZE * sizeof(char));
	memset(server_msg, '\0', BUFFER_SIZE * sizeof(char));
	memset(key, '\0', KEY_SIZE * sizeof(char));
	memset(enc_client_msg, '\0', BUFFER_SIZE * sizeof(char));
	memset(enc_server_msg, '\0', BUFFER_SIZE * sizeof(char));
	// Check usage & args
	if (argc != 2){
		fprintf(stderr, "Usage: %s listening_port\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress));  // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY); // Any address is allowed for connection to this process

	// Set up the socket
	if ((listenSocketFD = socket(AF_INET, SOCK_STREAM, 0)) == -1){ // Create the socket
		fprintf(stderr, "ERROR opening socket\n");
		exit(EXIT_FAILURE);
	}
	setsockopt(listenSocketFD, SOL_SOCKET, SO_REUSEADDR, (int*)1, sizeof(int)); // set UNIX up so that we can reuse ports
	// Enable the socket to begin listening by connecting socket to port
	if (bind(listenSocketFD, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0){
		fprintf(stderr, "ERROR on binding\n");
		exit(EXIT_FAILURE);
	}
	// Flip the socket on - it can now receive up to 5 connections
	if (listen(listenSocketFD, 5) < 0){
		fprintf(stderr, "ERROR on listening\n");
		exit(EXIT_FAILURE);
	}
	printf("Enter in the server's chat name: ");
	fgets(server_name, MAX_NAME_LENGTH, stdin);
	server_name[strlen(server_name)-1] = '\0';
	// Keep looping to accept incoming connections from clients
	while (1){ // fork and let the child process handle the rest
		sizeofClientInfo = sizeof(clientAddress);
		// Accept a connection, blocking if one is not available until one connects
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *) &clientAddress, &sizeofClientInfo); // Accept
		if (establishedConnectionFD < 0){
			fprintf(stderr, "ERROR on accept\n");
		}
		spawnPid = fork(); 
		if (spawnPid == 0){	// child process
			recvData(establishedConnectionFD, &key, sizeof(key), 0, argv[0], -1); // receives the key from otp_dec
			printf("key%s\n", key);
			while(1){
				do{
					recvData(establishedConnectionFD, &enc_client_msg, sizeof(enc_client_msg), MSG_WAITALL, argv[0], -1); // receives the ciphertext from otp_dec
				}while(enc_client_msg[0] == '\0'); // keep looping until we receive a message sent by the chat server

				decrypt(enc_client_msg, key, client_msg); // convert the ciphertext to plaintext
				printf("\r%s\n", client_msg);
				printf("%s> ", server_name);
				fgets(server_input, BUFFER_SIZE, stdin);
				server_input[strlen(server_input) - 1] = '\0';
				strcpy(server_msg, server_name); // 'client_name'
				strcat(server_msg, "> "); // 'cleint_name> '
				strcat(server_msg, server_input); // 'client_name> client_input'
				encrypt(server_msg, key, enc_server_msg);
				sendData(establishedConnectionFD, &enc_server_msg, sizeof(enc_server_msg), 0, argv[0], -1); // sends the plaintext back to otp_dec
				memset(server_input, '\0', BUFFER_SIZE * sizeof(char)); // null terminate the string
				memset(client_msg, '\0', BUFFER_SIZE * sizeof(char));
				memset(server_msg, '\0', BUFFER_SIZE * sizeof(char));
				memset(enc_client_msg, '\0', BUFFER_SIZE * sizeof(char));
				memset(enc_server_msg, '\0', BUFFER_SIZE * sizeof(char));
			}
			close(establishedConnectionFD); // Close the existing socket which is connected to the client
			establishedConnectionFD = -1;
			exit(EXIT_SUCCESS);
		}else if (spawnPid == -1){ // did the fork fail?
			close(establishedConnectionFD); // Close the existing socket which is connected to the client
			establishedConnectionFD = -1;
			fprintf(stderr, "ERROR on fork call\n");
		}else{ // parent process
			close(establishedConnectionFD); // Close the existing socket which is connected to the client
			establishedConnectionFD = -1;
			wait(NULL);
		}
	}

	close(listenSocketFD); // Close the listening socket

	return 0;
}
