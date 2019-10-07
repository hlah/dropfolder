#ifndef _DROPFOLDER_SYNC_MANAGER_
#define _DROPFOLDER_SYNC_MANAGER_

#include "watcher.hpp"
#include "connection.hpp"

#include <thread>

class SyncManager {
    public:
        /// Start client syncrionization
        SyncManager( 
                const std::string& addr, 
                int port, 
                const std::string& username,
                const std::string& sync_dir 
        );

        /// Start server syncroniztion
        SyncManager( int port );
        SyncManager( SyncManager&& other ) = default;

        ~SyncManager();
        
        /// Stop syncrionization
        void stop_sync();
    private:
        SyncManager( const SyncManager& other ) = delete;
        SyncManager& operator=( const SyncManager& other ) = delete;
        std::shared_ptr<std::thread> _thread;
        std::shared_ptr<bool> _stop;
        std::shared_ptr<Connection> _conn;
};

#endif // _DROPFOLDER_SYNC_MANAGER_
