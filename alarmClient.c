#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <semaphore.h>
#include "stems.h"

#define FIFO_NAME "./alarm_fifo"

/*
 * Send an HTTP request for the specified file 
 */
void clientSend(int fd, char *filename)
{
	char buf[MAXLINE];
	char hostname[MAXLINE];

	Gethostname(hostname, MAXLINE);

	/* Form and send the HTTP request */
	sprintf(buf, "POST %s HTTP/1.1\n", filename);
	sprintf(buf, "%shost: %s\n\r\n", buf, hostname);
	Rio_writen(fd, buf, strlen(buf));
}
  
/*
 * Read the HTTP response and print it out
 */
void clientPrint(int fd)
{
	rio_t rio;
	char buf[MAXBUF];  
	int length = 0;
	int n;
  
	Rio_readinitb(&rio, fd);

	/* Read and display the HTTP Header */
	n = Rio_readlineb(&rio, buf, MAXBUF);
	while (strcmp(buf, "\r\n") && (n > 0)) {
	  printf("Header: %s", buf);
	  n = Rio_readlineb(&rio, buf, MAXBUF);

	  /* If you want to look for certain HTTP tags... */
	  if (sscanf(buf, "Content-Length: %d ", &length) == 1) {
	    printf("Length = %d\n", length);
	  }
	}

	/* Read and display the HTTP Body */
	n = Rio_readlineb(&rio, buf, MAXBUF);
	while (n > 0) {
	  printf("%s", buf);
	  n = Rio_readlineb(&rio, buf, MAXBUF);
	}

	printf("\n");
}

/*
 * Read the HTTP response and print it out
 */
void clientPrintText(int fd)
{
	rio_t rio;
	char buf[MAXBUF];  
	int n;
  
	Rio_readinitb(&rio, fd);

	/* Read and display the HTTP Body */
	n = Rio_readlineb(&rio, buf, MAXBUF);
	while (n > 0) {
	  printf("%s", buf);
	  n = Rio_readlineb(&rio, buf, MAXBUF);
	}

	printf("\n");
}

/* currently, there is no loop. I will add loop later */
void userTask(char hostname[], int port, char webaddr[])
{
	int clientfd;

	clientfd = Open_clientfd(hostname, port);
	clientSend(clientfd, webaddr);
	// clientPrint(clientfd);
	clientPrintText(clientfd);
	Close(clientfd);
}

void getargs_cg(char hostname[], int *port, char webaddr[], float *threshold)
{
	FILE *fp;

	fp = fopen("config-ac.txt", "r");
	if (fp == NULL)
	  unix_error("config-ac.txt file does not open.");

	fscanf(fp, "%s", hostname);
	fscanf(fp, "%d", port);
	fscanf(fp, "%s", webaddr);
	fscanf(fp, "%f", threshold);
	fclose(fp);
}

void console(char hostname[], int *port, char webaddr[], float *threshold)
{
	int client_fifo_fd;
	float value;
	char buf[MAXLINE];	
	char body[MAXLINE];

	mkfifo(FIFO_NAME, 0666);
	do {
		client_fifo_fd = open(FIFO_NAME, O_RDONLY);
		if (client_fifo_fd == -1) {
			fprintf(stderr, "Client fifo failure\n");
			exit(EXIT_FAILURE);
		}

		read(client_fifo_fd, buf, MAXLINE);

		close(client_fifo_fd);

		strcpy(body, buf);	
		strtok(body, "=");
		strtok(NULL, "=");
		strtok(NULL, "=");
		value = atof(strtok(NULL, ""));

		if (value > *threshold) {
			userTask(hostname, *port, webaddr);
		}
	} while (1);

	unlink(FIFO_NAME);
}

int main(void)
{
	char hostname[MAXLINE], webaddr[MAXLINE];
	int port;
	float threshold;
  
	getargs_cg(hostname, &port, webaddr, &threshold);

	console(hostname, &port, webaddr, &threshold);

	// userTask(hostname, port, webaddr);

	return(0);
}
