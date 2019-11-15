#ifndef _DROPFOLDER_REPLICATION_SERVER
#define _DROPFOLDER_REPLICATION_SERVER

#include <thread>
#include <mutex>              // std::mutex, std::unique_lock
#include "sync_manager.hpp"
#include <condition_variable> // std::condition_variable, std::cv_status
#include <vector>

class ReplicationServer
{
    enum class State{
        Primary,
        Replication,
        Electing
    };
private:
    State status;

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


//	std::thread _thread_alive;
//	std::mutex RM_group_add_mutex;
//	std::vector<std::unique_ptr<SyncManager>> RM_group;
//	std::condition_variable failure_detection_cv;
//    std::mutex failure_detection_mutex;
//	ReplicationServer();

//	bool gotPrimaryRMMessage;
public:

	ReplicationServer(uint16_t c_port, uint16_t s_port);
	ReplicationServer(uint16_t c_port, uint16_t s_port, const std::string& p_addr, int p_port);

	void acceptClientsThread();
	void acceptServersThread();
	void cleanClientsThread();

	

//	void ImAlive_thread();
//	void failure_detection_thread();
//	void RMAddToGroup_thread();

	const static int FailureDetectionTimeout = 6;
};

#endif //_DROPFOLDER_REPLICATION_SERVER
