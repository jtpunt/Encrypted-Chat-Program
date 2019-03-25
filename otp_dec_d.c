// Name: Jonathan Perry
// Class: CS 344 - Operating Systems
// Project OTP
// 1. This program performs exactly like otp_enc_d, in syntax and usage. In this case, however, otp_dec_d will decrypt
//    ciphertext it is given, using the passed-in ciphertext and key. Thus, it returns plaintext again to otp_dec.
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
#include "otp_functions.h"


int main(int argc, char **argv){
	int listenSocketFD, establishedConnectionFD, portNumber;				
	socklen_t sizeofClientInfo;		
	struct sockaddr_in serverAddress, clientAddress;
	pid_t spawnPid;		
	bool isClient, isServer = false;
	size_t ciphertextSize, keySize;

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
			recvData(establishedConnectionFD, &isClient, sizeof(isClient), 0, argv[0], -1); // receives the client type from otp_dec - receives false if it's from otp_dec
			sendData(establishedConnectionFD, &isServer, sizeof(isServer), 0, argv[0], -1); // sends the client the server type to otp_dec - sends false
			// We want to make sure that we have a client/server communicating - when both are false, we can start encryption
			if (isClient == isServer && isClient == false && isServer == false){ 
				sendData(establishedConnectionFD, "otp_dec_d", 9, 0, argv[0], -1); // ensure that the connection to otp_dec_d is coming from otp_dec
				// if the connection is not coming from otp_dec, then otp_dec will close it's listening socket and exit
			}else{ // we do not have a client/server communicating that wants to do decryption
				continue; // skip the statements inside body of this loop, and start at the beginning of the loop for the next iteration
			}
			recvData(establishedConnectionFD, &ciphertextSize, sizeof(ciphertextSize), 0, argv[0], -1); // receives the ciphertext size from otp_dec
			recvData(establishedConnectionFD, &keySize, sizeof(keySize), 0, argv[0], -1); // receives the key size from otp_enc

			// 8. Note that the key passed in must be at least as big as the ciphertext.
			if (keySize < ciphertextSize){
				continue;
			}else{
				sendData(establishedConnectionFD, "valid key size", 14, 0, argv[0], -1);
			}
			
			char ciphertext[ciphertextSize];
			memset(ciphertext, '\0', ciphertextSize * sizeof(char)); // null terminate the string
			recvData(establishedConnectionFD, &ciphertext, sizeof(ciphertext), MSG_WAITALL, argv[0], -1); // receives the ciphertext from otp_dec

			char key[keySize];
			memset(key, '\0', keySize * sizeof(char));
			recvData(establishedConnectionFD, &key, sizeof(key), MSG_WAITALL, argv[0], -1); // receives the key from otp_dec

			char plaintext[ciphertextSize];
			memset(plaintext, '\0', ciphertextSize * sizeof(char)); // null terminate the string
			decrypt(ciphertext, key, plaintext); // convert the ciphertext to plaintext
			sendData(establishedConnectionFD, &plaintext, sizeof(plaintext), 0, argv[0], -1); // sends the plaintext back to otp_dec

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
