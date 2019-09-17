#include <iostream>

#include "common.hpp"
#include "client.hpp"

int main() {
    std::cout << "Hello, I am " << CLIENT_NAME << ", the value of pi is " << get_pi() << std::endl;
    return 0;
}
