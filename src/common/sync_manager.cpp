#include "sync_manager.hpp"

#include "message.hpp"

#include <fstream>
#include <cstring>
#include <sys/stat.h>
#include <cstring>
#include <sys/types.h>
#include <algorithm>

void sync_thread( 
        bool& stop, 
        Connection& conn,
        std::string username
);

void send_file( std::string filename, std::string sync_dir, Connection& conn );
void delete_file( std::string filename, Connection& conn );

SyncManager::SyncManager( 
        const std::string& addr, 
        int port, 
        const std::string& username,
        const std::string& sync_dir 
) : _conn{ Connection::connect( addr, port ) },
    _stop{ false }
{
    _thread = std::thread{ 
        sync_thread, 
        std::ref(_stop),
        std::ref(_conn),
        username,
    };
}

SyncManager::SyncManager( int port ) 
    : _conn{ Connection::listen( port ) },
      _stop{ false }
{
    _thread = std::thread{ 
        sync_thread, 
        std::ref(_stop),
        std::ref(_conn),
        std::string{}
    };
}

SyncManager::~SyncManager() {
    _stop = true;
    _thread.join();
}

void sync_thread( 
        bool& stop, 
        Connection& conn,
        std::string username
) {
    std::string sync_dir{};
    if( username.size() > 0 ) {
        sync_dir = "sync_dir";
        // client mode, send username to the server
        Message msg;
        msg.type = MessageType::USERNAME;
        std::strcpy(msg.filename, username.c_str());
        msg.file_length = 0;
        conn.send( (uint8_t*)&msg, sizeof(Message) );
    } else {
        // server mode, wait for client username
        ReceivedData data = conn.receive(-1);
        // TODO check if is USERNAME messsage
        Message* msg = (Message*)data.data.get();
        username = std::string{msg->filename};
        sync_dir = username + "/sync_dir";
        mkdir(username.c_str(), 0777);
    }

    // create dir TODO check if already exits and if was sucessful
    mkdir(sync_dir.c_str(), 0777);

    Watcher watcher;
    watcher.add_dir( sync_dir );

    std::vector<std::string> ignore;

    while( !stop ) {
        // check for sync dir modification
        auto event = watcher.next();

        auto ignore_it = std::find( ignore.begin(), ignore.end(), event.filename );
        // ingore files received by the server
        if( ignore_it != ignore.end() ) {
            ignore.erase( ignore_it );
        } else {
            switch(event.type) {
                case Watcher::EventType::MODIFIED:
                case Watcher::EventType::CREATED:
                    send_file( event.filename, sync_dir, conn );
                    break;
                case Watcher::EventType::REMOVED:
                    delete_file( event.filename, conn );
                    break;
                default:
                    break;
            }
        }

        // TODO get messages from server
        ReceivedData data = conn.receive();
        while( data.length != 0 ) {
            Message* message = (Message*)data.data.get();
            std::string filepath{ sync_dir + std::string{"/"} + message->filename };
            switch(message->type) {
                case MessageType::DELETE_FILE:
                    std::cout << "Deleted " << filepath << " localy." << std::endl;
                    std::remove( filepath.c_str() );
                    ignore.push_back( message->filename );
                    break;
                case MessageType::UPDATE_FILE:
                    std::cout << "Updated " << filepath << " localy." << std::endl;
                    std::ofstream ofs{ filepath.c_str(), std::ios::binary | std::ios::trunc };
                    std::cerr << message->file_length << "bytes" << std::endl;
                    ofs.write( message->bytes, message->file_length );
                    ignore.push_back( message->filename );
                    break;
            }

            data = conn.receive();
        }

    }
}

void send_file( std::string filename, std::string sync_dir, Connection& conn ) {
    std::string filepath{ sync_dir + std::string{"/"} + filename};
    std::ifstream ifs{ filepath, std::ios::binary};

    struct stat s;
    stat( filepath.c_str(), &s );
    int length = s.st_size;

    Message* msg = (Message*)new char[length + sizeof(Message)];
    msg->type = MessageType::UPDATE_FILE;
    std::strncpy( msg->filename, filename.c_str(), MESSAGE_MAX_FILENAME_SIZE );
    msg->file_length = length;

    ifs.read( msg->bytes, length );

    /// TODO send msg through connection
    conn.send( (uint8_t*)msg, length+sizeof(Message) );

    std::cout << "\033[2D"; 
    std::cerr << length << "bytes. ";
    std::cout << "updated '" << msg->filename << "' remotely.\n> " << std::flush;

    delete[] msg;
}

void delete_file( std::string filename, Connection& conn ) {
    Message msg;
    msg.type = MessageType::DELETE_FILE;
    std::strncpy( msg.filename, filename.c_str(), MESSAGE_MAX_FILENAME_SIZE );
    msg.file_length = 0;

    /// TODO send msg through connection
    conn.send( (uint8_t*)&msg, sizeof(Message) );

    std::cout << "\033[80D"; 
    std::cout << "deleted '" << filename << "' remotely.\n> " << std::flush;
}

