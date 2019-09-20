#include "watcher.hpp"

int main() {
    Watcher watcher;

    watcher.add_dir("sync_dir");

    bool watch = true;
    while(watch) {
        auto event = watcher.next();
        std::cout << event.type << ": " << event.filename << std::endl;
    }

    return 0;
}
