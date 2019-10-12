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

        bool alive() const { return !*_stop; }
        const std::string& status() const { return *_status; }

        SyncManager( const SyncManager& other ) = delete;
        SyncManager( SyncManager&& other ) = default;
        SyncManager& operator=( const SyncManager& other ) = delete;
        SyncManager& operator=( SyncManager&& other ) = default;

        ~SyncManager();
        
        /// Stop syncrionization
        void stop_sync();
    private:
        std::shared_ptr<std::thread> _thread;
        std::shared_ptr<bool> _stop{new bool{ false }};
        std::shared_ptr<Connection> _conn;
        std::shared_ptr<std::string> _status;
};

#endif // _DROPFOLDER_SYNC_MANAGER_
