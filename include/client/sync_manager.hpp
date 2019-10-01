#ifndef _DROPFOLDER_SYNC_MANAGER_
#define _DROPFOLDER_SYNC_MANAGER_

#include "watcher.hpp"

#include <thread>

class SyncManager {
    public:
        /// Start folder syncrionization
        SyncManager( 
                const std::string& addr, 
                int port, 
                const std::string& username,
                const std::string& sync_dir 
        );

        ~SyncManager();
        
        /// Stop syncrionization
        void stop_sync();
    private:
        std::thread _thread;
        bool _stop;
};

#endif _DROPFOLDER_SYNC_MANAGER_
