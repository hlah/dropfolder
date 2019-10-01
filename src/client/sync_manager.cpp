#include "sync_manager.hpp"

#include "message.hpp"

#include <fstream>
#include <cstring>

void sync_thread( 
        bool& stop, 
        std::string sync_dir 
);

void send_file( std::string filename );
void delete_file( std::string filename );

SyncManager::SyncManager( 
        const std::string& addr, 
        int port, 
        const std::string& username,
        const std::string& sync_dir 
        ) {

    /// TODO: connect to server


    _stop = false;
    _thread = std::thread{ 
        sync_thread, 
        std::ref(_stop),
        sync_dir
    };
}

SyncManager::~SyncManager() {
    _stop = true;
    _thread.join();
}

void sync_thread( 
        bool& stop, 
        std::string sync_dir 
) {
    Watcher watcher;
    watcher.add_dir( sync_dir );

    while( !stop ) {
        // check for sync dir modification
        auto event = watcher.next();

        switch(event.type) {
            case Watcher::EventType::MODIFIED:
            case Watcher::EventType::CREATED:
                send_file( event.filename );
                break;
            case Watcher::EventType::REMOVED:
                delete_file( event.filename );
                break;
            default:
                break;
        }

        // TODO get messages from server

    }
}

void send_file( std::string filename ) {
    std::ifstream ifs{ filename, std::ifstream::binary | std::ifstream::ate };
    long length = ifs.tellg();
    ifs.seekg(0);

    Message* msg = (Message*)new char[length + sizeof(Message)];
    msg->type = MessageType::UPDATE_FILE;
    std::strncpy( msg->filename, filename.c_str(), MESSAGE_MAX_FILENAME_SIZE );
    msg->file_length = length;

    ifs.read( msg->bytes, length );

    /// TODO send msg through connection
    std::cout << "\033[2D"; 
    std::cout << "updated '" << msg->filename << "' in the server.\n> " << std::flush;

    delete[] msg;
}

void delete_file( std::string filename ) {
    Message msg;
    msg.type = MessageType::DELETE_FILE;
    std::strncpy( msg.filename, filename.c_str(), MESSAGE_MAX_FILENAME_SIZE );
    msg.file_length = 0;

    /// TODO send msg through connection
    std::cout << "\033[80D"; 
    std::cout << "deleted '" << filename << "' in the server.\n> " << std::flush;
}

