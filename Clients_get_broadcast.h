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

#define BUFSIZE 1024

struct sockaddr_in servaddr;
int socket_id;

void set_get_broadcast(char *client_broadcast_port , bool flag){

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
    servaddr.sin_port = htons(atoi(client_broadcast_port));

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
    if(flag){
        struct timeval tv;
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        if (setsockopt(socket_id, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
            const char err[] ="setsockopt: timeval\n" ;
            write(STDERR_FILENO, err, sizeof(err)-1);
        }
    }

    const char set_heartbeat_msg[] ="Client broadcasting getting seted...\n" ;
    write(STDOUT_FILENO, set_heartbeat_msg, sizeof(set_heartbeat_msg)-1);
}

char* get_broadcast_msg(){
    char *temp1 = (char *)malloc(sizeof(char)* BUFSIZE);
    char recv_buf[BUFSIZE];

    if(recvfrom(socket_id, recv_buf, sizeof(recv_buf), 0,NULL , 0) > 0){       
        strcpy(temp1 , recv_buf);
        return temp1;
    }
    return NULL;
}

void close_broadcast_port(){
    const char set_heartbeat_msg[] ="End broadcasting...\n" ;
    write(STDOUT_FILENO, set_heartbeat_msg, sizeof(set_heartbeat_msg)-1);
    close( socket_id);
}