#ifndef _QUEUE_
#define _QUEUE_


#include <atomic>
namespace cvk{
    class spinlock{
        std::atomic_flag flag = ATOMIC_FLAG_INIT;
      public:
        void lock() {
            while (flag.test_and_set(std::memory_order_acquire)) {}
        }
        void unlock() {
            flag.clear(std::memory_order_release);
        }
    };

    class locker{
        cvk::spinlock& sl;
        bool unlocked = false;
      public:
        explicit locker(cvk::spinlock& spinlock)
        :sl(spinlock)
        {
            sl.lock();
        }
        ~locker(){
          if(not unlocked)
            sl.unlock();
        }
        void unlock(){
            sl.unlock();
            unlocked = true;
        }
    };
}


#include <deque>

template <class T>
class Queue
{
private:
    std::deque<T *> queue;
    cvk::spinlock sl;
public:
    void push(T *value);
    T get();
    T get_nowait();
};
#endif
