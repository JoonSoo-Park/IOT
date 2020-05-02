/*
 * clientPost.c: A very, very primitive HTTP client for sensor
 * 
 * To run, prepare config-cp.txt and try: 
 *      ./clientPost
 *
 * Sends one HTTP request to the specified HTTP server.
 * Get the HTTP response.
 */


#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/time.h>
#include "stems.h"

void clientPrint(int fd);

typedef enum {
	HELP = 0,
	NAME,
	VALUE,
	SEND,
	RANDOM,
	QUIT,
	CLI_ENUM_SIZE,
	ERROR
} CLI_ENUM;

static const char *CLI_STRING[] = {
	"help",
	"name",
	"value",
	"send",
	"random",
	"quit"
};

CLI_ENUM get_command(const char* command)
{
	char buf[MAXLINE];
	char *token;
	const char *delim = " \r\n";

	strcpy(buf, command);

	token = strtok(buf, delim);

	if (token == NULL)
		return ERROR;
	
	for (int i = 0; i < CLI_ENUM_SIZE; ++i) {
		if (!strcmp(CLI_STRING[i], token))
			return i;
	}

	return -1;
}

void cli_help()
{
	printf("help: list available commands.\n");
	printf("name: print current sensor name.\n");
	printf("name <sensor>: charnge sensor name to <sensor>\n");
	printf("value: print current value of sensor.\n");
	printf("value <n>: set sensor value to <n>.\n");
	printf("send: send (current sensor name, time, value) to server.\n");
	printf("random <n>: send (name, time, random value) to server <n> times.\n");
	printf("quit: quit the program.\n\n");
}

int cli_name(const char* str, char *name)
{
	int length = strlen(str) + 1;
	int position = 0;
	char *temp = (char*)malloc(length);
	const char delim[] = " ";
	char *tokens[2] = {0};
	char *token;

	memcpy(temp, str, length);

	token = strtok(temp, delim);
	while (token != NULL) {
		tokens[position++] = token;
		token = strtok(NULL, delim);
	}

	if (tokens[0] == NULL)
		return -1;

	if (tokens[1] != NULL) {
		strncpy(name, tokens[1], strlen(tokens[1]));
	}
	else
		printf("%s\n", name);

	return 1;
}

int cli_value(const char* str, float *value)
{
	int length = strlen(str) + 1;
	int position = 0;
	float new_value = 0.0f;
	char *temp = (char*)malloc(length);
	const char delim[] = " \r\n";
	char *tokens[2] = {0};
	char *token;

	memcpy(temp, str, length);

	token = strtok(temp, delim);
	while (token != NULL) {
		tokens[position++] = token;
		token = strtok(NULL, delim);
	}

	if (tokens[0] == NULL)
		return -1;

	if (tokens[1] != NULL) {
		new_value = atof(tokens[1]);
		*value = new_value;
	}
	else
		printf("%f\n", *value);

	return 1;
}

void clientSend(int fd, char *filename, char *body)
{
  	char buf[MAXLINE];
  	char hostname[MAXLINE];

  	Gethostname(hostname, MAXLINE);

  	/* Form and send the HTTP request */
  	sprintf(buf, "POST %s HTTP/1.1\n", filename);
  	sprintf(buf, "%sHost: %s\n", buf, hostname);
  	sprintf(buf, "%sContent-Type: text/plain; charset=utf-8\n", buf);
  	sprintf(buf, "%sContent-Length: %ld\n\r\n", buf, strlen(body));
  	sprintf(buf, "%s%s\n", buf, body);

  	Rio_writen(fd, buf, strlen(buf));
}
  
int cli_random(const char *str, char *sensorName, char *hostname, int port, char *filename, float time, float *value)
{
	int clientfd;
	int n = 1; // random <n>, if send then n is always 1
	int length = strlen(str) + 1;
	int position = 0;
	const char delim[] = " \r\n";
	char *temp = (char*)malloc(length);
	char *tokens[2] = {0};
	char *token;
	char msg[MAXLINE];

	memcpy(temp, str, length);

	token = strtok(temp, delim);
	while (token != NULL) {
		tokens[position++] = token;
		token = strtok(NULL, delim);
	}

	if (tokens[0] == NULL)
		return -1;

	if (tokens[1] != NULL) 
		n = atoi(tokens[1]);

	while (n--) {
  		sprintf(msg, "name=%s&time=%f&value=%f", sensorName, time, *value);
  		clientfd = Open_clientfd(hostname, port);
  		clientSend(clientfd, filename, msg);
  		clientPrint(clientfd);
  		Close(clientfd);
		(*value)++;
	}

	return 1;
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
  	  	/* If you want to look for certain HTTP tags... */
  	  	if (sscanf(buf, "Content-Length: %d ", &length) == 1)
  	  	  	printf("Length = %d\n", length);
  	  	printf("Header: %s", buf);
  	  	n = Rio_readlineb(&rio, buf, MAXBUF);
  	}

  	/* Read and display the HTTP Body */
  	n = Rio_readlineb(&rio, buf, MAXBUF);
  	while (n > 0) {
  	  	printf("%s", buf);
  	  	n = Rio_readlineb(&rio, buf, MAXBUF);
  	}
}

/* currently, there is no loop. I will add loop later */
void userTask(char *sensorName, char *hostname, int port, char *filename, float time, float *value)
{
  	// int clientfd;
	int position = 0, quit = 0;
	CLI_ENUM cli_number;
  	// char msg[MAXLINE];
	char buf[MAXLINE];

	// TODO
	// 1. get input
	// 2. find matching function using switch
	// 3. call function

	while (!quit) {
		position = 0;
		while (1) {
			char c;

			c = getchar();
			if (c == '\n' || c == EOF) {
				buf[position] = '\0';
				break;
			}
			buf[position++] = c;
		}
		fflush(stdin);

		cli_number = get_command(buf);
		
		if (cli_number == -1) {
			printf("Unrecognized command.\n");
			continue;
		}

		switch(cli_number) {
			case HELP:
				cli_help();
				break;
			case NAME:
				cli_name(buf, sensorName);
				break;
			case VALUE:
				cli_value(buf, value);
				break;
			case SEND:
				cli_random(buf, sensorName, hostname, port, filename, time, value);
				break;
			case RANDOM:
				cli_random(buf, sensorName, hostname, port, filename, time, value);
				break;
			case QUIT:
				quit = 1;
				break;
			default:
				break;
		}
	}


	/*
  	sprintf(msg, "name=%s&time=%f&value=%f", sensorName, time, value);
  	clientfd = Open_clientfd(hostname, port);
  	clientSend(clientfd, filename, msg);
  	clientPrint(clientfd);
  	Close(clientfd);
	*/
}

void getargs_cp(char *sensorName, char *hostname, int *port, char *filename, float *time, float *value)
{
  	FILE *fp;

  	fp = fopen("config-cp.txt", "r");
  	if (fp == NULL)
  	  	unix_error("config-cp.txt file does not open.");

  	fscanf(fp, "%s", sensorName);
  	fscanf(fp, "%s", hostname);
  	fscanf(fp, "%d", port);
  	fscanf(fp, "%s", filename);
  	fscanf(fp, "%f", time);
  	fscanf(fp, "%f", value);
  	fclose(fp);
}

int main(void)
{
  	char sensorName[MAXLINE], hostname[MAXLINE], filename[MAXLINE];
  	int port;
  	float time, value;

  	getargs_cp(sensorName, hostname, &port, filename, &time, &value);

  	userTask(sensorName, hostname, port, filename, time, &value);
  
  	return(0);
}
