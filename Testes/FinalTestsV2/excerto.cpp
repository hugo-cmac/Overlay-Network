#include <iostream>
#include <map>
#include <stack>
#include <string>
#include <iterator>


#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <netdb.h>
#include <unistd.h>


using namespace std;

template <typename T> class PriorityQueue {
    typedef struct List{
        T value;
        int n;
        struct List* next;
    }list;
    
    private:
       list* head = NULL;
    public:
        PriorityQueue(){};
        
        void push(T value, int n){
            list* temp = (list*) malloc(sizeof(list));
            temp->value = value;
            temp->n = n;
            temp->next = NULL;

            if (head == NULL){
                head = temp;
                head->next = NULL;

            }else{
                list* now = head;
                list* prev = NULL;

                while (now != NULL && now->n > n){//percorrer alertas
                    prev = now;
                    now = now->next;
                }
                if (now == NULL) {//final da lista
                    prev->next = temp;
                }else{
                    if(prev != NULL){//meio da lista
                        temp->next = now;
                        prev->next = temp;
                    }else{//inicio da lista
                        temp->next = head;
                        head = temp;
                    }
                }
            }               
        }

        T pop(){
            if (head != NULL){
                list* temp = head;
                head = head->next;
                T t = temp->value;
                free(temp);
                return t;
            }
            return NULL;
        }

        bool empty(){
            if (head == NULL)
                return true;
            return false;
        }

};



int main(int argc, char const *argv[]){

    PriorityQueue<int*> lost;

    int* n1 = (int*) malloc(sizeof(int));
    
    *n1 = 1;
    lost.push(n1,10);

    int* n2 = (int*) malloc(sizeof(int));
    *n2 = 2;
    lost.push(n2,2);

    int* n3 = (int*) malloc(sizeof(int));
    *n3 = 3;
    lost.push(n3,3);

    int* n4 = (int*) malloc(sizeof(int));
    *n4 = 4;
    lost.push(n4,4);

    int* n5 = (int*) malloc(sizeof(int));
    *n5 = 5;
    lost.push(n5,5);

    int* n6 = (int*) malloc(sizeof(int));
    *n6 = 6;
    lost.push(n6,7);

    int* n7 = (int*) malloc(sizeof(int));
    *n7 = 7;
    lost.push(n7,6);
    int* n = NULL;
    while (!lost.empty()){
        n = lost.pop();
        cout << *n << endl;
    }
    
    
    return 0;
}
