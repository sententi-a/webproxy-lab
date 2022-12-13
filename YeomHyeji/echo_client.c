/* for Echo client*/

#include "csapp.h"

/*
 * Opens client socket fd & connects to server's port 
 * reads user's input until it types EOF
 * usage : - <client host name> <server port number>
 */
int main(int argc, char **argv)
{
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    /* Host domain name or port number not given */
    if (argc != 3){
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    host = argv[1];
    port = argv[2];

    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd);

    /* Reads user's input until it gets EOF */
    while(Fgets(buf, MAXLINE, stdin) != NULL){
        Rio_writen(clientfd, buf, strlen(buf));
        Rio_readlineb(&rio, buf, MAXLINE);
        Fputs(buf, stdout);
    }
    
    Close(clientfd);
    exit(0);
}