#include "sync_manager.hpp"

#include "message.hpp"
#include "common.hpp"

#include <fstream>
#include <cstring>
#include <sys/stat.h>
#include <cstring>
#include <sys/types.h>
#include <algorithm>

void sync_thread(SyncManager *sm)
{
	sm->syncThread();
}

void print_msg( const std::string& msg, bool client_mode );

SyncManager::SyncManager( 
        const std::string& addr, 
        int port, 
        const std::string& username
) : _conn{ Connection::connect( addr, port ) },
    _stop{ new bool{false} }
{
	this->username= username;
	this->client_mode  = true;

    _thread = std::shared_ptr<std::thread>{ new std::thread{ 
        sync_thread, 
		this,
    }};
}

SyncManager::SyncManager( int port ) 
    : _conn{ Connection::listen( port ) },
      _stop{ new bool{false} }
{
	username = std::string{};
	this->client_mode = false;

    _thread = std::shared_ptr<std::thread>{ new std::thread{ 
        sync_thread, 
		this,
    }};
}

SyncManager::~SyncManager() {
    if( _thread.unique() ) {
        *_stop = true;
        _thread->join();
    }
}

void SyncManager::syncThread() {
    if( username.size() > 0 ) {
        sync_dir = "sync_dir";
        // client mode, send username to the server
        Message msg;
        msg.type = MessageType::USERNAME;
        std::strcpy(msg.filename, username.c_str());
        msg.file_length = 0;
        _conn->send( (uint8_t*)&msg, sizeof(Message) );
        print_msg( std::string{"synching '"} + sync_dir + std::string{"'"}, client_mode );
    } else {
        // server mode, wait for client username
        ReceivedData data = _conn->receive(-1);
        Message* msg = (Message*)data.data.get();
        username = std::string{msg->filename};
        sync_dir = username + "/sync_dir";
        mkdir(username.c_str(), 0777);
        mkdir(sync_dir.c_str(), 0777);
        // send all files
        if( msg->type == MessageType::USERNAME ) {
            for( const auto& filename : listdir( sync_dir ) ) {
                send_file( filename);
            }
            print_msg( std::string{"synching '"} + sync_dir + std::string{"'"}, client_mode );
        }

    }

    // create dir TODO check if already exits and if was sucessful
    mkdir(sync_dir.c_str(), 0777);

    Watcher watcher;
    watcher.add_dir( sync_dir );


    std::vector<std::string> ignore;

    bool dropped = false;
    while( !*_stop ) {
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
                    send_file( event.filename);
                    break;
                case Watcher::EventType::REMOVED:
                    delete_file( event.filename);
                    break;
                default:
                    break;
            }
        }

        // TODO get messages from server
        ReceivedData data = _conn->receive();
        while( data.length != 0 ) {
            Message* message = (Message*)data.data.get();
            std::string filepath{ sync_dir + std::string{"/"} + message->filename };
            switch(message->type) {
                case MessageType::DELETE_FILE:
                    print_msg(std::string{"Deleted "} + filepath + std::string{" localy."}, client_mode);
                    std::remove( filepath.c_str() );
                    ignore.push_back( message->filename );
                    break;
                case MessageType::UPDATE_FILE:
                    {
                        std::ofstream ofs{ filepath.c_str(), std::ios::binary | std::ios::trunc };
                        ofs.write( message->bytes, message->file_length );
                        ignore.push_back( message->filename );
                        print_msg(std::string{"Updated "} + filepath + std::string{" localy. ("} + std::to_string(message->file_length) + std::string{" bytes)"}, client_mode);
                    }
                    break;
                case MessageType::REQUEST_FILE:
                    send_file( message->filename);
                    break;
                case MessageType::REQUEST_FILE_LIST:
                    {
                        std::string list_str = printdir(sync_dir);
                        Message* list_msg = (Message*) new uint8_t[sizeof(Message) + list_str.size() + 1];
                        list_msg->type = MessageType::FILE_LIST;
                        list_msg->file_length = list_str.size()+1;
                        memcpy( list_msg->bytes, list_str.c_str(), list_msg->file_length+1 );
                        _conn->send( (uint8_t*)list_msg, sizeof(Message) + list_str.size()+1);
                    }
                    break;
                case MessageType::DROP:
                    dropped = true;
                    *_stop = true;
                    break;
            }

            data = _conn->receive();
        }
    }

    if( !dropped ) {
        Message drop_msg{ MessageType::DROP, "", 0 };
        _conn->send( (uint8_t*)&drop_msg, sizeof(Message) );
    }
}

void SyncManager::send_file(std::string filename)
{
    std::string filepath{ sync_dir + std::string{"/"} + filename};
    std::ifstream ifs{ filepath, std::ios::binary};

    if( !ifs.fail() ) {
        struct stat s;
        stat( filepath.c_str(), &s );
        int length = s.st_size;

        Message* msg = (Message*)new char[length + sizeof(Message)];
        msg->type = MessageType::UPDATE_FILE;
        std::strncpy( msg->filename, filename.c_str(), MESSAGE_MAX_FILENAME_SIZE );
        msg->file_length = length;

        ifs.read( msg->bytes, length );

        /// TODO send msg through connection
        _conn->send( (uint8_t*)msg, length+sizeof(Message) );

        print_msg(std::string{"Updated "} + filename + std::string{" remotely. ("} + std::to_string(length) + std::string{" bytes)"}, client_mode);

        delete[] msg;
    } else {
        Message msg{MessageType::NO_SUCH_FILE};
        std::strncpy( msg.filename, filename.c_str(), MESSAGE_MAX_FILENAME_SIZE );
        msg.file_length = 0;
        _conn->send( (uint8_t*)&msg, sizeof(Message) );
    }
}

void SyncManager::delete_file( std::string filename)
{
    Message msg;
    msg.type = MessageType::DELETE_FILE;
    std::strncpy( msg.filename, filename.c_str(), MESSAGE_MAX_FILENAME_SIZE );
    msg.file_length = 0;

    /// TODO send msg through connection
    _conn->send( (uint8_t*)&msg, sizeof(Message) );

    print_msg( std::string{"deleted "} + filename + std::string{" remotely"}, client_mode );
}

void print_msg( const std::string& msg, bool client_mode ) {
    if( client_mode ) {
        std::cout << "\033[80D"; 
    }
    std::cout << msg << std::endl;
    if( client_mode ) {
        std::cout << "> " << std::flush;
    }
}
