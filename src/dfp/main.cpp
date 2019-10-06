#include <iostream>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <list>
#include "dfp_service.h"
#include <fstream>

#define PORT 5000
int main(int argc, char*argv[])
{
	ofstream log_file;
	log_file.open ("log_file.txt");
	uint16_t port= PORT;
//	sscanf(argv[1],"%d",&port);

	struct sockaddr_in client[2];
#if 0
	client[0].sin_family = AF_INET;     
	client[0].sin_port = htons(4200);    
	inet_pton(AF_INET, "127.0.0.1", &(client[0].sin_addr));
	bzero(&(client[0].sin_zero), 8);  

	client[1].sin_family = AF_INET;     
	client[1].sin_port = htons(4300);    
	inet_pton(AF_INET, "127.0.0.1", &(client[1].sin_addr));
	bzero(&(client[1].sin_zero), 8);  
#endif

	list<dfp_service*> service_list;
	service_list.push_back( new dfp_service(PORT,NULL));                        //listen on 6000 
						//	dfp_service(6001,(struct sockaddr*)&client[0]),   //listen on 6001 send to 4200
						//	dfp_service(6002,(struct sockaddr*)&client[1])};   //listen on 6002 send to 4300

	sockaddr_in cli_addr;
	vector<uint8_t> data;

	char message_buf[64];
	while(1){

		for(auto const &service : service_list){
			while(service->numMessages() != 0){
				data= service->recvmessage(&cli_addr);
				log_file << "new Message(#" << data.size() <<  ") [" << inet_ntoa(*(in_addr*)(&cli_addr.sin_addr.s_addr)) << ":"
				     	  << ntohs(cli_addr.sin_port) << "]: " << data.data() << endl;
				if(strncmp((char *)data.data(),"new_user", 8)==0){
					uint16_t usr_port;
					htons( sscanf((char*)data.data(),"new_user %hu",&usr_port));    
					client[0].sin_family = AF_INET;
                    client[0].sin_port = htons(usr_port);
                    log_file << "new conn port: " << usr_port << endl;
					inet_pton(AF_INET, "127.0.0.1", &(client[0].sin_addr));
					bzero(&(client[0].sin_zero), 8);  

					port++;
					service_list.push_back(new dfp_service(port, (sockaddr*)&client[0]));
					sprintf(message_buf, "connect on %hu", port);
					service->sendmessage(message_buf, strlen(message_buf),(const struct sockaddr_in*) &cli_addr);

				}
			}

		}

	}
}




