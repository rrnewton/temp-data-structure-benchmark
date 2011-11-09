#include <iostream>
#include <assert.h>

#include "tbb/concurrent_queue.h"
#include "tbb/tick_count.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/task_group.h"

using namespace tbb;
using namespace std;

#ifndef BENCH_N
#   define BENCH_N  2000000
#endif

typedef struct {
  long e0;
  long e1;
  long e2;
  long e3;
  long e4;
  long e5;
  long e6;
  long e7;
} cacheline;

typedef concurrent_queue<cacheline> Queue;

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
  cacheline c;
  for (long i = 0; i < BENCH_N/8; i++) {
    c.e3 = c.e0 = i;
    q->push(c);
  }
}

void consumer(Queue *q) {
  cacheline c = { 0, 0, 0, 0, 0, 0, 0, 0 };
  while (c.e3 + 1 < BENCH_N/8) {
    while (!q->try_pop(c))
      ;/* busy wait on next element (lock contention?) */
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
  assert(!(ntasks % 2));
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

