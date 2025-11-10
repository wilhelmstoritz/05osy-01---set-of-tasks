//#include "bits/stdc++.h"
#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <string>
#include <vector>
#include <unistd.h>
#include <cstring>

#define N 10            // number of slots in the buffer

// global buffer implemented as circular buffer
std::string g_buffer[N];
int g_in_index = 0;     // index for producer to insert
int g_out_index = 0;    // index for consumer to remove

// POSIX semaphores - exactly 3 as required
sem_t g_mutex_sem;      // controls access to critical region
sem_t g_empty_sem;      // counts empty buffer slots
sem_t g_full_sem;       // counts full buffer slots

// helper function to insert item into buffer
void insert_item(const std::string& t_item) {
    g_buffer[g_in_index] = t_item;
    g_in_index = (g_in_index + 1) % N;
}

// helper function to remove item from buffer
std::string remove_item() {
    std::string l_item = g_buffer[g_out_index];
    g_out_index = (g_out_index + 1) % N;
    return l_item;
}

// producer function - produces one item
void producer(std::string* t_item) {
    sem_wait(&g_empty_sem);     // decrement empty count
    sem_wait(&g_mutex_sem);     // enter critical region
    insert_item(*t_item);       // put new item in buffer
    std::cout << "produced: " << *t_item << std::endl;
    sem_post(&g_mutex_sem);     // leave critical region
    sem_post(&g_full_sem);      // increment count of full slots
}

// consumer function - consumes one item
void consumer(std::string* t_item) {
    sem_wait(&g_full_sem);      // decrement full count
    sem_wait(&g_mutex_sem);     // enter critical region
    *t_item = remove_item();    // take item from buffer
    sem_post(&g_mutex_sem);     // leave critical region
    sem_post(&g_empty_sem);     // increment count of empty slots
    std::cout << "consumed: " << *t_item << std::endl;
}

// producer thread function
void* producer_thread(void* t_arg) {
    (void)t_arg;  // suppress unused parameter warning
    for (int i = 0; i < 20; i++) {
        std::string l_item = "item " + std::to_string(i);
        producer(&l_item);
        usleep(100000);  // sleep 100ms to simulate production time
    }
    return nullptr;
}

// consumer thread function
void* consumer_thread(void* t_arg) {
    (void)t_arg;  // suppress unused parameter warning
    for (int i = 0; i < 20; i++) {
        std::string l_item;
        consumer(&l_item);
        usleep(150000);  // sleep 150ms to simulate consumption time
    }
    return nullptr;
}

int main(void) {
    pthread_t l_prod_thread, l_cons_thread;
    
    // initialize semaphores
    sem_init(&g_mutex_sem, 0, 1);   // mutex starts at 1
    sem_init(&g_empty_sem, 0, N);   // initially all slots are empty
    sem_init(&g_full_sem, 0, 0);    // initially no slots are full

    std::cout << "starting producer-consumer with buffer size: " << N << std::endl;
    
    // create threads
    pthread_create(&l_prod_thread, nullptr, producer_thread, nullptr);
    pthread_create(&l_cons_thread, nullptr, consumer_thread, nullptr);
    
    // wait for threads to finish
    pthread_join(l_prod_thread, nullptr);
    pthread_join(l_cons_thread, nullptr);
    
    // cleanup semaphores
    sem_destroy(&g_mutex_sem);
    sem_destroy(&g_empty_sem);
    sem_destroy(&g_full_sem);
    
    std::cout << "producer-consumer finished" << std::endl;
    
    return EXIT_SUCCESS;
}
