#
# To compile, type "make" or make "all"
# To remove files, type "make clean"
#
OBJS = mysql_example.o server.o request.o stems.o clientGet.o clientPost.o alarmClient.o alarmServer.o alarm.o
TARGET = server

CC = gcc
CFLAGS = -g -Wall
CONFIGC = `mysql_config --cflags --libs`

LIBS = -lpthread 
MYSQLLIBS = -lmysqlclient

.SUFFIXES: .c .o 

all: mysql_example server clientPost clientGet dataGet.cgi dataPost.cgi alarm.cgi alarmClient alarmServer

mysql_example: mysql_example.o
	$(CC) $(CFLAGS) -o mysql_example mysql_example.o $(CONFIGC)

server: server.o request.o stems.o
	$(CC) $(CFLAGS) -o server server.o request.o stems.o $(LIBS)

clientGet: clientGet.o stems.o
	$(CC) $(CFLAGS) -o clientGet clientGet.o stems.o $(CONFIGC)

clientPost: clientPost.o stems.o
	$(CC) $(CFLAGS) -o clientPost clientPost.o stems.o $(LIBS)

dataGet.cgi: dataGet.c stems.h
	$(CC) $(CFLAGS) -o dataGet.cgi dataGet.c stems.o $(CONFIGC)

dataPost.cgi: dataPost.c stems.h
	$(CC) $(CFLAGS) -o dataPost.cgi dataPost.c stems.o $(CONFIGC)

alarmServer: alarmServer.o request.o stems.o
	$(CC) $(CFLAGS) -o alarmServer alarmServer.o request.o stems.o $(LIBS)

alarmClient: alarmClient.o stems.o
	$(CC) $(CFLAGS) -o alarmClient alarmClient.o stems.o $(CONFIGC)

alarm.cgi: alarm.c stems.h
	$(CC) $(CFLAGS) -o alarm.cgi alarm.c stems.o

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

server.o: stems.h request.h
clientGet.o: stems.h
clientPost.o: stems.h

clean:
	-rm -f $(OBJS) mysql_example server clientPost clientGet dataGet.cgi dataPost.cgi alarm.cgi alarmClient alarmServer alarm_fifo
