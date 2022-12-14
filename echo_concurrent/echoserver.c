#include "csapp.h"

void echo(int connfd)
{
    size_t n;
    char buf[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, connfd);
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0)
    {
        printf("Server received %d bytes.\n", (int)n);
        Rio_writen(connfd, buf, n);
    }
}

void *thread(void *vargp)
{
  int connfd = *((int *)vargp);

  Pthread_detach(pthread_self());

  Free(vargp);

  echo(connfd);

  Close(connfd);

  return NULL;
}

int main(int argc, char **argv)
{
    int listenfd, *connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    listenfd = Open_listenfd(argv[1]);
    
    while (1)
    {
        clientlen = sizeof(struct sockaddr_storage);
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        
        Pthread_create(&tid, NULL, thread, connfdp);
    }
    exit(0);
}

