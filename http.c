#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <Python.h>

// Prototypes
void help();
int create_tcp_socket();
char *get_ip(char *host);
char *build_get_query(char *host, char *page);
char *build_head_query(char *host, char *page);

// Defines
#define HOST "localhost"
#define PAGE "/index.php"
#define NAME "httphead"
#define PORT 80
#define USERAGENT "'; DROP TABLE useragents; --"

// Python wrapper
static PyObject* http_head(PyObject* self, PyObject* args)
{

    struct sockaddr_in *remote;
    int sock;
    int tmpres;
    char *ip;
    char *get;
    char buf[BUFSIZ+1]; // BUFSIZ elää stdio.hssa kiltisti. 
    char *host;
    char *page;
    
    // Get host, page from python
    if (!PyArg_ParseTuple(args, "ss", &host, &page))
         return NULL;
 
    // New sock
    sock = create_tcp_socket();
    ip = get_ip(host);
    fprintf(stderr, "IP is %s\n", ip);
    remote = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in *));
    remote->sin_family = AF_INET;
    tmpres = inet_pton(AF_INET, ip, (void *)(&(remote->sin_addr.s_addr)));
    
    if (tmpres < 0)    
    {
        perror("Can't set remote->sin_addr.s_addr");
        exit(1);
    }
    else if (tmpres == 0)
    {
        fprintf(stderr, "%s not a valid IP\n", ip);
        exit(1);
    }
    
    remote->sin_port = htons(PORT);
 
    if(connect(sock, (struct sockaddr *)remote, sizeof(struct sockaddr)) < 0){
        perror("Could not connect");
        exit(1);
    }
    get = build_head_query(host, page);
    fprintf(stderr, "Query is: %s", get);
 
    int sent = 0;
    int str_length = strlen(get);
    
    while(sent < str_length)
    {
        tmpres = send(sock, get+sent, strlen(get)-sent, 0);
        if(tmpres == -1){
            perror("Can't send query");
            exit(1);
        }
        sent += tmpres;
    }
    
    memset(buf, 0, sizeof(buf));

    int htmlstart = 0;
    char * htmlcontent;
    
    while((tmpres = recv(sock, buf, BUFSIZ, 0)) > 0){
        printf("Receiving..\n");
        if(htmlstart == 0)
        {
            // Will failz if \r\n part is split into two messages.
            htmlcontent = strstr(buf, "\r\n\r\n");
            if(htmlcontent != NULL){
                htmlstart = 1;
                htmlcontent += 4;
            }
        }
        else {
            htmlcontent = buf;
        }
        if (htmlstart) {
            fprintf(stdout, htmlcontent);
        }
        memset(buf, 0, tmpres);
    }
    if(tmpres < 0)
    {
        perror("Error receiving data");
    }
    free(get);
    free(remote);
    free(ip);
    close(sock);

    Py_RETURN_NONE;
}

static PyMethodDef HttpGetMethods[] =
{
    {"head", http_head, METH_VARARGS, "HTTP head"},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
inithttpget(void)
{
    (void) Py_InitModule("httpget", HttpGetMethods);
}



// Create a socket 
int create_tcp_socket()
{
    int sock;
    if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        perror("Can't create TCP socket");
        exit(1);
    }
    return sock;
}
 
// Returns user's IP
char *get_ip(char *host)
{
    struct hostent *hent;
    int iplen = 15; //XXX.XXX.XXX.XXX
    char *ip = (char *)malloc(iplen+1);
    memset(ip, 0, iplen+1);
    if((hent = gethostbyname(host)) == NULL)
    {
        herror("Can't get IP");
        exit(1);
    }
    if(inet_ntop(AF_INET, (void *)hent->h_addr_list[0], ip, iplen) == NULL)
    {
        perror("Can't resolve host");
        exit(1);
    }
    return ip;
}

// Usage text
void help()
{
    char *msg = "";
    printf("Usage: ./%s hostname page.\n", NAME);
    printf("Example: ./%s www.reddit.com /\n", NAME);
}

// HEAD /page HTTP/1.0
char *build_head_query(char *host, char *page)
{
    char *query;
    char *headpage = page;
    char *q = "HEAD /%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: %s\r\n\r\n";
    // Removing unneccessary "/"
    if(headpage[0] == '/'){
        headpage = headpage + 1;
    }
    query = (char *)malloc(strlen(host)+strlen(headpage)+strlen(USERAGENT)+strlen(q)-5);
    sprintf(query, q, headpage, host, USERAGENT);
    return query;
}
