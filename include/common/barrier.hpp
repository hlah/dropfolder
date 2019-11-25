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

        enum class State {
            NOT_SYNCHING,
            SYNCHING_REPLICAS, 
            SYNCHING_CLIENTS
        };
        State _state = State::NOT_SYNCHING;

        std::string _synching_user;
        unsigned int _synched_replicas_count = 0;
        unsigned int _synched_clients_count = 0;

        std::condition_variable _wait_not_synching_cv;
        std::condition_variable _wait_synching_clients_cv;

        std::string get_user( const std::string& file );

};

#endif // _DROPFOLDER_BARRIER_H_
