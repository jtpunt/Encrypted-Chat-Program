// Name: Jonathan Perry
// Class: CS 344 - Operating Systems
// Project OTP
// 1. Similarly, this program will connect to otp_dec_d and will ask it to decrypt ciphertext using a passed-in ciphertext and key,
//    and otherwise performs exactly like otp_enc, and must be runnable in the same three ways. otp_dec sohuld NOT be able to connec to to 
//    otp_enc_d, even if it tries to connect on the correct port - you'll need to have the programs reject each other, as described in
//    otp_enc.

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h> 
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <netdb.h>
#include "otp_functions.h"


int main(int argc, char **argv){
	int listenSocketFD, portNumber, ciphertextFD, keyFD;					
	struct sockaddr_in serverAddress;
	struct hostent *server;		
	struct stat ciphertextStat, keyStat;
	size_t ciphertextSize = 0, keySize = 0;
	bool isServer, isClient = false;
	char buffer[BUFFER_SIZE];
	memset(buffer, '\0', BUFFER_SIZE * sizeof(char));

	if (argc != 4){ // check usage & args
		fprintf(stderr, "Usage: %s ciphertext key port\n", argv[0]);
		exit(EXIT_SUCCESS);
	}

	stat(argv[1], &ciphertextStat); // get the ciphertext file size
	ciphertextSize = ciphertextStat.st_size; // assign ciphertext file size
	stat(argv[2], &keyStat); // get the key file size
	keySize = keyStat.st_size; // assign the key file size

	if (keySize < ciphertextSize){
		fprintf(stderr, "Error: key '%s' is too short\n", argv[2]);
		exit(EXIT_FAILURE);
	}
	if ((ciphertextFD = open(argv[1], O_RDONLY)) == -1){ // open the plaintext file
		fprintf(stderr, "Error: cannot open ciphertext file '%s'\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	char ciphertext[ciphertextSize];
	memset(ciphertext, '\0', ciphertextSize * sizeof(char)); // null terminate the string

	if (read(ciphertextFD, ciphertext, ciphertextSize) < 0){  // try to read the contents of ciphertextFD
		fprintf(stderr, "Error: cannot read ciphertext file '%s'\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	ciphertext[ciphertextSize - 1] = '\0'; // ensure that the end of the string is null terminated
	if (validateInput(ciphertext) != 1){  // ensure that the ciphertext file has A-Z chars or a space
		fprintf(stderr, "otp_dec error: input file '%s' contains bad characters\n", argv[1]);
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
		fprintf(stderr, "otp_dec error: input file '%s' contains bad characters\n", argv[2]);
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
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	memcpy(&serverAddress.sin_addr, server->h_addr_list[0], server->h_length);

	if (connect(listenSocketFD, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0){ // try to connect to otp_dec_d
		fprintf(stderr, "Error: could not contact otp_dec_d on port %d\n", portNumber);
		exit(2);
	}


	sendData(listenSocketFD, &isClient, sizeof(isClient), 0, argv[0], 1);  // sends the serrver the client type to otp_dec_d - sends false
	recvData(listenSocketFD, &isServer, sizeof(isServer), 0, argv[0], 1);  // Receives the server type from otp_dec_d receives false
	// We want to make sure that we have a client/server communicating that wants to do encryption - where isClient and isServer are both false
	if (isClient != isServer || isClient != false || isServer != false){
		fprintf(stderr, "Error: could not contact otp_dec_d on port %d\n", portNumber);
		close(listenSocketFD);
		exit(2);
	}

	memset(buffer, '\0', BUFFER_SIZE * sizeof(char)); // null terminate the string
	recvData(listenSocketFD, &buffer, sizeof(buffer), 0, argv[0], 1); // receives from otp_dec_d where the connection is coming from

	if (strcmp(buffer, "otp_dec_d") != 0){  // if the connection is not coming from otp_dec_d, stop the client process
		fprintf(stderr, "Error: could not contact otp_dec_d on port %d\n", portNumber);
		close(listenSocketFD);
		exit(2);
	}
	sendData(listenSocketFD, &ciphertextSize, sizeof(ciphertextSize), 0, argv[0], 1); // send otp_dec_d the ciphertext size
	sendData(listenSocketFD, &keySize, sizeof(keySize), 0, argv[0], 1);  // send otp_dec_d the key size
	memset(buffer, '\0', BUFFER_SIZE * sizeof(char)); // null terminate the string
	recvData(listenSocketFD, &buffer, sizeof(buffer), 0, argv[0], 1); // receive from otp_dec_d a message that the key size is equal to the ciphertext size

	if (strcmp(buffer, "valid key size") != 0){ // ensure that the key size is correct by checking what otp_dec_d sent
		fprintf(stderr, "Error: key '%s' is too short\n", argv[2]);
		close(listenSocketFD);
		exit(EXIT_FAILURE);
	}

	sendData(listenSocketFD, &ciphertext, sizeof(ciphertext), 0, argv[0], 1); // send otp_dec_d our ciphertext
	sendData(listenSocketFD, &key, sizeof(key), 0, argv[0], 1); // send otp_dec_d our key

	char plaintext[ciphertextSize]; 
	memset(plaintext, '\0', ciphertextSize * sizeof(char)); // null terminate the string
	recvData(listenSocketFD, &plaintext, sizeof(plaintext), MSG_WAITALL, argv[0], 1);  // receive from otp_dec_d our plaintext (decrypted) file

	fprintf(stdout, "%s\n", plaintext); // Redirect the plaintext to stdout, so we can redirect stdout to a a file
	close(listenSocketFD); // Close the listening socket
	return 0;

}