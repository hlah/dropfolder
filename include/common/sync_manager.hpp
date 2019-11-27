#ifndef _DROPFOLDER_SYNC_MANAGER_
#define _DROPFOLDER_SYNC_MANAGER_

#include "watcher.hpp"
#include "connection.hpp"
#include <mutex>
#include <condition_variable>

#include <thread>

class SyncManager {
    public:
		enum class SyncMode: uint8_t {
			Client,
			Server,
			ServerReconstruction,
			Primary,
			PrimaryReconstruction,
			Replicated
		};

        /// Start client syncronization
        SyncManager( 
                const std::string& addr, 
                int port, 
                const std::string& username
        );

        /// Start Replicated syncronization
        SyncManager( 
                const std::string& addr, 
                int port
        );

        /// Start server or primary syncroniztion
		//
		SyncManager(SyncMode mode, std::shared_ptr<Connection> conn);
		SyncManager(SyncMode mode, std::shared_ptr<Connection> conn, std::string user_name);
        SyncManager( int port, SyncMode mode= SyncMode::Server);
        SyncManager( SyncManager&& other ) = default;

        ~SyncManager();

        /// Is synchroinzation alive?
        bool alive() const { return !*_stop; }
        
        /// Stop syncrionization
        void stop_sync(){*_stop= true;};

		///get sync infos
		uint32_t getPeerIP() { return _conn->getPeerIP();}
		uint16_t getPeerPort() { return _conn->getPeerPort();}
		uint16_t getPort() { return _conn->getPort();}


	   //TODO: how to get username?? 
	   //      it's discovered after, at sync_thread function, not inside class SyncManager
		std::string getUsername() { return username;}

		void syncThread(void);
		bool isWatchingDir(){return watchingDir;}

    private:
		std::string sync_dir;
		std::string username;
		bool client_mode;	
		SyncMode operationMode;
        std::mutex username_mutex;
        std::condition_variable username_cv;
		bool watchingDir;

		void send_file(std::string filepath);
		void new_dir(std::string filepath);
		void delete_file( std::string filepath);
		void send_files( const std::string& root_dir, const std::string& files, const std::string name );
		void get_files( const std::string& file );

        SyncManager( const SyncManager& other ) = delete;
        SyncManager& operator=( const SyncManager& other ) = delete;
        std::shared_ptr<std::thread> _thread;
        std::shared_ptr<bool> _stop;
        std::shared_ptr<Connection> _conn;

};

#endif // _DROPFOLDER_SYNC_MANAGER_
