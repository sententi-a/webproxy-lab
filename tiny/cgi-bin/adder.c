#include "csapp.h"

int main(void) 
{
    char *buf, *p;
    char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
    int n1 = 0, n2 = 0;

    // args
    if ((buf = getenv("QUERY_STRING")) != NULL) 
    {
        p = strchr(buf, '&');
        *p = '\0';

        strcpy(arg1, buf);
        strcpy(arg2, p + 1);

        n1 = atoi(arg1);
        n2 = atoi(arg2);
    }

// Response body
#pragma GCC diagnostic ignored "-Wrestrict"
#pragma GCC diagnostic ignored "-Wformat-overflow="
    sprintf(content, "Welcome to adder.\r\n");
    sprintf(content, "%sWOW. So amazed you came here just to add TWO numbers. Just.. WOW.\r\n<p>", content);
    sprintf(content, "%sThe answer is: %d + %d = %d, and your computer figured that out in like what, 10 nanoseconds?\r\n<p>",
            content, n1, n2, n1 + n2);
    sprintf(content, "%sTHANK YOU SO MUCH. Please use your BRAIN next time when you need to ADD TWO NUMBERS.\r\n", content);
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

    // Generate the HTTP response
    printf("Connection: close\r\n");
    printf("Content-length: %d\r\n", (int)strlen(content));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", content);
    fflush(stdout);

    exit(0);
}
