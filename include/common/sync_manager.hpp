#ifndef _DROPFOLDER_SYNC_MANAGER_
#define _DROPFOLDER_SYNC_MANAGER_

#include "watcher.hpp"
#include "connection.hpp"

#include <thread>

class SyncManager {
    public:
        /// Start client syncronization
        SyncManager( 
                const std::string& addr, 
                int port, 
                const std::string& username
        );

        /// Start server syncroniztion
        SyncManager( int port );
        SyncManager( SyncManager&& other ) = default;

        ~SyncManager();

        /// Is synchroinzation alive?
        bool alive() const { return !*_stop; }
        
        /// Stop syncrionization
        void stop_sync();

		///get sync infos
		uint32_t getPeerIP() { return _conn->remoteIP();}
		uint16_t getPeerPort() { return _conn->port();}
	   //TODO: how to get username?? 
	   //      it's discovered after, at sync_thread function, not inside class SyncManager
		std::string getUsername() { return username;}

		void syncThread(void);
    private:
		std::string sync_dir;
		std::string username;
		bool client_mode;	

		void send_file(std::string filename);
		void delete_file( std::string filename);

        SyncManager( const SyncManager& other ) = delete;
        SyncManager& operator=( const SyncManager& other ) = delete;
        std::shared_ptr<std::thread> _thread;
        std::shared_ptr<bool> _stop;
        std::shared_ptr<Connection> _conn;

};

#endif // _DROPFOLDER_SYNC_MANAGER_
