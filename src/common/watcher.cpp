#include "watcher.hpp"

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <stdio.h>

#include "common.hpp"

#define MAX_EVENTS 1024
#define BUF_LEN MAX_EVENTS * ( sizeof(inotify_event) + NAME_MAX )

Watcher::Watcher() {
    _fd = inotify_init();
    if( _fd < 0 ) {
        throw WatcherException{ std::string{ strerror(errno) } };
    }
    int flags = fcntl(_fd, F_GETFL, 0);
    fcntl(_fd, F_SETFL, flags | O_NONBLOCK);
}

Watcher::~Watcher() {
    close(_fd);
}

void Watcher::add_dir(const std::string& dir_name, unsigned int recurse) {
    const auto flags = IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_TO | IN_MOVED_FROM;
    auto wd = inotify_add_watch(_fd, dir_name.c_str(), flags);
    std::cerr << "Watching " << dir_name << std::endl;
    if ( wd < 0 ) {
        throw WatcherException{ std::string{ strerror(errno) } + std::string{". dir_name: "}+dir_name };
    }
    if( recurse > 0 ) {
        for( const auto& subdir : listdir( dir_name, true ) ) {
            if( subdir != std::string{".."} && subdir != std::string{"."} ) {
                auto subdir_path = dir_name + std::string{"/"} + subdir;
                Event event{ Watcher::EventType::NEW_DIRECTORY, subdir_path };
                _event_queue.push(event);
                add_dir( subdir_path, recurse-1 );
            }
        }
    }
    _dir_map[dir_name] = wd;
    _dir_map_rev[wd] = dir_name;
    _dir_depths[wd] = recurse;
}

void Watcher::rm_dir(const std::string& dir_name) {
    if( _dir_map.count(dir_name) > 0 ) {
        inotify_rm_watch(_fd, _dir_map[dir_name]);
        _dir_map_rev.erase(_dir_map[dir_name]);
        _dir_depths.erase(_dir_map[dir_name]);
        _dir_map.erase(dir_name);
    }
}

Watcher::Event Watcher::next() {
    char buffer[BUF_LEN] = {0};

    if ( _event_queue.empty() ) {
        // read events
        auto bytes_read = read(_fd, buffer, BUF_LEN );
        if( bytes_read <= 0 ) {
            return Event{ Watcher::EventType::NOEVENT, "" };
        }

        long long i = 0;
        while( i < bytes_read ) {
            inotify_event* event_ptr = (inotify_event*)&buffer[i];
            auto full_name = std::string{ _dir_map_rev[event_ptr->wd] } + std::string{"/"} + std::string{ event_ptr->name };

            if( event_ptr->mask & (IN_CREATE | IN_MOVED_TO) && !(event_ptr->mask & IN_ISDIR) ) {
                Event event{ Watcher::EventType::CREATED, full_name };
                _event_queue.push(event);
            } else if( event_ptr->mask & (IN_CREATE | IN_MOVED_TO) && (event_ptr->mask & IN_ISDIR) ) {
                Event event{ Watcher::EventType::NEW_DIRECTORY, full_name };
                _event_queue.push(event);
                if( _dir_depths[ event_ptr->wd ] > 0 ) {
                    add_dir( full_name, _dir_depths[ event_ptr->wd ]-1 );
                }
            } else if( event_ptr->mask & IN_MODIFY && !(event_ptr->mask & IN_ISDIR) ) {
                Event event{ Watcher::EventType::MODIFIED, full_name };
                _event_queue.push(event);
            } else if( event_ptr->mask & (IN_DELETE | IN_MOVED_FROM) && !(event_ptr->mask & IN_ISDIR) ) {
                Event event{ Watcher::EventType::REMOVED, full_name };
                _event_queue.push(event);
            } 

            i += sizeof(inotify_event) + event_ptr->len;
        }
        
    }

    Event event{  Watcher::EventType::NOEVENT, ""  };
    if( _event_queue.size() > 0 ) {
        event = _event_queue.front();
        _event_queue.pop();
    }
    return event;
}

void Watcher::discard() {
    char buffer[BUF_LEN] = {0};
    auto bytes_read = read(_fd, buffer, BUF_LEN );
    while( bytes_read > 0 ) {
        bytes_read = read(_fd, buffer, BUF_LEN );
    }

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
        case Watcher::EventType::NOEVENT:
            os << "No event";
            break;
        default:
            os << "Unknown";
    }
    return os;
}

