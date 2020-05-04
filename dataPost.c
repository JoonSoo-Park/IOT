#include "stems.h"
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>

//
// This program is intended to help you test your web server.
// 

int main(int argc, char *argv[])
{
	int bodyLength;
	char *method;
	char str[MAXLINE];

	method = getenv("REQUEST_METHOD");

	if (strcmp(method, "POST") == 0) {
		bodyLength = atoi(getenv("CONTENT_LENGTH"));
		if(bodyLength > 0) 
			read(STDIN_FILENO, str, bodyLength + 1);
	}
  
	printf("HTTP/1.0 200 OK\r\n");
	printf("Server: My Web Server\r\n");
	printf("Content-Length: %ld\r\n", strlen(str));
	printf("Content-Type: text/plain\r\n\r\n");
	printf("%s\n",str);
	fflush(stdout);
	return(0);
}
