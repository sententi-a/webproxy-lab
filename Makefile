CC = gcc
CFLAGS = -g -Wall -DDEBUG
LDFLAGS = -lpthread

all: proxy

rwqueue.o: rwqueue.c rwqueue.h
	$(CC) $(CFLAGS) -c rwqueue.c

cache.o: cache.c cache.h
	$(CC) $(CFLAGS) -c cache.c

sbuf.o: sbuf.c sbuf.h
	$(CC) $(CFLAGS) -c sbuf.c

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

proxy.o: proxy.c proxy.h csapp.h sbuf.h
	$(CC) $(CFLAGS) -c proxy.c

proxy: proxy.o csapp.o sbuf.o cache.o rwqueue.o
	$(CC) $(CFLAGS) proxy.o csapp.o sbuf.o cache.o rwqueue.o -o proxy $(LDFLAGS)

# Creates a tarball in ../proxylab-handin.tar that you can then
# hand in. DO NOT MODIFY THIS!
handin:
	(make clean; cd ..; tar cvf $(USER)-proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")

clean:
	rm -f *~ *.o proxy core *.tar *.zip *.gzip *.bzip *.gz
