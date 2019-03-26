// Name: Jonathan Perry
// Class: CS 344 - Operating Systems
// Project OTP
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

#define BUFFER_SIZE 500
#define MAX_CHARS 27

int getNumRepr(char letter);
char getCharRepr(int num);
int modulo(int a, int b);
int validateInput(char *string);
void sendData(int socket, void *buffer, size_t length, int flags, char* args, int exitValue);
void recvData(int socket, void *buffer, size_t length, int flags, char* args, int exitValue);
void encrypt(char *plaintext, char *key, char *ciphertext);
void decrypt(char *ciphertext, char *key, char *plaintext);