/*
 * clientGet.c: A very, very primitive HTTP client for console.
 * 
 * To run, prepare config-cg.txt and try: 
 *      ./clientGet
 *
 * Sends one HTTP request to the specified HTTP server.
 * Prints out the HTTP response.
 *
 * For testing your server, you will want to modify this client.  
 *
 * When we test your server, we will be using modifications to this client.
 *
 */


#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <semaphore.h>
#include <mysql/mysql.h>
#include "stems.h"

/*
 * Send an HTTP request for the specified file 
 */
void clientSend(int fd, char *filename)
{
	char buf[MAXLINE];
	char hostname[MAXLINE];

	Gethostname(hostname, MAXLINE);

	/* Form and send the HTTP request */
	sprintf(buf, "GET %s HTTP/1.1\n", filename);
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

void getargs_cg(char hostname[], int *port, char webaddr[])
{
	FILE *fp;

	fp = fopen("config-cg2.txt", "r");
	if (fp == NULL)
	  unix_error("config-cg.txt file does not open.");

	fscanf(fp, "%s", hostname);
	fscanf(fp, "%d", port);
	fscanf(fp, "%s", webaddr);
	fclose(fp);
}

// console programming
typedef enum {
	HELP = 0,
	CLI_LIST,
	INFO,
	GET,
	WARNINGS,
	QUIT,
	EXIT,
	CLI_ENUM_SIZE,
	NOT_FOUND,
	ERROR
} CLI_ENUM;

static const char *CLI_STRING[] = {
	"HELP",
	"LIST", 
	"INFO",
	"GET",
	"WARNINGS",
	"QUIT",
	"EXIT"
};

CLI_ENUM get_command(const char* command)
{
	char buf[MAXLINE];
	char *token;
	const char *delim = " \r\n";

	strncpy(buf, command, strlen(command) + 1);

	token = strtok(buf, delim);

	for (int i = 0; i < strlen(token); ++i)
		token[i] = toupper(token[i]);

	if (token == NULL)
		return ERROR;

	for (int i = 0; i < CLI_ENUM_SIZE; ++i) {
		if (!strcmp(CLI_STRING[i], token))
			return i;
	}

	return NOT_FOUND;
}

void help()
{
	printf("help: list available commands.\n");
	printf("list: list all sensor name.\n");
	printf("info <sensor>: print <sensor> name, count and average.\n");
	printf("get <sensor> <n>: print <sensor>'s last <n> time and value.\n");
	printf("warnings: print all warnings in DB.\n");
	printf("quit: quit the program.\n");
	printf("exit: exit the program.\n");
}

void getargs_database(char host[], char user[], char password[], char db[])
{
	FILE *fp;

	fp = fopen("mysql-iotserver.txt", "r");
	if (fp == NULL)
	  unix_error("mysql-iotserver.txt file does not open in clientGet.c.\n");

	fscanf(fp, "%s", host);
	fscanf(fp, "%s", user);
	fscanf(fp, "%s", password);
	fscanf(fp, "%s", db);
	fclose(fp);
}

void finish_with_error(MYSQL *conn) 
{
    fprintf(stderr, "%s\n", mysql_error(conn));
    mysql_close(conn);
    exit(1);
}

void make_query(MYSQL *conn, const char *query)
{
	if (mysql_query(conn, query)) {
		finish_with_error(conn);
	}
}

void warnings()
{
	// connect DB
	// print all warnings
	// delete all warnings

	char host[256];
	char user[256];
	char password[256];
	char db[256];
	char query[MAXLINE];

	MYSQL *conn;
	MYSQL_ROW row;

	conn = mysql_init(NULL);
	if (conn == NULL) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		exit(1);
	}

	getargs_database(host, user, password, db);

    if (mysql_real_connect(conn, host, user, password, db, 0, NULL, 0) == NULL) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        mysql_close(conn);
        exit(1);
    }

	// print from newest data.
	sprintf(query, "SELECT * FROM warnings ORDER BY time DESC;");
	make_query(conn, query);

	MYSQL_RES *result;
	result = mysql_store_result(conn);
	if (result == NULL) {
		finish_with_error(conn);
	}

	int num_fields = mysql_num_fields(result);
	while ((row = mysql_fetch_row(result))) {
		for (int i =0; i < num_fields; ++i) {
			printf("%s\t", row[i] ? row[i] : "NULL");
		}
		printf("\n");
	}

	mysql_free_result(result);
	mysql_close(conn);
}

void console(char hostname[], int port, char webaddr[])
{
	const char *delim = " \r\n";
	char input[MAXLINE];
	char ToSend[MAXLINE];
	int cli_number;
	int quit = 0;

	while (!quit) {
		char *option = NULL;
		char *option2 = NULL;
		int N = 1;

		printf(">> ");
		fgets(input, MAXLINE, stdin);
		input[strlen(input) - 1] = '\0';

		if (strlen(input) == 0) continue;

		// copy without '\0'
		memset(ToSend, 0, MAXLINE);
		strncpy(ToSend, webaddr, strlen(webaddr));

		cli_number = get_command(input);

		if (cli_number == ERROR) {
			puts("No input!!!\n");
			continue;
		}

		if (cli_number == NOT_FOUND) {
			puts("Unrecognized command.\n");
			continue;
		}

		strtok(input, delim);
		option = strtok(NULL, delim);
		option2 = strtok(NULL, delim);

		switch(cli_number) {
			case HELP:
				help();
				continue;
				break;
			case CLI_LIST:
				sprintf(ToSend, "%scommand=LIST", ToSend);
				// cli_list(ToSend);
				break;
			case INFO:
				if (option != NULL) {
					sprintf(ToSend, "%scommand=INFO&value=%s", ToSend, option);
				}
				else {
					printf("Usage : INFO <name>.\n");
					continue;
				}
				break;
			case GET:
					if (option == NULL) {
						printf("Usage : GET <name> <n>.\n");
						continue;
					}
					if (option2 != NULL)
						N = atoi(option2);
					sprintf(ToSend, "%sNAME=%s&N=%d", ToSend, option, N);
				break;
			case WARNINGS:
				// read from database
				warnings();
				break;
			case QUIT: case EXIT:
				quit = 1;
				break;
			default:
				break;
		}

		if (quit) break;
		userTask(hostname, port, ToSend);
	}
}

int main(void)
{
	char hostname[MAXLINE], webaddr[MAXLINE];
	pid_t pid;
	int port;
  
	getargs_cg(hostname, &port, webaddr);

	pid = Fork();
	if (pid == 0) {
		Execve("alarmServer", NULL, NULL);
	}

	console(hostname, port, webaddr);

	// kill alarmserver
	kill(pid, SIGINT);

	// userTask(hostname, port, webaddr);
  
	return(0);
}
