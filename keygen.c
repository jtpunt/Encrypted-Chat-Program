// 1. This program createa a key file of specified length.
// 2. The characters in the file generated will be any of the 27 allowd characters, generated using the standard UNIX
//    randomized methods.
// 3. Do not create spaces every 5 characters, as has been historically done.
// 4. Note that you specifically do not have to do any fancy random number generation: we're not looking for crpytographically 
//    secure random number generator! rand() is just fine.
// 5. The last character keygen outputs should be a newline.
// 6. All error text must be output to stderr, if any.
// The syntax for keygen is as follows: keygen keylength
// 	 - where keylength is the length of the key file in characters.
//   - keygen outputs to stdout
//   - Here is an example run, which redirects stdout to a key file of 256 characters called "mykey"
//   - (note that mykey is 257 characters long because of the newline): keygen 256 > mykey
#include <assert.h>
#include <stdbool.h> 
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "otp_functions.c"
#define MAX_CHARS 27

// This function sends random characters in the alphabet (including a space character) plus a newline character to stdout.
void fillKeyTable(int length){
	int idx, i;
	for (i = 0; i < length; i++){
		fprintf(stdout, "%c", getCharRepr(0 + rand() % MAX_CHARS)); // prints a random alphabetical character or space to stdout
	}
	fprintf(stdout, "\n"); // add a newline character to the end of the key
}
int main(int argc, char **argv){
	// check for correct number of arguments
	if (argc != 2){
		fprintf(stderr, "Usage: %s key_length\n", argv[0]);
		exit(EXIT_SUCCESS);
	}
	srand(time(NULL));
	fillKeyTable(atoi(argv[1]));
	return 0;
}
