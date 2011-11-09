#include <iostream>

#include "tbb/concurrent_queue.h"
#include "tbb/tick_count.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/task_group.h"

using namespace tbb;
using namespace std;

#ifndef BENCH_N
#   define BENCH_N  2000000
#endif

typedef concurrent_queue<long> Queue;

task_group *t_group;
static Queue **queue;
static int ntasks = 0;

static void producer_consumer(Queue *q);

class producer_consumer_helper_functor {
  Queue *queue;
public:
  producer_consumer_helper_functor(Queue *q) :
      queue(q) {
  }
  void operator()() const {
    producer_consumer(queue);
  }
};

void producer_consumer(Queue *q) {
  long j;
  for (long i = 0; i < BENCH_N; i++) {
    q->push(i);
    while (!q->try_pop(j))
        ;/* busy wait on next element (lock contention?) */
  }
}

void init(int nthreads) {
  ntasks = nthreads;
  task_scheduler_init init(nthreads);
  t_group = new task_group;
  queue = new Queue*[ntasks];
  for (int i = 0; i < ntasks; i++)
    queue[i] = NULL;
}

void reset() {
  /* XXX: should probably just .clear() */
  for (int i = 0; i < ntasks; i++) {
    delete queue[i];
    queue[i] = new Queue();
  }
}

void kernel() {
  for (int i = 0; i < ntasks; i++)
    t_group->run(producer_consumer_helper_functor(queue[i]));
  t_group->wait();
}

void finalize() {
  for (int i = 0; i < ntasks; i++)
    delete queue[i];
  delete t_group;
}

