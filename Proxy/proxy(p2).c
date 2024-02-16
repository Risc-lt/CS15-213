#include <stdio.h>
#include <stdlib.h>

#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* Max Length predefined*/
#define METHOD_MAX_LEN 7          // Method
#define HOST_MAX_LEN 2048         // Host
#define PORT_MAX_LEN 6            // Port
#define PATH_MAX_LINE 2048        // Path
#define VERSION_MAX_LEN 10        // HTTP version
#define HEADER_NAME_MAX_LEN 20    // Name of request header
#define HEADER_VALUE_MAX_LEN 200  // Content of request header
#define HEADER_MAX_NUM 20         // Number of request

#define THREAD_NUM 4              // Number of thread
#define SBUFSIZE 16               // Size of buffer

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

typedef struct {
    char method[METHOD_MAX_LEN];    
    char host[HOST_MAX_LEN];        
    char port[PORT_MAX_LEN];    // Client port        
    char path[PATH_MAX_LINE];       
    char version[VERSION_MAX_LEN];  
} Request_Line;

typedef struct {
    char name[HEADER_NAME_MAX_LEN];    
    char value[HEADER_VALUE_MAX_LEN];  
} Request_Header;

typedef struct {
    Request_Line request_line;                       
    Request_Header request_headers[HEADER_MAX_NUM];  
    int request_header_num;                          
} Request;

typedef struct {
    int *buf;    /* Buffer array */
    int n;       /* Maximun number of slots */
    int front;   /* buf[(front + 1) % n] is the first item */
    int rear;    /* buf[rear % n] is the last item */
    sem_t mutex; /* Protects access to buf */
    sem_t slots; /* Counts available slots */
    sem_t items; /* Counts available items */
} sbuf_t;

void read_request(int fd, Request *request);
void read_request_line(rio_t *rp, char *buf, Request_Line *request_line);
void read_request_header(rio_t *rp, char *buf, Request_Header *request_header);
void parse_uri(char *uri, char *host, char *port, char *path);
void doit(int fd);
int send_request(Request *request);
void forward_response(int connfd, int targetfd);
void sigpipe_handler(int sig);
void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);
void *thread(void *vargp);

sbuf_t sbuf; /* Shared buffer of connected descriptors */

int main(int argc, char **argv) {
    int listenfd, connfd;  // Server listening and connection file descriptor
    char hostname[MAXLINE], port[MAXLINE];  // Clinet
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  // socket address

    pthread_t tid;

    /* Check command-line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    Signal(SIGPIPE, sigpipe_handler); 

    listenfd = Open_listenfd(argv[1]);  // start listen

    sbuf_init(&sbuf, SBUFSIZE);
    for (int i = 0; i < THREAD_NUM; i++) {  // create working threads
        Pthread_create(&tid, NULL, thread, NULL);
    }

    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(
            listenfd, (SA *)&clientaddr,
            &clientlen);  // change listenfd to connfd and store information in clientaddr
        Getnameinfo(
            (SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
            0);  // parse lientaddr for host and port
        printf("Accepted connection from (%s, %s)\n", hostname,
               port);                
        sbuf_insert(&sbuf, connfd);  // Add connection to buffer
    }
}

void *thread(void *vargp) {
    Pthread_detach(Pthread_self());  // detach every connected thread
    while (1) {
        int connfd = sbuf_remove(&sbuf);  // Get connection from buffer
        doit(connfd);                     // handle connection
        Close(connfd);                    
    }
}

void sigpipe_handler(int sig) {
    fprintf(stderr, "Error: Connection reset by peer!\n");
}

/* Create an empty, bounded, shared FIFO buffer with n slots */
void sbuf_init(sbuf_t *sp, int n) {
    sp->buf = Calloc(n, sizeof(int));
    sp->n = n;                  /* Buffer holds max of n items */
    sp->front = sp->rear = 0;   /* Empty buffer iff front == rear */
    Sem_init(&sp->mutex, 0, 1); /* Binary semaphore for locking */
    Sem_init(&sp->slots, 0, n); /* Initially, buf has n empty slots */
    Sem_init(&sp->items, 0, 0); /* Initially, buf has zero data items */
}

/* Clean up buffer sp */
void sbuf_deinit(sbuf_t *sp) { Free(sp->buf); }

/* Insert item onto the rear of shared buffer sp */
void sbuf_insert(sbuf_t *sp, int item) {
    P(&sp->slots);                          /* Wait for available slot */
    P(&sp->mutex);                          /* Lock the buffer */
    sp->buf[(++sp->rear) % (sp->n)] = item; /* Insert the item */
    V(&sp->mutex);                          /* Unlock the buffer */
    V(&sp->items);                          /* Announce available item */
}

/* Remove and return the first item from buffer sp */
int sbuf_remove(sbuf_t *sp) {
    int item;
    P(&sp->items);                           /* Wait for available item */
    P(&sp->mutex);                           /* Lock the buffer */
    item = sp->buf[(++sp->front) % (sp->n)]; /* Remove the item */
    V(&sp->mutex);                           /* Unlock the buffer */
    V(&sp->slots);                           /* Announce available slot */
    return item;
}

void read_request(int fd, Request *request) {
    char buf[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, fd);    // connect the descriptor fd with a read buffer at address rp
    read_request_line(&rio, buf, &request->request_line); 
    Rio_readlineb(&rio, buf, MAXLINE);

    Request_Header *header = request->request_headers;
    request->request_header_num = 0;

    while (strcmp(buf, "\r\n")) {  // Loop to read request header
        read_request_header(&rio, buf, header++);
        request->request_header_num++;
        Rio_readlineb(&rio, buf, MAXLINE);
    }
}

void read_request_line(rio_t *rp, char *buf, Request_Line *request_line) {
    char uri[MAXLINE];
    Rio_readlineb(rp, buf, MAXLINE);
    printf("Request: %s\n", buf);

    if (sscanf(buf, "%s %s %s", request_line->method, uri,
               request_line->version) <
        3) {  // divide the request line by space and store the data
        fprintf(stderr, "Error: invalid request line!\n");
        exit(1);
    }
    if (strcasecmp(request_line->method, "GET")) { 
        fprintf(stderr, "Method not implemented!\n");
        exit(1);
    }
    if (strcasecmp(request_line->version, "HTTP/1.0") &&
        strcasecmp(request_line->version, "HTTP/1.1")) {  // check the version of HTTP
        fprintf(stderr, "HTTP version not recognized!\n");
        exit(1);
    }
    parse_uri(uri, request_line->host, request_line->port,
              request_line->path); 
}

void read_request_header(rio_t *rp, char *buf, Request_Header *request_header) {
    Rio_readlineb(rp, buf, MAXLINE);

    char *c = strstr(buf, ": ");  // divide the buf by ":" to get name and value
    if (!c) {                    
        fprintf(stderr, "Error: invalid header: %s", buf);
        exit(1);
    }

    *c = '\0';
    strcpy(request_header->name, buf); 
    strcpy(request_header->value, c + 2);
}

void parse_uri(char *uri, char *host, char *port, char *path) {
    char *path_start, *port_start;

    if (strstr(uri, "http://") != uri) {
        fprintf(stderr, "Error: invalid uri!\n");
        exit(1);
    }

    uri += strlen("http://");   // get the address of start of the request
    if ((port_start = strstr(uri, ":")) == NULL) {
        strcpy(port, "80");
    } else {
        *port_start = '\0';
        strcpy(host, uri);
        uri = port_start + 1;
    }

    if ((path_start = strstr(uri, "/")) == NULL) {
        strcpy(path, "/");
    } else {
        strcpy(path, path_start);
        *path_start = '\0';
    }

    if (port_start) {
        strcpy(port, uri);
    } else {
        strcpy(host, uri);
    }
}

int send_request(Request *request) {
    int clientfd;
    char content[MAXLINE];
    Request_Line *request_line = &request->request_line;

    clientfd = Open_clientfd(request_line->host,
                             request_line->port);  // Initiate a new connection to the target server
    rio_t rio;

    // print the request header and write it to the socket of client
    Rio_readinitb(&rio, clientfd);
    sprintf(content, "%s %s HTTP/1.0\r\n", request_line->method,
            request_line->path);
    Rio_writen(clientfd, content, strlen(content));
    sprintf(content, "Host: %s:%s\r\n", request_line->host,
            request_line->port);
    Rio_writen(clientfd, content, strlen(content));
    sprintf(content, "User-Agent: %s\r\n", user_agent_hdr);  
    Rio_writen(clientfd, content, strlen(content));
    sprintf(content, "Connection: close\r\n");  
    Rio_writen(clientfd, content, strlen(content));
    sprintf(content, "Proxy-Connection: close\r\n");  
    Rio_writen(clientfd, content, strlen(content));

    for (int i = 0; i < request->request_header_num;
         i++) {  // Read and send other request headers 
        char *name = request->request_headers[i].name;
        char *value = request->request_headers[i].value;
        if (!strcasecmp(name, "Host") || !strcasecmp(name, "User-Agent") ||
            !strcasecmp(name, "Connection") ||
            !strcasecmp(name, "Proxy-Connection")) {
            continue;
        }
        sprintf(content, "%s%s: %s\r\n", content, name, value);
        Rio_writen(clientfd, content, strlen(content));
    }
    Rio_writen(clientfd, "\r\n",
               2 * sizeof(char));  // empty line terminates headers
    return clientfd;
}

void forward_response(int connfd, int targetfd) {
    rio_t rio;
    int n;
    char buf[MAXLINE];
    Rio_readinitb(&rio, targetfd);
    while (
        (n = Rio_readlineb(&rio, buf, MAXLINE))) {  // Read one line from target host response 
        Rio_writen(connfd, buf, n);  // Write the line to the connection with the client
    }
}

void doit(int fd) {
    Request request;
    read_request(fd, &request);                 // receive client request
    int clientfd = send_request(&request);      // sent connection request to target host
    forward_response(fd, clientfd);    // forward target's response to client
    Close(clientfd);               
}