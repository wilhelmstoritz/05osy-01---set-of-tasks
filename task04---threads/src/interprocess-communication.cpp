#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <string>
#include <vector>
#include <unistd.h>
#include <cstring>

#define N 10            /* number of slots in the buffer */

/* Global buffer implemented as circular buffer */
std::string buffer[N];
int in_index = 0;       /* index for producer to insert */
int out_index = 0;      /* index for consumer to remove */

/* POSIX semaphores - exactly 3 as required */
sem_t mutex_sem;        /* controls access to critical region */
sem_t empty_sem;        /* counts empty buffer slots */
sem_t full_sem;         /* counts full buffer slots */

/* Helper function to insert item into buffer */
void insert_item(const std::string& item) {
    buffer[in_index] = item;
    in_index = (in_index + 1) % N;
}

/* Helper function to remove item from buffer */
std::string remove_item() {
    std::string item = buffer[out_index];
    out_index = (out_index + 1) % N;
    return item;
}

/* Producer function - produces one item */
void producer(std::string* item) {
    sem_wait(&empty_sem);           /* decrement empty count */
    sem_wait(&mutex_sem);           /* enter critical region */
    insert_item(*item);             /* put new item in buffer */
    std::cout << "Produced: " << *item << std::endl;
    sem_post(&mutex_sem);           /* leave critical region */
    sem_post(&full_sem);            /* increment count of full slots */
}

/* Consumer function - consumes one item */
void consumer(std::string* item) {
    sem_wait(&full_sem);            /* decrement full count */
    sem_wait(&mutex_sem);           /* enter critical region */
    *item = remove_item();          /* take item from buffer */
    sem_post(&mutex_sem);           /* leave critical region */
    sem_post(&empty_sem);           /* increment count of empty slots */
    std::cout << "Consumed: " << *item << std::endl;
}

/* Producer thread function */
void* producer_thread(void* arg) {
    (void)arg;  /* suppress unused parameter warning */
    for (int i = 0; i < 20; i++) {
        std::string item = "Item_" + std::to_string(i);
        producer(&item);
        usleep(100000);  /* sleep 100ms to simulate production time */
    }
    return nullptr;
}

/* Consumer thread function */
void* consumer_thread(void* arg) {
    (void)arg;  /* suppress unused parameter warning */
    for (int i = 0; i < 20; i++) {
        std::string item;
        consumer(&item);
        usleep(150000);  /* sleep 150ms to simulate consumption time */
    }
    return nullptr;
}

int main(void) {
    pthread_t prod_thread, cons_thread;
    
    /* Initialize semaphores */
    sem_init(&mutex_sem, 0, 1);     /* mutex starts at 1 */
    sem_init(&empty_sem, 0, N);     /* initially all slots are empty */
    sem_init(&full_sem, 0, 0);      /* initially no slots are full */
    
    std::cout << "Starting producer-consumer with buffer size: " << N << std::endl;
    
    /* Create threads */
    pthread_create(&prod_thread, nullptr, producer_thread, nullptr);
    pthread_create(&cons_thread, nullptr, consumer_thread, nullptr);
    
    /* Wait for threads to finish */
    pthread_join(prod_thread, nullptr);
    pthread_join(cons_thread, nullptr);
    
    /* Cleanup semaphores */
    sem_destroy(&mutex_sem);
    sem_destroy(&empty_sem);
    sem_destroy(&full_sem);
    
    std::cout << "Producer-consumer finished." << std::endl;
    
    return 0;
}
