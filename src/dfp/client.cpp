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
#define PORT 6001

using namespace std;

int main(int argc, char*argv[])
{
	struct sockaddr_in client;
    sockaddr_in serv_addr;
	struct hostent *server;
	client.sin_family = AF_INET;     
	client.sin_port = htons(4200);    
//	client.sin_addr.s_addr = INADDR_ANY;
	inet_pton(AF_INET, "127.0.0.1", &(client.sin_addr));
	bzero(&(client.sin_zero), 8);  

	server = gethostbyname(argv[1]);
	if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }	

	serv_addr.sin_family = AF_INET;     
	serv_addr.sin_port = htons(PORT);    
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	bzero(&(serv_addr.sin_zero), 8);  

	dfp_service service= { dfp_service(4200, (struct sockaddr*)&serv_addr)};

    std::cout << "passed init_service." <<endl;
	
    sockaddr_in cli_addr;
	vector<uint8_t> data;
    char buffer[256];

	while(1){
		printf("Enter the message: ");
		bzero(buffer, 256);
		fgets(buffer, 256, stdin);

		printf("alloc size: %ld.\n", strlen(buffer));
		
		service.sendmessage(buffer, strlen(buffer));

	}
	return 0;
}


