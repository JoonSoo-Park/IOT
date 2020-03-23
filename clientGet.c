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
}

/* currently, there is no loop. I will add loop later */
void userTask(char hostname[], int port, char webaddr[])
{
  int clientfd;

  clientfd = Open_clientfd(hostname, port);
  clientSend(clientfd, webaddr);
  clientPrint(clientfd);
  Close(clientfd);
}

void getargs_cg(char hostname[], int *port, char webaddr[])
{
  FILE *fp;

  fp = fopen("config-cg.txt", "r");
  if (fp == NULL)
    unix_error("config-cg.txt file does not open.");

  fscanf(fp, "%s", hostname);
  fscanf(fp, "%d", port);
  fscanf(fp, "%s", webaddr);
  fclose(fp);
}

int main(void)
{
  char hostname[MAXLINE], webaddr[MAXLINE];
  int port;
  
  getargs_cg(hostname, &port, webaddr);

  userTask(hostname, port, webaddr);
  
  return(0);
}
