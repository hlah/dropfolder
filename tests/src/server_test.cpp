#include "connection.hpp"

#include <iostream>
#include <vector>

int main() {
    std::vector<Connection> conns;
    int conn_port = 10000;
    auto conn = Connection::listen( 4001 , conn_port );
    std::cout << "Connected to client (client port= " << conn.port() << ")\n";

    ReceivedData data{};
    while( data.length == 0 ) {
        data = conn.receive();
    }

    std::cout << "Message received: " << (const char*)data.data.get() << std::endl;

    return 0;
}
