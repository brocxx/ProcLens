#include "http_server.h"
#include "api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET Socket;
#define BAD_SOCKET INVALID_SOCKET
#define close_socket closesocket
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
typedef int Socket;
#define BAD_SOCKET (-1)
#define close_socket close
#endif

/* Send every byte, including partial kernel writes; O(bytes). */
static int send_all(Socket socket, const char *data, size_t length)
{
    while (length > 0) {
        int chunk = (int)(length > 16384 ? 16384 : length);
        int sent = (int)send(socket, data, chunk, 0);
        if (sent <= 0) return 0;
        data += sent; length -= (size_t)sent;
    }
    return 1;
}

static const char *reason(int status)
{
    switch (status) {
    case 200: return "OK";
    case 400: return "Bad Request";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 409: return "Conflict";
    case 422: return "Unprocessable Content";
    default: return "Internal Server Error";
    }
}

/* Temporary response streams make Content-Length exact without fixed buffers. */
static void respond(Socket socket,int status,const char *type,FILE *body)
{
    char header[512], buffer[8192];
    long length;
    int header_length;
    size_t read_count;
    if (fflush(body)!=0 || fseek(body,0,SEEK_END)!=0) return;
    length=ftell(body);
    if (length<0 || fseek(body,0,SEEK_SET)!=0) return;
    header_length=snprintf(header,sizeof(header),
        "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %ld\r\n"
        "Cache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\n"
        "Referrer-Policy: no-referrer\r\nConnection: close\r\n\r\n",status,reason(status),type,length);
    if (header_length<0 || (size_t)header_length>=sizeof(header)) return;
    if (!send_all(socket,header,(size_t)header_length)) return;
    while ((read_count=fread(buffer,1,sizeof(buffer),body))>0)
        if (!send_all(socket,buffer,read_count)) break;
}

/* Fixed allowlist: no filesystem path is derived from untrusted input. */
static FILE *static_file(const char *path,const char **type)
{
    if (strcmp(path,"/")==0 || strcmp(path,"/index.html")==0) {
        *type="text/html; charset=utf-8"; return fopen("ui/index.html","rb");
    }
    if (strcmp(path,"/styles.css")==0) {
        *type="text/css; charset=utf-8"; return fopen("ui/styles.css","rb");
    }
    if (strcmp(path,"/app.js")==0) {
        *type="application/javascript; charset=utf-8"; return fopen("ui/app.js","rb");
    }
    return NULL;
}

/* Socket timeout and a fixed header limit bound incomplete requests. */
static int read_request(Socket socket,char *buffer,size_t size)
{
    size_t used=0;
    while (used+1<size) {
        int received=(int)recv(socket,buffer+used,(int)(size-used-1),0);
        if (received<=0) return 0;
        used+=(size_t)received; buffer[used]=0;
        if (strstr(buffer,"\r\n\r\n")!=NULL) return 1;
    }
    return 0;
}

static int same_header(const char *line,const char *name)
{
    while (*name) {
        char a=*line++, b=*name++;
        if (a>='A' && a<='Z') a=(char)(a-'A'+'a');
        if (b>='A' && b<='Z') b=(char)(b-'A'+'a');
        if (a!=b) return 0;
    }
    return *line==':';
}

/* Host/Origin validation protects the read-only local capture from other sites. */
static int valid_headers(char *request,unsigned short port)
{
    char host[64],origin[80];
    char *line=strstr(request,"\r\n");
    int host_seen=0;
    snprintf(host,sizeof(host),"127.0.0.1:%u",(unsigned int)port);
    snprintf(origin,sizeof(origin),"http://127.0.0.1:%u",(unsigned int)port);
    while (line!=NULL) {
        char *end,*value;
        line+=2; if (*line=='\r') break;
        end=strstr(line,"\r\n"); if (end==NULL) return 0;
        *end=0; value=strchr(line,':'); if (value==NULL) return 0;
        value++; while (*value==' ' || *value=='\t') value++;
        if (same_header(line,"Host")) {
            if (host_seen++ || strcmp(value,host)!=0) return 0;
        }
        if (same_header(line,"Origin") && strcmp(value,origin)!=0) return 0;
        if (same_header(line,"Content-Length") && strcmp(value,"0")!=0) return 0;
        if (same_header(line,"Transfer-Encoding")) return 0;
        *end='\r'; line=end;
    }
    return host_seen==1;
}

static void handle_client(Socket client,ApiState *state,unsigned short port)
{
    char request[8192],method[16],target[1024],version[16],extra;
    const char *type="application/json; charset=utf-8";
    char *query;
    FILE *body=tmpfile();
    int status=400;
    if (body==NULL) return;
    if (!read_request(client,request,sizeof(request)) ||
        sscanf(request,"%15s %1023s %15s%c",method,target,version,&extra)!=4 ||
        strcmp(version,"HTTP/1.1")!=0 || extra!='\r') {
        fputs("{\"error\":\"Malformed or oversized request.\"}",body);
    } else if (!valid_headers(request,port)) {
        status=403; fputs("{\"error\":\"Only same-origin loopback requests without bodies are accepted.\"}",body);
    } else {
        query=strchr(target,'?'); if (query!=NULL) *query++=0; else query=target+strlen(target);
        if (strncmp(target,"/api/",5)==0) status=api_handle(state,method,target,query,body,&type);
        else if (strcmp(method,"GET")!=0) {
            status=405; fputs("{\"error\":\"Static assets require GET.\"}",body);
        } else {
            FILE *asset=static_file(target,&type);
            if (asset!=NULL) { fclose(body); body=asset; status=200; }
            else { status=404; fputs("{\"error\":\"Not found. Start the server from the project folder.\"}",body); }
        }
    }
    respond(client,status,type,body); fclose(body);
}

static void set_timeout(Socket client)
{
#ifdef _WIN32
    DWORD timeout=5000;
#else
    struct timeval timeout={5,0};
#endif
    setsockopt(client,SOL_SOCKET,SO_RCVTIMEO,(const char *)&timeout,sizeof(timeout));
    setsockopt(client,SOL_SOCKET,SO_SNDTIMEO,(const char *)&timeout,sizeof(timeout));
}

/* Single-threaded loopback server owns one session; stop using Ctrl+C. */
int http_server_run(unsigned short port)
{
    Socket server;
    struct sockaddr_in address;
    ApiState state;
#ifdef _WIN32
    WSADATA data;
    if (WSAStartup(MAKEWORD(2,2),&data)!=0) return 0;
#else
    signal(SIGPIPE,SIG_IGN);
#endif
    server=socket(AF_INET,SOCK_STREAM,0);
    if (server==BAD_SOCKET) return 0;
    memset(&address,0,sizeof(address));
    address.sin_family=AF_INET; address.sin_port=htons(port);
    address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    if (bind(server,(struct sockaddr *)&address,sizeof(address))!=0 || listen(server,8)!=0) {
        close_socket(server); return 0;
    }
    api_init(&state);
    printf("ProcLens: http://127.0.0.1:%u\nSingle-CPU simulation, not the real OS scheduler.\n"
           "Read-only: no processes are controlled. Bursts are estimates.\n",(unsigned int)port);
    fflush(stdout);
    for (;;) {
        Socket client=accept(server,NULL,NULL);
        if (client==BAD_SOCKET) break;
        set_timeout(client); handle_client(client,&state,port); close_socket(client);
    }
    api_destroy(&state); close_socket(server);
#ifdef _WIN32
    WSACleanup();
#endif
    return 1;
}
