#include <pthread.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include "dfp_service.h"
#include <stdlib.h>
#include <arpa/inet.h>

using namespace std; 

#define MAX_CHUNK_SIZE 512

#define PACKET_HEADER_SIZE (sizeof(packet)-sizeof(const char* ))

void* recv_thread(void*arg);

vector<vector<uint8_t>> dfp_service::split_message(uint16_t msgID, uint8_t* message, uint32_t len)
{
	
	vector<vector<uint8_t>> splited_message;
	uint16_t msg_index=0;

	packet header;
	header.type= PACKET_TYPE_DATA;
	header.msgID= msgID;
	header.total_size= len;
	header.seqn= 0;


	while(msg_index<len){
		vector<uint8_t> segment;
		int payload_index;
		for(payload_index=0;payload_index< MAX_PAYLOAD_LEN; payload_index++){
			if(msg_index+payload_index >= len){
				break;
			}
			segment.push_back(message[msg_index+payload_index]);
		}
		header.seqn= msg_index;
		header.payload_length= payload_index;
		segment.insert(segment.begin(), (uint8_t *)&header, (uint8_t *)&header + PACKET_HEADER_SIZE);
		msg_index+= payload_index;
		splited_message.push_back(segment);
	}
	return splited_message;
}



void dfp_service::WAIT_ACK(struct sockaddr_in* dest_addr, packet *sent_pckt)
{
    sockaddr_in src_addr;
	int n;
    bool gotACK= false;

	do{
		while(this->ackQueue.empty());

		ackPacket new_packet (this->ackQueue.front());
		this->ackQueue.pop();
		memcpy(&src_addr, &new_packet.src_addr, sizeof(sockaddr_in));

        if(src_addr.sin_addr.s_addr == dest_addr->sin_addr.s_addr &&
		   src_addr.sin_port == dest_addr->sin_port &&
		   new_packet.header.msgID == sent_pckt->msgID &&
           new_packet.header.seqn == sent_pckt->seqn){
            gotACK= true;
        }
    }while(!gotACK);
}


void dfp_service::send_segments(vector<vector<uint8_t>> segments, const sockaddr* dest_addr)
{
	int n;
	for(int i=0;i< segments.size();i++){
		n= sendto(this->sockfd, segments[i].data(), segments[i].size(), 0, (sockaddr *)dest_addr, sizeof(sockaddr_in));
		if(n<0){
			printf("error sending packet segment #%d", i);
		}
		WAIT_ACK((sockaddr_in*)dest_addr, (packet*)segments[i].data());
	}
}


int dfp_service::sendmessage(const void* buf, size_t len)
{
	if(this->dest_addr){
		sendmessage(buf, len, (sockaddr *)this->dest_addr);
	}else{
		printf("invalid call for sendmessage with no dst addr.\n");
	}
}

int dfp_service::sendmessage(const void* buf, size_t len, const struct sockaddr *dest_addr)
{
	send_segments(split_message(this->msgID++, (uint8_t*)buf, len), dest_addr);
}

vector<uint8_t> dfp_service::recvmessage(struct sockaddr_in *src_addr)
{
    //blocking implementation
    while(messageQueue.empty());

    message new_message ( messageQueue.front() );
    
	messageQueue.pop();

	memcpy(src_addr, &new_message.src_addr, sizeof(sockaddr_in));
    return new_message.data;
}

int dfp_service::numMessages()
{
	return messageQueue.size();
}

dfp_service::~dfp_service(void)
{
	if(dest_addr)
		free(this->dest_addr);
}

dfp_service::dfp_service(int port, struct sockaddr *dest_addr)
{

	if(dest_addr !=NULL){
		this->dest_addr= (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
		memcpy(this->dest_addr, dest_addr, sizeof(struct sockaddr_in));
	}else{
		this->dest_addr= NULL;
	}

	msgID= 0;
	state= STATE_IDLE;

	printf("init service on [:%d].\n", port);
	this->port= port;

	socklen_t clilen;
	struct sockaddr_in serv_addr;
		
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		printf("ERROR opening socket.\n");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port =  htons(port);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_addr.sin_zero), 8);    
	 
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0)
		printf("ERROR on binding.\n");
	
	pthread_create(&th_recv, NULL, recv_thread, this);
	
}

dfp_service::dfp_service(int port)
{
	dfp_service(port, (struct sockaddr *)NULL);
}


bool dfp_service::accepting_client(sockaddr_in *cli_addr)
{
    if(this->dest_addr!=NULL){
        if( !(this->dest_addr->sin_port == cli_addr->sin_port &&
              this->dest_addr->sin_addr.s_addr == cli_addr->sin_addr.s_addr)){
                return false;
        }
    }
    return true;
}

bool dfp_service::not_binded(void)
{
	return (this->dest_addr==NULL)? true:false;
}

void dfp_service::receive_thread(void)
{
	int n;

	vector<uint8_t> message;

	packet* pckt= (packet*)buf;
	struct sockaddr_in cli_addr;
	socklen_t clilen;
	clilen = sizeof(struct sockaddr_in);

	uint16_t current_message_id;
	uint32_t last_seqn;
	uint32_t next_seqn;
	while(1){
		/* receive from socket */
		n = recvfrom(sockfd, this->buf, UDP_PACKET_MAX_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen);
		if (n < 0){
			printf("ERROR on recvfrom.\n");
		}else{
			printf("Received a datagram(#%d) on (:%d) from(%s:%d)\n",n, this->port, 
						inet_ntoa(*(in_addr*)&cli_addr.sin_addr.s_addr), ntohs(cli_addr.sin_port));
		}

        if(accepting_client(&cli_addr)==false){
            printf("not expected client [expecting %s:%d and came %s:%d]. Dropping the packet;\n", 
                                       inet_ntoa(*(in_addr*)(&this->dest_addr->sin_addr.s_addr)), ntohs(this->dest_addr->sin_port), 
                                       inet_ntoa(*(in_addr*)(&cli_addr.sin_addr.s_addr)),   ntohs(cli_addr.sin_port));
            memset(this->buf, 0, n);
            continue;
        }


        //ACKS are sent direct to ackQueue
		if(pckt->type == PACKET_TYPE_ACK){
		    ackPacket ackPckt;
            memcpy(&ackPckt.header, pckt, sizeof(packet));
            memcpy(&ackPckt.src_addr, &cli_addr, clilen);
            this->ackQueue.push(ackPckt);
            continue;
		}

		unsigned char *payload = (unsigned char*)&(pckt->_payload);

//	    printf("payload: addr: %p. relative: %ld.\n", payload, payload - &buf[0]);
//  	printf("Messaage: %s.\n", payload);

//		printf("pckt.seqn=       %d\n",pckt->seqn);
//		printf("pckt.total_size= %d\n",pckt->total_size);
//		printf("pckt.length=     %d\n",pckt->payload_length);

        if(not_binded()){
        //no dest binded sockets receive only single packets
            message.insert(message.end(), payload, payload + pckt->payload_length );
            this->messageQueue.push({message, cli_addr} );
            message.clear();
            send_ACK(&cli_addr);
        }

		switch(state){

			case STATE_IDLE:
				printf("STATE IDLE.\n");
				if(pckt->seqn == 0){
					current_message_id= pckt->msgID;
					last_seqn= pckt->seqn;
					next_seqn= pckt->seqn+ pckt->payload_length;
                    message.insert(message.end(), payload, payload + pckt->payload_length);
					printf("idle: appended %ld bytes.\n", message.size());


					send_ACK();

					if(pckt->total_size <= pckt->seqn + pckt->payload_length){
						printf("idle: transmission completed.\n");
						printf("total_size: %u. received: %u", pckt->total_size, pckt->seqn+ pckt->payload_length);
						this->messageQueue.push({message, cli_addr});
						message.clear();
						state= STATE_IDLE;

					}else{
						state= STATE_RECEIVING;
					}
				}
				break;

			case STATE_RECEIVING:
				printf("STATE RECEIVING.\n");

				if(current_message_id!=pckt->msgID){
					printf("wrong id. ignoring.\n");
					continue;
				}

				//ACK retransmission
				if(pckt->seqn == last_seqn){
					printf("duplicated packet. Send ACK.\n");
					send_ACK();
				}

				//append to message
				if(pckt->seqn == next_seqn){
					last_seqn= pckt->seqn;
					next_seqn= pckt->seqn + pckt->payload_length;

                    message.insert(message.end(), payload, payload + pckt->payload_length);
					printf("receiving: total appended %ld bytes.\n", message.size());

					send_ACK();

					if(pckt->total_size <= pckt->seqn + pckt->payload_length){
						printf("receiving: transmission completed.\n");
						this->messageQueue.push({message, cli_addr});
						message.clear();
						state= STATE_IDLE;
					}
				}
		}
	}

}

void dfp_service::send_ACK(void)
{
    send_ACK(this->dest_addr);
}

void dfp_service::send_ACK(struct sockaddr_in* dest_addr)
{
	uint8_t ACK_buffer[PACKET_HEADER_SIZE]; 
	packet* pckt= (packet*)ACK_buffer;

	memcpy(ACK_buffer, buf, PACKET_HEADER_SIZE);
    
    pckt->type = PACKET_TYPE_ACK;
    pckt->total_size= 0;//PACKET_HEADER_SIZE;
    pckt->payload_length= 0;
    
    if(dest_addr!=NULL){
        n= sendto(sockfd, ACK_buffer, PACKET_HEADER_SIZE,0, (const struct sockaddr *)dest_addr, sizeof(sockaddr_in));
		if (n < 0) 
			printf("ERROR sendto");
    }
}

void* recv_thread(void*arg)
{
	dfp_service * dfp_inst= (dfp_service*)arg;
    dfp_inst->receive_thread();
}
