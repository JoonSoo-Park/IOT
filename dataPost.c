#include "stems.h"
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <mysql/mysql.h>

void finish_with_error(MYSQL *conn) 
{
    fprintf(stderr, "%s\n", mysql_error(conn));
    mysql_close(conn);
    exit(1);
}

void getargs(char *host, char *user, char *password, char *db)
{
	FILE *fp;

	fp = fopen("mysql-iotserver.txt", "r");
	if (fp == NULL)
		unix_error("mysql-iotserver.txt file does not open.");

	fscanf(fp, "%s", host);
	fscanf(fp, "%s", user);
	fscanf(fp, "%s", password);
	fscanf(fp, "%s", db);
	fclose(fp);
}

void parseCgiargs(char *cgiargs, char *ptr, size_t size)
{
	char buf[MAXLINE];
	char *t;

	strncpy(buf, ptr, size + 1);
	strtok(buf, "=");
	t = strtok(NULL, "");

	strncpy(cgiargs, t, strlen(t) + 1);
}

void make_query(MYSQL *conn, const char *query)
{
	if (mysql_query(conn, query)) {
		finish_with_error(conn);
	}
}

/*
1. check if sensorList exists
2. if not, then create table
3. insert into sensorList
4. check if sensor# table exists
5. if not, then create table sensor#
6. insert into sensor#
7. update cnt and ave
*/
void QUERY(MYSQL *conn, char *sensorName, float time, float value)
{
	int num_fields, id, time_integer = (int)time;
	int count;
	float average;
	char query[MAXLINE];
	MYSQL_RES *result;
	MYSQL_ROW row;

	// create sensorList table if not exists
	sprintf(query, "CREATE TABLE IF NOT EXISTS sensorList"
	"("
	"name VARCHAR(256) NOT NULL,"
	"id INT AUTO_INCREMENT PRIMARY KEY,"
	"cnt INT,"
	"ave FLOAT(6, 2)"
	");");

	make_query(conn, query);

	// check if sensor exists in sensorList
	sprintf(query, "SELECT COUNT(name) FROM sensorList WHERE name = \'%s\';", sensorName);
	make_query(conn, query);

	result = mysql_store_result(conn);
	if (result == NULL) {
		finish_with_error(conn);
	}

	row = mysql_fetch_row(result);
	if (atoi(row[0]) != 0) { // if sensor already exists then just insert values.
		// get sensor's id from sensorList
		sprintf(query, "SELECT id FROM sensorList WHERE name = \'%s\';", sensorName);
		make_query(conn, query);

		result = mysql_store_result(conn);
		if (result == NULL) {
			finish_with_error(conn);
		}

		row = mysql_fetch_row(result);
		id = atoi(row[0]);

		sprintf(query, "INSERT INTO sensor%d(time, value)"
			"VALUE(\'%d\', %f);", id, time_integer, value);
		make_query(conn, query);
	}
	else { // if this sensor is new to sensorList
		sprintf(query, "INSERT INTO sensorList(name, cnt, ave)"
		"VALUE('%s', 1, 0.0);", sensorName);
		make_query(conn, query);

		sprintf(query, "SELECT id FROM sensorList ORDER BY id DESC LIMIT 1;");
		make_query(conn, query);

		result = mysql_store_result(conn);
		if (result == NULL) {
			finish_with_error(conn);
		}

		row = mysql_fetch_row(result);
		id = atoi(row[0]);

		// create sensor# table -> sensor + id
		sprintf(query, "CREATE TABLE IF NOT EXISTS sensor%d"
			"("
			"time INT,"
			"value FLOAT(6, 2),"
			"idx INT AUTO_INCREMENT PRIMARY KEY"
			");", id);
		make_query(conn, query);

		sprintf(query, "INSERT INTO sensor%d(time, value)"
			"VALUE(\'%d\', %f);", id, time_integer, value);
		make_query(conn, query);
	}

	// UPDATE cnt and ave
	sprintf(query, "SELECT COUNT(*), AVG(value) FROM sensor%d;", id);
	make_query(conn, query);

	result = mysql_store_result(conn);
	if (result == NULL) {
		finish_with_error(conn);
	}

	row = mysql_fetch_row(result);
	count = atoi(row[0]);
	average = atof(row[1]);

	sprintf(query, "UPDATE sensorList SET cnt = %d, ave = %f WHERE name = \'%s\';", 
		count, average,sensorName);
	make_query(conn, query);

	mysql_free_result(result);
}

void POST(char *body, size_t size)
{
	MYSQL *conn;

	int position = 0;

	float time, value;

	char host[256];
	char user[256];
	char password[256];
	char db[256];

	char str[MAXLINE];
	char *ptr = NULL;
	char *cgiargs[MAXLINE];

	char args[3][MAXLINE]; // name, time, value

	getargs(host, user, password, db);

	conn = mysql_init(NULL);
	if (conn == NULL) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		return;
	}

	if (mysql_real_connect(conn, host, user, password, db, 0, NULL, 0) == NULL)
	{
        fprintf(stderr, "%s\n", mysql_error(conn));
        mysql_close(conn);
		return;
	}

	strncpy(str, body, size + 1);

	ptr = strtok(str, "&");
	while (ptr != NULL) {
		cgiargs[position++] = ptr;
		ptr = strtok(NULL, "&");
	}

	for (int i = 0; i < position; ++i) {
		parseCgiargs(args[i], cgiargs[i], strlen(cgiargs[i]));
	}

	time = atof(args[1]);
	value = atof(args[2]);

	QUERY(conn, args[0], time, value);

	mysql_close(conn);
}

int main(void)
{
	int bodyLength;
	char body[MAXLINE];
	char *method;

	method = getenv("REQUEST_METHOD");

	if (strcmp(method, "POST") == 0) {
		bodyLength = atoi(getenv("CONTENT_LENGTH"));
		if(bodyLength > 0) 
			read(STDIN_FILENO, body, bodyLength + 1);
	}

	printf("HTTP/1.0 200 OK\r\n");
	printf("Server: My Web Server\r\n");
	printf("Content-Length: %ld\r\n", strlen(body));
	printf("Content-Type: text/plain\r\n\r\n");
	printf("%s\n",body);

	POST(body, strlen(body));

	fflush(stdout);
	return(0);
}
