#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";
static const char *request_line_fmt = "GET %s HTTP/1.0\r\n";
static const char *host_hdr_fmt = "Host: %s\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_hdr = "Proxy-Connection: close\r\n";
static const char *end_of_hdr = "\r\n";

static const char *connection_key = "Connection";
static const char *user_agent_key = "User-Agent";
static const char *proxy_connection_key = "Proxy-Connection";
static const char *host_key = "Host";

/* 
 * 1. proxy server의 소켓은 두 개 (하나는 클라이언트와 연결, 다른 하나는 서버와 연결)
 * 2. client에서 온 요청을 읽고, parsing한 다음 그 request를 서버로 send
 * 3. server에서 온 요청을 읽고, 그 응답을 그대로 클라이언트에게 forward
*/ 
void doit(int fd);
void parse_uri(char *uri, char *hostname, char *path, int *port); // parse client's request line & determine endserver's hostname, path, port
void make_http_header(char *http_hdr, char *hostname, char *path, int port, rio_t *clinet_rio); // makes http header to endserver
int connect_endserver(char *hostname, int port, char *http_header);


/* connects to client */
int main(int argc, char **argv) {
  int listenfd, connfd;
  socklen_t client_socklen;
  struct sockaddr_storage client_sockaddr; // same as socketaddr_in
  char client_hostname[MAXLINE], service_port[MAXLINE];

  /* Usage: ./proxy port_num */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>", argv[0]);
    exit(1);
  }

  /* open listenfd & wait for client's connection */
  listenfd = Open_listenfd(argv[1]);
  while(1) {
    client_socklen = sizeof(client_sockaddr); // 28bytes
    connfd = Accept(listenfd, (SA *)&client_sockaddr, &client_socklen);

    /* Connection Accepted (w/ client)*/
    Getnameinfo((SA *)&client_sockaddr, client_socklen, client_hostname, MAXLINE, service_port, MAXLINE, 0); //sockadd struct -> corresponding host & service name 
    printf("Accepted connection from (%s, %s)", client_hostname, service_port);

    /* Sequential proxy handling */
    doit(connfd);
    Close(connfd);
  }
  return 0;
}

/* Handle client HTTP transaction */
void doit(int fd)
{
  int endserver_fd, endserver_port;
  char endserver_http_header[MAXLINE];
  char endserver_hostname[MAXLINE], endserver_path[MAXLINE];

  char buf[MAXLINE];
  /* variables for storing request line arguments*/
  char method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  
  rio_t client_rio, server_rio;

  /* Read Client's HTTP Request */
  Rio_readinitb(&client_rio, fd);
  Rio_readlineb(&client_rio, buf, MAXLINE);
  sscanf(buf, "%s %s %s", method, uri, version); // (EX) GET http://www.naver.com:80/ HTTP/1.0

  /* Other requests type except GET are strictly optional */
  if (strcasecmp(method, "GET")) {
    printf("Proxy doesn't implement the method.");
    return;
  }
  /* Parse the URI(from client) & determine endserver's hostname, path, port*/
  parse_uri(uri, endserver_hostname, endserver_path, &endserver_port);

  /* Build the http header that will be sent to the endserver*/
  make_http_header(endserver_http_header, endserver_hostname, endserver_path, endserver_port, &client_rio);

  /* Connect to the end server*/
  endserver_fd = connect_endserver(endserver_hostname, endserver_port, endserver_http_header);
  if (endserver_fd < 0) {
    printf("Connection to the server failed...\n");
    return;
  }

  /* Connect server_rio to endserver_fd */
  Rio_readinitb(&server_rio, endserver_fd);
  /* Send HTTP request to the endserver*/
  Rio_writen(endserver_fd, endserver_http_header, strlen(endserver_http_header));
  
  /* Read HTTP response from endserver & sends it to client */
  size_t n;
  while ((n = Rio_readlineb(&server_rio, buf, MAXLINE)) != 0) {
    printf("Proxy received %d bytes, now sending to client...\n", n);
    Rio_writen(fd, buf, n);
  }
  Close(endserver_fd);
}

/* Parse the URI to get hostname, file path and portnum */
void parse_uri(char *uri, char *hostname, char *path, int *port)
{
  *port = 80;

  char *ptr1 = strstr(uri, "//");
  ptr1 = ptr1 != NULL? ptr1+2:uri; // if uri doesn't include "//", just points to the start of www.~ 
  
  char *ptr2 = strstr(ptr1, ":");
  if (ptr2 != NULL){
    *ptr2 = '\0';
    sscanf(ptr1, "%s", hostname); // (EX) hostname="www.naver.com"
    sscanf(ptr2+1, "%d%s", port, path); //(EX) port=80 path="/webtoon/3432&42525"
  }
  /* if there's no portnum in client's request (EX) www.naver.com/webtoon/234&24 or www.naver.com */
  else {
    ptr2 = strstr(ptr1, "/");
    /* if there's filepath in client's request (EX) www.naver.com/webtoon/234&234 */ 
    if (ptr2 != NULL) {
      *ptr2 = '\0';
      sscanf(ptr1, "%s", hostname); // (EX) hostname="www.naver.com"
      *ptr2 = '/';
      sscanf(ptr2, "%s", path); // (EX) port=NULL path="/webtoon/3234&45435"
    }
    /* if there's no filepath in client's request (EX) www.naver.com */
    else {
      sscanf(ptr1, "%s", hostname);
    }
  }
  return;
}

/* Make Http request header (to end server)*/
void make_http_header(char *http_hdr, char *hostname, char *path, int port, rio_t *client_rio)
{
  char buf[MAXLINE], request_line[MAXLINE], other_hdrs[MAXLINE], host_hdr[MAXLINE];

  /* Request line */
  sprintf(request_line, request_line_fmt, path);

  /* Request headers */
  while(Rio_readlineb(client_rio, buf, MAXLINE) > 0) {

    if (strcmp(buf, end_of_hdr) == 0) break; // when it meets EOF 

    if (!strncasecmp(buf, host_key, strlen(host_key))) {
      strcpy(host_hdr, buf);
      continue;
    }

    if (!strncasecmp(buf, connection_key, strlen(connection_key)) && !strncasecmp(buf, proxy_connection_key, strlen(proxy_connection_key)) && !strncasecmp(buf, user_agent_key, strlen(user_agent_key))) {
      strcat(other_hdrs, buf);
    }
  }

  /* if there's no host header in client's request */ 
  if (strlen(host_hdr) == 0) {
    sprintf(host_hdr, host_hdr_fmt, hostname);
  }

  /* make new request line&headers */
  sprintf(http_hdr, "%s%s%s%s%s%s%s", request_line, host_hdr, connection_hdr, proxy_hdr, user_agent_hdr, other_hdrs, end_of_hdr);

  return;
}

/* Connect to the end server */
inline int connect_endserver(char *hostname, int port, char *http_header)
{
  char endserver_port[100];
  sprintf(endserver_port, "%d", port);
  return Open_clientfd(hostname, endserver_port);
}