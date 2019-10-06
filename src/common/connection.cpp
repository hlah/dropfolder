#include "connection.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>

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

    poll_socket( socketfd, timelimit );

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

    return Connection{socketfd};
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

    poll_socket( socketfd, -1 );

    Packet packet;
    sockaddr_in client_addr;
    socklen_t client_addr_size;
    if( recvfrom( socketfd, &packet, sizeof(Packet), 0, (sockaddr*)&client_addr, &client_addr_size ) == -1) {
        throw_errno("Could not reveive connection");
    }
    close(socketfd);

    if( packet.type != PacketType::Conn ) {
        throw ConnectionException{ "Unexpected packet" };
    }

    
    // create new socket for connection maintenance
    socketfd = build_socket();
    int current_port = min_range;
    while( current_port <= max_range ) {
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(current_port);
        server_addr.sin_addr.s_addr = INADDR_ANY;
        bzero(&(server_addr).sin_zero, 8);

        if( bind( socketfd, (sockaddr*)&server_addr, sizeof(server_addr) ) == 0 ) {
            break;
        }
    }

    if( current_port > max_range ) {
        throw_errno("Could not bind socket");
    }

    packet = Packet{ PacketType::NewPort, (uint16_t)current_port };
    if( sendto( socketfd, (const void*)&packet, sizeof(Packet), 0, (sockaddr*)&client_addr, sizeof(sockaddr_in) ) == -1) {
        throw_errno("Could not send packet");
    }

    poll_socket( socketfd, timelimit );
    if(recvfrom( socketfd, &packet, sizeof(Packet), 0, (sockaddr*)&client_addr, &client_addr_size ) == -1) {
        throw_errno("Could not receive ack");
    }
    if( packet.type != PacketType::Ack ) {
        throw ConnectionException{ "Unexpected packet" };
    }

    return Connection{socketfd};
}


void Connection::throw_errno( const std::string& str ) {
    throw ConnectionException{ str + std::string{": "} + strerror( errno ) };
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

void Connection::poll_socket( int socketfd, int timelimit ) {
    pollfd pfd;
    pfd.fd = socketfd;
    pfd.events = POLLIN;
    auto response = poll(&pfd, 1, timelimit);
    if( response == -1 ) {
        throw_errno("Could not poll socket");
    } else if( response == 0 ) {
        throw ConnectionException{ "Connection timed out." };
    }
}
