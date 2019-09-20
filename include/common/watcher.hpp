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
            UNKNOWKN
        };

        // Struct with event information
        struct Event {
            EventType type;
            std::string filename;
        };

        // Create a new watcher instance
        Watcher();

        ~Watcher();

        // Add direcotry to be watched
        void add_dir(const std::string& dir_name);

        // Add direcotry to be watched
        void rm_dir(const std::string& dir_name);

        // Wait for new event
        Event next();

    private:
        int _fd;
        std::unordered_map<std::string, int> _dir_map;
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
