// Name: Jonathan Perry
// Date: 7/28/2018
// Class: CS 372
// Project: Project 1
#include <assert.h>
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
#define KEY_SIZE 256
#define BUFFER_SIZE 500
#define MAX_NAME_LENGTH 10
#define MAX_CHARS 27

// Function Prototypes
int createSocket(char *);
struct sockaddr_in * createSocketType(int, struct hostent*);
void connectSocket(int listenSocketFD, struct sockaddr_in *servAddr, int portNumber);
void sendData(int socket, void *buffer, size_t length, int flags, char* args, int exitValue);
void recvData(int socket, void *buffer, size_t length, int flags, char* args, int exitValue);

// This function creates a socket file descriptor and returns it. 
// If any errors occur while creating the socket file descriptor, an error message and the
// program name sent in to this function as an argument is printed to the screen.
int createSocket(char * cmd){
	int listenSocketFD;
	if ((listenSocketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		fprintf(stderr, "Error: %s socket creation failed\n", cmd);
		exit(EXIT_FAILURE);
	}
	return listenSocketFD;
}
// This function creates a pointer to socket variable, where it sets the address family to recognize IPv4 addresses, 
// sets the port number specified by the first argument, and sets the address specifed by the second argument.
struct sockaddr_in * createSocketType(int portNumber, struct hostent *server){
	struct sockaddr_in *servAddr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
	assert(servAddr != 0);
	servAddr->sin_family = AF_INET; // Create a network-capable socket for IPv4
	servAddr->sin_port = htons(portNumber); // Store the port number
	memcpy(&servAddr->sin_addr, server->h_addr_list[0], server->h_length);
	return servAddr;
}
// This function connects the client to the server by using the socket file descripter sent in as argument 1,
// the socket variable sent in from argument 2 that contains information about the server we want to connect to.
// If there's an error connection, an error message specifying the port number we attempted to connect to is printed 
// to the console.
void connectSocket(int listenSocketFD, struct sockaddr_in *servAddr, int portNumber){
	printf("in connectSocket..\n");
	if (connect(listenSocketFD, (struct sockaddr *) servAddr, sizeof(*servAddr)) < 0){ // try to connect to otp_enc_d
		fprintf(stderr, "Error: could not contact server on port %d\n", portNumber);
		exit(2);
	}
}

// This function is used to send data between the client and server. If the exitValue is not -1,
// then the function will call exit with the exitValue argument passed in.
void sendData(int socket, void *buffer, size_t length, int flags, char* args, int exitValue){
	if((send(socket, buffer, length, flags)) == 0){
		fprintf(stderr, "Error: File %s Failed Sending Data\n", args);
		if(exitValue != -1){
			exit(exitValue);
		}
	}
}

// This function is used to receive data between the client and server. If the exitValue is not -1,
// then the function will call exit with the exitValue argument passed in.
void recvData(int socket, void *buffer, size_t length, int flags, char* args, int exitValue){
	if ((recv(socket, buffer, length, flags)) == 0){
		fprintf(stderr, "Error: File %s Failed Receiving Data\n", args);
		if(exitValue != -1){
			exit(exitValue);
		}
	}
}
// This function receives a character literal and returns it nth position in the alphabaet. 
// etc, 'A' = 0, 'B' = 1, ' ' (a space) = 27 and so on.
int getNumRepr(char letter){
	int pos;
	if (letter >= 'A' && letter <= 'Z'){
  		pos = letter - 'A';
	}
 	else{
 		pos = 26;
 	}
 	return pos;
}
// This is a function helper to assist with modulo arithmetic operations.
int modulo(int a, int b){
    int r = a % b;
    return r < 0 ? r + b : r;
}
// Position represents the nth character in the alphabet. etc, 'A' = 0, 'B' = 1, ' ' (a space) = 27 and so on.
// This function receives an ASCII character code and returns its character literal.
char getCharRepr(int pos){
	if(pos >= 0 && pos < MAX_CHARS - 1){ // all 26 characters of the alphabet
		return pos + 65; // converts into ASCII code and returns the character literal of that ASCII code
	}
	else{ // pos is equal to 27, which is a space character
		return ' ';
	}
}
// This function sends random characters in the alphabet (including a space character) plus a newline character to stdout.
void fillKeyTable(int length, char* key){
	int idx, i;
	for (i = 0; i < length; i++){
		key[i] = getCharRepr(0 + rand() % MAX_CHARS);
		// strcpy(&key[i], c);
		// fprintf(stdout, "%c", getCharRepr(0 + rand() % MAX_CHARS)); // prints a random alphabetical character or space to stdout
	}
	key[KEY_SIZE] = '\n';
	// strcpy(key[KEY_SIZE], "\n");
	// fprintf(stdout, "\n"); // add a newline character to the end of the key
	printf("Done\n");
}
void encrypt(char* plaintext, char* key, char* ciphertext){
	char characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
	int i;
	for(i = 0; i < strlen(plaintext); i++){
		if(plaintext[i] == '\0'){
			ciphertext[i] = plaintext[i];
		}else{
			int plaintextCharPos = getNumRepr(plaintext[i]); // get the position of current plaintext letter in the alphabet. ETC 'A' = position 0, 'B' = position 1
			int keyCharPos = getNumRepr(key[i]); // get the position of the encrypted letter in the alphabet, same as above
			int ciphertextCharPos = modulo(plaintextCharPos + keyCharPos, MAX_CHARS); 
			ciphertext[i] = getCharRepr(ciphertextCharPos); // assign the encrypted char valueps
		}
	}
}
int main(int argc, char **argv){
	char client_name[MAX_NAME_LENGTH], server_name[MAX_NAME_LENGTH], client_input[BUFFER_SIZE], client_msg[BUFFER_SIZE], server_msg[BUFFER_SIZE];
	char enc_client_msg[BUFFER_SIZE], key[KEY_SIZE];
	int listenSocketFD, portNumber;					
	struct sockaddr_in *servAddr;	
	struct hostent *server;		

	memset(client_name, '\0', MAX_NAME_LENGTH * sizeof(char)); // null terminate the string
	memset(server_name, '\0', MAX_NAME_LENGTH * sizeof(char)); 
	memset(client_input, '\0', BUFFER_SIZE * sizeof(char));
	memset(client_msg, '\0', BUFFER_SIZE * sizeof(char));
	memset(server_msg, '\0', BUFFER_SIZE * sizeof(char));
	memset(key, '\0', KEY_SIZE * sizeof(char));
	memset(enc_client_msg, '\0', BUFFER_SIZE * sizeof(char));
	if (argc != 3){ // check usage & args
		fprintf(stderr, "Usage: %s hostname port number\n", argv[0]);
		exit(EXIT_SUCCESS);
	}

	// Set up the socket
	listenSocketFD = createSocket(argv[0]); // sends the program name 

	server = gethostbyname(argv[1]); // do a DNS lookup and return address information
	if (server == NULL){
		fprintf(stderr, "Error: %s could not get host by name\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	portNumber = atoi(argv[2]); // Get the port number, convert to an integer from a string
	servAddr = createSocketType(portNumber, server);

	printf("Enter in the client's chat name: ");
	fgets(client_name, MAX_NAME_LENGTH, stdin);
	client_name[strlen(client_name)-1] = '\0';
	connectSocket(listenSocketFD, servAddr, portNumber);
	fillKeyTable(KEY_SIZE, key);
	sendData(listenSocketFD, &key, sizeof(key), 0, argv[0], 1); // send the key
	printf("key received\n");
	printf("%s\n", key);
	while(1){
		printf("%s> ", client_name);
		fgets(client_input, BUFFER_SIZE, stdin);
		client_input[strlen(client_input) - 1] = '\0';
		strcpy(client_msg, client_name); // 'client_name'
		strcat(client_msg, "> "); // 'cleint_name> '
		strcat(client_msg, client_input); // 'client_name> client_input'
		if(!strcmp(client_input, "\\quit")){ // Did the client user enter in '\quit'?
			char str[] = "Exiting";
			sendData(listenSocketFD, &str, sizeof(str), 0, argv[0], 1); // message the chat server so it closes its socket with the client
			printf("Exiting..\n");
			break; // close the 
		}
		encrypt(client_msg, key, enc_client_msg);
		printf("%s\n", enc_client_msg);
		sendData(listenSocketFD, &enc_client_msg, sizeof(enc_client_msg), 0, argv[0], 1); // sends the users message to the chat server
		do{
			recvData(listenSocketFD, &server_msg, sizeof(server_msg), 0, argv[0], 1); // Receives the users message sent by the chat server
		}while(server_msg[0] == '\0'); // keep looping until we receive a message sent by the chat server
		printf("%s\n", server_msg);
		if(!strcmp(server_msg, "Server has closed the connection")){
			break; // exit out of the loop to close the socket
		}
		memset(client_input, '\0', BUFFER_SIZE * sizeof(char));
		memset(client_msg, '\0', BUFFER_SIZE * sizeof(char));
		memset(server_msg, '\0', BUFFER_SIZE * sizeof(char));
		memset(enc_client_msg, '\0', BUFFER_SIZE * sizeof(char));
	}

	free(servAddr);
	close(listenSocketFD); // Close the connection socket

	return 0;
}