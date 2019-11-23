#include "barrier.hpp"

#include <mutex>

void Barrier::wait() {
    // wait for all threads to exit barrier
    std::unique_lock<std::mutex> lock{_mutex};
    if( _counter == 0 ) {
        _cv2.wait(lock, [this]{ return _counter != 0; });
    }

    // wait for other threads
    if( --_counter == 0 ) {
        _cv.notify_all();
    } else {
        _cv.wait(lock, [this] { return _counter == 0; });
    }

    // reset when all threads exited
    _n--;
    if( _n == 0 ) {
        _counter = _new_n;
        _n = _new_n;
        _cv2.notify_all();
    }
}

void Barrier::increment_n() {
    // wait for all threads to exit barrier
    std::unique_lock<std::mutex> lock{_mutex};
    if( _n == _new_n && _counter == _new_n ) {
        _counter++;
        _n++;
    }
    _new_n++;
}
