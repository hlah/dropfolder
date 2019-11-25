#include "barrier.hpp"

#include <iostream>
#include <thread>

void Barrier::replica_synching( const std::string& file ) {
    std::unique_lock<std::mutex> lock{_mutex};
    auto user = get_user( file );
    if( user.size() > 0 ) {
        while( true ) {
            if( _state == State::NOT_SYNCHING ) {
                _state = State::SYNCHING_REPLICAS;
                std::cerr << ">>>>>>> Barrier state: SYNCHING_REPLICAS\n";
                _synching_user = user;
                break;
            } else if( _state == State::SYNCHING_REPLICAS && _synching_user == user ) {
                break;
            } else {
                std::cerr << ">>>>>>> Replica Waiting (" << std::this_thread::get_id() << ")\n";
                _wait_not_synching_cv.wait( lock, [this]{ return _state != State::SYNCHING_CLIENTS; } );
            }
        }
        std::cerr << ">>>>>>> Replica Synching (" << file << ") (" << std::this_thread::get_id() << ")\n";
    }
}

void Barrier::replica_finished( const std::string& file) {
    std::unique_lock<std::mutex> lock{_mutex};
    auto user = get_user( file );
    if( user.size() > 0 ) {
        _synched_replicas_count++;
        std::cerr << ">>>>>>> Replica Finished (" << _synched_replicas_count << ") (" << std::this_thread::get_id() << ")\n";
        if( _synched_replicas_count == _replica_count ) {
            _state = State::SYNCHING_CLIENTS;
            std::cerr << ">>>>>>> Barrier state: SYNCHING_CLIENTS\n";
            _wait_synching_clients_cv.notify_all();
        } else {
            _wait_synching_clients_cv.wait( lock );
        }
    }
}

void Barrier::client_synching( const std::string& user ) { 
    std::unique_lock<std::mutex> lock{_mutex};
    while( true ) {
        if( _state == State::SYNCHING_CLIENTS && user == _synching_user ) {
            break;
        } else {
            std::cerr << ">>>>>>> Client Waiting (" << std::this_thread::get_id() << ")\n";
            _wait_synching_clients_cv.wait( lock, [this,user]{ return _state == State::SYNCHING_CLIENTS && _synching_user == user; } );
        }
    }
    std::cerr << ">>>>>>> Client Synching (" << std::this_thread::get_id() << ")\n";
}

void Barrier::client_finished( const std::string& user ) {
    std::unique_lock<std::mutex> lock{_mutex};
    _synched_clients_count++;
    std::cerr << ">>>>>>> Client Finished  (" << _synched_clients_count << ")(" << std::this_thread::get_id() << ")\n";
    if( _synched_clients_count == _client_count[_synching_user] ) {
        _state = State::NOT_SYNCHING;
        std::cerr << ">>>>>>> Barrier state: NOT_SYNCHING\n";
        _synched_clients_count = 0;
        _synched_replicas_count = 0;
        _wait_not_synching_cv.notify_all();
    }
}

void Barrier::add_replica() {
    std::unique_lock<std::mutex> lock{_mutex};
    if( _state != State::NOT_SYNCHING ) {
        std::cerr << ">>>>>> Waiting to add replica\n";
        _wait_not_synching_cv.wait( lock, [this]{ return _state == State::NOT_SYNCHING; } );
    }
    std::cerr << ">>>>>> Added replica\n";
    _replica_count++;
}

void Barrier::remove_replica() {
    std::unique_lock<std::mutex> lock{_mutex};
    if( _state != State::NOT_SYNCHING ) {
        std::cerr << ">>>>>> Waiting to remove replica\n";
        _wait_not_synching_cv.wait( lock, [this]{ return _state == State::NOT_SYNCHING; } );
    }
    std::cerr << ">>>>>> Removed replica\n";
    _replica_count--;
}

void Barrier::add_client( const std::string& user ) {
    std::unique_lock<std::mutex> lock{_mutex};
    if( _state != State::NOT_SYNCHING ) {
        std::cerr << ">>>>>> Waiting to add client '" << user << "'\n";
        _wait_not_synching_cv.wait( lock, [this]{ return _state == State::NOT_SYNCHING; } );
    }
    std::cerr << ">>>>>> Added client '" << user << "'\n";
    _client_count[user]++;
}

void Barrier::remove_client( const std::string& user ) {
    std::unique_lock<std::mutex> lock{_mutex};
    if( _state != State::NOT_SYNCHING ) {
        std::cerr << ">>>>>> Waiting to remove client '" << user << "'\n";
        _wait_not_synching_cv.wait( lock, [this]{ return _state == State::NOT_SYNCHING; } );
    }
    std::cerr << ">>>>>> Removed client '" << user << "'\n";
    _client_count[user]--;
}


std::string Barrier::get_user( const std::string& file ) {
    std::string username;
    auto username_init = file.find_first_of('/');
    auto username_end = file.find_first_of('/', username_init+1);
    if( username_end != std::string::npos ) {
        auto user_username = std::string{ file, username_init+1, username_end-username_init-1 };
        if( file.size() > user_username.size() + 15 ) {
            username = user_username;
        }
    }
    return username;
}
