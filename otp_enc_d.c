// Name: Jonathan Perry
// Class: CS 344 - Operating Systems
// Project OTP
// 1. This program will run in the background as a daemon. Upon execution,  otp_enc_d must output an error if it cannot be run due to a
//    network error, such as the ports being unavailable.
// 2. Its function is to perform the actual encoding, as descripted above in the wikipedia quote.
// 3. The program will listen on a particular port/socket, assigned when it is first ran.
// 4. When a connection is made, otp_enc_d must call accept() to generate the socket used for actual communication,
//    and then use a seperate process to handle the rest of the transaction, which will occur on the newly accepted socket.
// 5. This child process of otp_enc_d must first check to make sure it is communicating with otp_enc.
// 6. After reviewing that the connection to otp_enc_d is coming from otp_enc, this child receives from otp_enc plaintext
//    and a key via the communication socket (not the original listen socket).
// 7. The otp_enc_d child will then write back the ciphertext to the otp_enc process that it is connected to via the same 
//    communication socket. 
// 8. Note that the key passed in must be at least as big as the plaintext.
// 9. Your version of otp_enc_d must support up to 5 concurrent socket connections running at the same time; this is different
//    than the # of processes that could queue up on your listening socket (which is specified in the 2nd parameter of the listen() call)
// 10. Again, only in the child process will the actual encryption take place, and the ciphertext be written back: the original daemon
//     process continues listening for new connections, not encrypting data.
// 11. In terms of creating that child process as described above, you may either create with fork() a new process every time a connection
//     is made, or set up a pool of five processes at the beinning of the program, befefore connections are allowed, to handle your encryption tasks.
// 12. As above, your system must be able to do five seperate encryptions at once, using either method you choose.
// 13. In all error situations, this program must output errors to stderr as appropritae (see grading script for details), but should not crash or
//     otherwise exit, unless the errors happens when the program is starting up (i.e. are apart of the networking start up protocols like bind()).
// 14. If given bad input, once running, otp_enc_d should recognize the bad input, report an error to stderr, and continue to run.
// 15. Generally speaking, though, this daemon shouldn't receive bad input, since that should be discovered and handled in the client first.
// 16. All error text must be output to stderr.
// 17. This program, and the other 3 network programs, should use "localhost" as the target IP address/host.
// 18. This make them use the actual computer they all share as the target for the networking connections.

// Use this syntax for otp_enc_d: otp_enc_d listening_port
// 	- listening_port is the port that otp_enc_d should lsiten on. You will always start otp_enc_d in the background, as follows (the port 57171
//    is just a example; your should be able to use any port): otp_enc_d 57171 &
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h> 
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "otp_functions.h"

int main(int argc, char **argv){
	int listenSocketFD, establishedConnectionFD, portNumber;			
	socklen_t sizeofClientInfo;		
	struct sockaddr_in serverAddress, clientAddress;	
	pid_t spawnPid;	
	bool isClient, isServer = true;	
	size_t plaintextSize, keySize;

	if (argc != 2){ // Check usage & args
		fprintf(stderr, "Usage: %s listening_port\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress));  // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);  // Any address is allowed for connection to this process
	// 	1. This program will run in the background as a daemon. Upon execution,  otp_enc_d must output an error if it cannot be run due to a
	// 	   network error, such as the ports being unavailable.
	// Set up the socket
	if ((listenSocketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0){ // Create the socket
		fprintf(stderr, "ERROR opening socket\n");
		exit(EXIT_FAILURE);
	}
	setsockopt(listenSocketFD, SOL_SOCKET, SO_REUSEADDR, (int*)1, sizeof(int));
	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0){  // Connect socket to port
		fprintf(stderr, "ERROR on binding\n");
		exit(EXIT_FAILURE);
	}
	// Flip the socket on - it can now receive up to 5 connections
	if (listen(listenSocketFD, 5) < 0){
		fprintf(stderr, "ERROR on listening\n");
		exit(EXIT_FAILURE);
	}
	// Keep looping to accept incoming connections from clients
	while (1){
		sizeofClientInfo = sizeof(clientAddress); 
		// 4. When a connection is made, otp_enc_d must call accept() to generate the socket used for actual communication,
 		//    and then use a seperate process to handle the rest of the transaction, which will occur on the newly accepted socket.
 		// Accept a connection, blocking if one is not available until one connects
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *) &clientAddress, &sizeofClientInfo); // Accept
		if (establishedConnectionFD < 0){
			fprintf(stderr, "ERROR on accept\n");
		}

		spawnPid = fork(); // fork and let the child process handle the rest
		if (spawnPid == 0){	 // child process?
			recvData(establishedConnectionFD, &isClient, sizeof(isClient), 0, argv[0], -1); // receives the client type from otp_enc- receives true if it's from otp_enc
			sendData(establishedConnectionFD, &isServer, sizeof(isServer), 0, argv[0], -1); // sends the client the server type to otp_enc - sends true
			// We want to make sure that we have a client/server communicating - when both are true, we can start encryption
			if (isClient == isServer && isClient == true && isServer == true){ 
				sendData(establishedConnectionFD,"otp_enc_d", 9, 0, argv[0], -1); // ensure that the connection to otp_enc_d is coming from otp_enc
				// if the connection is not coming from otp_enc, then otp_enc will close it's listening socket and exit
			}else{ // we do not have a client/server communicating that wants to do encryption 
				continue; //  skip the statements inside body of this loop, and start at the beginning of the loop for the next iteration
			}
			// 6 After reviewing that the connection to otp_enc_d is coming from otp_enc, this child receives from otp_enc plaintext
			//    and a key via the communication socket (not the original listen socket).
			recvData(establishedConnectionFD, &plaintextSize, sizeof(plaintextSize), 0, argv[0], -1); // receives the plaintext size from otp_enc
			recvData(establishedConnectionFD, &keySize, sizeof(keySize), 0, argv[0], -1); // receives the key size from otp_enc
			// 8. Note that the key passed in must be at least as big as the plaintext.
			if (keySize < plaintextSize){
				continue;
			}else{
				sendData(establishedConnectionFD, "valid key size", 14, 0, argv[0], -1);
			}

			char plaintext[plaintextSize];
			memset(plaintext, '\0', plaintextSize * sizeof(char)); // null terminate the string
			recvData(establishedConnectionFD, plaintext, sizeof(plaintext), MSG_WAITALL, argv[0], -1); // receives the plaintext from otp_enc

			char key[keySize];
			memset(key, '\0', keySize * sizeof(char)); // null terminate the string
			recvData(establishedConnectionFD, key, sizeof(key), MSG_WAITALL, argv[0], -1); // receives the key from otp_enc

			char ciphertext[plaintextSize];
			memset(ciphertext, '\0', plaintextSize * sizeof(char)); // null terminate the string from otp_enc
			encrypt(plaintext, key, ciphertext); // converts the plaintext to ciphertext
			// 7. The otp_enc_d child will then write back the ciphertext to the otp_enc process that it is connected to via the same 
 			//    communication socket.
 			sendData(establishedConnectionFD, ciphertext, sizeof(ciphertext), 0, argv[0], -1); // sends the ciphertext back to otp_enc

			close(establishedConnectionFD);  // Close the existing socket which is connected to the client
			establishedConnectionFD = -1;
			exit(EXIT_SUCCESS);	
		}else if (spawnPid == -1){ // fork failed?
			close(establishedConnectionFD); // Close the listening socket
			establishedConnectionFD = -1;
			fprintf(stderr, "ERROR on fork call\n");
		}else{ // parent?
			close(establishedConnectionFD); // Close the existing socket which is connected to the client
			establishedConnectionFD = -1;
			wait(NULL); // wait for child processes
		}
	}
	close(listenSocketFD); // Close the listening socket

	return 0;
}