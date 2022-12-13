/* for echo server */

#include "csapp.h"

void echo(int connfd);

/*
 * Opens listen fd & accepts client's connection request
 * waits client's request and responds to it
 * usage : - <server port num>
 */
int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr; 
    char client_hostname[MAXLINE], client_port[MAXLINE];
    
    /* No port num given */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
    }

    /* Opens listenfd & binds the fd to server ip address */
    char *port = argv[1];
    listenfd = Open_listenfd(port);

    /* Waits for requests of client */
    while(1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        /* Prints connected client's domain name & port number */
        Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        echo(connfd);
        Close(connfd);
    }
    exit(0);
}

/* Reads client's request and responds to it with exactly the same word */
void echo(int connfd)
{
    size_t n;
    char buf[MAXLINE];
    rio_t rio;
    
    /* until client types EOF */
    Rio_readinitb(&rio, connfd);
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        printf("server received %d bytes\n", (int)n);
        Rio_writen(connfd, buf, n);
    }
}