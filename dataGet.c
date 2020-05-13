#include "stems.h"
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>
#include <mysql/mysql.h>

//
// This program is intended to help you test your web server.
// You can use it to test that you are correctly having multiple 
// threads handling http requests.
//
// htmlReturn() is used if client program is a general web client
// program like Google Chrome. textReturn() is used for a client
// program in a embedded system.
//
// Standalone test:
// # export QUERY_STRING="name=temperature&time=3003.2&value=33.0"
// # ./dataGet.cgi

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

void seperate(char *dest[], char *ptr, const char *delim)
{
	int position = 0;
	char *temp;

	temp = strsep(&ptr, delim);
	while (temp != NULL) {
		dest[position++] = temp;
		temp = strsep(&ptr, delim);
	}
}

void htmlReturn(void)
{
	char content[MAXLINE];
	char *buf;
	char *ptr;

	/* Make the response body */
	sprintf(content, "%s<html>\r\n<head>\r\n", content);
	sprintf(content, "%s<title>CGI test result</title>\r\n", content);
	sprintf(content, "%s</head>\r\n", content);
	sprintf(content, "%s<body>\r\n", content);
	sprintf(content, "%s<h2>Welcome to the CGI program</h2>\r\n", content);
	buf = getenv("QUERY_STRING");
	sprintf(content,"%s<p>Env : %s</p>\r\n", content, buf);
	ptr = strsep(&buf, "&");
	while (ptr != NULL){
	  sprintf(content, "%s%s\r\n", content, ptr);
	  ptr = strsep(&buf, "&");
	}
	sprintf(content, "%s</body>\r\n</html>\r\n", content);

	/* Generate the HTTP response */
	printf("Content-Length: %ld\r\n", strlen(content));
	printf("Content-Type: text/html\r\n\r\n");
	printf("%s", content);

	fflush(stdout);
}

void textReturn(MYSQL *conn, char *args[])
{
	MYSQL_RES *result; 
	MYSQL_ROW row;

	char content[MAXLINE];
	char Query[MAXLINE];
	char *command = NULL;
	char *query = NULL;
	char *option = NULL;

	command = strtok(args[0], "=");
	query = strtok(NULL, "=");

	memset(Query, 0, MAXLINE);

	if (strcmp(command, "command") == 0) {
		if (strcmp(query, "LIST") == 0) {
			sprintf(Query, "SELECT name FROM sensorList;");

			make_query(conn, Query);

			result = mysql_store_result(conn);
			if (result == NULL) {
				finish_with_error(conn);
			}

			sprintf(content, "%s<sensors name>\n", content);
			while ((row = mysql_fetch_row(result)))
			{
				sprintf(content, "%s%s\t", content, row[0]);
			}
		}
		else if (strcmp(query, "INFO") == 0) {
			strtok(args[1], "=");
			option = strtok(NULL, "=");

			sprintf(Query, "SELECT name, cnt, ave FROM sensorList WHERE name = \'%s\';", option);

			make_query(conn, Query);

			result = mysql_store_result(conn);
			if (result == NULL) {
				finish_with_error(conn);
			}

			row = mysql_fetch_row(result);
			sprintf(content, "%sname : %s\n", content, row[0]);
			sprintf(content, "%scount : %s\n", content, row[1]);
			sprintf(content, "%saverage : %s\n", content, row[2]);
		}
	}
	else if (strcmp(command, "NAME") == 0) {
		int N = 1, id;
		time_t raw_time;

		strtok(args[1], "=");
		option = strtok(NULL, "=");
		N = atoi(option);

		sprintf(Query, "SELECT id FROM sensorList WHERE name = \'%s\';", query);	

		make_query(conn, Query);

		result = mysql_store_result(conn);
		if (result == NULL) {
			finish_with_error(conn);
		}

		row = mysql_fetch_row(result);
		id = atoi(row[0]);

		sprintf(Query, "SELECT time, value FROM sensor%d "
			"ORDER BY time DESC LIMIT %d;", id, N);

		make_query(conn, Query);

		result = mysql_store_result(conn);
		if (result == NULL) {
			finish_with_error(conn);
		}

		while ((row = mysql_fetch_row(result))) {
			raw_time = atoi(row[0]);
			sprintf(content, "%stime : %s", content, ctime(&raw_time));
			sprintf(content, "%svalue : %s\n", content, row[1]);
		}
	}

	/* Generate the HTTP response */
	printf("%s", content);
	fflush(stdout);

	mysql_free_result(result);
}

int main(void)
{
	MYSQL *conn = mysql_init(NULL);

	char *cgiargs = getenv("QUERY_STRING");
	char *args[10] = {NULL};

	seperate(args, cgiargs, "&");

	if (mysql_real_connect(conn, "localhost", "iotserver", "password", "server", 0, NULL, 0) == NULL) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		mysql_close(conn);
		exit(1);
	}

	textReturn(conn, args);

	// to use htmlReturn, use clientPrint() in clientGet.c
	// htmlReturn();

	// to use textReturn, use clientPrintText() in clientGet.c
	// textReturn();

	mysql_close(conn);
	return(0);
}


/*
void textReturn(void)
{
	char content[MAXLINE];
	char *buf;
	char *ptr;

	buf = getenv("QUERY_STRING");
	sprintf(content,"%sEnv : %s\n", content, buf);
	ptr = strsep(&buf, "&");
	while (ptr != NULL){
	  sprintf(content, "%s%s\n", content, ptr);
	  ptr = strsep(&buf, "&");
	}
  
	// Generate the HTTP response
	printf("Content-Length: %ld\n", strlen(content));
	printf("Content-Type: text/plain\r\n\r\n");
	printf("%s", content);
	fflush(stdout);
}
*/