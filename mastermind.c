/*  Computer Systems Project 2
	Author: Kaven Peng user: kavenp
	ID: 696573

	Mastermind helper functions
*/

#include "mastermind.h"

#define NUMCHAR 4

int valid(char *guess)
{
	int i;
	if (strlen(guess) != 4)
	{
		return 0;
	}
	/* Checks if in colour array or all uppercase */
	for (i = 0; i < strlen(guess); i++) {
		if (!strchr(colours, *guess) || islower(*guess)) {
			/* not in colours */
			return 0;
		}
		guess++;
	}
	return 1;
}

int winCheck(char *code, char *guess) {
	/* Only checks letters which are not \n and \0 */
	while (*guess && *guess != '\n') {
		if (*code != *guess) {
			return 0;
		}
		code++;
		guess++;
	}
	return 1;
}

int posCheck(char *code, char *guess) {
	int correct = 0;
	while (*guess && *guess != '\n') {
		if (*code == *guess) {
			correct++;
		}
		code++;
		guess++;
	}
	return correct;
}

/* Finds number of shared letters between code and guess */
int letterCheck(char *code, char *guess) {
	char *codeCheck, *guessCheck;
	int check[4];
	/* Initialize all values of check array to 0 */
	memset(check, 0, sizeof(check));
	int i, correct = 0;
	codeCheck = code;
	guessCheck = guess;

	while (*codeCheck && *codeCheck != '\n') {
		guessCheck = guess;
		i = 0;
		while (*guessCheck && *guessCheck != '\n') {
			if (*codeCheck == *guessCheck && check[i] == 0) {
				check[i] = 1;
				correct++;
				break;
			}
			i++;
			guessCheck++;
		}
		codeCheck++;
	}
	return correct;
}

