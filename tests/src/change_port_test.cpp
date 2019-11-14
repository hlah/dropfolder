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
#include <iostream>

/* programa para testar a mudanca do endereco do cliente. funciona so para o localhost.
 * [locahost:15000] manda um pacote do tipo ChangeServer para o a porta do cliente passada por argumento e recebe o Ack de volta
 *
 * Usage:
 *      ./change_port_test client_port
 */

enum class PacketType : uint8_t {
    Null,
    Data,
    Ack,
    NewPort,
    Conn,
    ChangeServerAddr
};

enum class recvState : uint8_t {
    Waiting,
    RetrievingData
};

// packet struct
struct Packet {
    PacketType type;
    uint16_t msg_id;         // for PacketType::NewPort is the value of the new port
    uint32_t seqn;       
    uint32_t total;
    uint16_t payload_length;
    char* data;

    Packet( PacketType type=PacketType::Null, uint16_t msg_id=0, uint32_t seqn=0, uint32_t total=1, uint16_t payload_length=0 ) 
        : type{type}, msg_id{msg_id}, seqn{seqn}, total{total}, payload_length{payload_length} {}
};

int main(int argc, char **argv)
{
    if(argc <2){
        std::cout << "usage: change_port_test port" << std::endl;
    }

	sockaddr_in my_addr;
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(15000);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(my_addr).sin_zero, 8);

    int socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if( bind( socketfd, (sockaddr*)&my_addr, sizeof(my_addr) ) == -1) {
        std::cout << "Could not bind socket\n";
    }

	sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(atoi(argv[1]));
    dest_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(dest_addr).sin_zero, 8);


    Packet packet{ PacketType::ChangeServerAddr};

    if( sendto( socketfd, (const void*)&packet, sizeof(Packet), 0, (sockaddr*)&dest_addr, sizeof(sockaddr_in) ) == -1) {
        std::cout << "error sending packet" << std::endl;
        
    }else{
        sockaddr_in sender_addr;
        socklen_t sender_addr_len= sizeof(sockaddr_in);
        /* receive from socket */
        if(recvfrom( socketfd, (void*)&packet, sizeof(Packet), 0, (sockaddr*)&sender_addr, &sender_addr_len ) == -1) {
            std::cout << "error receiveing packet" << std::endl;
        }
        if(packet.type== PacketType::Ack){
            std::cout << "Got ACK!" << std::endl;
        }
    }

}

