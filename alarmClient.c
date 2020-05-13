#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <semaphore.h>
#include <mysql/mysql.h>
#include "stems.h"

#define FIFO_NAME "./alarm_fifo"

void getargs_WarningTable(char host[], char user[], char password[], char db[]);
void WarningTable(char *msg);

/*
 * Send an HTTP request for the specified file 
 */
void clientSend(int fd, char *filename, char *body)
{
	char buf[MAXLINE];
	char hostname[MAXLINE];

	Gethostname(hostname, MAXLINE);

	/* Form and send the HTTP request */
	sprintf(buf, "POST %s HTTP/1.1\n", filename);
	sprintf(buf, "%shost: %s\n", buf, hostname);
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
void userTask(char hostname[], int port, char webaddr[], char *body)
{
	int clientfd;

	clientfd = Open_clientfd_alarm(hostname, port);
	if (clientfd >= 0) {
		clientSend(clientfd, webaddr, body);
		// clientPrint(clientfd);
		clientPrintText(clientfd);
		Close(clientfd);
	}
	else if (clientfd == -1) {
		printf("Server is not connected\n");
		printf("Updating Database...\n");

		// Database에 올리기
		WarningTable(body);


		printf("Database updated\n");
	}
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

void getargs_WarningTable(char host[], char user[], char password[], char db[])
{
	FILE *fp;

	fp = fopen("mysql-iotserver.txt", "r");
	if (fp == NULL)
		unix_error("mysql-iotserver.txt file does not open at WarningTable.");

	fscanf(fp, "%s", host);
	fscanf(fp, "%s", user);
	fscanf(fp, "%s", password);
	fscanf(fp, "%s", db);
	fclose(fp);
}

void WarningTable(char *msg)
{
	MYSQL *conn;

	char host[256];
	char user[256];
	char password[256];
	char db[256];
	char buf[MAXLINE];
	char *name;
	char *time;
	float value;

	strncpy(buf, msg, strlen(msg) + 1);

	getargs_WarningTable(host, user, password, db);

	conn = mysql_init(NULL);
	if (conn == NULL) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		return;
	}

	if (mysql_real_connect(conn, host, user, password, db, 0, NULL, 0) == NULL) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		mysql_close(conn);
		return;
	}

	strtok(buf, "=");
	name = strtok(NULL, "&");

	strtok(NULL, "=");
	time = strtok(NULL, "&");

	strtok(NULL, "=");
	value = atof(strtok(NULL, "&"));

	char query[MAXLINE];
	sprintf(query, "CREATE TABLE IF NOT EXISTS warnings"
	"("
	"name VARCHAR(256) NOT NULL,"
	"time INT,"
	"value FLOAT(6,2)"
	");");
	
	if (mysql_query(conn, query)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		mysql_close(conn);
		exit(1);
	}

	sprintf(query, "INSERT INTO warnings(name, time ,value)"
	"VALUE(\'%s\', %d, %f);", name, atoi(time), value);

	if (mysql_query(conn, query)) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		mysql_close(conn);
		exit(1);
	}

	mysql_close(conn);
}


void console(char hostname[], int *port, char webaddr[], float *threshold)
{
	int client_fifo_fd;
	float value;
	char buf[MAXLINE];	
	char body[MAXLINE];
	char msg[MAXLINE];

	mkfifo(FIFO_NAME, 0666);
	do {
		client_fifo_fd = open(FIFO_NAME, O_RDONLY);
		if (client_fifo_fd == -1) {
			fprintf(stderr, "Client fifo failure\n");
			exit(EXIT_FAILURE);
		}

		read(client_fifo_fd, buf, MAXLINE);

		close(client_fifo_fd);

		strcpy(msg, buf);

		strcpy(body, buf);	
		strtok(body, "=");
		strtok(NULL, "=");
		strtok(NULL, "=");
		value = atof(strtok(NULL, ""));

		if (value > *threshold) {
			userTask(hostname, *port, webaddr, msg);
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
