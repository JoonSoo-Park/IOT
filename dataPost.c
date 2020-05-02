#include "stems.h"
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>

//
// This program is intended to help you test your web server.
// 

int main(void)
{
  char *ContentLength;
  char *RestContents;
  char content[MAXLINE + MAXLINE];
  char extra[MAXLINE];
  int length;

  ContentLength = getenv("CONTENT_LENGTH");
  RestContents = getenv("REST_CONTENTS");
  length = atoi(ContentLength);

  strcpy(content, RestContents);

  if (length > strlen(content)) {
    read(STDIN_FILENO, extra, sizeof(extra));
    sprintf(content, "%s%s", content, extra);
  }

  printf("HTTP/1.0 200 OK\r\n");
  printf("Server: My Web Server\r\n");
  printf("Content-Length: %ld\r\n", strlen(content));
  printf("Content-Type: text/plain\r\n\r\n");
  printf("\n%s", content);
  fflush(stdout);
  return(0);
}
