#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <signal.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
#include <fcntl.h>

#include "Client_Heartbeat.h"
#include "Clients_send_broadcast.h"
#include "Clients_get_broadcast.h"

#define BUFSIZE 1024

struct client_data{
    int port;
    char IP[16];
    char* username;
    char* who_to_play;
};

struct rival{
    char *port;
    char *IP;
    char *username;
};

char* toArray(int number){
    char *numberArray = (char *)malloc(sizeof(char)*10);
    sprintf(numberArray,"%d", number);
    return numberArray;
}
		
void send_to_server(int sockfd, struct client_data *client){
	char send_buf[BUFSIZE];

    strcpy(send_buf , client->IP);
    strcat(send_buf , " ");
    strcat(send_buf , toArray(client->port));
    strcat(send_buf , " ");
    strcat(send_buf , client->username);
    strcat(send_buf , " ");
    strcat(send_buf , client->who_to_play);
    strcat(send_buf , "\0");
    
    send(sockfd, send_buf, strlen(send_buf), 0);
}
	
void connect_request(int *sockfd, struct sockaddr_in *server_addr ,char* IP , char* server_port){
	if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Socket");
		exit(1);
	}
	server_addr->sin_family = AF_INET;
	server_addr->sin_port = htons(atoi(server_port));
	server_addr->sin_addr.s_addr = inet_addr(IP);
	memset(server_addr->sin_zero, ' ', sizeof server_addr->sin_zero);

	if(connect(*sockfd, (struct sockaddr *)server_addr, sizeof(struct sockaddr)) == -1) {
		perror("connect");
		exit(1);
	}
}

struct rival* recv_from_server(int sockfd){
    char recv_buf[BUFSIZE];
	int nbyte_recvd;

    struct rival *temp = (struct rival*)malloc(sizeof(struct rival));
   
    memset(recv_buf, 0, sizeof recv_buf);
    nbyte_recvd = recv(sockfd, recv_buf, BUFSIZE, 0);
    
    if(strcmp(recv_buf , "wait") == 0){
        char msg[] = "please wait for another player...\n";
        write(STDOUT_FILENO, msg, sizeof(msg)-1);
        temp->IP = NULL;
        temp->port = NULL;
        temp->username = NULL;
    }else if(strcmp(recv_buf , "wait for friend") == 0){
        char msg[] = "please wait for your friend becoming online...\n";
        write(STDOUT_FILENO, msg, sizeof(msg)-1);
        temp->IP = NULL;
        temp->port = NULL;
        temp->username = NULL;
    }else{
        char msg[] = "rival data is: ";
        write(STDOUT_FILENO, msg, sizeof(msg)-1); 
        write(STDOUT_FILENO, recv_buf, sizeof(recv_buf)-1);
        temp->IP = strtok(recv_buf, " ");
        temp->port = strtok(NULL , " ");
        temp->username = strtok(NULL , " ");
    }

    return temp;
}

int send_dataTo_server(char* IP , char* server_port , struct client_data *client , int *socketfd){
    
	struct sockaddr_in server_addr;
	
	connect_request(socketfd, &server_addr, IP, server_port);

	send_to_server(*socketfd , client);

}

struct sockaddr_in set_client_socket(int *client_port , char client_IP[16] , int *client_socketfd){
    struct sockaddr_in client_address;
    if ((*client_socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        const char err[] ="err in openning socket for client\n" ;
        write(STDERR_FILENO, err, sizeof(err)-1);
        exit(EXIT_FAILURE);
    }

    client_address.sin_family = AF_INET;
    client_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    client_address.sin_port = 0;

    if(bind(*client_socketfd, (struct sockaddr *)&client_address, sizeof(client_address)) < 0){
        const char err[] ="err in binding socket for client\n" ;
        write(STDERR_FILENO, err, sizeof(err)-1);
        exit(EXIT_FAILURE);
    }

    socklen_t slen;
    slen = sizeof(client_address);


    if (getsockname(*client_socketfd, (struct sockaddr *) &client_address, &slen) < 0){
        const char err[] ="err in naming socket for client\n" ;
        write(STDERR_FILENO, err, sizeof(err)-1);
        exit(EXIT_FAILURE);
    }

    inet_ntop(AF_INET, &client_address.sin_addr, client_IP, 16);
	*client_port = ntohs(client_address.sin_port);

    const char client_socket_msg[] = "Client setup its socket...\n";
    write(STDOUT_FILENO, client_socket_msg, sizeof(client_socket_msg)-1);
    const char client_port_msg[] = "Client port is:  ";
    write(STDOUT_FILENO, client_port_msg, sizeof(client_port_msg)-1);
    printf("%d \n" , *client_port);
    const char client_IP_msg[] = "Client IP is:  ";
    write(STDOUT_FILENO, client_IP_msg, sizeof(client_IP_msg)-1);
    printf("%s\n" , client_IP);

    return client_address;
}

void wait_for_connection(int client_socketfd, int *new_socketfd , struct sockaddr_in client_address){
    int addrlen = sizeof(client_address);
    if (listen(client_socketfd, 3) < 0) 
    { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    } 
    if ((*new_socketfd = accept(client_socketfd, (struct sockaddr *)&client_address, (socklen_t*)&addrlen))<0) 
    { 
        perror("accept"); 
        exit(EXIT_FAILURE); 
    }
}

void receive_first_data_from_rival(int socketfd){
	int nbytes_recvd;
	char recv_buf[BUFSIZE] = " ";
	if ((nbytes_recvd = recv(socketfd, recv_buf, BUFSIZE, 0)) <= 0) {
        const char err[] ="waiting for rival...\n" ;
        write(STDERR_FILENO, err, sizeof(err)-1);
    }else{
        printf("rival data is : %s\n", recv_buf);
	}	
}

void connect_to_other_client(int client_socketfd, struct rival rval){
    struct sockaddr_in rival_addr;

    rival_addr.sin_family = AF_INET;
    rival_addr.sin_addr.s_addr = inet_addr(rval.IP); 
    rival_addr.sin_port = htons(atoi(rval.port));

    if (connect(client_socketfd, (struct sockaddr *)&rival_addr, sizeof(struct sockaddr)) != 0) { 
        perror("connect");
        exit(EXIT_FAILURE); 
    }
}

void send_first_data_to_rival(int client_socketfd , struct client_data client){
    
    char send_buf[BUFSIZE];

    strcpy(send_buf , client.IP);
    strcat(send_buf , " ");
    strcat(send_buf , toArray(client.port));
    strcat(send_buf , " ");
    strcat(send_buf , client.username);
    strcat(send_buf , "\0");
    if (send(client_socketfd, send_buf, strlen(send_buf), 0) < 0){
        perror("send");
    }
}

void send_data_to_rival(int socketfd){
    char send_buf[BUFSIZE];
    int x , y;
    const char play_with_friend_msg[] ="Where do you want to shoot?!(input example: x y)\n";
    write(STDOUT_FILENO, play_with_friend_msg, sizeof(play_with_friend_msg)-1);
    
    scanf("%i", &x);
    scanf("%i", &y);
    strcpy(send_buf , toArray(x));
    strcat(send_buf , " ");
    strcat(send_buf , toArray(y));
    strcat(send_buf , "\0");
    if (send(socketfd, send_buf, strlen(send_buf), 0) < 0){
        perror("send");
    }
}

void receive_data_from_rival(int socketfd , int *x , int *y){
    int nbytes_recvd;
    char recv_buf[BUFSIZE];
	char *temp = (char *)malloc(sizeof(char)* BUFSIZE);
    char *temp_x , *temp_y;
    memset(recv_buf, 0, sizeof recv_buf);

    const char play_with_friend_msg[] ="wait for your rival shoot...\n";
    write(STDOUT_FILENO, play_with_friend_msg, sizeof(play_with_friend_msg)-1); 

    nbytes_recvd = recv(socketfd, recv_buf, BUFSIZE, 0);
    strcpy(temp , recv_buf);
    temp_x = strtok(temp , " ");
    temp_y = strtok(NULL , " ");

    *x = atoi(temp_x);
    *y = atoi(temp_y);

    printf("your rival hit: %s position\n", recv_buf);
		
}

void send_goal_msg(int socketfd){
    char send_buf[BUFSIZE];

    strcpy(send_buf , "goal");
    strcat(send_buf , "\0");
    if (send(socketfd, send_buf, strlen(send_buf), 0) < 0){
        perror("send");
    }
}

void send_failure_msg(int socketfd){
    char send_buf[BUFSIZE];

    strcpy(send_buf , "fail");
    strcat(send_buf , "\0");
    if (send(socketfd, send_buf, strlen(send_buf), 0) < 0){
        perror("send");
    }
}

void get_goal_msg(int socketfd){
    int nbytes_recvd;
    char recv_buf[BUFSIZE];
    memset(recv_buf, 0, sizeof recv_buf);

    nbytes_recvd = recv(socketfd, recv_buf, BUFSIZE, 0);

    if(strcmp(recv_buf , "goal") == 0){
        const char play_with_friend_msg[] ="goal...\n";
        write(STDOUT_FILENO, play_with_friend_msg, sizeof(play_with_friend_msg)-1);

    }else if(strcmp(recv_buf , "fail") == 0){
        const char play_with_friend_msg[] ="fail...\n";
        write(STDOUT_FILENO, play_with_friend_msg, sizeof(play_with_friend_msg)-1);

    }
}

void first_person_game(int socketfd , char map[] , int size_map){
    int x , y;

    send_data_to_rival(socketfd);
    get_goal_msg(socketfd);
    receive_data_from_rival(socketfd , &x , &y);
    if(map[(y-1)*10 + x - 1] =='1'){
        send_goal_msg(socketfd);
    }else{
        send_failure_msg(socketfd);
    }
}

void second_person_game(int socketfd , char map[] , int size_map){
    int x , y;

    receive_data_from_rival(socketfd , &x , &y);
    if(map[(y-1)*10 + x - 1] =='1'){
        send_goal_msg(socketfd);
    }else{
        send_failure_msg(socketfd);
    }
    send_data_to_rival(socketfd);
    get_goal_msg(socketfd);
}

int main(int argc , char* argv[]){

    if(argc != 3){
        const char err[] ="Usage:./Client <SERVER HEARTBEAT PORT> <CLIENTS BROADCAST PORT>\n" ;
        write(STDERR_FILENO, err, sizeof(err)-1);
        exit(1);
    }

    struct client_data client;
    struct rival *rval = (struct rival*)malloc(sizeof(struct rival));
    int client_socketfd , new_socketfd;
    struct sockaddr_in client_address; //= (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
    bool turn ,flag = false;
    char file[BUFSIZE];
    int size_map = 0;
    memset(file, 0, sizeof file);
    char input_buffer1[20] , input_buffer2[20] , input_buffer3[20] , input_buffer4[20];
    const char username_msg[] ="Please insert your username: " ;
    write(STDOUT_FILENO, username_msg, sizeof(username_msg)-1);
    
    scanf("%s", input_buffer1);
    client.username = (char*) input_buffer1;

    char *server_heartbeat_port = argv[1];
    char *client_broadcast_port = argv[2];

    char *IP , *server_port;
    bool server_is_alive = false;

    set_get_heartbeat(server_heartbeat_port);

    char *message = (char*)malloc(sizeof(char)* 100);

    server_is_alive = get_heartbeat(message);

    if(server_is_alive){
        IP = strtok(message, " ");
        server_port = strtok(NULL , " ");
    }

    client_address = set_client_socket(&client.port , client.IP , &client_socketfd);

    const char play_with_friend_msg[] ="Do you want to play with your friend?[yes/no] ";
    write(STDOUT_FILENO, play_with_friend_msg, sizeof(play_with_friend_msg)-1);
    scanf("%s", input_buffer2);

    char* play_with_friend;
    
    if(strcmp(input_buffer2 , "yes") == 0){
        play_with_friend = "yes";
        const char friend_username[] ="please insert your friend username: ";
        write(STDOUT_FILENO, friend_username, sizeof(friend_username)-1);
        scanf("%s", input_buffer3);
        client.who_to_play = (char*)input_buffer3;
    }else{
        play_with_friend = "no";
        client.who_to_play = "nobody";
    }

    const char map_file_name[] ="Please specify your map file name: ";
    write(STDOUT_FILENO, map_file_name, sizeof(map_file_name)-1);
    scanf("%s", input_buffer4);

    int file_fd = open(input_buffer4, O_RDONLY);
    if(file_fd < 0){
        const char err[] ="Can not open file...\n" ;
        write(STDERR_FILENO, err, sizeof(err)-1);
    }else{
        char c;
        while (read(file_fd, &c, 1) == 1)
            if(c == '1' || c=='0')
            {
                file[size_map] = c;
                size_map++;
            }
        const char read_file_msg[] ="read file successfully...\n" ;
        write(STDOUT_FILENO, read_file_msg, sizeof(read_file_msg)-1);
    }

    if(server_is_alive){
        int socketfd = 0;
        send_dataTo_server(IP , server_port , &client , &socketfd);
        rval->IP = NULL;
        rval->port = NULL;
        rval->username = NULL;
        rval = recv_from_server(socketfd);
        if(rval->port == NULL){
            wait_for_connection(client_socketfd, &new_socketfd , client_address);
            receive_first_data_from_rival(new_socketfd);
            turn = true;
        }else{
            connect_to_other_client(client_socketfd , *rval);
            send_first_data_to_rival(client_socketfd , client);
            turn = false;
        }
    }else{
        char *recv_msg , *who_to_play;
        set_get_broadcast(client_broadcast_port , true);
        int count = 0;
        while(1){
            count++;
            if(count > 10)
                break;
            recv_msg = get_broadcast_msg();
            if(recv_msg == NULL){
                flag = false;
                break;
            }else{
                rval->IP = strtok(recv_msg, " ");
                rval->port = strtok(NULL , " ");
                rval->username = strtok(NULL , " ");
                who_to_play = strtok(NULL , "\n");

                if(strcmp(rval->username , client.username) == 0){
                    
                    continue;
                }else if(strcmp(client.who_to_play , "nobody") != 0 && strcmp(client.who_to_play , rval->username) != 0){
                    continue;
                }else if(strcmp(who_to_play , "nobody") != 0 && strcmp(who_to_play , client.username) != 0){
                    continue;
                }else if( strcmp(client.who_to_play , "nobody") != 0 && strcmp(client.who_to_play , rval->username) == 0){
                    flag = true;
                    connect_to_other_client(client_socketfd , *rval);
                    send_first_data_to_rival(client_socketfd , client);
                    turn = false;
                    break;
                }else if( strcmp(who_to_play , "nobody") != 0 && strcmp(who_to_play , client.username) == 0){
                    flag = true;
                    connect_to_other_client(client_socketfd , *rval);
                    send_first_data_to_rival(client_socketfd , client);
                    turn = false;
                    break;
                }else{
                    flag = true;
                    connect_to_other_client(client_socketfd , *rval);
                    send_first_data_to_rival(client_socketfd , client);
                    turn = false;
                    break;
                }
            }
        }
        if(!flag){
            pid_t pid;
            pid = fork();
            if(pid == 0){
                char sending_to_client_msg[BUFSIZE];
                strcpy(sending_to_client_msg , client.IP);
                strcat(sending_to_client_msg , " ");
                strcat(sending_to_client_msg , toArray(client.port));
                strcat(sending_to_client_msg , " ");
                strcat(sending_to_client_msg , client.username);
                strcat(sending_to_client_msg , " ");
                strcat(sending_to_client_msg , client.who_to_play);
                strcat(sending_to_client_msg , "\n");
                set_get_broadcast(client_broadcast_port , false);
                set_send_broadcast(client_broadcast_port, sending_to_client_msg);
                const char broadcasting_msg[] ="broadcasting....\n" ;
                write(STDOUT_FILENO, broadcasting_msg, sizeof(broadcasting_msg)-1);
                broadcasting(0);
                
                const char get_broadcast[] ="waiting for rival...\n" ;
                write(STDOUT_FILENO, get_broadcast, sizeof(get_broadcast)-1); 
                recv_msg = get_broadcast_msg();
                flag = false;
                close_broadcast_port();             
            }else{
                wait_for_connection(client_socketfd, &new_socketfd , client_address);
                receive_first_data_from_rival(new_socketfd);
                kill(pid , SIGKILL);
                turn = true;
            }
        }
    }

    const char err[] ="\n\t\t\t***** game started *****\n\n" ;
    write(STDERR_FILENO, err, sizeof(err)-1);

    while(1){
            if(turn){
                first_person_game(new_socketfd , file , size_map);
            }else{
                second_person_game(client_socketfd , file , size_map);
            }
    }

    return 0;
}