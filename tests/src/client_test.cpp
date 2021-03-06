#include "connection.hpp"

#include <iostream>

int main() {
    auto conn = Connection::connect(ConnMode::Normal, "localhost", 4001 );
    std::cout << "Connected to server (server port= " << conn->getPeerPort() << ")\n";
    std::cout << "my port: " << conn->getPort() << ")\n";

    while( true ) {
        std::cout << "Send message: ";
        std::string str;
        std::getline( std::cin, str );
        conn->send( (uint8_t*)str.c_str(), str.size()+1 );
    }


}
