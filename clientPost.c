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
void userTask(char *myname, char *hostname, int port, char *filename, float time, float value)
{
  	int clientfd;
  	char msg[MAXLINE];

  	sprintf(msg, "name=%s&time=%f&value=%f", myname, time, value);
  	clientfd = Open_clientfd(hostname, port);
  	clientSend(clientfd, filename, msg);
  	clientPrint(clientfd);
  	Close(clientfd);
}

void getargs_cp(char *myname, char *hostname, int *port, char *filename, float *time, float *value)
{
  	FILE *fp;

  	fp = fopen("config-cp.txt", "r");
  	if (fp == NULL)
  	  unix_error("config-cp.txt file does not open.");

  	fscanf(fp, "%s", myname);
  	fscanf(fp, "%s", hostname);
  	fscanf(fp, "%d", port);
  	fscanf(fp, "%s", filename);
  	fscanf(fp, "%f", time);
  	fscanf(fp, "%f", value);
  	fclose(fp);
}

// console programming
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
	const char *delim = " \r\n=";

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

void cli_name(char *sensorName, char *changeTo, size_t size)
{
	if (changeTo == NULL) {
		printf("Current sensor is \'%s\'\n", sensorName);
	}
	else {
		strncpy(sensorName, changeTo, size);
		sensorName[size] = '\0';

		printf("Sensor name is changed to \'%s\'\n", sensorName);
	}

	return;
}

void cli_value(float *value, char *changeTo)
{
	if (changeTo == NULL) {
		printf("Current value of sensor is %f.\n", *value);
	}
	else {
		*value = atof(changeTo);
		printf("Sensor value is changes to %f.\n", *value);
	}

	return;
}

void cli_random(char *sensorName, char *hostname, int port, char *filename, float time, float value, int n)
{
	while (n--) {
		userTask(sensorName, hostname, port, filename, time, value);
		value += 1.0;
	}
}

void console(char *sensorName, char *hostname, int port, char *filename, float time, float value)
{
	const char *delim = " \r\n=";
	char input[MAXLINE];
	int cli_number;
	int length = 0;
	int quit = 0;

	while (!quit) {
		char *command = NULL;
		char *option = NULL;

		printf(">> ");
		fgets(input, MAXLINE, stdin);
		input[strlen(input) - 1] = '\0';

		cli_number = get_command(input);

		if (cli_number == -1) {
			printf("Unrecognized command.\n");
			continue;
		}

		command = strtok(input, delim);
		option = strtok(NULL, delim);

		switch(cli_number) {
			case HELP:
				cli_help();
				break;
			case NAME:
				if (option != NULL) length = strlen(option);
				cli_name(sensorName, option, length);
				break;
			case VALUE:
				cli_value(&value, option);
				break;
			case SEND:
				userTask(sensorName, hostname, port, filename, time, value);
				break;
			case RANDOM:
				cli_random(sensorName, hostname, port, filename, time, value, atoi(option));
				break;
			case QUIT:
				quit = 1;
				break;
			default:
				break;
		}
	}
}

int main(void)
{
	char myname[MAXLINE], hostname[MAXLINE], filename[MAXLINE];
	int port;
	float time, value;

	getargs_cp(myname, hostname, &port, filename, &time, &value);

	// userTask(myname, hostname, port, filename, time, value);

	console(myname, hostname, port, filename, time, value);
  
	return(0);
}
