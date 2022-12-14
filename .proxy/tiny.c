#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

int main(int argc, char **argv) 
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    // Check command line args
    if (argc != 2) 
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);

    while (1) 
    {
        clientlen = sizeof(clientaddr);

        // line:netp:tiny:accept
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);  

        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);

        printf("Accepted connection from (%s, %s)\n", hostname, port);

        // line:netp:tiny:doit
        doit(connfd);

        // line:netp:tiny:close
        Close(connfd);
  }
}

void doit(int fd)
{
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

    // Read request line and headers
    Rio_readinitb(&rio, fd);

    // line:netp:doit:readrequest
    if (!Rio_readlineb(&rio, buf, MAXLINE)) { return; } 
        
    printf("%s", buf);

    // line:netp:doit:parserequest
    sscanf(buf, "%s %s %s", method, uri, version);
    
    // line:netp:doit:beginrequesterr
    if (strcasecmp(method, "GET"))
    {
        clienterror(fd, 
                    method, 
                    "501", 
                    "Not Implemented",
                    "Tiny does not implement this method");
        return;
    }                       
    // line:netp:doit:endrequesterr

    // line:netp:doit:readrequesthdrs
    read_requesthdrs(&rio); 

    // Parse URI from GET request
    // line:netp:doit:staticcheck
    is_static = parse_uri(uri, filename, cgiargs);

    // line:netp:doit:beginnotfound
    if (stat(filename, &sbuf) < 0) 
    {
        clienterror(fd, 
                    filename, 
                    "404", 
                    "Not found",
                    "Tiny couldn't find this file");
        return;
    }
    // line:netp:doit:endnotfound

    // Serve static content
    if (is_static) 
    {
        // line:netp:doit:readable
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) 
        {
            clienterror(fd,
                        filename,
                        "403",
                        "Forbidden",
                        "Tiny couldn't read the file");
            return;
        }

        // line:netp:doit:servestatic
        serve_static(fd, filename, sbuf.st_size);
    }
    // Serve dynamic content
    else
    {
        // line:netp:doit:executable
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
        {
            clienterror(fd, 
                        filename, 
                        "403", 
                        "Forbidden",
                        "Tiny couldn't run the CGI program");
            return;
        }

        // line:netp:doit:servedynamic
        serve_dynamic(fd, filename, cgiargs); 
    }
}

void read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);

    printf("%s", buf);

    // line:netp:readhdrs:checkterm
    while (strcmp(buf, "\r\n")) 
    {
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }

    return;
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
    char *ptr;

    // Static content
    // line:netp:parseuri:isstatic
    if (!strstr(uri, "cgi-bin")) 
    {
        // line:netp:parseuri:clearcgi
        strcpy(cgiargs, "");

        // line:netp:parseuri:beginconvert1
        strcpy(filename, ".");
        // line:netp:parseuri:endconvert1
        strcat(filename, uri);

        // line:netp:parseuri:slashcheck
        if (uri[strlen(uri) - 1] == '/')
        {
            // line:netp:parseuri:appenddefault
            strcat(filename, "home.html");
        }
            
        return 1;
    }
    else 
    {
        // Dynamic content
        // line:netp:parseuri:isdynamic
        // line:netp:parseuri:beginextract
        ptr = index(uri, '?');
        if (ptr) 
        {
            strcpy(cgiargs, ptr + 1);
            *ptr = '\0';
        } 
        else
        {
            // line:netp:parseuri:endextract
            strcpy(cgiargs, "");

            // line:netp:parseuri:beginconvert2
            strcpy(filename, ".");
            // line:netp:parseuri:endconvert2
            strcat(filename, uri);   

            return 0;
        }
    }
}

void serve_static(int fd, char *filename, int filesize)
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];

    // Send response headers to client
    // line:netp:servestatic:getfiletype
    get_filetype(filename, filetype);

    // line:netp:servestatic:beginserve
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));

    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    sprintf(buf, "Content-length: %d\r\n", filesize);
    Rio_writen(fd, buf, strlen(buf));

    sprintf(buf, "Content-type: %s\r\n\r\n", filetype);
    Rio_writen(fd, buf, strlen(buf)); 
    // line:netp:servestatic:endserve

    //////////////////////////////////////////////

    // Send response body to client
    // line:netp:servestatic:open
    srcfd = Open(filename, O_RDONLY, 0);

    // line:netp:servestatic:mmap
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);

    // line:netp:servestatic:close
    Close(srcfd);                      

    // line:netp:servestatic:write
    Rio_writen(fd, srcp, filesize);     

    // line:netp:servestatic:munmap
    Munmap(srcp, filesize);                                     
}

void get_filetype(char *filename, char *filetype)
{
    if (strstr(filename, ".html"))     { strcpy(filetype, "text/html"); }
    else if (strstr(filename, ".gif")) { strcpy(filetype, "image/gif"); }
    else if (strstr(filename, ".png")) { strcpy(filetype, "image/png"); }
    else if (strstr(filename, ".jpg")) { strcpy(filetype, "image/jpeg"); }
    else                               { strcpy(filetype, "text/plain"); }
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
    char buf[MAXLINE], *emptylist[] = {NULL};

    // Return first part of HTTP response
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));

    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    // Child 
    // line:netp:servedynamic:fork
    if (Fork() == 0) 
    {
        // line:netp:servedynamic:setenv
        setenv("QUERY_STRING", cgiargs, 1);

        // Redirect stdout to client
        // line:netp:servedynamic:dup2
        Dup2(fd, STDOUT_FILENO);

        // Run CGI program
        // line:netp:servedynamic:execve
        Execve(filename, emptylist, environ);
    }
    // Parent waits for and reaps child
    // line:netp:servedynamic:wait
    Wait(NULL);
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char buf[MAXLINE];

    // Print the HTTP response headers
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));

    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    ///////////////////////////////////////////////

    // Print the HTTP response body
    sprintf(buf, "<html><title>Tiny Error</title>");
    Rio_writen(fd, buf, strlen(buf));

    sprintf(buf, "<body bgcolor=""ffffff"">\r\n");
    Rio_writen(fd, buf, strlen(buf));

    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));

    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));

    sprintf(buf, "<hr><em>The Krafton Jungle Tiny Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}
