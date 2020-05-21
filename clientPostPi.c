/*
 * clientPost.c: A very, very primitive HTTP client for sensor
 * 
 * To run, prepare config-cp.txt and try: 
 *      ./clientPost
 *
 * Sends one HTTP request to the specified HTTP server.
 * Get the HTTP response.
 */

#include <wiringPi.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/time.h>
#include "stems.h"

#define MAXTIMINGS 83
#define DHTPIN 7
#define MAX_COUNT 100000

struct timeval timeValue;

int dht11_dat[5] = {0, } ;
int c = 0;

void initWatch() {
	gettimeofday(&timeValue, NULL);
}

int read_dht11_dat()
{
	uint8_t laststate = HIGH ;
	uint8_t counter = 0 ;
	uint8_t j = 0, i ;

	dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0 ;

	printf("read_dht11_dat() %d\n", c++ % MAX_COUNT);
	pinMode(DHTPIN, OUTPUT) ;
	digitalWrite(DHTPIN, LOW) ;
	delay(18) ;
	digitalWrite(DHTPIN, HIGH) ;
	delayMicroseconds(30) ;
	pinMode(DHTPIN, INPUT) ;
	for (i = 0; i < MAXTIMINGS; i++) {
		counter = 0 ;
		while ( digitalRead(DHTPIN) == laststate) {
			counter++ ;
			delayMicroseconds(1) ;
			if (counter == 200) break ;
		}
		laststate = digitalRead(DHTPIN) ;
		if (counter == 200) break ; // if while breaked by timer, break for
		if ((i >= 4) && (i % 2 == 0)) {
			dht11_dat[j / 8] <<= 1 ;
			if (counter > 20) dht11_dat[j / 8] |= 1 ;
			j++ ;
		}
	}
	if ((j >= 40) && (dht11_dat[4] == ((dht11_dat[0] + dht11_dat[1] + dht11_dat[2] +
	dht11_dat[3]) & 0xff))) {
		initWatch();
		printf("humidity = %d.%d %% Temperature = %d.%d *C \n", dht11_dat[0],
		dht11_dat[1], dht11_dat[2], dht11_dat[3]) ;
		c = 0;
		return 1;
	}
	else {
		// printf("Data get failed\n") ;
		return 0;
	}
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

	printf("%s", buf);
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

void getargs_cp(char *hostname, int *port, char *filename, int *delay)
{
  	FILE *fp;

  	fp = fopen("config-pi.txt", "r");
  	if (fp == NULL)
  	  unix_error("config-cp.txt file does not open.");

  	fscanf(fp, "%s", hostname); 	// ip address
  	fscanf(fp, "%d", port);		// port
  	fscanf(fp, "%s", filename);	// filename
  	fscanf(fp, "%d", delay);	// threshold
  	fclose(fp);
}


/* currently, there is no loop. I will add loop later */
void userTask(char *myname, char *hostname, int port, char *filename, double time, char* value)
{
  	int clientfd;
  	char msg[MAXLINE];

  	sprintf(msg, "name=%s&time=%lf&value=%s", myname, time, value);
  	clientfd = Open_clientfd(hostname, port);
	clientSend(clientfd, filename, msg);
  	// clientPrint(clientfd);
  	Close(clientfd);
}

int main(void)
{
	char hostname[MAXLINE], myname[MAXLINE], filename[MAXLINE], buf[MAXLINE];
	int port, delayMilli, res;
	double time;

	// myname: sensor name
	// hostname: IP address

	getargs_cp(hostname, &port, filename, &delayMilli);

	if (wiringPiSetup() == -1) exit(1) ;

	// TODO:
	// 1. read_dht_dat()
	// 2. send temperature -> dht11_dat[0].dht11_dat[1]
	// 3. send humidity -> dht11_dat[2].dht11_dat[3]
	
	while (1) {
		res = read_dht11_dat();
		if (res > 0) {
			time = (double)(timeValue.tv_sec);
			sprintf(buf, "%d.%d", dht11_dat[2], dht11_dat[3]);
			userTask("temperaturePi", hostname, port, filename, time, buf);

			delay(delayMilli / 2);

			sprintf(buf, "%d.%d", dht11_dat[0], dht11_dat[1]);
			userTask("humidityPi", hostname, port, filename, time, buf);

			delay(delayMilli / 2);
		}
		else {
			delay(delayMilli);
		}
	}
  
	return(0);
}
