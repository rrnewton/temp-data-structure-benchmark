#ifndef FFQUEUE_H_
#define FFQUEUE_H_

#include <cds/user_setup/cache_line.h>

namespace cds {
    namespace queue {

/** FastForward Queue Implementation.
 *
 * John Giacomoni, Tipp Mosley and Manish Vachharajani.
 * FastForward for Efficient Pipeline Parallelism: A Cache-Optimized Concurrent Lock-Free Queue.
 * PPoP '08.
 */
template<typename T, unsigned int Size>
class FastForwardQueue {
public:
   enum {Capacity = Size+1};

   FastForwardQueue(T sentinel) : SENTINEL(sentinel), tail(0), head(0)
   {
     for (int i = 0; i < Capacity; i++)
       buffer[i] = SENTINEL;
   }
   virtual ~FastForwardQueue() {}

   bool enqueue(T& data);
   bool dequeue(T& data);

   /// Synonym for \ref enqueue
   bool add( T& data )                { return enqueue( data ); }
   /// Synonym for \ref dequeue
   bool tryRemoveAny( T& data )                { return dequeue( data ); }

   void initThread( int threadId ) {}
   void finiThread() {};

private:
   volatile unsigned int tail __attribute__ ((aligned (64)));
   char padding1[c_nCacheLineSize - sizeof(tail)]; // cacheline padding (64 bytes cacheline)
   volatile unsigned int head; // output index
   char padding2[c_nCacheLineSize - sizeof(head)];
   const T SENTINEL;
   char padding3[c_nCacheLineSize - sizeof(SENTINEL)];
   T buffer[Capacity] __attribute__ ((aligned (64)));

   inline unsigned int next(unsigned int idx_) const {
      return (idx_+1) % Capacity;
   }
};


template<typename T, unsigned int Size>
bool FastForwardQueue<T, Size>::enqueue(T& data)
{
   if(!(SENTINEL == buffer[head]))
     return false;

   buffer[head] = data;
   head = next(head);
   return true;
}

template<typename T, unsigned int Size>
bool FastForwardQueue<T, Size>::dequeue(T& data)
{
  data = buffer[tail];
  if(SENTINEL == data)
    return false;

  buffer[tail] = SENTINEL;
  tail = next(tail);
  return true;
}

    }
}
#endif /* FFQUEUE_H_ */
