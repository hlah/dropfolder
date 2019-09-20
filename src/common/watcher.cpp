#include "watcher.hpp"

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/inotify.h>

#define MAX_EVENTS 1024
#define BUF_LEN MAX_EVENTS * ( sizeof(inotify_event) + NAME_MAX )

Watcher::Watcher() {
    _fd = inotify_init();
    if( _fd < 0 ) {
        throw WatcherException{ std::string{ strerror(errno) } };
    }
}

Watcher::~Watcher() {
    close(_fd);
}

void Watcher::add_dir(const std::string& dir_name) {
    const auto flags = IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_TO | IN_MOVED_FROM;
    auto wd = inotify_add_watch(_fd, dir_name.c_str(), flags);
    if ( wd < 0 ) {
        throw WatcherException{ std::string{ strerror(errno) } };
    }
    _dir_map[dir_name] = wd;
}

void Watcher::rm_dir(const std::string& dir_name) {
    if( _dir_map.count(dir_name) > 0 ) {
        inotify_rm_watch(_fd, _dir_map[dir_name]);
        _dir_map.erase(dir_name);
    }
}

Watcher::Event Watcher::next() {
    char buffer[BUF_LEN] = {0};

    if ( _event_queue.empty() ) {
        // read events
        auto bytes_read = read(_fd, buffer, BUF_LEN );
        if( bytes_read < 0 ) {
            throw WatcherException{ std::string{ strerror(errno) } };
        }

        long long i = 0;
        while( i < bytes_read ) {
            inotify_event* event_ptr = (inotify_event*)&buffer[i];

            if( event_ptr->mask & (IN_CREATE | IN_MOVED_TO) ) {
                Event event{ Watcher::EventType::CREATED, event_ptr->name };
                _event_queue.push(event);
            } else if( event_ptr->mask & IN_MODIFY ) {
                Event event{ Watcher::EventType::MODIFIED, event_ptr->name };
                _event_queue.push(event);
            } else if( event_ptr->mask & (IN_DELETE | IN_MOVED_FROM) ) {
                Event event{ Watcher::EventType::REMOVED, event_ptr->name };
                _event_queue.push(event);
            }

            i += sizeof(inotify_event) + event_ptr->len;
        }
        
    }

    auto event = _event_queue.front();
    _event_queue.pop();
    return event;
}

std::ostream& operator<<(std::ostream& os, const Watcher::EventType& e_type) {
    switch(e_type) {
        case Watcher::EventType::MODIFIED:
            os << "Modified";
            break;
        case Watcher::EventType::REMOVED:
            os << "Removed";
            break;
        case Watcher::EventType::CREATED:
            os << "Created";
            break;
        default:
            os << "Unknown";
    }
    return os;
}

