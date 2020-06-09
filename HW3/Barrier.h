#ifndef BARRIER_H_
#define BARRIER_H_

#include <semaphore.h>

class Barrier {
private:
    sem_t barrier;
    sem_t barrier2;
    sem_t mutex;
    unsigned int N;
    unsigned int waiting_threads;
public:
    Barrier(unsigned int num_of_threads);
    void wait();
    ~Barrier();

	// TODO: define the member variables
	// Remember: you can only use semaphores!
};

#endif // BARRIER_H_

