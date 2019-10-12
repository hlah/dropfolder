#include "sync_manager.hpp"

#include <iostream>
#include <vector>

void print_usage();

int main(int argc, char** argv) {
    if( argc < 2 ) {
        print_usage();
        return 1;
    }
    int port = std::atoi( argv[1] );

    std::vector<SyncManager> conns;
    while( true ) {
        conns.emplace_back( port );
        for( int i=0; i<conns.size(); i++ ) {
            if( !conns[i].alive() ) {
                std::cout << "Dropped connection: " << conns[i].status() << std::endl;
                conns.erase( conns.begin() + i );
                i--;
            }
        }
    }

    return 0;
}

void print_usage() {
    std::cerr
        << "Usage:" << std::endl
        << "server <port>" << std::endl
    ;
}
