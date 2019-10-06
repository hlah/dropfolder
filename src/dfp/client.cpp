#include <iostream>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include "dfp_service.h"
#define SERVER_PORT 5000
#define CLIENT_PORT 4200


using namespace std;

int main(int argc, char*argv[])
{
	uint16_t port= CLIENT_PORT;
	struct sockaddr_in client;
    sockaddr_in serv_addr;
	struct hostent *server;
	client.sin_family = AF_INET;     
	client.sin_port = htons(CLIENT_PORT);    
//	client.sin_addr.s_addr = INADDR_ANY;
	inet_pton(AF_INET, "127.0.0.1", &(client.sin_addr));
	bzero(&(client.sin_zero), 8);

	if(argc<2){
		printf("usage:\n\t client localhost\n");
		return 0;
	}
	server = gethostbyname(argv[1]);
	if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }	

	serv_addr.sin_family = AF_INET;     
	serv_addr.sin_port = htons(SERVER_PORT);    
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(serv_addr.sin_zero), 8);  

	dfp_service service= { dfp_service(port, (struct sockaddr*)&serv_addr)};  //listening on 4200. Send to [127.0.0.1:6001]
	dfp_service *service2;

	
 //   sockaddr_in cli_addr;
	vector<uint8_t> data;
    char buffer[256];
	vector<uint8_t> message;
	uint16_t server_new_port;
	bool use_other_conn;
	while(1){
		printf("Enter the message: ");
		bzero(buffer, 256);
		fgets(buffer, 256, stdin);

		printf("msg size: %ld.\n", strlen(buffer));
		

		while(service.numMessages() != 0){
			message= service.recvmessage();
			if(strncmp((char*)message.data(), "connect on", 10)==0){
				sscanf((char*)message.data(), "connect on %hu", &server_new_port);
				serv_addr.sin_port = htons(server_new_port);
			    service2= new dfp_service(4500, (struct sockaddr*)&serv_addr);
				use_other_conn= true;
			}
		}
		if(use_other_conn){
			service2->sendmessage(buffer, strlen(buffer));
		}else{
			service.sendmessage(buffer, strlen(buffer));
		}

	}
	return 0;
}


