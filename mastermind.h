/*  Computer Systems Project 2
	Author: Kaven Peng user: kavenp
	ID: 696573

	Mastermind header file
*/

#ifndef __MASTERMIND__
#define __MASTERMIND__

#ifndef __GNUC__
#define __attribute__(x)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Global array */
const char colours[6] __attribute__((weak)) = {'A', 'B', 'C', 'D', 'E', 'F'};

int valid(char *guess);
int winCheck(char *code, char *guess);
int posCheck(char *code, char *guess);
int letterCheck(char *code, char *guess);

#endif