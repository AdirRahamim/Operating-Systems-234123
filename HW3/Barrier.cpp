#include "Barrier.h"

Barrier::Barrier(unsigned int num_of_threads) : N(num_of_threads), counter(0){
    sem_init(&sem, 0, 0);
    sem_init(&mutex, 0 , 1);
    sem_init(&sem2, 0, 0);
}



Barrier::~Barrier() {
    sem_destroy(&sem);
    sem_destroy(&mutex);
    sem_destroy(&sem2);
}

void Barrier::wait() {
    //Lock critical part
    sem_wait(&mutex);
    counter++;

    //If all threads arrived barrier - unlock them
    if(counter == N){
        for(unsigned int i=0; i<N; i++){
            sem_post(&sem);
        }
    }

    sem_post(&mutex);

    //Make all threads wait
    sem_wait(&sem);
    sem_wait(&mutex);
    counter--;

    if(counter == 0){
        for(unsigned int i=0; i<N; i++){
            sem_post(&sem2);
        }
    }

    sem_post(&mutex);
    sem_wait(&sem2);
}
