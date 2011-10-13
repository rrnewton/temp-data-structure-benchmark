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

static void producer(Queue *q);
static void consumer(Queue *q);

class producer_helper_functor {
  Queue *queue;
public:
  producer_helper_functor(Queue *q) :
      queue(q) {
  }

  void operator()() const {
    producer(queue);
  }
};

class consumer_helper_functor {
  Queue *queue;
public:
  consumer_helper_functor(Queue *q) :
      queue(q) {
  }

  void operator()() const {
    consumer(queue);
  }
};

void producer(Queue *q) {
  for (long i = 0; i < BENCH_N; i++)
    q->push(i);
}

void consumer(Queue *q) {
  for (long i = 0; i < BENCH_N; i++) {
    long j;
    while (!q->try_pop(j))
      ;/* busy wait on next element (lock contention?) */
    if (i != j)
      cout << "Unexpected value: got " << j << " while expecting " << i << endl;
  }
}

void reset() {
  /* XXX: should probably just .clear() */
  for (int i = 0; i < ntasks/2; i++) {
    delete queue[i];
    queue[i] = new Queue();
  }
}

void init(int nthreads) {
  ntasks = nthreads;
  task_scheduler_init(nthreads+0);
  t_group = new task_group;
  queue = new Queue*[ntasks];
  for (int i = 0; i < ntasks/2; i++)
    queue[i] = NULL;
}

void kernel() {
  for (int i = 0; i < ntasks/2; i++) {
    t_group->run(producer_helper_functor(queue[i]));
    t_group->run(consumer_helper_functor(queue[i]));
  }
  t_group->wait();
}

void finalize() {
  for (int i = 0; i < ntasks/2; i++)
    delete queue[i];
  delete t_group;
}

