#include "barrier.hpp"

#include <iostream>

void Barrier::replica_synching( const std::string& file ) {
    std::unique_lock<std::mutex> lock{_mutex};
    auto user = get_user( file );
    if( user.size() > 0 ) {
        if( !_synching ) {
            _synching = true;
            _synching_user = user;
        }
        if( user != _synching_user ) {
            std::cerr << ">>>>>> Waiting for " << _synching_user << "\n";
            _wait_others_users_cv.wait( lock, [this,user] { return !_synching || user == _synching_user; } );
            _synching = true;
            _synching_user = user;
        } 
        std::cerr << ">>>>>> Replica synching " <<_synching_user << " file " << file << "\n";
    }
}

void Barrier::replica_finished( const std::string& file) {
    std::unique_lock<std::mutex> lock{_mutex};
    auto user = get_user( file );
    if( user.size() > 0 ) {
        std::cerr << ">>>>>> Replica synched " <<_synching_user << " file " << file << "\n";
        _replicas_synched++;
        if( _replicas_synched == _replica_count ) {
            _wait_for_replicas_cv.notify_all();
        }
        std::cerr << ">>>>>> " << _clients_synched << " < " << _client_count[_synching_user] << "?\n";
        if( _clients_synched < _client_count[_synching_user] ) {
            _replicas_waiting++;
            std::cerr << ">>>>>> Waithing for client synching " <<_synching_user << " file " << file << "\n";
            _wait_for_clients_cv.wait( lock, [this]{ return _clients_synched == _client_count[_synching_user]; } );
            _replicas_waiting--;
            _wait_for_replicas_stop_waiting_cv.notify_all();
        }
        std::cerr << ">>>>>> Finished client synching " <<_synching_user << " file " << file << "\n";
    }
}

void Barrier::client_synching( const std::string& user ) { 
    std::unique_lock<std::mutex> lock{_mutex};
    if( !_synching ) {
        _synching = true;
        _synching_user = user;
    }
    if( user != _synching_user ) {
        std::cerr << ">>>>>> Waiting for " << _synching_user << "\n";
        _wait_others_users_cv.wait( lock, [this,user] { return !_synching || user == _synching_user; } );
        _synching = true;
        _synching_user = user;
    } 
    if( _replicas_synched < _replica_count ) {
        std::cerr << ">>>>>> Waithing for replica synching " <<_synching_user << " file \n";
        _wait_for_replicas_cv.wait( lock, [this]{ return _replicas_synched == _replica_count; } );
    }
    std::cerr << ">>>>>> Client synching " <<_synching_user << " file \n";
}

void Barrier::client_finished( const std::string& user ) {
    std::cerr << ">>>>>> Client synched " <<_synching_user << " file \n";
    std::unique_lock<std::mutex> lock{_mutex};
    _clients_synched++;
    if( _clients_synched == _client_count[_synching_user] ) {
        std::cerr << " ===================\n";
        _wait_for_clients_cv.notify_all();
        _wait_for_replicas_stop_waiting_cv.wait( lock, [this]{ return _replicas_waiting == 0; } );
        _synching = false;
        _replicas_synched = 0;
        _clients_synched = 0;
        _wait_others_users_cv.notify_all();
        _wait_not_synching_cv.notify_all();
    }
}

void Barrier::add_replica() {
    std::unique_lock<std::mutex> lock{_mutex};
    if( _synching ) {
        std::cerr << ">>>>>> Waithing to add replica\n";
        _wait_not_synching_cv.wait( lock, [this]{ return !_synching; } );
    }
    std::cerr << ">>>>>> Added replica\n";
    _replica_count++;
}

void Barrier::remove_replica() {
    std::unique_lock<std::mutex> lock{_mutex};
    if( _synching ) {
        std::cerr << ">>>>>> Waithing to remove replica\n";
        _wait_not_synching_cv.wait( lock, [this]{ return !_synching; } );
    }
    std::cerr << ">>>>>> Removed replica\n";
    _replica_count--;
}

void Barrier::add_client( const std::string& user ) {
    std::unique_lock<std::mutex> lock{_mutex};
    if( _synching ) {
        std::cerr << ">>>>>> Waithing to add client '" << user << "'\n";
        _wait_not_synching_cv.wait( lock, [this]{ return !_synching; } );
    }
    std::cerr << ">>>>>> Added client '" << user << "'\n";
    _client_count[user]++;
}

void Barrier::remove_client( const std::string& user ) {
    std::unique_lock<std::mutex> lock{_mutex};
    if( _synching ) {
        std::cerr << ">>>>>> Waithing to remove client '" << user << "'\n";
        _wait_not_synching_cv.wait( lock, [this]{ return !_synching; } );
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
