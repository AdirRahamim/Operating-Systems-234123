#ifndef BARRIER_H_
#define BARRIER_H_

#include <semaphore.h>

class Barrier {
private:
    sem_t sem;
    sem_t mutex;
    sem_t sem2;
    unsigned int N;
    int counter;
public:
    Barrier(unsigned int num_of_threads);
    void wait();
    ~Barrier();

	// TODO: define the member variables
	// Remember: you can only use semaphores!
};

#endif // BARRIER_H_

