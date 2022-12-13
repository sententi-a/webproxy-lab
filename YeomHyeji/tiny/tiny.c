/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

/*
 * Opens listenfd & waits for client's connection
 */
int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args (./tiny <port num>) */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  printf("Waiting for client's request...\n");

  while (1) {  
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); 
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   
    Close(connfd);  
  }
}

/*
 * Only accpet GET method
 * analyze URI & determine if the request is for static contents or dynamic contents)
 */
void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE]; 
  rio_t rio;

  /* Read request line */
  Rio_readinitb(&rio, fd); //connect rio to fd
  Rio_readlineb(&rio, buf, MAXLINE); //read request line from client and copy it to buf (EX) GET /cgi-bin/adder?1300&4000 HTTP/1.0 ~~~~
  printf("Request headers: \n");
  printf("%s", buf); 
  sscanf(buf, "%s %s %s", method, uri, version); // allocates string in buf respectively to method, uri, version

  /* If http request is not GET, raise 501 error */
  if (strcasecmp(method, "GET")) {
    printf("501");
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  /* Read request headers */
  read_requesthdrs(&rio);

  /* Parse URI from GET request */
  is_static = parse_uri(uri, filename, cgiargs);

  if (stat(filename, &sbuf) < 0){ //stat(filename or path, buf struct that saves file status and info); Returns -1 on error
    printf("404");
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }

  /* Serves static content */
  if (is_static) {
    /* determine if content is executable */
    // S_ISREG : isregular (check if it's general file or not)
    // S_IRUSR : has read authority
    // S_IXUSR : has exec authority 
    // st_mode : file's mode (16bits)
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size);
  }
  
  /* Serves dynamic content*/
  else{
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs);
  }
}


/* Read HTTP request header */
void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE); // ignore request line

  /* read request headers until it meets "\r\n(empty line)" */
  while(strcmp(buf, "\r\n")){
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }

  return;
}

/*
 * Analyzes static content or dynamic content based on uri parameter
 * and copies it to filename and cgiargs
 */
int parse_uri(char *uri, char *filename, char *cgiargs) 
{
  char *ptr;

  /* Static Content */
  if (!strstr(uri, "cgi-bin")) {
    strcpy(cgiargs, "");
    strcpy(filename, "."); 
    strcat(filename, uri);
    if (uri[strlen(uri)-1] == '/') {
      strcat(filename, "home.html"); // (EX) ./home.html
    }
    return 1;
  }

  /* Dynamic Content */
  else {
    ptr = index(uri, '?');

    if (ptr){ // extract cgi argument (EX) 31434&324
      strcpy(cgiargs, ptr+1);
      *ptr = '\0';
    }
    else {
      strcpy(cgiargs, "");
    }

    strcpy(filename, ".");
    strcat(filename, uri); // (EX) ./cgi-bin/adder?324&342
    return 0;
  }
}

/*
 * Send response header(info about which content server will send)
 * Open request file read-only & save file content on VM area
 * Send the file content to client through connfd
 */
void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* Store string(going to be response line & response headers) in buffer buf */
  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);

  /*****Send response headers to client*****/
  // info of content that server sends to client
  Rio_writen(fd, buf, strlen(buf));
  /* Prints Response header on server terminal */
  printf("Response headers: \n");
  printf("%s", buf);

  /*****Send response body to client*****/
  /* Open(filename or path, flags, mode) : Returns file descriptor */
  srcfd = Open(filename, O_RDONLY, 0); // O_RDONLY : open file read-only
  /* mapping request file to vm area */
  // PROT_ : sets memory security mode 
    // PROT_EXEC:executable, PROT_READ:readable, NONE:inaccessible, WRITE:writable
  // flags : 
    // MAP_FIXED:only use designated address, MAP_SHARED:share object with every process, MAP_PRIVATE: doesn't share area with other process
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); 
  Close(srcfd);
  /* send file to client */
  Rio_writen(fd, srcp, filesize); // sends file contents in srcp to client through connfd
  Munmap(srcp, filesize);
}

/*
 * Forks child process that inherits parents' memoryy
 * Set "QUERY_STRING" env variable to cgiargs
 * Sets connfd to STDOUT_FILENO
 * Executes CGI program 
 */
void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = {NULL};
  /* Send HTTP Response line and header to client */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));


  if (Fork() == 0) {
    /* setenv(const char* name, const char* value, int overwrite) : reset env variable 'name' to value */
    setenv("QUERY_STRING", cgiargs, 1);
    /* Dup2(arg1, arg2) : copy arg1 to arg2 */
    Dup2(fd, STDOUT_FILENO); // Redirect stdout to client: parent process's standard output - connfd
    Execve(filename, emptylist, environ); // Load CGI program and run (environ: to use QUERY_STRING)
  }

  Wait(NULL); // Parent process waits child process to be closed 
}

/*
 * Derive file type from filename
 */
void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else 
    strcpy(filetype, "text/plain");
}

/*
 * Sends HTTP response if there's an error
 */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web Server</em>\r\n", body);

  /* Send the HTTP response line&headers to client */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body)); // "\r\n(empty line)" terminates response headers
  Rio_writen(fd, buf, strlen(buf));
  /* Send the HTTP response body */
  Rio_writen(fd, body, strlen(body));
}