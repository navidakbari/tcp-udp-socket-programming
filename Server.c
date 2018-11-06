#include <stdio.h>
#include <sys/socket.h>
#include <signal.h>
#include <fcntl.h>
#include <resolv.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>    
#include <errno.h> 
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros  
#include "Server_Heartbeat.h"

#define SERVER_PORT 9000
#define SERVER_IP "127.0.0.1"

#define BUFSIZE 1024
#define NUM_OF_CLIENT 20

struct client{
    char* port;
    char* IP;
    char* username;
    char* who_to_play;
    bool online;
    int socketfd;
    bool ans_request;
};

void send_to_client(struct client *clients_data, fd_set *master){

    int i , j, num_of_wating = 0 , index_wating = 0;
    bool first = true;

    for(i = NUM_OF_CLIENT - 1  ; i >= 0 ; i--){
        if(clients_data[i].online){
            for(j = 0 ; j < NUM_OF_CLIENT && i!=j ; j++){
                if(clients_data[j].online && (strcmp(clients_data[i].who_to_play , clients_data[j].username)==0 || strcmp(clients_data[j].who_to_play , clients_data[i].username)==0)){
                    clients_data[i].online = false;
                    clients_data[j].online = false;

                    char send_buf[BUFSIZE];

                    strcpy(send_buf , clients_data[j].IP);
                    strcat(send_buf , " ");
                    strcat(send_buf , clients_data[j].port);
                    strcat(send_buf , " ");
                    strcat(send_buf , clients_data[j].username);
                    strcat(send_buf , "\0");

                    if (FD_ISSET(clients_data[i].socketfd, master))
                        if (send(clients_data[i].socketfd, send_buf, strlen(send_buf), 0) < 0){
                            perror("send");
                        }
                    clients_data[j].ans_request = true;
                    return;
                }
            }
            
            if(strcmp(clients_data[i].who_to_play , "nobody") != 0){
                clients_data[i].ans_request = true;
                char msg[] = "wait for friend\0";
                if (FD_ISSET(clients_data[i].socketfd, master))
                    if (send(clients_data[i].socketfd, msg, strlen(msg), 0) < 0){
                        perror("send");
                    }
                return;
            }

            if(clients_data[i].online && strcmp(clients_data[i].who_to_play , "nobody") == 0){
                for(j = 0 ; j < NUM_OF_CLIENT && j!=i ; j++){
                    if(clients_data[j].online && strcmp(clients_data[j].who_to_play , "nobody") == 0){
                        char send_buf[BUFSIZE];
                        clients_data[i].online = false;
                        clients_data[j].online = false;
                        strcpy(send_buf , clients_data[j].IP);
                        strcat(send_buf , " ");
                        strcat(send_buf , clients_data[j].port);
                        strcat(send_buf , " ");
                        strcat(send_buf , clients_data[j].username);
                        strcat(send_buf , "\n");
                        clients_data[i].ans_request = true;
                        if (FD_ISSET(clients_data[i].socketfd, master))
                            if (send(clients_data[i].socketfd, send_buf, strlen(send_buf), 0) < 0){
                                perror("send");
                            }
                        return;
                    }
                }
                char msg[] = "wait\0";
                if (FD_ISSET(clients_data[i].socketfd, master))
                    if (send(clients_data[i].socketfd, msg, strlen(msg), 0) < 0){
                        perror("send");
                    }

                return;
            }     
        }
    }
}
		
void receive_from_clients(int i, fd_set *master, int sockfd, int fdmax, struct client *temp , struct client *clients_data){
	int nbytes_recvd;
	char recv_buf[BUFSIZE] = " ";
	if ((nbytes_recvd = recv(i, recv_buf, BUFSIZE, 0)) <= 0) {
        int j;
        for(j = 0 ; j < NUM_OF_CLIENT ; j++)
            if(clients_data[j].socketfd == i)
            {
                char send_buf[BUFSIZE];
                strcpy(send_buf , "user: ");
                strcat(send_buf , clients_data[j].username);
                strcat(send_buf , " is logged out");
                strcat(send_buf , "\n");
                write(STDERR_FILENO, send_buf, strlen(send_buf));
                clients_data[j].IP = NULL;
                clients_data[j].port = NULL;
                clients_data[j].username = NULL;
                clients_data[j].who_to_play = NULL;
                clients_data[j].online = false;
                clients_data[j].socketfd = -1;
            }
		close(i);
		FD_CLR(i, master);
    }else{
		char *temp1 = (char *)malloc(sizeof(char)* BUFSIZE);
        strcpy(temp1 , recv_buf);
        temp->online = true;
        temp->socketfd = i;
        temp->IP = strtok(temp1, " ");
        temp->port = strtok(NULL , " ");
        temp->username = strtok(NULL , " ");
        temp->who_to_play = strtok(NULL , " ");

        printf("client data is : %s\n", recv_buf);
	}	
}
		
void connection_accept(fd_set *master, int *fdmax, int sockfd, struct sockaddr_in *client_addr){
	int newsockfd;
	socklen_t addrlen;
	addrlen = sizeof(struct sockaddr_in);

	if((newsockfd = accept(sockfd, (struct sockaddr *)client_addr, &addrlen)) == -1) {
		const char err[] ="server accept is not working\n" ;
        write(STDERR_FILENO, err, sizeof(err)-1);
        exit(1);
	}else {
		FD_SET(newsockfd, master);
		if(newsockfd > *fdmax){
			*fdmax = newsockfd;
		}
		printf("new connection from %s on port %d \n",inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
	}
}

void setup_server(int *sockfd, struct sockaddr_in *my_addr){		
	if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        const char err[] ="server socket is not working\n" ;
        write(STDERR_FILENO, err, sizeof(err)-1);
        exit(1);
	}
		
	my_addr->sin_family = AF_INET;
	my_addr->sin_port = htons(SERVER_PORT);
	my_addr->sin_addr.s_addr = inet_addr(SERVER_IP);
	memset(my_addr->sin_zero, ' ', sizeof my_addr->sin_zero);
	
    int yes = 1;	
	if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        const char err[] ="server setsocketopt is not working\n" ;
        write(STDERR_FILENO, err, sizeof(err)-1);
        exit(1);
	}
		
	if (bind(*sockfd, (struct sockaddr *)my_addr, sizeof(struct sockaddr)) == -1) {
		const char err[] ="server unable to bind\n" ;
        write(STDERR_FILENO, err, sizeof(err)-1);
        exit(1);
	}

	if (listen(*sockfd, 10) == -1) {
        const char err[] ="server can not listen\n" ;
        write(STDERR_FILENO, err, sizeof(err)-1);
        exit(1);
	}

    const char server_msg[] ="Server is Listening on port 9000\n" ;
    write(STDOUT_FILENO, server_msg, sizeof(server_msg)-1);
}

int main(int argc , char* argv[]){

    if(argc != 3){
        const char err[] ="Usage:./Server <HEARTBEAT PORT> <CLIENT BROADCAST PORT>\n" ;
        write(STDERR_FILENO, err, sizeof(err)-1);
        exit(1);
    }

    char *heartbeat_port = argv[1];
    char *client_broadcast_port = argv[2];

    //for handeling 2 process -> 1- sending heartbeat signal 2- connecting to clients
    pid_t pid = fork();
    
    if(pid == 0){
        struct client *clients_data =(struct client*)malloc(sizeof(struct client) * NUM_OF_CLIENT);
        
        int j;
        for(j = 0 ; j < 10 ; j++)
        {
            clients_data[j].IP = NULL;
            clients_data[j].port = NULL;
            clients_data[j].username = NULL;
            clients_data[j].who_to_play = NULL;
            clients_data[j].online = false;
            clients_data[j].socketfd = -1;
            clients_data[j].ans_request = false;
        }

        int sockfd = 0;
        struct sockaddr_in my_addr, client_addr;
        fd_set master , read_fds;
        
        FD_ZERO(&master);
        FD_ZERO(&read_fds);
        setup_server(&sockfd, &my_addr);
        FD_SET(sockfd, &master);
        
        int fdmax, i;
        fdmax = sockfd;
        struct client *temp = (struct client*) malloc(sizeof(struct client));
        while(1){
            read_fds = master;
            if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){
                const char err[] ="server select is not working\n" ;
                write(STDERR_FILENO, err, sizeof(err)-1);
                exit(4);
            }
            for (i = 0; i <= fdmax; i++){
                if (FD_ISSET(i, &read_fds)){
                    if (i == sockfd)
                        connection_accept(&master, &fdmax, sockfd, &client_addr);
                    else{
                        temp->IP = NULL;
                        temp->port = NULL;
                        temp->username = NULL;
                        temp->who_to_play = NULL;
                        temp->online = false;
                        temp->socketfd = -1;
                        receive_from_clients(i, &master, sockfd, fdmax , temp , clients_data);
                        
                        for(j = 0 ; j < NUM_OF_CLIENT ; j++)
                            if(clients_data[j].port == NULL){
                                clients_data[j] = *temp;
                                break;
                            } 
                        send_to_client(clients_data , &master);
                    }
                }
            }
        }
    }else{
        //handeling heartbeat sending from server
        set_send_heartbeat(heartbeat_port ,"127.0.0.1 9000\0");
        const char send_heartbeat_msg[] ="Hearbeat is sending....\n" ;
        write(STDOUT_FILENO, send_heartbeat_msg, sizeof(send_heartbeat_msg)-1);
        send_heartbeat();
        while(1){}      
    }
    return 0;
}