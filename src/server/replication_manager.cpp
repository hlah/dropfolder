#include <thread>             // std::thread
#include <chrono>             // std::chrono::seconds
#include <mutex>              // std::mutex, std::unique_lock
#include <condition_variable> // std::condition_variable, std::cv_status

class ReplicationManager
{
	std::thread _thread_alive;
	std::mutex RM_group_add_mutex;
	std::vector<std::unique_ptr<SyncManager>> RM_group;
	std::condition_variable failure_detection_cv;
    std::mutex failure_detection_mutex;

	bool gotPrimaryRMMessage;

	void ImAlive_thread();
	void failure_detection_thread();
	void RMAddToGroup_thread();

	static int FailureDetectionTimeout = 6;
};

void ImAlive_thread_starter(ReplicationManager *rm);


ReplicationManager::ReplicationManager()
{
	_thread_listenToRMs= new std::thread{ RMAddToGroupThread_starter, this};
	_thread_alive= new std::thread{ ImAlive_thread_starter, this};
	_thread_failure_detection= new std::thread{ failure_detection_thread_starter, this};
}

void failure_detection_thread_starter(ReplicatedManager *rm)
{
	rm->failure_detection_thread()
}

void ReplicatedManager::failure_detection_thread()
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

