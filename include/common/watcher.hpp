#ifndef _DROPFOLDER_WATCHER_HPP_
#define _DROPFOLDER_WATCHER_HPP_

#include <string>
#include <unordered_map>
#include <queue>
#include <iostream>

class Watcher {
    public:
        // Event types
        enum class EventType {
            MODIFIED,
            REMOVED,
            CREATED,
            UNKNOWKN,
            NOEVENT,
        };

        // Struct with event information
        struct Event {
            EventType type;
            std::string filename;
        };

        // Create a new watcher instance
        Watcher();

        ~Watcher();

        // Add directory to be watched, with recursive addition depth as second parameter
        void add_dir(const std::string& dir_name, unsigned int recurse=0);

        // Add direcotry to be watched
        void rm_dir(const std::string& dir_name);

        // Discard all evends
        void discard();

        // Wait for new event
        Event next();

    private:
        int _fd;
        std::unordered_map<std::string, int> _dir_map;
        std::unordered_map<int, std::string> _dir_map_rev;
        std::unordered_map<int, unsigned int> _dir_depths;
        std::queue<Event> _event_queue;
};

std::ostream& operator<<(std::ostream& os, const Watcher::EventType& e_type);

class WatcherException : public std::exception {
    public:
        WatcherException(const std::string& msg) : _msg{msg} {}
        const char* what() const throw() { return _msg.c_str(); }
    private:
        const std::string _msg;
};

#endif // _DROPFOLDER_WATCHER_HPP_
