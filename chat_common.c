#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include "chat_common.h"

// This is a function helper to assist with modulo arithmetic operations.
int modulo(int a, int b){
    int r = a % b;
    return r < 0 ? r + b : r;
}
// using ASCII codes to check that all string characters are between A-Z or a space
int validateInput(char *input){
	long i;						
	for (i = 0; i < strlen(input); i++){
		if(input[i] < 65 && input[i] != 32 || input[i] > 90){
			return 0;	
		}
	}
	return 1;
}
// This function is used to send data between the client and server. If the exitValue is not -1,
// then the function will call exit with the exitValue argument passed in.
void sendData(int socket, void *buffer, size_t length, int flags, char* args, int exitValue){
	printf("sending data...\n");
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
// Position represents the nth character in the ASCII table of decimal values.
// This function receives both plaintext and a key to encrypt the plaintext into ciphertext.
// Converting plaintext to ciphertext is done by adding the position of each plaintext char to the position of each 
// key char, then taking that result modulo 127 to return a valid char position within the ASCII table of decimal values.
// Finally, a 2nd level of encryption is performed on the original ciphertext by adding the position of each ciphertext char to the position of each
// key char, then taking that result modulu 127 to return a valid char position within the ASCII table of decimal values.
void encrypt(char* plaintext, char* key, char* ciphertext){
	char characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
	int i;
	for(i = 0; i < strlen(plaintext); i++){
		int plaintextCharPos = (int)(plaintext[i]); // get the ASCII position of current plaintext letter 
		int keyCharPos = (int)(key[i % strlen(key)]); // get the ASCII position of the key character
		int ciphertextCharPos = modulo(plaintextCharPos + keyCharPos, MAX_ASCII_CHARS); // get the ASCII position of the encrypted ciphertext letter
		ciphertextCharPos = modulo(ciphertextCharPos + keyCharPos, MAX_ASCII_CHARS); // get the ASCII position of the double encrypted ciphertext letter
		// printf("plaintextCharPos: %d %c\n", plaintextCharPos, getCharRepr(plaintextCharPos));
		// printf("keyCharPos: %d %c\n", keyCharPos, getCharRepr(keyCharPos));
		// printf("cipherCharPos: %d %c\n\n", ciphertextCharPos, getCharRepr(ciphertextCharPos));
		ciphertext[i] = (char)(ciphertextCharPos); // assign the encrypted char valueps
	}
}
// Position represents the nth character in the ASCII table of decimal values.
// This function receives both ciphertext and a key to decrypt the ciphertext into plaintext.
// Converting ciphertext to plaintext is done by subtracting the position of each ciphertext character by the position of each key character,
// then take that result modulo 127 to return a valid position within the ASCII table of decimal values.
// Finally, a 2nd level of decryption if performed on the original plaintext by subtracting the position of each ciphertext char by the position of each
// key char, then taking that result modulu 127 to return a valid char position within the ASCII table of decimal values.
void decrypt(char* ciphertext, char* key, char *plaintext){
	printf("decrypting data...\n");
	int i;
	for(i = 0; i < strlen(ciphertext); i++){
		int ciphertextCharPos = (int)(ciphertext[i]); // get the ASCII position of the double encrypted ciphertext letter
		int keyCharPos = (int)(key[i % strlen(key)]); // get the ASCII position of the current key char value
		// get the ASCII position from decrypting the double encrypted ciphertext letter to get the final encrypted ciphertext char
		ciphertextCharPos = modulo(ciphertextCharPos - keyCharPos, MAX_ASCII_CHARS); 
		int decryptedCharPos = modulo(ciphertextCharPos - keyCharPos, MAX_ASCII_CHARS); // get the ASCII position of the plaintext char
		// printf("ciphertextCharPos: %d %c\n", ciphertextCharPos, getCharRepr(ciphertextCharPos));
		// printf("keyCharPos: %d %c\n", keyCharPos, getCharRepr(keyCharPos));
		// printf("decryptedCharPos: %d %c\n\n", decryptedCharPos, getCharRepr(decryptedCharPos));
		plaintext[i] = (char)(decryptedCharPos);
	}
}