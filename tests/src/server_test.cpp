#include "connection.hpp"

#include <iostream>
#include <vector>

int main() {
    std::vector<Connection> conns;
    int conn_port = 10000;
    while(true) {
        auto conn = Connection::listen( 4001 , conn_port );
        std::cout << "Connected to client (client port= " << conn.port() << ")\n";
        conns.push_back(conn);
        conn_port++;
    }

    return 0;
}
