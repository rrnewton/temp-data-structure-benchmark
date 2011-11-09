#include <iostream>

#include "tbb/concurrent_queue.h"
#include "tbb/tick_count.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/task_group.h"

using namespace tbb;
using namespace std;

#define QUEUE_T concurrent_queue<int>
#define ELEMENTS_COUNT  1000000

task_group *t_group;


void producer(QUEUE_T *q) ;
void consumer(QUEUE_T *q);


class producer_helper_functor {
    QUEUE_T *queue;
public:
    producer_helper_functor(QUEUE_T *q) : queue(q) {};
    void operator() () const { producer(queue); }
};


class consumer_helper_functor {
    QUEUE_T *queue;
public:
    consumer_helper_functor(QUEUE_T *q) : queue(q) {};
    void operator() () const { consumer(queue); }
};


void producer(QUEUE_T *q) {
    for (int i = 0; i < ELEMENTS_COUNT; i++)
        q->push(i);
}

void consumer(QUEUE_T *q) {
    for (int i = 0; i < ELEMENTS_COUNT; i++) {
    	int j;
        while (!q->try_pop(j))
        	;/* busy wait on next element (lock contention?) */
        if (i != j)
            cout << "Unexpected value: got " << j << " while expecting " << i << endl;
    }

}

int main(int argc, char *argv[]) {
    
    task_scheduler_init(2);
    t_group = new task_group;
    
    QUEUE_T *QUEUE; 
    QUEUE = new QUEUE_T();
    
    
    tick_count t0 = tick_count::now();
    
    t_group->run(producer_helper_functor(QUEUE));
    t_group->run(consumer_helper_functor(QUEUE));
    
    t_group->wait();
    
    tick_count t1 = tick_count::now();
    
    cout << "Total time is " << (t1 - t0).seconds() * 1000 << "ms" << endl;
    
    delete QUEUE;
    return 0;
}
