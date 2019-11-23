#ifndef _DROPFOLDER_BARRIER_H_
#define _DROPFOLDER_BARRIER_H_

#include <mutex>
#include <condition_variable>>

class Barrier {

    public:
        Barrier( unsigned int n ) : _n{n}, _counter{n}, _new_n{n} {}
        void increment_n();
        void wait();

    private:
        unsigned int _n;
        unsigned int _counter;
        unsigned int _new_n;
        std::mutex _mutex;
        std::condition_variable _cv;
        std::condition_variable _cv2;

};

#endif // _DROPFOLDER_BARRIER_H_
