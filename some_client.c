#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFSIZE 4096
#define PORT 34567
#define ADDR "127.0.0.1"

int main()
{
    pid_t pid;
    int rcvd, msg,len;
    char buffer[BUFSIZE];
    key_t mqid;
    int cl_s,cl_addrlen;
    struct sockaddr_in srv_addr, cl_addr;

    cl_s=socket(PF_INET, SOCK_STREAM,0);
    bzero(&cl_addr,sizeof(srv_addr));
    cl_addr.sin_family=AF_INET;
    cl_addr.sin_port=htons(PORT);
    inet_aton(ADDR, &cl_addr.sin_addr);
    len=sizeof(cl_addr);
    if(connect(cl_s,(struct sockaddr*)&cl_addr,len)){
        perror("Can't connect!!");
        exit(-1);
    };


    memset(buffer,0,BUFSIZE);
    read(0,buffer, BUFSIZE);
    write(cl_s, buffer, BUFSIZE);

    while((rcvd=read(cl_s,buffer,BUFSIZE))>0){
        write(1,buffer,rcvd);
        memset(buffer,0,BUFSIZE);

        read(0,buffer, BUFSIZE);
        write(cl_s, buffer, BUFSIZE);

        memset(buffer,0,BUFSIZE);
    }


    shutdown(cl_s, SHUT_RDWR);
    close(cl_s);
    return 0;
}
