#include <iostream>
#include <cstdlib>

#include "common.hpp"
#include "sync_manager.hpp"
#include "message.hpp"

#include <fstream>
#include <cstring>
#include <sys/stat.h>

void print_usage();
void upload_file( 
        const std::string& filepath,
        const std::string& username, 
        const std::string& server_addr, 
        int port 
);
void download_file( 
        const std::string& filename,
        const std::string& username, 
        const std::string& server_addr, 
        int port 
);
void list_server( 
        const std::string& username, 
        const std::string& server_addr, 
        int port 
);
void delete_file( 
        const std::string& filename,
        const std::string& username, 
        const std::string& server_addr, 
        int port 
);


int main(int argc, char** argv) {
    if( argc < 4 ) {
        print_usage();
        return 1;
    }

    std::string username{ argv[1] };
    std::string server_addr{ argv[2] };
    int port = std::atoi( argv[3] );

    // TODO: Conecta com o servidor e inicia sincronização 
    SyncManager sync_manager{ server_addr, port, username};

    bool quit = false;
    std::string user_input;

    while( !quit ) {
        std::cout << "> " << std::flush;
        std::getline( std::cin, user_input );
        auto words = split( user_input );
        // nenhum comando digitado: ignora 
        if( !std::cin.eof() && words.size() == 0 ) {
            continue;
        }
        // user is quitting
        if( std::cin.eof() || words[0] == "exit" ) {
            std::cout << "Quitting." << std::endl;
            quit = true;
        } 
        // upload command
        else if( words[0] == "upload" ) {
            if( words.size() < 2 ) {
                std::cerr << "No filename provided." << std::endl;
            } else {
                auto filepath = words[1];
                try {
                    upload_file( filepath, username, server_addr, port );
                } catch( std::exception& e ) {
                    std::cout << "could not make upload: " << e.what() << std::endl;
                }
            }
        }
        // download command
        else if( words[0] == "download" ) {
            if( words.size() < 2 ) {
                std::cerr << "No filename provided." << std::endl;
            } else {
                auto filename = words[1];
                download_file( filename, username, server_addr, port );
            }
        }
        // delete command
        else if( words[0] == "delete" ) {
            if( words.size() < 2 ) {
                std::cerr << "No filename provided." << std::endl;
            } else {
                auto filename = words[1];
                delete_file( filename, username, server_addr, port );
            }
        }
        // listar arquivos no servidor
        else if( words[0] == "list_server" ) {
            list_server( username, server_addr, port );
        }
        // listar arquivos no cliente
        else if( words[0] == "list_client" ) {
            std::cout << printdir( "sync_dir" );
        }
        // criar diretório de sincronização
        else if( words[0] == "get_sync_dir" ) {
            // TODO: criar diretório de sincronização
            std::cout << "sync_dir already exists" << std::endl;
        } 
        else {
            std::cerr << "Unknown command '" << words[0] << "'" << std::endl;
        }


    }

    // TODO: Desconecta do servidor e para sincronização.

    return 0;
}

void print_usage() {
    std::cerr
        << "Usage:" << std::endl
        << "client <username> <server address> <port>" << std::endl
    ;
}

void upload_file( 
        const std::string& filepath,
        const std::string& username, 
        const std::string& server_addr, 
        int port 
        ) {
    auto conn = Connection::connect( server_addr, port );

    // send username
    Message username_msg{ MessageType::USERNAME_NOSYNC };
    std::strncpy( username_msg.filename, username.c_str(), MESSAGE_MAX_FILENAME_SIZE );
    username_msg.file_length = 0;
    conn->send( (uint8_t*)&username_msg, sizeof(Message) );

    // open file
    std::ifstream ifs{ filepath, std::ios::binary };
    if( ifs.fail() ) {
        std::cerr << "no such file" << std::endl;
    } else {
        struct stat s;
        stat( filepath.c_str(), &s );
        int length = s.st_size;
        auto base = basename( filepath );

        Message* file_msg = (Message*)new uint8_t[sizeof(Message) + length];
        file_msg->type = MessageType::UPDATE_FILE;
        std::strncpy( file_msg->filename, base.c_str(), MESSAGE_MAX_FILENAME_SIZE );
        file_msg->file_length = length;
        ifs.read( file_msg->bytes, length );
        conn->send( (uint8_t*)file_msg, length+sizeof(Message) );
    }
}

void download_file( 
        const std::string& filename,
        const std::string& username, 
        const std::string& server_addr, 
        int port 
        ) {
    auto conn = Connection::connect( server_addr, port );

    // send username
    Message username_msg{ MessageType::USERNAME_NOSYNC };
    std::strncpy( username_msg.filename, username.c_str(), MESSAGE_MAX_FILENAME_SIZE );
    username_msg.file_length = 0;
    conn->send( (uint8_t*)&username_msg, sizeof(Message) );

    // request file
    Message request_msg{ MessageType::REQUEST_FILE };
    std::strncpy( request_msg.filename, filename.c_str(), MESSAGE_MAX_FILENAME_SIZE );
    request_msg.file_length = 0;
    conn->send( (uint8_t*)&request_msg, sizeof(Message) );

    // receive file
    auto response = conn->receive( 5000 );
    if( response.length == 0 ) {
        std::cout << "Could not download file: timed out.\n";
    } else {
        Message* msg = (Message*)response.data.get();
        if( msg->type == MessageType::NO_SUCH_FILE ) {
            std::cout << "Could not download file: file does not exists." << std::endl;
        } else {
            std::ofstream ofs{ filename.c_str(), std::ios::binary | std::ios::trunc };
            ofs.write( msg->bytes, msg->file_length );
            std::cout << "Dowloaded file: " << filename << std::endl;
        }
    }


}

void list_server( 
        const std::string& username, 
        const std::string& server_addr, 
        int port 
        ) {
    auto conn = Connection::connect( server_addr, port );

    // send username
    Message username_msg{ MessageType::USERNAME_NOSYNC };
    std::strncpy( username_msg.filename, username.c_str(), MESSAGE_MAX_FILENAME_SIZE );
    username_msg.file_length = 0;
    conn->send( (uint8_t*)&username_msg, sizeof(Message) );

    // request file list
    Message request_msg{ MessageType::REQUEST_FILE_LIST };
    request_msg.file_length = 0;
    conn->send( (uint8_t*)&request_msg, sizeof(Message) );

    // receive file
    auto response = conn->receive( 5000 );
    if( response.length == 0 ) {
        std::cerr << "Could not get file list: timed out.\n";
    } else {
        Message* msg = (Message*)response.data.get();
        if( msg->type == MessageType::FILE_LIST ) {
            std::cout << "Server files:\n" << (const char *)msg->bytes << std::endl;
        } else {
            std::cerr << "Invalid response." << std::endl;
        } 
    }
}

void delete_file( 
        const std::string& filename,
        const std::string& username, 
        const std::string& server_addr, 
        int port 
        ) {
    auto conn = Connection::connect( server_addr, port );

    // send username
    Message username_msg{ MessageType::USERNAME_NOSYNC };
    std::strncpy( username_msg.filename, username.c_str(), MESSAGE_MAX_FILENAME_SIZE );
    username_msg.file_length = 0;
    conn->send( (uint8_t*)&username_msg, sizeof(Message) );

    // request file deletion
    Message delete_msg{ MessageType::DELETE_FILE };
    std::strncpy( delete_msg.filename, filename.c_str(), MESSAGE_MAX_FILENAME_SIZE );
    delete_msg.file_length = 0;
    conn->send( (uint8_t*)&delete_msg, sizeof(Message) );


}
