#include "replication_server.hpp"
#include "sync_manager.hpp"
#include "message.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>             // std::chrono::seconds
#include <algorithm>
#include "peers_register.hpp"

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

void failure_detection_thread(ReplicationServer *rs)
{
	rs->failureDetectionThread();
}

void electing_thread(ReplicationServer *rs, uint32_t candidateIP, uint16_t candidatePort)
{
	rs->electingThread(candidateIP, candidatePort);
}

ReplicationServer::ReplicationServer(uint16_t c_port, uint16_t s_port, uint16_t ctrl_port)
{
	state= ServerState::Primary;
	this->client_listen_port= c_port;
	this->server_listen_port= s_port;
    this->ctrl_port= ctrl_port;
	this->ctrl_port_accept= ctrl_port;

    std::remove("users/peers.info");

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

	this->ctrl_port_accept= ctrl_port;
    this->ctrl_port= ctrl_port;
    this->p_ctrl_port= p_ctrl_port;
    this->p_addr= p_addr;

    primary_sync = std::unique_ptr<SyncManager>( new SyncManager{p_addr, p_sync_port});

    primary_sync_port= primary_sync->getPort();

    auto conn = std::shared_ptr<Connection> (Connection::connect( ConnMode::Promiscuous, (const std::string&) p_addr, (int)p_ctrl_port) );
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

		//TODO: use condition variable, busy waiting sucks
		while(sync_manager->isWatchingDir()==false){};

        peers_info_mutex.lock();
        {
            std::ofstream ofs{"users/peers.info", std::ios::binary | std::ios::app };
            //S - ip port
            ofs << "ServerSync _ " << sync_manager->getPeerIP() << " " << sync_manager->getPeerPort() << std::endl;
			
			std::cout << "ServerSync _ " << std::hex << sync_manager->getPeerIP() << " " 
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

		//TODO: use condition variable, busy waiting sucks
		while(sync_manager->isWatchingDir()==false){};

        peers_info_mutex.lock();
        {
            std::ofstream ofs{"users/peers.info", std::ios::binary | std::ios::app };
            //C username ip port
            ofs << "Client " << sync_manager->getUsername() << " "
                << sync_manager->getPeerIP() << " " 
                << sync_manager->getPeerPort() << std::endl; 

			std::cout << "Client " << sync_manager->getUsername() << " "
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
			
			

            auto conn = std::shared_ptr<Connection> (Connection::listen (ctrl_port_accept));

            peers_info_mutex.lock();
            {
                std::ofstream ofs{"users/peers.info", std::ios::binary | std::ios::app };
                ofs << "ServerConn _ " << conn->getPeerIP() << " " << conn->getPeerPort() << std::endl; 

				std::cout << "ServerConn _ " << conn->getPeerIP() << " " << conn->getPeerPort() << std::endl; 
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

            std::this_thread::sleep_for( std::chrono::seconds( 2 ) );

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
					auto electing = std::thread{electing_thread, this, 0, 0};
					electing.join();
                } else {
                    Message* msg = (Message*)response.data.get();
                    if( msg->type == MessageType::PING_RESPONSE ) {
                        std::cout << "Got PING_RESPONSE(type: " << (int)msg->type << ")." << std::endl;

                    }else if(msg->type == MessageType::ELECTION){
                        //TODO: initialize election as knowing nothing
                        state= ServerState::Electing;
                        ElectionMessage * electionMsg= (ElectionMessage*)msg;

                        auto electing = std::thread{electing_thread, this, electionMsg->candidateIP, electionMsg->candidatePort };
                        electing.join();

                    }else {
                        std::cout << "Got unexpected Response(type: " << (int)msg->type << ")." << std::endl;
                    }
                }
            }catch(std::exception& e){
                std::cout << "No response: timed out on send. Begin Election\n";
                //TODO: state= ServerState::Electing;
				auto electing = std::thread{electing_thread, this, 0, 0};
				electing.join();
            }


		}else if(state == ServerState::Primary){
            cntrl_conns_mutex.lock();
            for(auto conn : cntrl_conns){
                if(conn->hasNewMessage()){
                    ReceivedData recvd= conn->receive();
                    Message* msg = (Message*)recvd.data.get();
                    if( msg->type == MessageType::PING_REQUEST ) {
                        std::cout << "Got PING_REQUEST(type: " << (int)msg->type << ")." << std::endl;
                        Message ping_msg{MessageType::PING_RESPONSE, "", 0};
                        conn->send((uint8_t*)&ping_msg, sizeof(Message));

                    } else {
                        std::cout << "Got unexpected Message(type: " << (int)msg->type << ")." << std::endl;
                    }
                }
            }
            cntrl_conns_mutex.unlock();
        }
	}
}



uint32_t getMyIP(Connection *conn)
{

	std::ifstream ifs{"users/peers.info"};
	std::vector<PeersReg> regs;

	if(ifs){
		std::string line;   

		while (std::getline(ifs, line)) {
			std::stringstream ss(line);
			PeersReg reg;
			ss >> reg;
			regs.push_back(reg);
			//std::cout << reg << std::endl;
		}
	}

	size_t i;
	for(i=0; i<regs.size();i++){
		std::vector<uint32_t> myIPs=  conn->getIP();
		for(auto ip : myIPs){
			if(regs[i]==PeersReg{"","", ip, conn->getPort()}){
				return ip;
			}
		}
	}
	return 0;
}

void getNeighbourAddress(Connection *conn, uint32_t &peerIp, uint16_t &peerPort)
{

	std::ifstream ifs{"users/peers.info"};
	std::vector<PeersReg> regs;

	if(ifs){
		std::string line;   

		while (std::getline(ifs, line)) {
			std::stringstream ss(line);
			PeersReg reg;
			ss >> reg;
            if(reg.type == "ServerConn")
                regs.push_back(reg);
			//std::cout << reg << std::endl;
		}
	}
//	for(auto reg : regs){
//		std::cout << reg << std::endl;
//	}

	std::sort(regs.begin(), regs.end());
	size_t i;

	for(i=0; i<regs.size();i++){
		if(regs[i]==PeersReg{"","", getMyIP(conn), conn->getPort()}){
			break;
		}
	}

	if(i == regs.size()){
		std::cout << "Error, my addr is not in the file!" << std::endl;
	}
	if(i == regs.size()-1){
		peerIp= regs[0].ip_addr;
		peerPort= regs[0].ip_port;
	}else{
		peerIp= regs[i+1].ip_addr;
		peerPort= regs[i+1].ip_port;
	}
}

void ReplicationServer::newPrimaryAddConns()
{
	std::cout << "newPrimaryAddConns" << std::endl;
	std::ifstream ifs{"users/peers.info"};
	std::vector<PeersReg> regs;

	if(ifs){
		std::string line;   

		while (std::getline(ifs, line)) {
			std::stringstream ss(line);
			PeersReg reg;
			ss >> reg;
			regs.push_back(reg);
//			std::cout << "added to regs" << std::endl;
			//std::cout << reg << std::endl;
		}
	}


	for(auto reg : regs){
//		std::cout << reg <<std::endl;
        if(reg.ip_addr == ctrl_IP  && reg.ip_port == ctrl_port){
            continue;
        }

        if(reg.ip_addr == ctrl_IP  && reg.ip_port == primary_sync_port){
            continue;
        }

        if(reg.type == "ServerConn"){
            std::cout << "AddConns: Treating reg:" << reg << std::endl;
            cntrl_conns_mutex.lock();
				std::shared_ptr<Connection> conn{Connection::reconnect(ConnMode::Promiscuous, reg.ip_addr, reg.ip_port)};
                cntrl_conns.push_back(std::move(conn));
            cntrl_conns_mutex.unlock();
        }

        if(reg.type == "ServerSync"){
            std::cout << "AddConns: Treating reg:" << reg << std::endl;
            std::shared_ptr<Connection> conn{Connection::reconnect(ConnMode::Normal, reg.ip_addr, reg.ip_port)};

            auto sync_manager = std::unique_ptr<SyncManager>( new SyncManager{SyncManager::SyncMode::PrimaryReconstruction, conn});

            //TODO: use condition variable, busy waiting sucks
            while(sync_manager->isWatchingDir()==false){};

            server_syncd_mutex.lock();
                server_syncd.push_back( std::move(sync_manager) );
            server_syncd_mutex.unlock();

            //std::cout << reg << std::endl;
        }

        if(reg.type == "Client"){
            std::cout << "AddConns: Treating reg:" << reg << std::endl;
            std::shared_ptr<Connection> conn{Connection::reconnect(ConnMode::Normal, reg.ip_addr, reg.ip_port)};

            auto sync_manager = std::unique_ptr<SyncManager>( new SyncManager{SyncManager::SyncMode::ServerReconstruction, 
                                                                               conn, reg.username});

            //TODO: use condition variable, busy waiting sucks
            while(sync_manager->isWatchingDir()==false){};

            client_syncd_mutex.lock();
                client_syncd.push_back( std::move(sync_manager) );
            client_syncd_mutex.unlock();
            //std::cout << reg << std::endl;
        }
	
    }

    std::ofstream ofs{"users/peers.info", std::ios::binary | std::ios::trunc };
	for(auto reg : regs){
        if(reg.ip_addr == ctrl_IP  && reg.ip_port == ctrl_port){
            continue;
        }

        if(reg.ip_addr == ctrl_IP  && reg.ip_port == primary_sync_port){
            continue;
        }
        ofs << reg << std::endl;
    }
    ofs.close();

}


void ReplicationServer::electingThread(uint32_t candidateIP, uint16_t candidatePort)
{
	uint16_t peerPort;
	uint32_t peerIp;

	std::shared_ptr<Connection> conn;

    std::cout << "(1)" << std::endl;
	cntrl_conns_mutex.lock();
		conn= cntrl_conns[0];
		cntrl_conns.pop_back();
	//	cntrl_conns.erase(cntrl_conns.begin());
    cntrl_conns_mutex.unlock();


	//ring formation
    getNeighbourAddress(conn.get(), peerIp, peerPort);
    std::cout << "neighbour Addr: " << peerIp << ":" << peerPort << std::endl;
    conn->setPeerIP(peerIp);
    conn->setPeerPort(peerPort);
   
    std::cout << "(2)" << std::endl;
    uint32_t myIP= getMyIP(conn.get());	
    ctrl_IP = myIP;
        
    std::cout << "myAddr: " << myIP << ":" << conn->getPort() << std::endl;

    if( PeersReg{"", "", candidateIP, candidatePort} < 
        PeersReg{"", "", myIP, conn->getPort()}){
        candidateIP= myIP;
        candidatePort= conn->getPort();
    }


    std::cout << "(3)" << std::endl;
    ElectionMessage newMessage{MessageType::ELECTION, candidateIP, candidatePort};
    conn->send((uint8_t*)&newMessage, sizeof(ElectionMessage));

    state= ServerState::Electing;

    std::cout << "(4)" << std::endl;
	while(state == ServerState::Electing){

		if(conn->hasNewMessage()){
			ReceivedData recvd= conn->receive();
			Message* msg = (Message*)recvd.data.get();

			switch(msg->type){
                case MessageType::ELECTION:
                    {

                        ElectionMessage *electionMsg= (ElectionMessage*)msg;

                        std::cout << "Got ElectionMsg{ "
                                  << electionMsg->candidateIP
                                  << " : "
                                  << electionMsg->candidatePort
                                  <<" }"<< std::endl;

                        if( PeersReg{"", "", myIP, conn->getPort()} < 
                            PeersReg{"", "", electionMsg->candidateIP, electionMsg->candidatePort} ){

                            std::cout << "will forward Message." << std::endl;

                           // ElectionMessage newMessage{MessageType::ELECTION, electionMsg->candidateIP, electionMsg->candidatePort};
                            conn->send((uint8_t*)electionMsg, sizeof(ElectionMessage));
                            std::cout << "Message forwarded." << std::endl;

                        }else if( PeersReg{"", "", myIP, conn->getPort()} == 
                                  PeersReg{"", "", electionMsg->candidateIP, electionMsg->candidatePort} ){
                            //I'm the new primary
                            std::cout << "I'm the new Primary!" << std::endl;

                            state= ServerState::Primary;

                            electionMsg->type= MessageType::ELECTED;
                            conn->send((uint8_t*)electionMsg, sizeof(ElectedMessage));

                            //delete primary_sync
							std::cerr << "before reset" << std::endl;
							primary_sync->stop_sync();
							std::cerr << "after reset" << std::endl;

							std::this_thread::sleep_for(std::chrono::seconds(1));
                            newPrimaryAddConns();
                        }
                    }
                    break;

                case MessageType::ELECTED:

                    state= ServerState::Replication;


                    {
                        //The new primary arose
                        //pass the message and set conn to primary
                        ElectedMessage *electedMsg= (ElectedMessage *)msg;


                        std::cout << "Got ELECTED Msg{ "
                                  << electedMsg->newPrimaryIP
                                  << " : "
                                  << electedMsg->newPrimaryPort
                                  <<" }"<< std::endl;


						//if neighbour is the elected, dont pass.
						
						if(!( PeersReg{"","", electedMsg->newPrimaryIP, electedMsg->newPrimaryPort} ==
                            PeersReg{"","", conn->getPeerIP(), conn->getPeerPort()})){

                            conn->send((uint8_t*)electedMsg, sizeof(ElectedMessage));
                        }
                        //conn->setPeerIP(electedMsg->newPrimaryIP);
                        //conn->setPeerPort(electedMsg->newPrimaryPort);

                        cntrl_conns_mutex.lock();
                            cntrl_conns.push_back(conn);
                        cntrl_conns_mutex.unlock();
						std::this_thread::sleep_for(std::chrono::seconds(2));
                    }
                    break;

                default:
                    std::cout << "Got unexpected Message(type: " << (int)msg->type << ")." << std::endl;
                    break;
			}
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
