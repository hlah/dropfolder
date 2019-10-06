#include "connection.hpp"

#include <iostream>

int main() {
    auto conn = Connection::connect( "localhost", 4001 );
    std::cout << "Connected to server (server port= " << conn.port() << ")\n";

    std::cout << "Send message: ";
    std::string str;
    std::getline( std::cin, str );

    conn.send( (uint8_t*)str.c_str(), str.size()+1 );

}
