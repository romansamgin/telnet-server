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

#define BUFSIZE 1024
#define ADDR "127.0.0.1"

void handle_sigchld(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0) 
    {     
    } 
}

int main(int argc, char* argv[])    {
    if (argc != 2) {
        puts("argc != 2");
        return -1;
    }
    
    int pfd[2];
    pid_t pid;
    int rcvd, len, cl_addrlen;
    int srv_s, srv_as;
    struct sockaddr_in srv_addr, cl_addr;
    char buffer[BUFSIZE];
    int PORT = atoi(argv[1]);

    srv_s = socket(PF_INET, SOCK_STREAM, 0);
    if (srv_s < 0) {
        perror("ERROR opening socket");
        exit(0);
    }   
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(PORT);
    inet_aton(ADDR, &srv_addr.sin_addr);
    len = sizeof(srv_addr);

    while (bind(srv_s, (struct sockaddr *)&srv_addr, len) < 0)  {
        puts("Port already in use, please choose another port: ");
        scanf("%i", &PORT);
        srv_addr.sin_port = htons(PORT);
    }

    listen(srv_s, 10);
    puts("Listening...");
    while(srv_as = accept(srv_s, (struct sockaddr*)&cl_addr, &len)){
        printf("Client connected\nIP:\t%s\nPort:\t%i\n", inet_ntoa(cl_addr.sin_addr), cl_addr.sin_port);
        signal(SIGCHLD, &handle_sigchld);
        pid = fork();
        if (pid == 0) {
            while((rcvd = recvfrom(srv_as, buffer, BUFSIZE, 0, (struct sockaddr *)&cl_addr,&cl_addrlen)) > 0){
                buffer[rcvd - 2] = '\0';        // remove \n\r
                char* cmd[] = {"/bin/sh", "-c", buffer, NULL};
                pipe(pfd);
                pid = fork();
                if (pid == -1) {
                    perror("Can't fork");
                    exit(-1);
                }
                else if (pid == 0) {
                    //меняем вывод exec() с stdout в пайп pfd[1], чтобы в родителе прочитать из pfd[0]
                    close(pfd[0]);
                    close(STDOUT_FILENO);
                    close(STDERR_FILENO);
                    dup2(pfd[1], STDOUT_FILENO);
                    dup2(pfd[1], STDERR_FILENO);
                    execvp(cmd[0], cmd);
                    close(pfd[1]);
                    exit(0);
                }
                else if (pid > 0){
                    //Ждем дочерний процесс, в котором был вызов exec 
                    wait(NULL);
                    close(pfd[1]);
                    //Читаем из пайпа результат запроса и посылаем в сокет клиенту 
 		            while((rcvd = read(pfd[0], buffer, BUFSIZE)) > 0){
                        send(srv_as, buffer, rcvd, MSG_NOSIGNAL);
                        memset(buffer, '\0', BUFSIZE);
                    }
                    write(srv_as,"\r", 1);
                    close(pfd[0]);
                }
            }
            close(srv_as);
            exit(0);
        } else if (pid > 0) {
            //Ждем дочерний процесс, чтобы не создавать зомби
            wait(NULL);
            close(srv_as);
        } else {
            perror("fork() failed");
        }
    }

    close(srv_s);
    return 0;
}
