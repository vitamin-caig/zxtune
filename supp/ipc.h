#ifndef __IPC_H_DEFINED__
#define __IPC_H_DEFINED__

#include <types.h>

#include <boost/noncopyable.hpp>

namespace ZXTune
{
  namespace IPC
  {
    class Mutex
    {
    public:
      Mutex();
      ~Mutex();

      void Lock();
      void Unlock();
    private:
      void* Data;
    };

    class Event
    {
    public:
      Event();
      ~Event();

      void Wait();
      bool Wait(uint32_t timeout);
      void Signal();
    private:
      void* Data;
    };

    typedef bool (*ThreadFunc)(void*);
    class Thread
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
