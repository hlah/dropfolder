#ifndef _DROPFOLDER_BARRIER_H_
#define _DROPFOLDER_BARRIER_H_

#include <mutex>
#include <condition_variable>>
#include <vector>
#include <map>

class Barrier {

    public:
        void replica_synching( const std::string& file);
        void replica_finished( const std::string& file);
        void client_synching( const std::string& user );
        void client_finished( const std::string& user );

        void add_replica();
        void remove_replica();
        void add_client( const std::string& user );
        void remove_client( const std::string& user );

    private:
        std::mutex _mutex;
        unsigned int _replica_count = 0;
        std::map<std::string, unsigned int> _client_count;
        std::string _synching_user;
        bool _synching = false;

        unsigned int _replicas_synched = 0;
        unsigned int _clients_synched = 0;
        unsigned int _replicas_waiting = 0;

        std::condition_variable _wait_others_users_cv;
        std::condition_variable _wait_for_replicas_cv;
        std::condition_variable _wait_for_clients_cv;
        std::condition_variable _wait_not_synching_cv;
        std::condition_variable _wait_for_replicas_stop_waiting_cv;

        std::string get_user( const std::string& file );

};

#endif // _DROPFOLDER_BARRIER_H_
