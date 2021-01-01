#include "threadpool.h"

namespace std{

    void ThreadPool::push(int index){
        node* n = (node*)malloc(sizeof(node));
        n->index = index;
        n->next = NULL;
        if (head == NULL){
            head = n;
            return;
        }else{
            node* x = head;
            while(x->next != NULL){
                x = x->next;
            }
            x->next = n;
        }
    }

    int ThreadPool::pop(){
        if (head == NULL){
            return -1;
        } else {
            node* n = head;
            head = head->next;
            int s = n->index;
            free(n);
            return s;
        }
    }

    void* ThreadPool::pool(){
        int index;
        while (1){
            pthread_mutex_lock(&mutex);
            if ((index = pop()) == -1){
                pthread_cond_wait(&cond, &mutex);
                index = pop();
            }
            pthread_mutex_unlock(&mutex);
            if (index != -1){
                work(index);
            }
        }
    }

    void* ThreadPool::jumpto(void* Object){
        return ((ThreadPool*)Object)->pool();
    }

    void ThreadPool::init(int nthreads, void* t, void (*func)(int)){
        slave = (pthread_t*) malloc(nthreads*sizeof(pthread_t));
        work = func;
        for (int i = 0; i < nthreads; ++i){
            pthread_create(&slave[i], NULL, jumpto, (void*) t);
            pthread_detach(slave[i]);
        }
    }

    void ThreadPool::sendWork(int index){
        pthread_mutex_lock(&mutex);
        push(index);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }
    
}

