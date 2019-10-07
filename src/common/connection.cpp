#include "connection.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>

#include <vector>

#include <iostream>

#define PAYLOAD_LEN 1024

Connection Connection::connect(const std::string& hostname, int port, int timelimit) {
    int socketfd = build_socket();
    auto host = get_host( hostname );

	sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = *((in_addr*)host->h_addr);
    bzero(&(addr).sin_zero, 8);

    Packet packet{ PacketType::Conn };
    if( sendto( socketfd, (const void*)&packet, sizeof(Packet), 0, (sockaddr*)&addr, sizeof(sockaddr_in) ) == -1) {
        throw_errno("Could not send Conn packet");
    }

    if( !poll_socket( socketfd, timelimit ) ) {
        throw ConnectionException{ "Connection timed out." };
    }

    if(recvfrom( socketfd, &packet, sizeof(Packet), 0, nullptr, nullptr ) == -1) {
        throw_errno("Could not reveive");
    }

    if( packet.type != PacketType::NewPort ) {
        throw ConnectionException{ "Unexpected response" };
    }

    // reconnect through new port
    addr.sin_family = AF_INET;
    addr.sin_port = htons(packet.msg_id);
    addr.sin_addr = *((in_addr*)host->h_addr);
    bzero(&(addr).sin_zero, 8);

    packet = Packet{ PacketType::Ack };
    if( sendto( socketfd, (const void*)&packet, sizeof(Packet), 0, (sockaddr*)&addr, sizeof(sockaddr_in) ) == -1) {
        throw_errno("Could not send Ack packet");
    }

    return std::move(Connection{socketfd, addr, timelimit});
}

Connection Connection::listen(int port, int min_range, int max_range, int timelimit) {
    int socketfd = build_socket();

	sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(server_addr).sin_zero, 8);

    if( bind( socketfd, (sockaddr*)&server_addr, sizeof(server_addr) ) == -1) {
        throw_errno("Could not bind socket");
    }

    if( !poll_socket( socketfd, -1 ) ) {
        throw ConnectionException{ "Connection timed out." };
    }

    Packet packet;
    sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(sockaddr_in);
    if( recvfrom( socketfd, &packet, sizeof(Packet), 0, (sockaddr*)&client_addr, &client_addr_size ) == -1) {
        throw_errno("Could not reveive connection");
    }
    close(socketfd);

    if( packet.type != PacketType::Conn ) {
        throw ConnectionException{ "Unexpected packet" };
    }

    
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
        throw_errno("Could not bind socket");
    }

    packet = Packet{ PacketType::NewPort, (uint16_t)current_port };
    if( sendto( socketfd, (const void*)&packet, sizeof(Packet), 0, (sockaddr*)&client_addr, sizeof(sockaddr_in) ) == -1) {
        throw_errno("Could not send packet");
    }

    if( !poll_socket( socketfd, timelimit ) ) {
        throw ConnectionException{ "Connection timed out." };
    }
    if(recvfrom( socketfd, &packet, sizeof(Packet), 0, (sockaddr*)&client_addr, &client_addr_size ) == -1) {
        throw_errno("Could not receive ack");
    }
    if( packet.type != PacketType::Ack ) {
        throw ConnectionException{ "Unexpected packet" };
    }

    return Connection{socketfd, client_addr, timelimit};
}

ReceivedData Connection::receive(int receive_timelimit) {
    if( poll_socket( _socket_fd, receive_timelimit ) ) {
        Packet* packet = (Packet*)new char[sizeof(Packet) + PAYLOAD_LEN];

        // get first packet
        socklen_t other_addr_size = sizeof(sockaddr_in);
        if(recvfrom( _socket_fd, packet, sizeof(Packet) + PAYLOAD_LEN, 0, (sockaddr*)&_other, &other_addr_size ) == -1) {
            throw_errno("Could not receive first packet");
        }


        size_t count = 1;
        size_t data_size = 0;
        size_t total = packet->total;


        // allocate buffer and bitset
        std::vector<bool> received( total, false );
        std::unique_ptr<uint8_t[]> data{ new uint8_t[total*PAYLOAD_LEN] };

        if( packet->type != PacketType::Data ) {
            throw ConnectionException{ "Unexpected packet" };
        }
        if( packet->payload_length < PAYLOAD_LEN && packet->seqn != total-1 ) {
            throw ConnectionException{ "Non last packet with less than total payload length" };
        }
        memcpy( &data[packet->seqn*PAYLOAD_LEN], &packet->data, packet->payload_length );
        received[packet->seqn] = true;
        data_size += packet->payload_length;

        // send ack
        packet->type = PacketType::Ack;
        packet->payload_length = 0;
        if( sendto( _socket_fd, (const void*)packet, sizeof(Packet), 0, (sockaddr*)&_other, sizeof(sockaddr_in) ) == -1) {
            throw_errno("Could not send ack.");
        }

        // get remaining packets
        while( count < total ) {
            poll_socket( _socket_fd, -1 );
            if(recvfrom( _socket_fd, packet, sizeof(Packet) + PAYLOAD_LEN, 0, (sockaddr*)&_other, &other_addr_size ) == -1) {
                throw_errno("Could not receive data");
            }
            if( packet->type != PacketType::Data ) {
                throw ConnectionException{ "Unexpected packet" };
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

            // send ack
            packet->type = PacketType::Ack;
            packet->payload_length = 0;
            if( sendto( _socket_fd, (const void*)packet, sizeof(Packet), 0, (sockaddr*)&_other, sizeof(sockaddr_in) ) == -1) {
                throw_errno("Could not send ack.");
            }

        }
        return ReceivedData{ std::move(data), data_size };
    } else {
        return ReceivedData{};
    }
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
        if( poll_socket( _socket_fd, _timelimit ) ) {
            Packet ack_packet;
            socklen_t other_addr_size = sizeof(sockaddr_in);
            if(recvfrom( _socket_fd, &ack_packet, sizeof(Packet), 0, (sockaddr*)&_other, &other_addr_size ) == -1) {
                throw_errno("Could not receive ack");
            }

            if( ack_packet.type != PacketType::Ack ) {
                throw ConnectionException{"Received unexpected packet."};
            }

            // update counters
            packet->seqn++;
            count+=1;
            size -= PAYLOAD_LEN;
            tries = 0;
        } else {
            tries++;
        }

        if( tries == _trylimit-1 ) {
            throw ConnectionException{"Connection timed out"};
        }
    }
}

Connection::~Connection() {
    if(_socket_fd != 0) {
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


/// Private functions

void Connection::throw_errno( const std::string& str ) {
    throw ConnectionException{ str + std::string{": "} + strerror( errno ) + std::string{" ("} + std::to_string(errno) + std::string{")"} };
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
