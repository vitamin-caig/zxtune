#ifndef __IPC_H_DEFINED__
#define __IPC_H_DEFINED__

#include <types.h>

#include <boost/noncopyable.hpp>

namespace ZXTune
{
  namespace IPC
  {
    class Mutex : private boost::noncopyable
    {
    public:
      Mutex();
      ~Mutex();

      void Lock();
      void Unlock();
    private:
      void* Data;
    };

    typedef bool (*ThreadFunc)(void*);
    class Thread : private boost::noncopyable
    {
    public:
      Thread();
      ~Thread();

      void Start(ThreadFunc func, void* param);
      void Stop();
    private:
      void* Data;
    };

    class Locker : private boost::noncopyable
    {
    public:
      Locker(Mutex& mtx) : Obj(mtx)
      {
        Obj.Lock();
      }

      ~Locker()
      {
        Obj.Unlock();
      }
    private:
      Mutex& Obj;
    };

    void Sleep(uint32_t milliseconds);
  }
}

#endif //__IPC_H_DEFINED__
