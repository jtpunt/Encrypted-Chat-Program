#include <assert.h>
#include <stdbool.h> 
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "chat_common.h"
#define MAX_CHARS 27
// https://en.wikipedia.org/wiki/One-time_pad
// This function sends random characters in the alphabet (including a space character) plus a newline character to stdout.
void fillKeyTable(int length, char* key){
	int idx, i;
	for (i = 0; i < length; i++){
		key[i] = 32 + rand() % MAX_CHARS;
		printf("%c", key[i]);
	}
	key[length] = '\0';
	printf("%c\n", key[length]);
	printf("Done\n");
}