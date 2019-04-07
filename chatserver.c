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
int main(int argc, char **argv){
	int listenSocketFD, establishedConnectionFD, portNumber, activity, max_sd;	
	char server_name[MAX_NAME_LENGTH], server_input[BUFFER_SIZE], server_msg[BUFFER_SIZE];	
	socklen_t sizeofClientInfo;		
	struct sockaddr_in serverAddress, clientAddress;
	pid_t spawnPid, spawnPid1;		
	bool isClient, isServer = false;
	size_t ciphertextSize, keySize;
	fd_set readFDs;
	// Watch stdin (FD 0) to see when it has input
	FD_ZERO(&readFDs); // Zero out the set of possible read file descriptors
	FD_SET(0, &readFDs); // Mark only FD 0 as the one we want to pay attention to
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
		FD_SET(establishedConnectionFD, &readFDs); // Mark only FD 0 as the one we want to pay attention to
		max_sd = establishedConnectionFD;   
		spawnPid = fork(); 
		if (spawnPid == 0){	// child process
			char plaintext[BUFFER_SIZE];
			memset(plaintext, '\0', BUFFER_SIZE * sizeof(char)); // null terminate the string
			char key[BUFFER_SIZE];
			memset(key, '\0', BUFFER_SIZE * sizeof(char));
			recvData(establishedConnectionFD, &key, sizeof(key), 0, argv[0], -1); // receives the key from otp_dec
			printf("key%s\n", key);
			char ciphertext[BUFFER_SIZE];
			if(spawnPid1 = fork() == 0){ // fork from this process - child process
				while(1){
					activity = select( max_sd + 1 , &readFDs , NULL , NULL , NULL); 
					memset(ciphertext, '\0', BUFFER_SIZE * sizeof(char)); // null terminate the string  
					if ((activity < 0) && (errno!=EINTR)){   
		    			printf("select error");   
		    		}   
					if(FD_ISSET(establishedConnectionFD , &readFDs)){ 
						recvData(establishedConnectionFD, &ciphertext, sizeof(ciphertext), MSG_WAITALL, argv[0], -1); // receives the ciphertext from otp_dec
						decrypt(ciphertext, key, plaintext); // convert the ciphertext to plaintext
						printf("\r%s\n", plaintext);
					}
					// printf("new fork..\n");
					// fflush(stdout);
					// memset(ciphertext, '\0', BUFFER_SIZE * sizeof(char)); // null terminate the string
					// do{
					// 	recvData(establishedConnectionFD, &ciphertext, sizeof(ciphertext), MSG_WAITALL, argv[0], -1); // receives the ciphertext from otp_dec
					// }while(ciphertext[0] == '\0'); // keep looping until we receive a message sent by the chat server
					// decrypt(ciphertext, key, plaintext); // convert the ciphertext to plaintext
					// printf("\r%s\n", plaintext);
				}
			}else if(spawnPid1 == -1){ // did the fork fail?
				perror("fork error");
				exit(EXIT_FAILURE);
			}
			else{ // parent process
				while(1){
					printf("%s> ", server_name);
					fgets(server_input, BUFFER_SIZE, stdin);
					server_input[strlen(server_input) - 1] = '\0';
					strcpy(server_msg, server_name); // 'client_name'
					strcat(server_msg, "> "); // 'cleint_name> '
					strcat(server_msg, server_input); // 'client_name> client_input'
					sendData(establishedConnectionFD, &server_msg, sizeof(server_msg), 0, argv[0], -1); // sends the plaintext back to otp_dec
				}
			}
			// }
			// while(1){
			// 	char ciphertext[BUFFER_SIZE];
			// 	memset(ciphertext, '\0', BUFFER_SIZE * sizeof(char)); // null terminate the string
			// 	do{
			// 		recvData(establishedConnectionFD, &ciphertext, sizeof(ciphertext), MSG_WAITALL, argv[0], -1); // receives the ciphertext from otp_dec
			// 	}while(ciphertext[0] == '\0'); // keep looping until we receive a message sent by the chat server
			// 	printf("ciphertext: %s\n", ciphertext);
	
			// 	decrypt(ciphertext, key, plaintext); // convert the ciphertext to plaintext
			// 	printf("plaintext: %s\n", plaintext);
			// 	memset(ciphertext, '\0', BUFFER_SIZE * sizeof(char)); // null terminate the string
			// }
			// sendData(establishedConnectionFD, &plaintext, sizeof(plaintext), 0, argv[0], -1); // sends the plaintext back to otp_dec

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
