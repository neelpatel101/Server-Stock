// Neel Patel 16752035
// Hamza Khalique 23076851

#include <stdio.h> 
#include <stdlib.h>
#include <string.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h> 

#define SH_TOK_BUFSIZE 64
#define SH_TOK_DELIM " \t\r\n\a"
#define MAXLINE 220

char *sh_read_line(void)
{
    char *line = NULL;
    size_t bufsize = 0;

    getline(&line, &bufsize, stdin);
    return line;
}

char **sh_split_line(char *line)
{
    int bufsize = SH_TOK_BUFSIZE;
    int position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token, **tokens_backup;

    token = strtok(line, SH_TOK_DELIM);
    while (token != NULL) 
    {
        tokens[position] = token;
        position++;

        token = strtok(NULL, SH_TOK_DELIM);
    }

    tokens[position] = NULL;
    return tokens;
}

int open_clientfd(char *hostname, char *port) 
{
    int clientfd, rc;
    struct addrinfo hints, *listp, *p;

    /* Get a list of potential server addresses */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;  /* Open a connection */
    hints.ai_flags = AI_NUMERICSERV;  /* ... using a numeric port arg. */
    hints.ai_flags |= AI_ADDRCONFIG;  /* Recommended for connections */

    if ((rc = getaddrinfo(hostname, port, &hints, &listp)) != 0) 
    {
        fprintf(stderr, "getaddrinfo failed (%s:%s): %s\n", hostname, port, gai_strerror(rc));
        return -2;
    }
  
    /* Walk the list for one that we can successfully connect to */
    for (p = listp; p; p = p->ai_next) 
    {
        /* Create a socket descriptor */
        if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) 
            continue; /* Socket failed, try the next */

        /* Connect to the server */
        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1) 
            break; /* Success */
        if (close(clientfd) < 0) { /* Connect failed, try another */  //line:netp:openclientfd:closefd
            fprintf(stderr, "open_clientfd: close failed: %s\n", strerror(errno));
            return -1;
        } 
    } 

    /* Clean up */
    freeaddrinfo(listp);
    if (!p) /* All connects failed */
        return -1;
    else    /* The last connect succeeded */
        return clientfd;
}

int main(int argc, char *argv[]) 
{ 
    int clientfd;
    char buffer[MAXLINE];

    char *host, *port;

    if (argc != 3) 
    {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }
    host = argv[1];
    port = argv[2];

    clientfd = open_clientfd(host, port);
    
    while (1)
    {
        char buf[MAXLINE];

        printf("> ");
        fgets(buf, MAXLINE, stdin);
        if (strcmp(buf, "quit\n") == 0)
        {
            break;
        }
        else
        {
            write(clientfd, buf, MAXLINE);
            read(clientfd, buffer, MAXLINE);
            
            if (strcmp(buffer, "empty") != 0)
                printf("%s\n", buffer);
        }
    }

    close(clientfd); //line:netp:echoclient:close
    exit(0);
    return 0;
}