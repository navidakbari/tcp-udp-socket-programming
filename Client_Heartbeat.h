#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <sys/time.h>
#include <stdbool.h>

struct sockaddr_in servaddr;
int socket_id;

void set_get_heartbeat(char *server_heartbeat_port){

    if((socket_id = socket(AF_INET , SOCK_DGRAM , 0)) < 0){
        const char err[] ="ocket creation\n" ;
        write(STDERR_FILENO, err, sizeof(err)-1);
        exit(EXIT_FAILURE);
    }

    u_int yes = 1;
    if(setsockopt(socket_id, SOL_SOCKET, SO_REUSEADDR, (char*) &yes, sizeof(yes)) < 0){
        const char err[] ="Reusing ADDR failed\n" ;
        write(STDERR_FILENO, err, sizeof(err)-1);
        exit(EXIT_FAILURE);
    }

    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(atoi(server_heartbeat_port));

    if((bind(socket_id , (const struct sockaddr *) &servaddr , sizeof(servaddr))) < 0 ){
        const char err[] ="bind failed\n" ;
        write(STDERR_FILENO, err, sizeof(err)-1);
        exit(EXIT_FAILURE); 
    }

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr("239.255.255.250");
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    
    if (setsockopt(socket_id, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq)) < 0){
        const char err[] ="setsockopt: mreq\n" ;
        write(STDERR_FILENO, err, sizeof(err)-1);
        exit(EXIT_FAILURE);
    }

    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    if (setsockopt(socket_id, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        const char err[] ="setsockopt: timeval\n" ;
        write(STDERR_FILENO, err, sizeof(err)-1);
    }

    const char set_heartbeat_msg[] ="Client Heartbeat receiving seted...\n" ;
    write(STDOUT_FILENO, set_heartbeat_msg, sizeof(set_heartbeat_msg)-1);
}

bool get_heartbeat(char* message){
    char msg[100];

    if(recvfrom(socket_id, msg, sizeof(msg), 0,NULL , 0) > 0){
        int i;
        for(i = 0 ; i <  strlen(msg) ; ++i)
            message[i] = msg[i];
        
        const char get_heartbeat[] ="Server is on...\n" ;
        write(STDOUT_FILENO, get_heartbeat, sizeof(get_heartbeat)-1);
        
        return true;
    }

    const char no_heartbeat[] ="Server is off...\n" ;
    write(STDOUT_FILENO, no_heartbeat, sizeof(no_heartbeat)-1);
    return false;

}