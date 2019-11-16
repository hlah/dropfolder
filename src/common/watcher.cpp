#include "watcher.hpp"

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/inotify.h>
#include <fcntl.h>

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
    if ( wd < 0 ) {
        throw WatcherException{ std::string{ strerror(errno) } };
    }
    if( recurse > 0 ) {
        for( const auto& subdir : listdir( dir_name, true ) ) {
            auto subdir_path = dir_name + std::string{"/"} + subdir;
            add_dir( subdir_path, recurse-1 );
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

            std::string part_name;
            auto first = full_name.find("/");
            if( first != std::string::npos ) {
                part_name = std::string{ full_name, first+1 };
            }

            if( event_ptr->mask & (IN_CREATE | IN_MOVED_TO) && !(event_ptr->mask & IN_ISDIR) ) {
                Event event{ Watcher::EventType::CREATED, part_name };
                _event_queue.push(event);
            } else if( event_ptr->mask & IN_MODIFY && !(event_ptr->mask & IN_ISDIR) ) {
                Event event{ Watcher::EventType::MODIFIED, part_name };
                _event_queue.push(event);
            } else if( event_ptr->mask & (IN_DELETE | IN_MOVED_FROM) && !(event_ptr->mask & IN_ISDIR) ) {
                Event event{ Watcher::EventType::REMOVED, part_name };
                _event_queue.push(event);
            } else if( event_ptr->mask & (IN_CREATE | IN_MOVED_TO) && (event_ptr->mask & IN_ISDIR) ) {
                if( _dir_depths[ event_ptr->wd ] > 0 ) {
                    add_dir( full_name, _dir_depths[ event_ptr->wd ]-1 );
                }
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

