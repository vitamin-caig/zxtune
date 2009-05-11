#include "../ipc.h"

#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#include <cassert>

namespace
{
  void CheckError(bool ok)
  {
    if (!ok)
    {
      throw errno;//TODO
    }
  }
}

namespace ZXTune
{
  namespace IPC
  {
    Mutex::Mutex() : Data(new pthread_mutex_t)
    {
      pthread_mutex_init(static_cast<pthread_mutex_t*>(Data), 0);
    }

    Mutex::~Mutex()
    {
      pthread_mutex_destroy(static_cast<pthread_mutex_t*>(Data));
      delete static_cast<pthread_mutex_t*>(Data);
    }

    void Mutex::Lock()
    {
      pthread_mutex_lock(static_cast<pthread_mutex_t*>(Data));
    }

    void Mutex::Unlock()
    {
      pthread_mutex_unlock(static_cast<pthread_mutex_t*>(Data));
    }

    struct ThreadData
    {
      ThreadData() : Handle(0), Function(0), Parameter(0)
      {
      }
      pthread_t Handle;
      ThreadFunc Function;
      void* Parameter;
    };

    void* ThreadFuncWrapper(void* param)
    {
      ThreadData* const data(static_cast<ThreadData*>(param));
      assert(data);
      assert(data->Function);
      while ((*data->Function)(data->Parameter))
      {
      }
      return 0;
    }

    Thread::Thread() : Data(new ThreadData)
    {
    }

    Thread::~Thread()
    {
      assert(static_cast<ThreadData*>(Data)->Handle == 0);
      delete static_cast<ThreadData*>(Data);
    }

    void Thread::Start(ThreadFunc func, void* param)
    {
      ThreadData* const data(static_cast<ThreadData*>(Data));
      if (data->Handle != 0)
      {
        Stop();
      }
      data->Function = func;
      data->Parameter = param;
      pthread_create(&data->Handle, 0, ThreadFuncWrapper, Data);
      assert(data->Handle);
      CheckError(0 != data->Handle);
    }

    void Thread::Stop()
    {
      ThreadData* const data(static_cast<ThreadData*>(Data));
      if (data->Handle != 0)
      {
        pthread_join(data->Handle, 0);
        data->Handle = 0;
      }
    }

    void Sleep(uint32_t milliseconds)
    {
      usleep(milliseconds);
    }
  }
}
