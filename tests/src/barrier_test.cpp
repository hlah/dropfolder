#include "barrier.hpp"

#include <thread>
#include <iostream>
#include <chrono>

void f( Barrier& b, int i ) {
    std::cout << i << " waiting...\n";
    b.wait();
    std::cout << i << " leaving...\n";
}

int main() {

    Barrier b{2};

    auto t1 = std::thread{ f, std::ref(b), 1 };
    std::this_thread::sleep_for( std::chrono::seconds(2) );
    b.increment_n();
    auto t2 = std::thread{ f, std::ref(b), 2 };

    t1.join();
    t2.join();
    std::cout << "\n";

    t1 = std::thread{ f, std::ref(b), 1 };
    std::this_thread::sleep_for( std::chrono::seconds(2) );
    t2 = std::thread{ f, std::ref(b), 2 };
    std::this_thread::sleep_for( std::chrono::seconds(2) );
    auto t3 = std::thread{ f, std::ref(b), 3 };

    t1.join();
    t2.join();
    t3.join();

    return 0;
}
