#include "connection.hpp"

#include <iostream>

int main() {
    auto conn = Connection::connect( "localhost", 4001 );
    std::cout << "Connected to server (server port= " << conn.port() << ")\n";
}
