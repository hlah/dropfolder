#ifndef _DROPFOLDER_REPLICATION_SERVER
#define _DROPFOLDER_REPLICATION_SERVER

#include "sync_manager.hpp"
#include "barrier.hpp"

#include <thread>
#include <mutex>              // std::mutex, std::unique_lock
#include <condition_variable> // std::condition_variable, std::cv_status
#include <vector>


enum class ServerState: uint8_t{
    Primary=0,
    Replication,
    Electing
};


class ReplicationServer
{
private:
    ServerState state;

	//manage clients
	uint16_t client_listen_port;
	std::mutex client_syncd_mutex;
	std::vector<std::unique_ptr<SyncManager>> client_syncd;

	//manager other server connections
	uint16_t server_listen_port;
	std::mutex server_syncd_mutex;
	std::vector<std::unique_ptr<SyncManager>> server_syncd;

	//replicatedServer connection with primary
	std::unique_ptr<SyncManager> primary_sync;

    //primary ctrl connections with replicated servers
    std::mutex cntrl_conns_mutex;
    std::vector<std::shared_ptr<Connection>> cntrl_conns;

    std::mutex peers_info_mutex;

    uint16_t ctrl_port;
    std::string p_addr;
    uint16_t p_ctrl_port;

    // barrier to make server wait replication synchronization before sync clients
    std::shared_ptr<Barrier> _sync_barrier;

//	std::thread _thread_alive;
//	std::mutex RM_group_add_mutex;
//	std::vector<std::unique_ptr<SyncManager>> RM_group;
//	std::condition_variable failure_detection_cv;
    std::mutex failure_detection_mutex;

//	bool gotPrimaryRMMessage;
public:

	ReplicationServer(uint16_t c_port, uint16_t s_port, uint16_t ctrl_port);

    ReplicationServer(uint16_t c_port, uint16_t s_port, uint16_t ctrl_port, const std::string& p_addr, int p_sync_port, uint16_t p_ctrl_port);

	void acceptClientsThread();
	void acceptServersThread();
	void acceptCtrlThread();
	void cleanClientsThread();
    void failureDetectionThread();
	

//	void ImAlive_thread();
//	void RMAddToGroup_thread();

	const static int FailureDetectionTimeout = 6;
};

#endif //_DROPFOLDER_REPLICATION_SERVER
