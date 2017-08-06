#include <stdio.h>
#include "csapp.h"
#include "sbuf.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define NTHREADS 4
#define SBUFSIZE 16

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void doit(int fd, int id);
void read_requesthdrs(rio_t *rp, char* buf);
int parserequest(int fd, char* buf, char* method, char* host, char* port, char* uri, char* version);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void *thread(void* ti);
int reader(char* buf, int id);
void writer(char* tmp, int id);
void initialize_cache();
void pipehandler(int sig);

sbuf_t sbuf;
int readcnt;
sem_t mutex, w;
char *cache;
char *object_cache[NTHREADS];

int main(int argc, char**argv)
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;
    int *id[NTHREADS];

    signal(SIGPIPE,pipehandler);

    readcnt = 0;
    Sem_init(&mutex, 0, 1);
    Sem_init(&w, 0, 1);

    /* Check command line args */
    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(1);
    }
    listenfd = Open_listenfd(argv[1]);

    sbuf_init(&sbuf,SBUFSIZE);
    for (int i = 0; i < NTHREADS; ++i)
    {
    	id[i] = Malloc(sizeof(int));
    	*(id[i]) = i;
    	Pthread_create(&tid,NULL,thread,(void*)id[i]);
    }

    initialize_cache();
    strcpy(cache,"");

    while (1) {
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //line:netp:tiny:accept
	    Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
	                    port, MAXLINE, 0);
	    printf("Accepted connection from (%s, %s)\n", hostname, port);
		sbuf_insert(&sbuf, connfd);
    }
    return 0;
}

void pipehandler(int sig) {
	return;
}

void initialize_cache() {
	cache = Malloc(MAX_CACHE_SIZE);
	for(int i = 0; i < MAX_CACHE_SIZE/MAX_OBJECT_SIZE; i++) {
		*(int*)(cache+i*MAX_OBJECT_SIZE) = 0;
		*(int*)(cache+i*MAX_OBJECT_SIZE+4) = 0;
	}
}

void *thread(void* ti) {
	int id = *(int*)ti;
	object_cache[id] = Malloc(MAX_OBJECT_SIZE);
	Pthread_detach(pthread_self());
	while(1){
		int connfd = sbuf_remove(&sbuf);
		doit(connfd, id);
		Close(connfd);
	}
}
void doit(int fd, int id) 
{
    char buf[MAXLINE*10] = {};
    char info_buf[MAX_OBJECT_SIZE] = {};
    char method[MAXLINE] = {};
    char uri[MAXLINE] = {};
    char version[MAXLINE] = {};
    char port[MAXLINE] = {};
    char host[MAXLINE] = {};
    char tmp[MAXLINE*10] = {};
    rio_t rio;
    rio_t clientrio;
    int clientfd;
    int overflowflag = 0;
    int hitflag = 0;
    int len;

    strcpy(object_cache[id],"");
    /* Read request line and headers */
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE))  //line:netp:doit:readrequest
        return;
    if(!parserequest(fd,buf,method,host,port,uri,version))       //line:netp:doit:parserequest
    	return;                                               //line:netp:doit:endrequesterr
    
    //reader
    strcpy(tmp,buf);
    hitflag = reader(buf,id);
    read_requesthdrs(&rio,buf);                              //line:netp:doit:readrequesthdrs
    if (hitflag)
    {
    	Rio_writen(fd,object_cache[id],strlen(object_cache[id]));
    	return;
    }

    clientfd = Open_clientfd(host,port); // connect to server
    Rio_readinitb(&clientrio,clientfd);   // write request to server
    Rio_writen(clientfd,buf,strlen(buf)); // 

    while(len = Rio_readlineb(&clientrio,info_buf,MAX_OBJECT_SIZE)) { //read server reply and write to client
    	if(strlen(object_cache[id]) + strlen(info_buf) <= MAX_OBJECT_SIZE)
    		strcat(object_cache[id],info_buf);
    	else
    		overflowflag = 1;
    	Rio_writen(fd,info_buf,len);
    }
    Close(clientfd);
    //writer
    if(!overflowflag) writer(tmp, id);
}

int reader(char* buf, int id) {
	int flag = 0;
	P(&mutex);
	readcnt++;
	if (readcnt == 1)
		P(&w);
	V(&mutex);
	for (int i = 0; i < MAX_CACHE_SIZE/MAX_OBJECT_SIZE; ++i)
	{
		if (*(int*)(cache+i*MAX_OBJECT_SIZE) == 1)
		{
			*(int*)(cache+i*MAX_OBJECT_SIZE+4) += 1;
			if(strstr((cache+i*MAX_OBJECT_SIZE+8),buf)) {
				strcpy(object_cache[id],cache+i*MAX_OBJECT_SIZE+8+strlen(buf));
				*(int*)(cache+i*MAX_OBJECT_SIZE+4) = 0;
				flag = 1;
				break;
			}
		}
	}

	P(&mutex);
	readcnt--;
	if (readcnt == 0)
		V(&w);
	V(&mutex);
	printf("read flag  %d\n", flag);
	return flag;
}

void writer(char* tmp, int id) {
	int max = 0;
	int ch = 0;
	int cnt = 0;
	P(&w);
	for (int i = 0; i < MAX_CACHE_SIZE/MAX_OBJECT_SIZE; ++i)
	{
		if(*(int*)(cache+i*MAX_OBJECT_SIZE) == 0) {
			ch = i;
			break;
		}
		cnt = *(int*)(cache+i*MAX_OBJECT_SIZE+4);
		cnt += 1;
		if(cnt > max) {
			max = cnt;
			ch = i;
		}
	}

	strcpy(cache+ch*MAX_OBJECT_SIZE+8,tmp);
	strcpy(cache+ch*MAX_OBJECT_SIZE+8+strlen(tmp),object_cache[id]);
	*(int*)(cache+ch*MAX_OBJECT_SIZE+4) = 0;
	*(int*)(cache+ch*MAX_OBJECT_SIZE) = 1;
	V(&w);
	printf("write in %d\n", ch);
}
/*
 * read_requesthdrs - read HTTP request headers
 */
/* $begin read_requesthdrs */
void read_requesthdrs(rio_t *rp, char* buf) 
{
	char tmp[MAXLINE] = {};

    while(strcmp(buf+strlen(buf)-3, "\n\r\n")) {          //line:netp:readhdrs:checkterm
		Rio_readlineb(rp, tmp, MAXLINE);
	    if(!strstr(tmp,"User-Agent") && !strstr(tmp,"Host") && !strstr(tmp,"Connection") && !strstr(tmp,"Proxy-Connection"))
	    	strcat(buf,tmp);
    }
    printf("%s", buf);
    return;
}
/* $end read_requesthdrs */

int parserequest(int fd, char* buf, char* method,char* host, char* port, char* uri, char* version) {
	char url[MAXLINE];
	char tmp[MAXLINE];
	char Ho[MAXLINE] = "Host: ";
	char Co[MAXLINE] = "Connection: close\r\n";
	char Pr[MAXLINE] = "Proxy-Connection: close\r\n";
	sscanf(buf, "%s %s %s",method,url,version);
	if (strcasecmp(method, "GET")) {                     //line:netp:doit:beginrequesterr
        clienterror(fd, method, "501", "Not Implemented",
                    "proxy does not implement this method");
        return 0;
    }
    if (!strstr(version, "HTTP/")) {                     //line:netp:doit:beginrequesterr
        clienterror(fd, version, "502", "Wrong Version",
                    "You should input right http version");
        return 0;
    }
    sscanf(url, "http://%s",tmp);
    if(strstr(tmp,":"))
    	sscanf(tmp,"%[^:]:%[0-9]%s",host,port,uri);
    else
    	sscanf(tmp,"%[^/]%s",host,uri);
    if (strlen(host) == 0)
    {
    	clienterror(fd, host, "503", "Wrong Hostname",
                    "You should input right hostname");
    	return 0;
    }
    if(strlen(uri) == 0) strcpy(uri,"/");
    if(strlen(port) == 0) strcpy(port,"80");

    strcat(Ho, host);
    strcat(Ho, ":");
    strcat(Ho, port);
    strcat(Ho, "\r\n");
    strcpy(buf, "GET ");
    strcat(buf, uri);
    strcat(buf, " HTTP/1.0\r\n");
    strcat(buf, user_agent_hdr);
    strcat(buf, Ho);
    strcat(buf, Co);
    strcat(buf, Pr);
    // printf("%s\n", buf);
    // printf("%s\n", host);
    // printf("%s\n", port);
    // printf("%s\n", uri);
    return 1;
}

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>proxy Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Proxy</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
/* $end clienterror */