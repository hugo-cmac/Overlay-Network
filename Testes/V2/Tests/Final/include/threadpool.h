
#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <cstdlib>
#include <pthread.h>

namespace std{

    class ThreadPool{

        typedef struct Node{
            int index;
            struct Node* next;
        }node;
        
        typedef void (*funcPtr) (int);

        private:
            node* head = NULL;
            funcPtr work = NULL;

            pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;								//
            pthread_cond_t cond = PTHREAD_COND_INITIALIZER;	
            pthread_t  *slave;	

            void push(int index);

            int pop();

            void* pool();

            static void* jumpto(void* Object);
            
        public:
            void init(int nthreads, void* t, void (*func)(int));

            void sendWork(int index);
    };
}

#endif