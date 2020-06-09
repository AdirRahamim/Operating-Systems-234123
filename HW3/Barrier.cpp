#include "Barrier.h"

Barrier::Barrier(unsigned int num_of_threads) : N(num_of_threads), waiting_threads(0){
    sem_init(&barrier, 0, 0);
    sem_init(&barrier2, 0, 1);
    sem_init(&mutex, 0 , 1);
}



Barrier::~Barrier() {
    sem_destroy(&barrier);
    sem_destroy(&mutex);
    sem_destroy(&barrier2);
}

void Barrier::wait() {
    sem_wait(&mutex);
    waiting_threads += 1;
    if(waiting_threads == N){
        sem_wait(&barrier2);
        sem_post(&barrier);
    }
    sem_post(&mutex);

    sem_wait(&barrier);
    sem_post(&barrier);

    sem_wait(&mutex);
    waiting_threads -= 1;
    if(waiting_threads == 0){
        sem_wait(&barrier);
        sem_post(&barrier2);
    }
    sem_post(&mutex);

    sem_wait(&barrier2);
    sem_post(&barrier2);
}
