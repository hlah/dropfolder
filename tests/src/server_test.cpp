#include "connection.hpp"

#include <iostream>
#include <vector>
#include <thread>

#include <mutex>

std::mutex m;

void get_messages( std::vector<Connection>& conns );

int main() {
    std::vector<Connection> conns;

    std::thread msg_thread{ get_messages, std::ref(conns) };

    int conn_port = 10000;
    while(true) {
        auto conn = Connection::listen( 4001 , conn_port );
        std::cout << "Connected to client (client port= " << conn.port() << ")\n";
        m.lock();
        conns.push_back( std::move(conn) );
        m.unlock();
    }

    msg_thread.join();

    return 0;
}

void get_messages( std::vector<Connection>& conns ) {
    while(true) {
        m.lock();
        for( size_t i=0; i<conns.size(); i++ ) {
            ReceivedData data = conns[i].receive();
            if( data.length > 0 ) {
                std::cout << "Message from " << i << " : " << (const char*)data.data.get() << std::endl;
            } 
        }
        m.unlock();
        std::this_thread::sleep_for( std::chrono::milliseconds(200) );
    }
}
