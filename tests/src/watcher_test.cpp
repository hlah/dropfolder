#include "watcher.hpp"

#include <thread>

int main() {
    Watcher watcher;

    watcher.add_dir("sync_dir");

    bool watch = true;
    while(watch) {
        auto event = watcher.next();
        std::cout << event.type << ": " << event.filename << std::endl;
        if( event.type == Watcher::EventType::NOEVENT )
            std::this_thread::sleep_for( std::chrono::milliseconds(200) );
    }

    return 0;
}
