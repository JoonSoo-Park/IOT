#include "stems.h"
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>

//
// This program is intended to help you test your web server.
// 

int main(int argc, char *argv[])
{
  char *astr = "Currently, CGI program is running, but argument passing is not implemented.";
  
  printf("HTTP/1.0 200 OK\r\n");
  printf("Server: My Web Server\r\n");
  printf("Content-Length: %ld\r\n", strlen(astr));
  printf("Content-Type: text/plain\r\n\r\n");
  printf("%s\n",astr);
  fflush(stdout);
  return(0);
}
