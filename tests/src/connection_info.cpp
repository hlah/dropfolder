#include "sync_manager.hpp"

#include <iostream>
#include <vector>
#include <mutex>
#include <chrono>

std::vector<std::unique_ptr<SyncManager>> conns_g;
std::mutex conn_mutex_g;

void print_usage();
void accept_thread(int port);
void clean_thread();
void addr_thread();

int main(int argc, char** argv) {
    if( argc < 2 ) {
        print_usage();
        return 1;
    }
    int port = std::atoi( argv[1] );

    //std::vector<std::unique_ptr<SyncManager>> conns;
    auto accepter = std::thread{accept_thread, port};
    auto cleaner = std::thread{clean_thread};
    auto verify_addr = std::thread{addr_thread};

	

    accepter.join();
    cleaner.join();
    verify_addr.join();

    return 0;
}


void addr_thread()
{
	while(true){
		conn_mutex_g.lock();
		for( const auto& conn : conns_g) {
			std::cout << conn->getUsername() << " syncAddr: [" 
					  << conn->getPeerIP() << ":"
					  << conn->getPeerPort() << "]\n";
		}
		conn_mutex_g.unlock();
		std::cout << std::endl;
		std::this_thread::sleep_for( std::chrono::seconds( 5 ) );
	}
}

void accept_thread(int port) {
    while( true ) {
        auto sync_manager = std::unique_ptr<SyncManager>( new SyncManager{port} );
        conn_mutex_g.lock();
        conns_g.push_back( std::move(sync_manager) );
        conn_mutex_g.unlock();
    }
}

void clean_thread() {
    while(true) {
        conn_mutex_g.lock();
        for( int i=0; i<conns_g.size(); i++ ) {
            if( !conns_g[i]->alive() ) {
                std::cerr << "Dropped connection\n";
                conns_g.erase( conns_g.begin() + i );
                i--;
            }
        }
        conn_mutex_g.unlock();
        std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
    }
}

void print_usage() {
    std::cerr
        << "Usage:" << std::endl
        << "server <port>" << std::endl
    ;
}
