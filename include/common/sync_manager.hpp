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
        std::thread _thread;
        bool _stop;
        Connection _conn;
};

#endif _DROPFOLDER_SYNC_MANAGER_
