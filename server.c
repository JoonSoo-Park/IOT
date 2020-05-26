#include "stems.h"
#include "request.h"
#include "./queue.h"
#include <semaphore.h>

// 
// To run:
// 1. Edit config-ws.txt with following contents
//    <port number>
// 2. Run by typing executable file
//    ./server
// Most of the work is done within routines written in request.c
//

queue *q;

sem_t mutex;

void getargs_ws(int *port, int *pool, int *elements)
{
	FILE *fp;

	if ((fp = fopen("config-ws.txt", "r")) == NULL)
		unix_error("config-ws.txt file does not open.");

	fscanf(fp, "%d", port);
	fscanf(fp, "%d", pool);
	fscanf(fp, "%d", elements);
	fclose(fp);
}

void consumer(int connfd, long arrivalTime)
{
	requestHandle(connfd, arrivalTime);
	Close(connfd);
}

void queueAdd(int fd)
{
	sem_wait(&mutex);

	push(q, fd);

	sem_post(&mutex);
}

int queue_get()
{
	sem_wait(&mutex);

	int val = peek(q);
	pop(q);

	sem_post(&mutex);

	return val;
} 

void *threadHandler(void *vargs)
{
	int connfd;
	int *number = (int*)vargs;

	while (1) {
		while (empty(q)) { // wait if queue is empty.
			;
		}
		connfd = queue_get();

   		consumer(connfd, getWatch());
		printf("thread: %d\n", *number);
	}
}


int main(void)
{
	pid_t pid;
	int listenfd, connfd, port, clientlen, pool, elements;
	struct sockaddr_in clientaddr;
	pthread_t threadPool[128];

	initWatch();
	getargs_ws(&port, &pool, &elements);

	// launch alarm client process
	pid = Fork();
	if (pid == 0) {
		Execve("alarmClient", NULL, NULL);
	}

	sem_init(&mutex, 0, 1);
	q = createQueue(elements);
	for (int i = 0; i < pool; ++i) {
		pthread_create(&threadPool[i], NULL, threadHandler, (void*)&i);
	}

	listenfd = Open_listenfd(port);
	while (1) {
		clientlen = sizeof(clientaddr);
		// connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
		queueAdd(Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen));
		// consumer(connfd, getWatch());
	}

	sem_destroy(&mutex);
	return(0);
}