#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include "chat_common.h"
// This function receives a character literal and returns it nth position in the alphabaet. 
// etc, 'A' = 0, 'B' = 1, ' ' (a space) = 27 and so on.
int getNumRepr(char letter){
	int pos;
	if (letter >= 'A' && letter <= 'Z'){
  		pos = letter - 'A';
	}else if(letter >= 'a' && letter <= 'z'){
		pos = letter - 'a';
	}
 	else{
 		pos = 26;
 	}
 	return pos;
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
// Position represents the nth character in the alphabet. etc, 'A' = 0, 'B' = 1, ' ' (a space) = 27 and so on.
// This function receives both plaintext and a key to encrypt the plaintext into ciphertext.
// Converting plaintext to ciphertext is done by adding the position of each plaintext letter to the position of each 
// key letter, then take that result modulo 27 to return a valid position within the alphabet.
void encrypt(char* plaintext, char* key, char* ciphertext){
	char characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
	int i;
	for(i = 0; i < strlen(plaintext); i++){
		int plaintextCharPos = getNumRepr(plaintext[i]); // get the position of current plaintext letter in the alphabet. ETC 'A' = position 0, 'B' = position 1
		int keyCharPos = getNumRepr(key[i]); // get the position of the encrypted letter in the alphabet, same as above
		int ciphertextCharPos = modulo(plaintextCharPos + keyCharPos, MAX_CHARS); 
		ciphertext[i] = getCharRepr(ciphertextCharPos); // assign the encrypted char valueps
	}
}
// Position represents the nth character in the alphabet. etc, 'A' = 0, 'B' = 1, ' ' (a space) = 27 and so on.
// This function receives both ciphertext and a key to decrypt the ciphertext into plaintext.
// Converting ciphertext to plaintext is done by subtracting the position of each ciphertext character to the position of each key character,
// then take that result modulo 27 to return a valid position within the alphabet.
void decrypt(char* ciphertext, char* key, char *plaintext){
	printf("decrypting data...\n");
	int i;
	for(i = 0; i < strlen(ciphertext); i++){
		int ciphertextCharPos = getNumRepr(ciphertext[i]); // get the position of the encrypted ciphertext letter in the alphabet
		int keyCharPos = getNumRepr(key[i]); // get the position of the current key char value
		int decryptedCharPos = modulo(ciphertextCharPos - keyCharPos, MAX_CHARS);
		printf("ciphertextCharPos: %d %c\n", ciphertextCharPos, getCharRepr(ciphertextCharPos));
		printf("keyCharPos: %d %c\n", keyCharPos, getCharRepr(keyCharPos));
		printf("decryptedCharPos: %d %c\n", decryptedCharPos, getCharRepr(decryptedCharPos));
		plaintext[i] = getCharRepr(decryptedCharPos);
		printf("char: %c\n", plaintext[i]);
	}
}