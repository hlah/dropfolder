#include <iostream>
#include <cstdlib>

#include "common.hpp"
#include "sync_manager.hpp"

#include <fstream>

void print_usage();

int main(int argc, char** argv) {
    if( argc < 4 ) {
        print_usage();
        return 1;
    }

    std::string username{ argv[1] };
    std::string server_addr{ argv[2] };
    int port = std::atoi( argv[3] );

    // TODO: Conecta com o servidor e inicia sincronização 
    SyncManager sync_manager{ server_addr, port, username, "./sync_dir" };

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
                auto filename = words[1];
                // TODO: fazer upload do arquivo
                std::ifstream ifs{ filename, std::ios::binary };
                if( ifs.fail() ) {
                    std::cerr << "no such file" << std::endl;
                } else {
                    auto base = basename( filename );
                    std::ofstream ofs{ std::string{"sync_dir/"} + base, std::ios::binary };
                    ofs << ifs.rdbuf();
                }
            }
        }
        // download command
        else if( words[0] == "download" ) {
            if( words.size() < 2 ) {
                std::cerr << "No filename provided." << std::endl;
            } else {
                auto filename = words[1];
                std::ifstream ifs{ std::string{"sync_dir/"} + filename, std::ios::binary };
                if( ifs.fail() ) {
                    std::cerr << "no such file" << std::endl;
                } else {
                    auto base = basename( filename );
                    std::ofstream ofs{ std::string{"./"} + base, std::ios::binary };
                    ofs << ifs.rdbuf();
                }

            }
        }
        // delete command
        else if( words[0] == "delete" ) {
            if( words.size() < 2 ) {
                std::cerr << "No filename provided." << std::endl;
            } else {
                auto filename = words[1];
                // TODO: deletar arquivo
                std::cerr << "Not implemented." << std::endl;
            }
        }
        // listar arquivos no servidor
        else if( words[0] == "list_server" ) {
            std::cout << printdir( "sync_dir" );
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
