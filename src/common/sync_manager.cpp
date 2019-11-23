#include "sync_manager.hpp"

#include "message.hpp"
#include "common.hpp"

#include <fstream>
#include <cstring>
#include <sys/stat.h>
#include <cstring>
#include <sys/types.h>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>

void sync_thread(SyncManager *sm)
{
	sm->syncThread();
}

void print_msg( const std::string& msg, bool client_mode );

SyncManager::SyncManager( 
        const std::string& addr, 
        int port, 
        const std::string& username
) : _stop{ new bool{false} },
    _conn{ Connection::connect( addr, port ) }
    
{
	this->username= username;
	this->client_mode  = true;
	this->operationMode = SyncMode::Client;
	this->watchingDir= false;

    _thread = std::shared_ptr<std::thread>{ new std::thread{ 
        sync_thread, 
		this,
    }};
}

SyncManager::SyncManager( 
        const std::string& addr, 
        int port
) : _stop{ new bool{false} },
    _conn{ Connection::connect( addr, port ) }
    
{
	this->username= std::string{};
	this->client_mode  = true;
	this->operationMode = SyncMode::Replicated;
	this->watchingDir= false;

    std::cout << "host port: " << _conn->getPort() << std::endl;

    _thread = std::shared_ptr<std::thread>{ new std::thread{ 
        sync_thread, 
		this,
    }};
}

SyncManager::SyncManager( int port, SyncMode mode)
    : _stop{ new bool{false} },
      _conn{ Connection::listen( port ) }
{
	username = std::string{};
	this->client_mode = false;
	this->operationMode = mode;
	this->watchingDir= false;

    _thread = std::shared_ptr<std::thread>{ new std::thread{ 
        sync_thread, 
		this,
    }};

    if(mode==SyncMode::Server){
        std::unique_lock<std::mutex> lck (username_mutex);
        while(getUsername()==""){
			std::cout << "will block waiting for username!" << std::endl;
            username_cv.wait(lck);
        }
    }
}

SyncManager::~SyncManager() {
    if( _thread.unique() ) {
        *_stop = true;
        _thread->join();
    }
}

void SyncManager::syncThread() {
	switch(operationMode)
	{
        case SyncMode::Client:
            {
                sync_dir = "sync_dir";
                // client mode, send username to the server
                Message msg;
                msg.type = MessageType::USERNAME;
                std::strcpy(msg.filename, username.c_str());
                msg.file_length = 0;
                _conn->send( (uint8_t*)&msg, sizeof(Message) );
                print_msg( std::string{"synching '"} + sync_dir + std::string{"'"}, client_mode );
            }
            break;
        case SyncMode::Server:
            // server mode, wait for client username
            {
                ReceivedData data = _conn->receive(-1);
                Message* msg = (Message*)data.data.get();
                if( msg->type == MessageType::USERNAME ) {
                    username = std::string{msg->filename};

                    username_cv.notify_one();

                    sync_dir = std::string{"users/"} + username + "/sync_dir";
                    mkdir("users", 0777);
                    mkdir((std::string{"users/"} + username).c_str(), 0777);
                    mkdir(sync_dir.c_str(), 0777);
                    // send all files
                    send_files( std::string{"users/"}+username, std::string{"sync_dir"}, username );
                    print_msg( std::string{"synching '"} + sync_dir + std::string{"'"}, client_mode );
                }else{
                    std::cout << "unexpected message. expecting "
                              << (uint8_t)MessageType::USERNAME
                              << "came" << (uint8_t)msg->type << std::endl;
                }
            }
            break;
        case SyncMode::Primary:
            // Primary server mode, transmit all client directories
            {
                //TODO: ZIP CLIENTS DIRECTORY AND TRANSMIT
                //      TODO JOB No 4
//                std::cerr <<  "TODO: Fix Sync between primary and Replicated Servers";
//                std::abort();
                sync_dir = "users";
                mkdir(sync_dir.c_str(), 0777);
                // send all files
                //filename= zip(sync_dir);
                //send_file( filename);

                send_files( std::string{"./"}, std::string{"users/*"}, std::string{"users"});
                print_msg( std::string{"synching '"} + sync_dir + std::string{"'"}, client_mode );
            }
            break;
        case SyncMode::Replicated:
            {
                //TODO: ZIP CLIENTS DIRECTORY AND TRANSMIT
                //      TODO JOB No 4
//                std::cerr <<  "TODO: Fix Sync between primary and Replicated Servers";
//                std::abort();

                ReceivedData data = _conn->receive(-1);
                Message* msg = (Message*)data.data.get();
                sync_dir = "users";
                mkdir(sync_dir.c_str(), 0777);
         
                if(msg->type == MessageType::UPDATE_FILES){
					{
						std::cout << "msg->filename: " << msg->filename << "." << std::endl;
                        std::ofstream ofs{ msg->filename , std::ios::binary | std::ios::trunc };
                        ofs.write( msg->bytes, msg->file_length );
					}
                        get_files( msg->filename );
                }else{
                    std::cout << "unexpected message. expecting "
                              << (uint8_t)MessageType::UPDATE_FILES
                              << "came" << (uint8_t)msg->type << std::endl;
                }

                print_msg( std::string{"synching '"} + sync_dir + std::string{"'"}, client_mode );
            }
            break;
	}

    // create dir TODO check if already exits and if was sucessful
    mkdir(sync_dir.c_str(), 0777);

    Watcher watcher;
    std::cerr << "Watching " << sync_dir << std::endl;
    if(operationMode == SyncMode::Primary ||
       operationMode == SyncMode::Replicated){ 
        watcher.add_dir( sync_dir, 1);
    }else{
        watcher.add_dir( sync_dir );
    }
	this->watchingDir= true;

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
                    std::cerr << "Sending " << event.filename << std::endl;
                    send_file( event.filename );
                    break;
                case Watcher::EventType::REMOVED:
                    delete_file( event.filename);
                    break;
                default:
                    break;
            }
        }

        // TODO get messages from server
        while( _conn->hasNewMessage()) {
            ReceivedData data = _conn->receive();
            Message* message = (Message*)data.data.get();
            std::string filepath{ sync_dir + std::string{"/"} + message->filename };
            switch(message->type) {
                case MessageType::DELETE_FILE:
                    print_msg(std::string{"Deleted "} + filepath + std::string{" localy."}, client_mode);
                    std::remove( filepath.c_str() );
                    ignore.push_back( filepath );
                    break;
                case MessageType::UPDATE_FILE:
                    {
                        std::ofstream ofs{ filepath.c_str(), std::ios::binary | std::ios::trunc };
                        ofs.write( message->bytes, message->file_length );
                        ignore.push_back( filepath );
                        print_msg(std::string{"Updated "} + filepath + std::string{" localy. ("} + std::to_string(message->file_length) + std::string{" bytes)"}, client_mode);
                    }
                    break;
                case MessageType::UPDATE_FILES:
                    {
                        std::ofstream ofs{message->filename, std::ios::binary | std::ios::trunc };
                        ofs.write( message->bytes, message->file_length );
                    }
                    std::cout << message->filename << " written(" << message->file_length << " bytes)." << std::endl;

                    get_files( message->filename );
                    watcher.discard();
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
                        delete[] list_msg;
                    }
                    break;
                case MessageType::DROP:
                    dropped = true;
                    *_stop = true;
                    break;

                default:
                    break;
            }

        }
    }

    if( !dropped ) {
        Message drop_msg{ MessageType::DROP, "", 0 };
        _conn->send( (uint8_t*)&drop_msg, sizeof(Message) );
    }
}

void SyncManager::send_file(std::string filepath)
{
    std::ifstream ifs{ filepath, std::ios::binary};

    // remove sync_dir prefix
    std::string filename{ filepath, sync_dir.size()+1 };

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

void SyncManager::delete_file( std::string filepath)
{
    // remove sync_dir prefix
    std::string filename{ filepath, sync_dir.size()+1 };

    Message msg;
    msg.type = MessageType::DELETE_FILE;
    std::strncpy( msg.filename, filename.c_str(), MESSAGE_MAX_FILENAME_SIZE );
    msg.file_length = 0;

    /// TODO send msg through connection
    _conn->send( (uint8_t*)&msg, sizeof(Message) );

    print_msg( std::string{"deleted "} + filename + std::string{" remotely"}, client_mode );
}



void SyncManager::send_files( const std::string& root_dir, const std::string& files, const std::string name ) {
    auto current_dir = std::string( get_current_dir_name() );
    chdir( root_dir.c_str() );
    std::string name_ext = name + std::string{".tar.gz"};
    std::string command 
        = std::string{"tar -czf "} 
        + name_ext
        + std::string{" "}
        + files;

    std::system( command.c_str() );

    // send compressed file

    std::ifstream ifs{ name_ext, std::ios::binary};

    struct stat s;
    stat( name_ext.c_str(), &s );
    int length = s.st_size;

    Message* msg = (Message*)new char[length + sizeof(Message)];
    msg->type = MessageType::UPDATE_FILES;
    std::strncpy( msg->filename, name_ext.c_str(), MESSAGE_MAX_FILENAME_SIZE );
    msg->file_length = length;

    ifs.read( msg->bytes, length );
    _conn->send( (uint8_t*)msg, length+sizeof(Message) );

    print_msg(std::string{"Updated "} + name_ext + std::string{" remotely. ("} + std::to_string(length) + std::string{" bytes)"}, client_mode);

    chdir( current_dir.c_str() );
    std::remove( name_ext.c_str() );
    delete[] msg;
}

void SyncManager::get_files( const std::string& file ) {
    std::string command 
        = std::string{"tar -xzf "} 
        + file;

    std::cout << "executing command:" << command << std::endl;
    std::system( command.c_str() );
    std::remove( file.c_str() );
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

