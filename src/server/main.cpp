#include <iostream>

#include "common.hpp"
#include "server.hpp"

int main() {
    std::cout << "Hello, I am " << SERVER_NAME << ", the value of pi is " << get_pi() << std::endl;
    return 0;
}
