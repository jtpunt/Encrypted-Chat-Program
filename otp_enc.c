// Name: Jonathan Perry
// Class: CS 344 - Operating Systems
// Project OTP
// 1. This program connects to otp_enc_d, and asks it to perform a one-time pad style encryption.
// 2. By itself, otp_enc doesn't do the encryption - otp_end_d does. 
// 3. When otp_receives the ciphertext back from otp_enc_d, it should output it to stdout. Thus, otp_enc can be launched in any
//   of the following methods,and should send its outut appropriately.
// 4. If otp_enc receives key or plaintext file with ANY bad characters in them, or the key file is shorter than the plaintext, 
//    then it should terminate, send appropriate error text to stderr, and set the exit value to 1.
// 5. otp_enc should NOT be able to connecT to otp_dec_d, even if it tries to connect n the correct port - you'll need to have the
//    programs reject each other. If this happens, otp_enc should report the rejection to stderr and then terminate itself.
// 6. In more detail: if otp_enc cannot connect to the otp_dec_d server, for any reason (including that it has accidentally tried
//    to connect to the otp_dec_d server), it should report this error to stderr with the attempted port, and set the exit value to 2.
//    2. Otherwise, upon successfully running and terminating, otp_enc should set the exit value to 0.
// 7. Again, any and all error text must be output to stderr(not into plaintext or ciphertext file).

// The syntax is: otp_enc plaintext key port
//                otp_enc myplaintext mykey 57171
//			      otp_enc myplaintext mykey 57171 > myciphertext
// 				  otp_enc myplaintext mykey 57171 > myciphertext &
// 	- in this syntax, plaintext is the name of a file in the current directory that contains the plaintext you wish to encrypt.
//  - similarly, key contains the encrpytion key you wish to use to encrpyt the text. 
//  - port is the port that otp_enc should attempt to connect to otp_enc_d on.

// This file is the client
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <netdb.h>
#include "otp_functions.h"

int main(int argc, char **argv){
	int listenSocketFD, portNumber, plaintextFD, keyFD;					
	struct sockaddr_in servAddr;	
	struct hostent *server;		
	struct stat plaintextStat, keyStat;
	size_t plaintextSize, keySize;
	bool isServer, isClient = true;
	char buffer[BUFFER_SIZE];
	memset(buffer, '\0', BUFFER_SIZE * sizeof(char));

	if (argc != 4){ // check usage & args
		fprintf(stderr, "Usage: %s plaintext key port\n", argv[0]);
		exit(EXIT_SUCCESS);
	}

	stat(argv[1], &plaintextStat); // get the plaintext file size
	plaintextSize = plaintextStat.st_size;  // assign plaintext file size
	stat(argv[2], &keyStat); // get the key file size
	keySize = keyStat.st_size; // assign key file size

	if (keySize < plaintextSize){	
		fprintf(stderr, "Error: key '%s' is too short\n", argv[2]);
		exit(EXIT_FAILURE);
	}

	if ((plaintextFD = open(argv[1], O_RDONLY)) == -1){ // open the plaintext file
		fprintf(stderr, "Error: cannot open plaintext file '%s'\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	char plaintext[plaintextSize];
	memset(plaintext, '\0', plaintextSize * sizeof(char)); // null terminate the string

	if (read(plaintextFD, plaintext, plaintextSize) < 0){ // try to read the contents of plaintextFD
		fprintf(stderr, "Error: cannot read plaintext file '%s'\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	plaintext[plaintextSize - 1] = '\0'; // ensure that the end of the string is null terminated
	if (validateInput(plaintext) != 1){ // ensure that the plaintext file has A-Z chars or a space
		fprintf(stderr, "otp_enc error: input file '%s' contains bad characters\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	if ((keyFD = open(argv[2], O_RDONLY)) == -1){ // open the key text file
		fprintf(stderr, "Error: cannot open key file '%s'\n", argv[2]);
		exit(EXIT_FAILURE);
	}

	char key[keySize];
	memset(key, '\0', keySize * sizeof(char)); // null terminate the string

	if (read(keyFD, key, keySize) < 0){ // try to read the contents of keyFD
		fprintf(stderr, "Error: cannot read key file '%s'\n", argv[2]);
		exit(EXIT_FAILURE);
	}
	key[keySize - 1] = '\0'; // ensure that the end of the string is null terminated
	if (validateInput(key) != 1){ // ensure that the key file input has A-Z chars or a space
		fprintf(stderr, "otp_enc Error: input file '%s' contains bad characters\n", argv[2]);
		exit(EXIT_FAILURE);
	}
	// Set up the socket
	if ((listenSocketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		fprintf(stderr, "Error: %s socket creation failed\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	server = gethostbyname("localhost");
	if (server == NULL){
		fprintf(stderr, "Error: %s could not get host by name\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	servAddr.sin_family = AF_INET; // Create a network-capable socket
	servAddr.sin_port = htons(portNumber); // Store the port number
	memcpy(&servAddr.sin_addr, server->h_addr_list[0], server->h_length);

	if (connect(listenSocketFD, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0){ // try to connect to otp_enc_d
		fprintf(stderr, "Error: could not contact otp_enc_d on port %d\n", portNumber);
		exit(2);
	}

	sendData(listenSocketFD, &isClient, sizeof(isClient), 0, argv[0], 1); // sends the serrver the client type to otp_enc_d - sends true
	recvData(listenSocketFD, &isServer, sizeof(isServer), 0, argv[0], 1); // Receives the server type from otp_enc_d receives true
	// We want to make sure that we have a client/server communicating that wants to do encryption - where isClient and isServer are both true
	if (isClient != isServer || isClient != true || isServer != true){
		fprintf(stderr, "Error: could not contact otp_enc_d on port %d\n", portNumber);
		close(listenSocketFD);
		exit(2);
	}
	
	memset(buffer, '\0', BUFFER_SIZE * sizeof(char)); // null terminate the string
	recvData(listenSocketFD, buffer, sizeof(buffer), 0, argv[0], 1); // receives from otp_enc_d where the connection is coming from

	if (strcmp(buffer, "otp_enc_d") != 0){ // if the connection is not coming from otp_enc_d, stop the client process
		fprintf(stderr, "Error: could not contact otp_enc_d on port %d\n", portNumber);
		close(listenSocketFD);
		exit(2);
	}

	sendData(listenSocketFD, &plaintextSize, sizeof(plaintextSize), 0, argv[0], 1); // send otp_enc_d the plaintext size
	sendData(listenSocketFD, &keySize, sizeof(keySize), 0, argv[0], 1); // send otp_enc_d the key size
	memset(buffer, '\0', BUFFER_SIZE * sizeof(char)); // null terminate the string
	recvData(listenSocketFD, &buffer, sizeof(buffer), 0, argv[0], 1); // receive from otp_enc_d a message that the key size is equal to the plaintext size

	if (strcmp(buffer, "valid key size") != 0){ // ensure that the key size is correct by checking what otp_enc_d sent
		fprintf(stderr, "Error: key '%s' is too short\n", argv[2]);
		close(listenSocketFD);
		exit(EXIT_FAILURE);
	}
	sendData(listenSocketFD, &plaintext, sizeof(plaintext), 0, argv[0], 1); // send otp_enc_d our plaintext
	sendData(listenSocketFD, &key, sizeof(key), 0, argv[0], 1); // send otp_enc_d our key

	char ciphertext[plaintextSize];
	memset(ciphertext, '\0', plaintextSize * sizeof(char)); // null terminate the string
	recvData(listenSocketFD, ciphertext, sizeof(ciphertext), MSG_WAITALL, argv[0], 1); // receive from otp_enc_d our ciphertext (encrypted) file

	fprintf(stdout, "%s\n", ciphertext); // Redirect the ciphertext to stdout, so we can redirect stdout to a a file
	close(listenSocketFD); // Close the listening socket

	return 0;
}