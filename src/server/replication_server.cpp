#include "replication_server.hpp"
#include "sync_manager.hpp"

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


ReplicationServer::ReplicationServer(uint16_t c_port, uint16_t s_port)
{
	status= State::Primary;
	this->client_listen_port= c_port;
	this->server_listen_port= s_port;

    auto accepter_clients = std::thread{accept_clients_thread, this};
    auto accepter_servers = std::thread{accept_servers_thread, this};
    auto cleaner_clients = std::thread{clean_thread, this};

    accepter_clients.join();
    accepter_servers.join();
    cleaner_clients.join();
}


ReplicationServer::ReplicationServer(uint16_t c_port, uint16_t s_port, const std::string& p_addr, int p_port)
{
	status= State::Replication;

	this->client_listen_port= c_port;
	this->server_listen_port= s_port;

    primary_sync = std::unique_ptr<SyncManager>( new SyncManager{p_addr, p_port});

    auto accepter_clients = std::thread{accept_clients_thread, this};
    auto accepter_servers = std::thread{accept_servers_thread, this};
    auto cleaner_clients = std::thread{clean_thread, this};

    accepter_clients.join();
    accepter_servers.join();
    cleaner_clients.join();
}

void ReplicationServer::acceptServersThread() {
    while( true ) {
        auto sync_manager = std::unique_ptr<SyncManager>( new SyncManager{server_listen_port, 
														      SyncManager::SyncMode::Primary});
        server_syncd_mutex.lock();
        server_syncd.push_back( std::move(sync_manager) );
        server_syncd_mutex.unlock();
    }
}

void ReplicationServer::acceptClientsThread() {
    while( true ) {
        auto sync_manager = std::unique_ptr<SyncManager>( new SyncManager{client_listen_port, 
														      SyncManager::SyncMode::Server});
        client_syncd_mutex.lock();
        client_syncd.push_back( std::move(sync_manager) );
        client_syncd_mutex.unlock();
    }
}

void ReplicationServer::cleanClientsThread() {
    while(true) {
        client_syncd_mutex.lock();
        for( int i=0; i<client_syncd.size(); i++ ) {
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


#if 0
void ImAlive_thread_starter(ReplicationServer *rm);

ReplicationServer::ReplicationServer()
{
	_thread_listenToRMs= new std::thread{ RMAddToGroupThread_starter, this};
	_thread_alive= new std::thread{ ImAlive_thread_starter, this};
	_thread_failure_detection= new std::thread{ failure_detection_thread_starter, this};
}

void failure_detection_thread_starter(ReplicatedServer *rm)
{
	rm->failure_detection_thread()
}

void ReplicationServer::failure_detection_thread()
{
	if(!isPrimaryRM){
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
