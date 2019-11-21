#include "replication_server.hpp"
#include "sync_manager.hpp"
#include "message.hpp"

#include <fstream>
#include <iostream>
#include <chrono>             // std::chrono::seconds


void accept_servers_thread(ReplicationServer *rs) {
	rs->acceptServersThread();
}

void accept_clients_thread(ReplicationServer *rs) {
	rs->acceptClientsThread();
}

void clean_thread(ReplicationServer *rs)
{
	rs->cleanClientsThread();
}

void accept_ctrl_thread(ReplicationServer *rs)
{
    rs->acceptCtrlThread();
}

void failure_detection_thread(ReplicationServer *rm)
{
	rm->failureDetectionThread();
}

ReplicationServer::ReplicationServer(uint16_t c_port, uint16_t s_port, uint16_t ctrl_port)
{
	state= ServerState::Primary;
	this->client_listen_port= c_port;
	this->server_listen_port= s_port;
    this->ctrl_port= ctrl_port;

    auto accepter_clients = std::thread{accept_clients_thread, this};
    auto accepter_servers = std::thread{accept_servers_thread, this};
    auto cleaner_clients = std::thread{clean_thread, this};
    auto accepter_cntrl_servers = std::thread{accept_ctrl_thread, this};
    auto failure_detection = std::thread{failure_detection_thread, this};


    accepter_clients.join();
    accepter_servers.join();
    cleaner_clients.join();
    accepter_cntrl_servers.join();
    failure_detection.join();
}


ReplicationServer::ReplicationServer(uint16_t c_port, uint16_t s_port, uint16_t ctrl_port, const std::string& p_addr, int p_sync_port, uint16_t p_ctrl_port)
{
	state= ServerState::Replication;

	this->client_listen_port= c_port;
	this->server_listen_port= s_port;

    this->ctrl_port= ctrl_port;
    this->p_ctrl_port= p_ctrl_port;
    this->p_addr= p_addr;

    primary_sync = std::unique_ptr<SyncManager>( new SyncManager{p_addr, p_sync_port});

    auto conn = std::shared_ptr<Connection> (Connection::connect((const std::string&) p_addr, (int)p_ctrl_port) );
    this->ctrl_port= conn->getPort();
    cntrl_conns.push_back( std::move(conn) );


    auto accepter_clients = std::thread{accept_clients_thread, this};
    auto accepter_servers = std::thread{accept_servers_thread, this};
    auto cleaner_clients = std::thread{clean_thread, this};
    auto accepter_cntrl_servers = std::thread{accept_ctrl_thread, this};
    auto failure_detection = std::thread{failure_detection_thread, this};



    accepter_clients.join();
    accepter_servers.join();
    cleaner_clients.join();
    accepter_cntrl_servers.join();
    failure_detection.join();
}

void ReplicationServer::acceptServersThread() {
    while( true ) {
        auto sync_manager = std::unique_ptr<SyncManager>( new SyncManager{server_listen_port, 
														      SyncManager::SyncMode::Primary});

        peers_info_mutex.lock();
        {
            std::ofstream ofs{"users/peers.info", std::ios::binary | std::ios::app };
            //S - ip port
            ofs << "ServerSync _ " << std::hex << sync_manager->getPeerIP() << " " 
                << sync_manager->getPeerPort() << std::endl;
        }
        peers_info_mutex.unlock();

        server_syncd_mutex.lock();
        server_syncd.push_back( std::move(sync_manager) );
        server_syncd_mutex.unlock();
    }
}

void ReplicationServer::acceptClientsThread() {
    while( true ) {
        auto sync_manager = std::unique_ptr<SyncManager>( new SyncManager{client_listen_port, 
														      SyncManager::SyncMode::Server});

        peers_info_mutex.lock();
        {
            std::ofstream ofs{"users/peers.info", std::ios::binary | std::ios::app };
            //C username ip port
            ofs << "Client " << sync_manager->getUsername() << " "
                << std::hex << sync_manager->getPeerIP() << " " 
                << sync_manager->getPeerPort() << std::endl; 
        }
        peers_info_mutex.unlock();

        client_syncd_mutex.lock();
        client_syncd.push_back( std::move(sync_manager) );
        client_syncd_mutex.unlock();
    }
}

void ReplicationServer::cleanClientsThread() {
    while(true) {
        client_syncd_mutex.lock();
        for( uint32_t i=0; i<client_syncd.size(); i++ ) {
            if( !client_syncd[i]->alive() ) {
                std::cerr << "Dropped connection\n";
                client_syncd.erase( client_syncd.begin() + i );
                i--;
            }
        }
        client_syncd_mutex.unlock();
        std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
    }
}

void ReplicationServer::acceptCtrlThread()
{
    while( true ) {
        if(state== ServerState::Primary){
            std::cout << "primary listening on " << ctrl_port << std::endl;
            auto conn = std::shared_ptr<Connection> (Connection::listen (ctrl_port));

            peers_info_mutex.lock();
            {
                std::ofstream ofs{"users/peers.info", std::ios::binary | std::ios::app };
                ofs << "ServerConn _ " << std::hex << conn->getPeerIP() << " " << conn->getPeerPort() << std::endl; 
            }
            peers_info_mutex.unlock();

            cntrl_conns_mutex.lock();
            cntrl_conns.push_back( std::move(conn) );
            cntrl_conns_mutex.unlock();
        }else{
            std::this_thread::sleep_for( std::chrono::seconds(1));
        }
    }
}


void ReplicationServer::failureDetectionThread()
{
    while(true){
        if(state == ServerState::Replication){
            Message ping_msg{MessageType::PING_REQUEST, "", 0};
            std::shared_ptr<Connection> conn;

            cntrl_conns_mutex.lock();
            conn= cntrl_conns[0];
            cntrl_conns_mutex.unlock();

            try{
                conn->send((uint8_t*)&ping_msg, sizeof(Message));

                ReceivedData response = conn->receive( 5000 );
                if( response.length == 0 ) {
                    std::cout << "No response: timed out. Begin Election\n";
                    //TODO: state= ServerState::Electing;

                } else {
                    Message* msg = (Message*)response.data.get();
                    if( msg->type == MessageType::PING_RESPONSE ) {

                    } else {
                        std::cout << "Got unexpected Response(type: " << (uint8_t)msg->type << ")." << std::endl;
                    }
                }
            }catch(std::exception& e){
                std::cout << "No response: timed out on send. Begin Election\n";
                //TODO: state= ServerState::Electing;
            }

            std::this_thread::sleep_for( std::chrono::seconds( 2 ) );

		}else if(state == ServerState::Primary){
            cntrl_conns_mutex.lock();
            for(auto conn : cntrl_conns){
                if(conn->hasNewMessage()){
                    ReceivedData recvd= conn->receive();
                    Message* msg = (Message*)recvd.data.get();
                    if( msg->type == MessageType::PING_REQUEST ) {
                        Message ping_msg{MessageType::PING_RESPONSE, "", 0};
                        conn->send((uint8_t*)&ping_msg, sizeof(Message));

                    } else {
                        std::cout << "Got unexpected Message(type: " << (uint8_t)msg->type << ")." << std::endl;
                    }
                }
            }
            cntrl_conns_mutex.unlock();
        }
	}
}


#if 0
void ImAlive_thread_starter(ReplicationServer *rm);

ReplicationServer::ReplicationServer()
{
	_thread_listenToRMs= new std::thread{ RMAddToGroupThread_starter, this};
	_thread_alive= new std::thread{ ImAlive_thread_starter, this};
	_thread_failure_detection= new std::thread{ failure_detection_thread_starter, this};
}


void ReplicationServer::failureDetectionThread()
{
	if(status == State::Replication){
		while(true){
            std::unique_lock<std::mutex> lck (failure_detection_mutex);
            while(!gotRMMessage){
                if(cv.wait_for(lck, std::chrono::seconds(ReplicationManager::FailureDetectionTimeout)) == std::cv_status::timeout){
                    //TODO::PRIMARY RM failed, invoke election...
                    std::cout << "PRIMARY FAILED" << std::endl;
                    return;
                }
            }
            gotRMMessage = false;
		}
	}
}


void RMAddToGroupThread_starter(ReplicatedManager *rm)
{
	rm->RMAddToGroup_thread();
}

void ReplicationManger::RMAddToGroup_thread()
{
    //TODO: new constructor for syncManager that embeds RM sync behaviour
	auto sync_manager = std::unique_ptr<SyncManager>( new SyncManager{port, isReplicatedServer=true} );
	RM_group_add_mutex.lock();
	RM_group.push_back( std::move(sync_manager) );
	RM_group_add_mutex.unlock();
}


void ImAlive_thread_starter(ReplicationManager *rm)
{
	rm->ImAlive_thread();
}

void ReplicationManager::ImAlive_thread()
{
	while(true){
		if(isPrimaryRM){
			RM_group_add_mutex.lock();
				//mount packet with RM's infos
			RM_group_add_mutex.unlock();
			//For all RMs in the group, send group infos, that is:
			//Rm_i [IP:port]

		}
		sleep(2);
	}
}
#endif
