#include "connection.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>
#include <vector>
#include <chrono>
#include <ifaddrs.h>

#include <iostream>

#define PAYLOAD_LEN 1024

std::shared_ptr<Connection> Connection::connect(ConnMode mode, const std::string& hostname, int port, int timelimit) {
    int socketfd = build_socket();
    auto host = get_host( hostname );

	sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = *((in_addr*)host->h_addr);
    bzero(&(addr).sin_zero, 8);

    Packet packet{ PacketType::Conn };
	do{
		packet.type= PacketType::Conn;
		if( sendto( socketfd, (const void*)&packet, sizeof(Packet), 0, (sockaddr*)&addr, sizeof(sockaddr_in) ) == -1) {
			throw_errno("Could not send Conn packet");
		}

		if( !poll_socket( socketfd, timelimit ) ) {
			throw ConnectionException{ "Connection timed out." };
		}

		if(recvfrom( socketfd, &packet, sizeof(Packet), 0, nullptr, nullptr ) == -1) {
			throw_errno("Could not reveive");
		}

	}while( packet.type != PacketType::NewPort );
   

    // reconnect through new port
    addr.sin_family = AF_INET;
    addr.sin_port = htons(packet.msg_id);
    addr.sin_addr = *((in_addr*)host->h_addr);
    bzero(&(addr).sin_zero, 8);

    packet = Packet{ PacketType::Ack };
    if( sendto( socketfd, (const void*)&packet, sizeof(Packet), 0, (sockaddr*)&addr, sizeof(sockaddr_in) ) == -1) {
        throw_errno("Could not send Ack packet");
    }

    return std::shared_ptr<Connection>{new Connection{mode, socketfd, addr, timelimit}};
}

std::shared_ptr<Connection> Connection::listen(int port, int min_range, int max_range, int timelimit) {
    int socketfd = build_socket();

	sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(server_addr).sin_zero, 8);

	int enable = 1;
	if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
		throw_errno("setsockopt(SO_REUSEADDR) failed");;
	}

    if( bind( socketfd, (sockaddr*)&server_addr, sizeof(server_addr) ) == -1) {
		std::string err_msg{"Could not bind socket(1), port: "};
		err_msg +=std::to_string(port);
        throw_errno(err_msg.c_str());
        //throw_errno("Could not bind socket(1)");
    }

	Packet packet;
	sockaddr_in client_addr;
	socklen_t client_addr_size = sizeof(sockaddr_in);
	do{
		if( !poll_socket( socketfd, -1 ) ) {
			throw ConnectionException{ "Connection timed out." };
		}

		if( recvfrom( socketfd, &packet, sizeof(Packet), 0, (sockaddr*)&client_addr, &client_addr_size ) == -1) {
			throw_errno("Could not reveive connection");
		}

	}while(packet.type != PacketType::Conn);

	close(socketfd);
    
    // create new socket for connection maintenance
    int current_port = min_range;
    while( current_port <= max_range ) {
        socketfd = build_socket();
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(current_port);
        server_addr.sin_addr.s_addr = INADDR_ANY;
        bzero(&(server_addr).sin_zero, 8);

        if( bind( socketfd, (sockaddr*)&server_addr, sizeof(server_addr) ) == 0 ) {
            break;
        } 

        close(socketfd);
        current_port++;
    }


    if( current_port > max_range ) {
        throw_errno("Could not bind socket(2)");
    }

    int tries = 3;
   	do{
		packet = Packet{ PacketType::NewPort, (uint16_t)current_port };
		if( sendto( socketfd, (const void*)&packet, sizeof(Packet), 0, (sockaddr*)&client_addr, sizeof(sockaddr_in) ) == -1) {
			throw_errno("Could not send packet");
		}

		if( !poll_socket( socketfd, timelimit ) ) {
			tries--;
			continue;
		}

		if(recvfrom( socketfd, &packet, sizeof(Packet), 0, (sockaddr*)&client_addr, &client_addr_size ) == -1) {
			throw_errno("Could not receive ack");
		}
	}while(tries && packet.type != PacketType::Ack);

	if( tries==0) {
		throw ConnectionException{ "Could not receive ack" };
	}

    return std::shared_ptr<Connection>{new Connection{ConnMode::Normal, socketfd, client_addr, timelimit}};
}



std::shared_ptr<Connection> Connection::reconnect(ConnMode mode, uint32_t peerIP, uint16_t peerPort, int min_range, int max_range, int timelimit) {

//	std::cout << "RECONNECT" <<std::endl;
	sockaddr_in server_addr;

	sockaddr_in dest_addr;
//	socklen_t dest_addr_size = sizeof(sockaddr_in);

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(peerPort);
    dest_addr.sin_addr.s_addr = htonl(peerIP);
    bzero(&(dest_addr).sin_zero, 8);
    
    int socketfd;
    // create new socket for connection maintenance
    int current_port = min_range;
    while( current_port <= max_range ) {
        socketfd = build_socket();
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(current_port);
        server_addr.sin_addr.s_addr = INADDR_ANY;
        bzero(&(server_addr).sin_zero, 8);

        if( bind( socketfd, (sockaddr*)&server_addr, sizeof(server_addr) ) == 0 ) {
            break;
        } 

        close(socketfd);
        current_port++;
    }

    if( current_port > max_range ) {
        throw_errno("Could not bind socket(2)");
    }

    Packet packet{ PacketType::ChangeServerAddr};

    int tries=3;
    while(tries>0){
        if( sendto( socketfd, (const void*)&packet, sizeof(Packet), 0, (sockaddr*)&dest_addr, sizeof(sockaddr_in) ) == -1) {
            std::cout << "error sending packet(1)" << std::endl;
            
        }else{
            sockaddr_in sender_addr;
            socklen_t sender_addr_len= sizeof(sockaddr_in);

            /* receive from socket */
            if(poll_socket(socketfd, timelimit)){
                if(recvfrom( socketfd, (void*)&packet, sizeof(Packet), 0, (sockaddr*)&sender_addr, &sender_addr_len ) == -1) {
                    std::cout << "error receiveing packet" << std::endl;
                }
                if(packet.type== PacketType::Ack){
                    std::cout << "Got ACK!" << std::endl;

					if(sender_addr.sin_port != dest_addr.sin_port){
						std::cout << "DIFFERENT DESTINATION!!" << std::endl;
					}
                    break;
                }
            }
        }
		std::cout << "will retransmit" << std::endl;
        tries--;
    }

    if(tries==0){
        throw ConnectionException{"Reconnection timed out"};
    }

	std::cout << "reconnect: _otherPort: " << ntohs(dest_addr.sin_port) << std::endl;
    return std::shared_ptr<Connection>{new Connection{mode, socketfd, dest_addr, timelimit}};
}






//TODO: create a thread for receive
ReceivedData Connection::receive(int receive_timelimit) {
    // wait for message or timeout
    auto prev = std::chrono::steady_clock::now();
    while(recvQueue.empty() ) {
        if( receive_timelimit >= 0 ) {
            auto now = std::chrono::steady_clock::now();
            //auto int_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - prev);
            if( now-prev > std::chrono::milliseconds{receive_timelimit} ) {
                break;
            }
        }
    }

    if(!recvQueue.empty()){
        pthread_mutex_lock(&recvQueueMutex);
        ReceivedData message {std::move(recvQueue.front().data), recvQueue.front().length };
        recvQueue.pop();
        pthread_mutex_unlock(&recvQueueMutex);
        return {std::move(message.data), message.length};
    } else {
        return ReceivedData{};
    }
}




template<class T>
int Connection::poll_queue(std::queue<T>* a_queue, pthread_cond_t* cond, pthread_mutex_t* mutex, int _timelimit_ms)
{
    int               rc;
    struct timespec   ts;
    struct timeval    tp;


    pthread_mutex_lock(mutex);
        while(a_queue->empty()){
			rc =  gettimeofday(&tp, NULL);
			if(rc<0){
				throw_errno("failed gettimeofdat");
			}
			/* Convert from timeval to timespec */
			ts.tv_sec  = tp.tv_sec;
			ts.tv_nsec = tp.tv_usec * 1000;
			ts.tv_sec += _timelimit_ms/1000;

			if(_timelimit_ms < 0){
				pthread_cond_wait(cond, mutex);
                pthread_mutex_unlock(mutex);
				return 1;

			}else if(pthread_cond_timedwait(cond, mutex, &ts)== ETIMEDOUT){
                pthread_mutex_unlock(mutex);
				printf("poll queue timed out!\n");
                return 0;
            }
        }
    pthread_mutex_unlock(mutex);
    return 1;
}


void Connection::send(uint8_t* data, size_t size) {
    size_t count = 0;
    uint32_t total = (uint32_t)((size+PAYLOAD_LEN) / PAYLOAD_LEN);

    Packet* packet = (Packet*)new char[sizeof(Packet) + PAYLOAD_LEN];
    packet->type = PacketType::Data;
    packet->msg_id = _next_msg_id++;
    packet->seqn = 0;
    packet->total = total;

    int tries = 0;

    //empties ackQueue
    pthread_mutex_lock(&ackQueueMutex);
    while(!ackQueue.empty()){
		printf("packet remaining in queue!\n");
        ackQueue.pop();
    }
    pthread_mutex_unlock(&ackQueueMutex);

    while( count < total ) {
        if( count < total-1 ) {
            packet->payload_length = PAYLOAD_LEN;
        } else {
            packet->payload_length = size;
        }

        memcpy( &packet->data, &data[packet->seqn*PAYLOAD_LEN], packet->payload_length );

        if( sendto( _socket_fd, (const void*)packet, sizeof(Packet)+packet->payload_length, 0, (sockaddr*)&_other, sizeof(sockaddr_in) ) == -1) {
            throw_errno("Could not send data packet");
        }

        // wait for ack
        if( poll_queue(&ackQueue, &ackQueueCond, &ackQueueMutex, _timelimit)){

            pthread_mutex_lock(&ackQueueMutex);
            Packet *ack_packet= ackQueue.front();
            ackQueue.pop();
            pthread_mutex_unlock(&ackQueueMutex);

			if(ack_packet->seqn != packet->seqn){
				printf("different ack packet[got %d, expected %d]\n", ack_packet->seqn, packet->seqn);
			}
			//printf("GOT ACK!\n");
            // update counters
            packet->seqn++;
            count+=1;
            size -= PAYLOAD_LEN;
            tries = 0;
        } else {
			printf("%d: retransmit to %d\n", getPort(), ntohs(_other.sin_port));
            tries++;
        }

        if( tries == _trylimit-1 ) {
            throw ConnectionException{"Connection timed out"};
        }
    }
}

Connection::~Connection() {
    if(_socket_fd != 0) {
        _quit_thread = true;
        pthread_join(_recv_thread, nullptr);
        close(_socket_fd);
    }
}


Connection::Connection( Connection&& conn ) noexcept 
    : _socket_fd{conn._socket_fd}, _other{conn._other}, _timelimit{conn._timelimit}, _trylimit{conn._trylimit}
{
    conn._socket_fd = 0;
}

Connection& Connection::operator=( Connection&& conn ) {
    close(_socket_fd);
    _socket_fd = conn._socket_fd;
    _other = conn._other;
    _timelimit = conn._timelimit;
    _trylimit = conn._trylimit;
    conn._socket_fd = 0;
    return *this;
}

void Connection::sendAck(Packet* packet, sockaddr_in *sender_addr)
{
    // send ack
    packet->type = PacketType::Ack;
    packet->payload_length = 0;
    if( sendto( _socket_fd, (const void*)packet, sizeof(Packet), 0, (sockaddr*)sender_addr, sizeof(sockaddr_in) ) == -1) {
        throw_errno("Could not send ack.");
    }
}

void* recv_thread(void*arg)
{
	Connection * conn_inst= (Connection*)arg;
    conn_inst->receive_thread();
    pthread_exit(0);
}

void Connection::receive_thread()
{
	struct sockaddr_in sender_addr;
	socklen_t sender_addr_len = sizeof(struct sockaddr_in);

	uint16_t current_message_id;
//	uint32_t last_seqn;
//	uint32_t next_seqn;
    
    recvState state= recvState::Waiting;

    size_t count;
    size_t data_size;
    size_t total;

    // allocate buffer and bitset
    std::vector<bool> received;
    std::unique_ptr<uint8_t[]> data;

    Packet* packet = (Packet*)new char[sizeof(Packet) + PAYLOAD_LEN];
	while(!_quit_thread){
        if( !poll_socket( _socket_fd, 100 ) ) {
            continue;
        }


		/* receive from socket */
        if(recvfrom( _socket_fd, packet, sizeof(Packet) + PAYLOAD_LEN, 0, (sockaddr*)&sender_addr, &sender_addr_len ) == -1) {
			throw_errno("Could not receive ack");
        }

		//std::cout << "." << std::endl;


		if(packet->type == PacketType::ChangeServerAddr){
            char str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(sender_addr.sin_addr), str, INET_ADDRSTRLEN);
            std::cout << "conn" << getPort() << ": changed server_addr from " << str << ":" << ntohs(_other.sin_port)
            		  << " to " << str << ":" << ntohs(sender_addr.sin_port) << std::endl;
            _other.sin_addr.s_addr= sender_addr.sin_addr.s_addr;
            _other.sin_port= sender_addr.sin_port;
            sendAck(packet, &sender_addr);
            continue;

		}
        if(mode != ConnMode::Promiscuous){
            if(sender_addr.sin_addr.s_addr !=  _other.sin_addr.s_addr ||
                sender_addr.sin_port != _other.sin_port){
                continue;
            }
        }

        //ACKS are sent direct to ackQueue
		if(packet->type == PacketType::Ack){
            pthread_mutex_lock(&ackQueueMutex);

            this->ackQueue.push(packet);
            pthread_cond_signal(&ackQueueCond);

            pthread_mutex_unlock(&ackQueueMutex);
            continue;
		}

        if( packet->type != PacketType::Data ) {
            throw ConnectionException{ "Unexpected packet" };
        }

        switch(state){
            case recvState::Waiting:
                count = 1;
                data_size = 0;
                total = packet->total;

                current_message_id= packet->msg_id;

                // allocate buffer and bitset
                received = std::vector<bool>(total, false);
                data= std::unique_ptr<uint8_t[]>{ new uint8_t[total*PAYLOAD_LEN] };

                if( packet->payload_length < PAYLOAD_LEN && packet->seqn != total-1 ) {
                    throw ConnectionException{ "Non last packet with less than total payload length" };
                }

                memcpy( &data[packet->seqn*PAYLOAD_LEN], &packet->data, packet->payload_length );
                received[packet->seqn] = true;
                data_size += packet->payload_length;
                
			//	printf("received seqno:%d/%d(count:%d)\n", packet->seqn, total-1, count);
                sendAck(packet, &sender_addr);
                if( count < total ) {
                    state= recvState::RetrievingData;
                }else{
                    pthread_mutex_lock(&recvQueueMutex);
                    this->recvQueue.push(ReceivedData{ std::move(data), data_size });
                    pthread_mutex_unlock(&recvQueueMutex);
                }
                break;

            case recvState::RetrievingData:

                // get remaining packets
                //

                if(current_message_id!= packet->msg_id){
                    //discart different ids
                    continue;
                }

                if( packet->payload_length < PAYLOAD_LEN && packet->seqn != total-1 ) {
                    throw ConnectionException{ "Non last packet with less than total payload length" };
                }

                if( !received[packet->seqn] ) {
                    count++;
                    memcpy( &data[packet->seqn*PAYLOAD_LEN], &packet->data, packet->payload_length );
                    received[packet->seqn] = true;
                    data_size += packet->payload_length;
                }

                sendAck(packet, &sender_addr);
				//printf("received seqno:%d/%d(count:%d)\n", packet->seqn, total-1, count);

                if( count >= total ) {
                    state= recvState::Waiting;

                    pthread_mutex_lock(&recvQueueMutex);
                    this->recvQueue.push(ReceivedData{ std::move(data), data_size });
                    pthread_mutex_unlock(&recvQueueMutex);
                }
        }
	}
}


/// Private functions
Connection::Connection(ConnMode mode, int socket_fd, sockaddr_in other, int timelimit, int trylimit) 
	: _socket_fd{socket_fd}, _other{other}, _timelimit{timelimit}, _trylimit{trylimit} 
{
//	std::cout << "Connection: _otherIp: " << ntohs(other.sin_port) << std::endl;
    this->mode= mode;
    pthread_cond_init(&ackQueueCond, NULL);
    pthread_mutex_init(&ackQueueMutex, NULL);
    pthread_mutex_init(&recvQueueMutex, NULL);
    
	pthread_create(&_recv_thread, NULL, recv_thread, this);
}

void Connection::throw_errno( const std::string& str ) {
    throw ConnectionException{ str + std::string{": "} + strerror( errno ) + std::string{" ("} + std::to_string(errno) + std::string{")"} };
}

uint16_t Connection::getPort()
{
    struct sockaddr_in addr;
    socklen_t len;
    if(getsockname(_socket_fd, (sockaddr*)&addr, &len) == 0){
        return ntohs(addr.sin_port);
    }
    return 0;
}








static std::vector<uint32_t> getIPs()
{
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
   // void * tmpAddrPtr=NULL;
    getifaddrs(&ifAddrStruct);
	std::vector<uint32_t> IPs;

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }

        if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
			IPs.push_back(ntohl(((struct sockaddr_in *)ifa->ifa_addr)->sin_addr.s_addr));	

            //tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            //char addressBuffer[INET_ADDRSTRLEN];
            //inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            //printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer); 
            //printf("IP Address 32bits: 0x%X\n", *(unsigned int *)tmpAddrPtr); 
        }
	}
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
    return IPs;
}






std::vector<uint32_t> Connection::getIP()
{
//    struct sockaddr_in addr;
//    socklen_t len;
//    if(getsockname(_socket_fd, (sockaddr*)&addr, &len) == 0){
//		if( ntohl(addr.sin_addr.s_addr) == 0){
//			return getIPs();
//		}
//		std::vector<uint32_t> v{ntohl(addr.sin_addr.s_addr)};
//		return v;
//    }
	return getIPs();
}

int Connection::build_socket() {
    int socketfd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    if( socketfd == -1 ) {
        throw_errno("Failed socket creation");
    }
    return socketfd;
}

hostent* Connection::get_host(const std::string& hostname) {
    auto host = gethostbyname(hostname.c_str());
    if( host == nullptr ) {
        throw ConnectionException{ hstrerror(h_errno) };
    }
    return host;
}

bool Connection::poll_socket( int socketfd, int timelimit ) {
    pollfd pfd;
    pfd.fd = socketfd;
    pfd.events = POLLIN;
    auto response = poll(&pfd, 1, timelimit);
    if( response == -1 ) {
        throw_errno("Could not poll socket");
    } else if( response == 0 ) {
        return false;
    }
    return true;
}
