# Makefile for Proxy Lab 
#
# You may modify this file any way you like (except for the handin
# rule). You instructor will type "make" on your specific Makefile to
# build your proxy from sources.

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

all: proxy

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

proxy.o: proxy.c csapp.h
	$(CC) $(CFLAGS) -c proxy.c

proxy: proxy.o csapp.o
	$(CC) $(CFLAGS) proxy.o csapp.o -o proxy $(LDFLAGS)

echo_server.o: echo_server.c csapp.h
	$(CC) $(CFLAGS) -c echo_server.c

echo_client.o: echo_client.c csapp.h
	$(CC) $(CFLAGS) -c echo_client.c 

echoserver: echo_server.o csapp.o
	$(CC) $(CFLAGS) echo_server.o csapp.o -o echoserver $(LDFLAGS)

echoclient: echo_client.o csapp.o
	$(CC) $(CFLAGS) echo_client.o csapp.o -o echoclient $(LDFLAGS)

echoclean:
	rm -f *.o echoclient echoserver


# Creates a tarball in ../proxylab-handin.tar that you can then
# hand in. DO NOT MODIFY THIS!
handin:
	(make clean; cd ..; tar cvf $(USER)-proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")

clean:
	rm -f *~ *.o proxy core *.tar *.zip *.gzip *.bzip *.gz

