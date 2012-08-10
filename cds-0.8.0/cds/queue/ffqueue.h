#ifndef FFQUEUE_H_
#define FFQUEUE_H_

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

   FastForwardQueue() : tail(0), head(0){ memset(buffer, 0, sizeof(T)*Capacity); }
   virtual ~FastForwardQueue() {}

   bool enqueue(T& data);
   bool dequeue(T& data);

   /// Synonym for \ref enqueue
   bool push( T& data )                { return enqueue( data ); }
   /// Synonym for \ref dequeue
   bool pop( T& data )                { return dequeue( data ); }

private:
   volatile unsigned int tail; // input index
   T buffer[Capacity];
   volatile unsigned int head; // output index

   inline unsigned int next(unsigned int idx_) const {
      return (idx_+1) % Capacity;
   }
};


template<typename T, unsigned int Size>
bool FastForwardQueue<T, Size>::enqueue(T& data)
{
   if(!(((void*)0) == buffer[head]))
     return false;

   buffer[head] = data;
   head = next(head);
   return true;
}

template<typename T, unsigned int Size>
bool FastForwardQueue<T, Size>::dequeue(T& data)
{
  data = buffer[tail];
  if(((void*)0) == data)
    return false;

  buffer[tail] = ((void*)0);
  tail = next(tail);
  return true;
}

    }
}
#endif /* FFQUEUE_H_ */
