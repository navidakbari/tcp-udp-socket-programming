#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>

struct sockaddr_in clientaddr;
int socket_id;
char *msg;

void set_send_broadcast(char *port , char* message){
    msg = message;
    if((socket_id = socket(AF_INET , SOCK_DGRAM , 0)) < 0){
        const char err[] ="socket creation\n" ;
        write(STDERR_FILENO, err, sizeof(err)-1);
        exit(EXIT_FAILURE);
    }

    clientaddr.sin_family = AF_INET; // IPv4
    clientaddr.sin_addr.s_addr = inet_addr("239.255.255.250");
    clientaddr.sin_port = htons(atoi(port));
    
    const char set_heartbeat_msg[] = "Client broadcasting sending seted...\n";
    write(STDOUT_FILENO, set_heartbeat_msg, sizeof(set_heartbeat_msg)-1);

}

void broadcasting(int k){
    if(sendto(socket_id , msg , strlen(msg) , 0 , (const struct sockaddr *) &clientaddr , sizeof(clientaddr)) < 0){
        const char err[] ="can't send on port\n" ;
        write(STDERR_FILENO, err, sizeof(err)-1);
        exit(EXIT_FAILURE);
    }
    
    signal(SIGALRM, broadcasting);
    alarm(1);
}