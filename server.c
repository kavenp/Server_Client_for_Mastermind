/*  Computer Systems Project 2
	Author: Kaven Peng user: kavenp
	ID: 696573

	Server program for Mastermind game
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <sys/resource.h>
#include "mastermind.h"

/* define */
#define CODE_LEN 5
#define GUESS_NUM 10
#define BUFFER_SIZE 512
#define NUM_COLOURS 6
#define LARGE_PRIME 7001
#define MAX_CLI 20
#define on_error(...) { fprintf(stderr, __VA_ARGS__); fflush(stderr); exit(1); }

/* typedef */
typedef struct {
	int socket;
	char *code, *client_ip, *serv_ip;
} handle_t;

/* function prototypes */
void *client_handler(void *);
char *randomCode();
void reply(char *code, char *guess, char *out_buf, int guessN,
           int sock, handle_t *handle);
char* getTime();
void sigHandler(int dummy);


/* global variables */
pthread_mutex_t lock;
FILE *fp;
int clientN = 0, successN = 0;
int initUsage;
char log_buf[BUFFER_SIZE];
time_t curTime;
int seed = 1;
struct rusage r_usage;
int sock_ids[MAX_CLI], mem_used[MAX_CLI]; 
char *cli_ips[MAX_CLI];


int main(int argc, char **argv)
{
	int sockfd, newsockfd, portno;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	int randomFlag = 0;
	struct rusage r_usage;
	getrusage(RUSAGE_SELF, &r_usage);
	initUsage = r_usage.ru_maxrss;
	/* clear initial global */
	memset(sock_ids, 0, sizeof(sock_ids));
	memset(mem_used, 0, sizeof(mem_used));
	memset(cli_ips, 0, sizeof(cli_ips));

	if (argc == 3)
	{
		/* Use argv[2] as default code */
		randomFlag = 0;
	} else if (argc == 2)
	{
		/* Set random code flag */
		randomFlag = 1;
	} else {
		on_error("Incorrect number of args.");
	}
	/* Init mutex lock */
	if (pthread_mutex_init(&lock, NULL) != 0) {
		on_error("Error initializing mutex.");
	}

	/* Create TCP socket */

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0)
	{
		on_error("Error opening socket.");
	}


	bzero((char *) &serv_addr, sizeof(serv_addr));

	portno = atoi(argv[1]);

	/* Create address we're going to listen on (given port number)
	 - converted to network byte order & any IP address for
	 this machine */

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);  

	/* Bind address to the socket */

	if (bind(sockfd, (struct sockaddr *) &serv_addr,
	         sizeof(serv_addr)) < 0)
	{
		on_error("Error binding.");
	}
	puts("Bind done.\n");

	/* Listen on socket - means we're ready to accept connections -
	 incoming connection requests will be queued */

	listen(sockfd, 20);

	clilen = sizeof(cli_addr);

	fp = fopen("log.txt", "w");
	/* Handle Crtl + C */
	signal(SIGINT, sigHandler);
	/* Handle SIGTERM */
	signal(SIGTERM, sigHandler);
	/* Time tracking */
	curTime = time(NULL);
	/* init thread and handle */
	pthread_t thread_id;
	handle_t *gameHandle;

	/* Accept a connection */
	while ((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
	                           &clilen)))
	{
		puts("Connection accepted.\n");
		if (newsockfd < 0)
		{
			on_error("Error on accept.");
		}
		gameHandle = (handle_t *)malloc(sizeof(handle_t));
		gameHandle->socket = newsockfd;
		gameHandle->serv_ip = malloc(INET_ADDRSTRLEN + 1);
		gameHandle->client_ip = malloc(INET_ADDRSTRLEN + 1);
		strcpy(gameHandle->serv_ip, (char *)inet_ntoa(serv_addr.sin_addr));
		strcpy(gameHandle->client_ip, (char *)inet_ntoa(cli_addr.sin_addr));
		if (randomFlag) {
			/* Create random code and use*/
			gameHandle->code = randomCode();
		} else {
			/* Default code */
			gameHandle->code = malloc(CODE_LEN);
			gameHandle->code = argv[2];
		}
		printf("CODE IS : %s\n", gameHandle->code);
		if (pthread_create(&thread_id, NULL, client_handler,
		                   (void *) gameHandle) < 0)
		{
			on_error("Error with thread creation.");
		}
		puts("Handler assigned.\n");
	}
	pthread_mutex_destroy(&lock);
	return 0;
}

/*	Handles all clients
 */

void *client_handler(void *gameHandle)
{
	handle_t *pack = (handle_t *)gameHandle;
	int sock = pack->socket;
	int guessN = GUESS_NUM;
	int read_size;
	char in_buf[BUFFER_SIZE], out_buf[BUFFER_SIZE];

	pthread_detach(pthread_self());
	/* Clear buffers */
	memset(in_buf, 0, sizeof(in_buf));
	memset(out_buf, 0, sizeof(out_buf));
	/* Initial write to log for client */
	/* Locked to gurantee concurrency */
	pthread_mutex_lock(&lock);
	sprintf(log_buf,
	        "[%s](%s)(soc_id %d) client connected\n[%s](%s) server secret = %s\n",
	        getTime(), pack->client_ip, sock,
	        getTime(), pack->serv_ip, pack->code);
	fprintf(fp, log_buf);
	memset(log_buf, 0, sizeof(log_buf));
	pthread_mutex_unlock(&lock);
	/* Welcome */
	sprintf(out_buf, "Try to guess a 4 character combination from these choices\n");
	strcat(out_buf, "( A, B, C, D, E, F ). Hints given will be in the form of [b:m]\n");
	strcat(out_buf, "where b is the number of characters in correct position and m is\n");
	strcat(out_buf, "the number of shared characters with the answer in incorrect positions.");
	strcat(out_buf, "\nGood Luck!\n\nEnter guess: ");
	write(sock, out_buf, strlen(out_buf));
	memset(out_buf, 0, sizeof(out_buf));
	while ((read_size = recv(sock, in_buf, sizeof(in_buf), 0)) > 0)
	{
		guessN--;
		/* locks reply so log is written correctly */
		pthread_mutex_lock(&lock);
		reply(pack->code, in_buf, out_buf, guessN, sock, pack);
		write(sock, out_buf, strlen(out_buf));
		memset(out_buf, 0, sizeof(out_buf));
		fprintf(fp, log_buf);
		memset(log_buf, 0, sizeof(log_buf));
		pthread_mutex_unlock(&lock);
		/* Clear buffers */
		memset(in_buf, 0, sizeof(in_buf));
	}
	if (read_size == 0) {
		puts("Client disconnected");
		/* Tracks each threads usage */
		pthread_mutex_lock(&lock);
		getrusage(RUSAGE_SELF, &r_usage);
		sock_ids[clientN] = sock;
		mem_used[clientN] = r_usage.ru_maxrss;
		cli_ips[clientN] = pack->client_ip;
		clientN++;
		pthread_mutex_unlock(&lock);
		fflush(stdout);
	} else if (read_size < 0) {
		on_error("Error recv failed.")
	}
	close(sock);
	return 0;
}

/* Creates a new random code */
char *randomCode()
{
	int i, randNum;
	char *generate = malloc(CODE_LEN);
	srand(seed);
	for (i = 0; i < CODE_LEN - 1; i++)
	{
		randNum = rand() % NUM_COLOURS;
		generate[i] = colours[randNum];
	}
	generate[CODE_LEN - 1] = '\0';
	/* change seed to ensure different threads get different codes*/
	seed += LARGE_PRIME - 1 + time(NULL);
	return generate;
}

/* Determines servers reply by setting out_buf */
void reply(char *code, char *guess, char *out_buf, int guessN,
           int sock, handle_t *handle)
{
	int posNum = posCheck(code, guess);
	int letterNum = letterCheck(code, guess) - posNum;
	if (letterNum < 0)
	{
		letterNum = 0;
	}
	if (valid(guess))
	{
		if (winCheck(code, guess))
		{
			/* Success! */
			sprintf(out_buf, "SUCCESS.\n");
			sprintf(log_buf, "[%s](%s)(soc_id %d) correct guess = %s SUCCESS. Game Over.\n"
			        , getTime(), handle->client_ip, sock, guess);
			/* update successN */
			successN++;
		} else if (guessN <= 0) {
			/* Failed */
			sprintf(out_buf, "FAILURE.\n");
			sprintf(log_buf, 
				"[%s](%s)(soc_id %d) final guess = %s FAILURE. Code was %s\n",
			        getTime(), handle->client_ip, sock, guess, code);
		} else {
			/* Still guessing */
			sprintf(out_buf,
			        "Hint : [%d:%d]\nYou have %d guesses left.\nEnter guess: ",
			        posNum, letterNum, guessN);
			sprintf(log_buf,
			        "[%s](%s)(soc_id %d) client's guess = %s\n[%s](%s) server's hint  = [%d:%d]\n",
			        getTime(), handle->client_ip, sock, guess, getTime(), handle->serv_ip, posNum,
			        letterNum);
		}
	} else {
		sprintf(out_buf, "INVALID.\nYou have %d guesses left.\nEnter guess: ",
				guessN);
		sprintf(log_buf, "[%s](%s)(soc_id %d) invalid guess = %s INVALID.\n",
		        getTime(), handle->client_ip, sock, guess);
	}
}

/* Returns current time as a string */
char* getTime() {
	char *timeString = malloc(256);
	time(&curTime);
	struct tm *dt = localtime(&curTime);
	strftime(timeString, 256, "%d %m %y %T", dt);
	return timeString;
}

/* Handles signals SIGINT and SIGTERM */
void sigHandler(int signo) {
	if (signo == SIGINT || signo == SIGTERM)
	{
		struct rusage r_usage;
		getrusage(RUSAGE_SELF, &r_usage);
		fprintf(fp, "\n\n--------------------STATISTICS---------------------\n");
		fprintf(fp, "Number of clients connected: %d\n", clientN);
		fprintf(fp, "Number of clients that were successful: %d\n", successN);
		fprintf(fp, "Initial memory available: %dKB\n", initUsage);
		fprintf(fp, "Time spent executing in user mode: %ld.%04ld secs\n",
		        r_usage.ru_utime.tv_sec, r_usage.ru_utime.tv_usec);
		fprintf(fp, "Time spent executing in kernel mode: %ld.%04ld secs\n",
		        r_usage.ru_stime.tv_sec, r_usage.ru_stime.tv_usec);
		fprintf(fp, "\n---------------------------------------------------\n");
		int i = 0;
		while (sock_ids[i]) {
			fprintf(fp, "Client : (%s) (soc_id %d) \n", cli_ips[i], sock_ids[i]);
			fprintf(fp, "Memory available when running: %dKB\n\n", mem_used[i]);
			i++;
		}
		fclose(fp);
		exit(0);
	}
}


