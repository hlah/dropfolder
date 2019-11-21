#include "sync_manager.hpp"

#include <iostream>
#include <vector>
#include <mutex>
#include <chrono>
#include "replication_server.hpp"

void print_usage();

int main(int argc, char** argv) {
    if( argc < 4 ) {
        print_usage();
        return 1;
    }
    int c_port = std::atoi( argv[1] );
    int s_port = std::atoi( argv[2] );
    uint16_t ctrl_port = std::atoi( argv[3] ); //primary server port
	if(argc == 7){
        std::string p_addr{argv[4]};                 //primary server addr
        int p_port = std::atoi( argv[5] );           //primary server port
        uint16_t p_ctrl_port = std::atoi( argv[6] ); //primary server port for ctrl messages

		//replicated server
		ReplicationServer rs(c_port, s_port, ctrl_port, p_addr, p_port, p_ctrl_port);
	}else{
		//primary server
		ReplicationServer rs(c_port, s_port, ctrl_port);
	}

    return 0;
}


void print_usage() {
    std::cerr
        << "Usage:" << std::endl
        << "server <c_port> <s_port> <ctrl_port> [p_addr p_port p_ctrl_port]" << std::endl
        << "    <c_port>      : port accepting clients" << std::endl
        << "    <s_port>      : port accepting replicated servers" << std::endl
        << "    <ctrl_port>   : port used to convey replication ctrl messages" << std::endl
        << "    <p_addr>      : ip_addr of primary server (for replicated servers)" << std::endl
        << "    <p_port>      : port of primary server (for replicated servers)" << std::endl << std::endl
        << "    <p_ctrl_port> : port of primary server used for ctrl messages (for replicated servers)" << std::endl << std::endl
        << "examples:" << std::endl <<  std::endl
        << "server listen for clients on port 12000 and for replicated servers on 13000:" << std::endl
        << "    ./server 12000 13000 14000" << std::endl <<std::endl
        << "server connecting on primary on example above:" << std::endl
        << "    ./server 12001 13001 14001 localhost 13000 14000" << std::endl;
   
}
