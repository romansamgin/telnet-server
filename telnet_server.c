#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#define BUFSIZE 4096
#define ARGSSIZE 4
#define PORT 34567 
#define ADDR "127.0.0.1"


void handle_sigchld(int sig) {
  while (waitpid(-1, NULL, WNOHANG) > 0) {}
}


int main()
{
    pid_t pid;
    int pfd[2];
    int rcvd, msg,len;
    char buffer[BUFSIZE];
    memset(buffer,0,BUFSIZE);
    key_t mqid;
    int srv_s,cl_s,srv_as,cl_addrlen;
    struct sockaddr_in srv_addr, cl_addr;

    srv_s=socket(PF_INET,SOCK_STREAM,0);
    if (srv_s < 0)
        perror("ERROR opening socket");

    if (setsockopt(srv_s, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

#ifdef SO_REUSEPORT
    if (setsockopt(srv_s, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEPORT) failed");
#endif

    bzero(&srv_addr,sizeof(srv_addr));
    srv_addr.sin_family=AF_INET;
    srv_addr.sin_port=htons(PORT);
    inet_aton(ADDR, &srv_addr.sin_addr);
   
    if(bind(srv_s,(struct sockaddr*)&srv_addr,sizeof(srv_addr))){
        perror("bind() failed");
        exit(0);
    };

    listen(srv_s, 10);

    while(srv_as=accept(srv_s,(struct sockaddr*)&cl_addr,&len)){
        pid = fork();

        signal(SIGCHLD,&handle_sigchld);

        if (pid==0) {
            while((rcvd=recvfrom(srv_as,buffer,BUFSIZE,0,(struct sockaddr *)&cl_addr,&cl_addrlen))>0){
                buffer[rcvd - 2] = '\0';
                char ** args = (char**)malloc(ARGSSIZE * sizeof(char*));
                args[0] = "/bin/sh";
                args[1] = "-c";
                args[2] = buffer;
                args[3] = NULL;

                pipe(pfd);
                pid=fork();

                if (pid == -1) {
                    perror("Can't fork");
                    exit(-1);
                }
                else
                if(pid>0){
                    wait(NULL);
                    close(pfd[1]);
                    puts("Received:");
                    memset(buffer,0,BUFSIZE);
 		    while((rcvd=read(pfd[0],buffer,BUFSIZE))>0){
                        write(1,buffer,rcvd);
                        send(srv_as,buffer,rcvd, MSG_NOSIGNAL);
                        memset(buffer,0,BUFSIZE);
                    }

                    write(srv_as,"\r", 1);

                    puts("");
                    close(pfd[0]);
                }
                else {
                    close(pfd[0]);
                    close(STDOUT_FILENO);
                    close(STDERR_FILENO);
                    dup2(pfd[1], STDOUT_FILENO);
                    dup2(pfd[1], STDERR_FILENO);
                    execvp(args[0],args);
                    close(pfd[1]);
                    exit(0);
                }

            }
            close(srv_as);
            exit(0);
        } else if (pid>0) {
            //wait(NULL);
            //waitpid(-1, NULL, WNOHANG);
            close(srv_as);
        } else {
            perror("fork() failed");
        }
    }

    //Cleanup
    shutdown(srv_s,SHUT_RDWR);
    close(srv_s);
    return 0;
}
