
/*  Computer Systems Project 2
	Author: Kaven Peng user: kavenp
	ID: 696573

	Client for Mastermind game server.c
    To run: start the server, then the client 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>

#define BUFFER_SIZE 512

void sigHandler(int dummy);
void sendEnd(char *out_buf, char *sock);

int main(int argc, char**argv)
{
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	char in_buf[BUFFER_SIZE], out_buf[BUFFER_SIZE];

	if (argc < 3)
	{
		fprintf(stderr, "usage %s hostname port\n", argv[0]);
		exit(0);
	}

	portno = atoi(argv[2]);


	/* Translate host name into peer's IP address ;
	 * This is name translation service by the operating system
	 */
	server = gethostbyname(argv[1]);

	if (server == NULL)
	{
		fprintf(stderr, "ERROR, no such host\n");
		exit(0);
	}

	/* Building data structures for socket */

	bzero((char *) &serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;

	bcopy((char *)server->h_addr,
	      (char *)&serv_addr.sin_addr.s_addr,
	      server->h_length);

	serv_addr.sin_port = htons(portno);

	/* Create TCP socket -- active open
	* Preliminary steps: Setup: creation of active open socket
	*/

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0)
	{
		perror("ERROR opening socket");
		exit(0);
	}

	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("ERROR connecting");
		exit(0);
	}
	/* clear buffers */
	memset(in_buf, 0, sizeof(in_buf));
	memset(out_buf, 0, sizeof(out_buf));

	/* Do processing
	*/

	puts("Connected\n");
	while ((n = recv(sockfd, in_buf, sizeof(in_buf), 0)) > 0)
	{
		printf("%s", in_buf);
		if ((strcmp(in_buf, "FAILURE.\n") == 0) ||
		        (strcmp(in_buf, "SUCCESS.\n") == 0))
		{
			break;
		}
		memset(in_buf, 0, sizeof(in_buf));
		scanf("%s" , out_buf);

		if (send(sockfd , out_buf , strlen(out_buf) , 0) < 0)
		{
			puts("Send failed");
			return 1;
		}
		/* clear buffers */
		memset(out_buf, 0, sizeof(out_buf));
	}

	close(sockfd);
	return 0;
}

