#include <iostream>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>

#include "dfp_service.h"

#define PORT 5000
int main(int argc, char*argv[])
{
	int port= PORT;
//	sscanf(argv[1],"%d",&port);

	struct sockaddr_in client[2];
	client[0].sin_family = AF_INET;     
	client[0].sin_port = htons(4200);    
	inet_pton(AF_INET, "127.0.0.1", &(client[0].sin_addr));
	bzero(&(client[0].sin_zero), 8);  

	client[1].sin_family = AF_INET;     
	client[1].sin_port = htons(4300);    
	inet_pton(AF_INET, "127.0.0.1", &(client[1].sin_addr));
	bzero(&(client[1].sin_zero), 8);  


	dfp_service service[]= {dfp_service(6000,NULL),                        //listen on 6000 
							dfp_service(6001,(struct sockaddr*)&client[0]),   //listen on 6001 send to 4200
							dfp_service(6002,(struct sockaddr*)&client[1])};   //listen on 6002 send to 4300

	std::cout << "passed init_service." <<endl;
	sockaddr_in cli_addr;
	vector<uint8_t> data;
	while(1){
		for(int i=0;i<3;i++){
			while(service[i].numMessages() != 0){
				data= service[i].recvmessage(&cli_addr);
				std::cout << "new Message [" << inet_ntoa(*(in_addr*)(&cli_addr.sin_addr.s_addr)) << ":"
				     	  << ntohs(cli_addr.sin_port) << "]: " << data.data() << endl;
			}
		}
	}
}




