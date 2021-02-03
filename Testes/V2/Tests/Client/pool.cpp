#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

using namespace std;

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

		void push(int index){
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

		int pop(){
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

		void* pool(){
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

		static void* jumpto(void* Object){
			return ((ThreadPool*)Object)->pool();
		}

	public:
		void init(int nthreads, void* t, void (*func)(int)){
			slave = (pthread_t*) malloc(nthreads*sizeof(pthread_t));
			work = func;
			for (int i = 0; i < nthreads; ++i){
				pthread_create(&slave[i], NULL, jumpto, (void*) t);
				pthread_detach(slave[i]);
			}
		}

		void sendWork(int index){
			pthread_mutex_lock(&mutex);
			push(index);
			pthread_cond_signal(&cond);
			pthread_mutex_unlock(&mutex);
		}
};

void print(int i){
	puts("Here\n");
}

int main(int argc, char const *argv[]){
	ThreadPool t;
	t.init(10, (void*) &t, print);

	while(1){
		getchar();
		t.sendWork(0);
	}

	return 0;
}