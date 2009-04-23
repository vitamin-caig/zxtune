#include "../ipc.h"

#include <windows.h>

#include <cassert>

namespace
{
  const unsigned THREAD_WAIT_TIMEOUT = 10000;//10 sec is enough
  void CheckError(bool ok)
  {
    if (!ok)
    {
      throw ::GetLastError();//TODO
    }
  }
}

namespace ZXTune
{
  namespace IPC
  {
    Mutex::Mutex() : Data(new ::CRITICAL_SECTION)
    {
      ::InitializeCriticalSection(static_cast<::LPCRITICAL_SECTION>(Data));
    }

    Mutex::~Mutex()
    {
      ::DeleteCriticalSection(static_cast<::LPCRITICAL_SECTION>(Data));
      delete static_cast<::CRITICAL_SECTION*>(Data);
    }

    void Mutex::Lock()
    {
      ::EnterCriticalSection(static_cast<::LPCRITICAL_SECTION>(Data));
    }

    void Mutex::Unlock()
    {
      ::LeaveCriticalSection(static_cast<::LPCRITICAL_SECTION>(Data));
    }

    struct ThreadData
    {
      ThreadData() : Handle(INVALID_HANDLE_VALUE), Id(0), Function(0), Parameter(0)
      {
      }
      ::HANDLE Handle;
      ::DWORD Id;
      ThreadFunc Function;
      void* Parameter;
    };

    ::DWORD WINAPI ThreadFuncWrapper(::LPVOID param)
    {
      ThreadData* const data(static_cast<ThreadData*>(param));
      assert(data);
      assert(data->Function);
      SetThreadPriority(data->Handle, THREAD_PRIORITY_TIME_CRITICAL);
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
      assert(static_cast<ThreadData*>(Data)->Handle == INVALID_HANDLE_VALUE);
      delete static_cast<ThreadData*>(Data);
    }

    void Thread::Start(ThreadFunc func, void* param)
    {
      ThreadData* const data(static_cast<ThreadData*>(Data));
      assert(data->Handle == INVALID_HANDLE_VALUE);
      data->Function = func;
      data->Parameter = param;
      data->Handle = ::CreateThread(0, 0, ThreadFuncWrapper, Data, 0, &data->Id);
      assert(data->Handle);
      CheckError(0 != data->Handle);
    }

    void Thread::Stop()
    {
      ThreadData* const data(static_cast<ThreadData*>(Data));
      assert(data->Handle != INVALID_HANDLE_VALUE);
      ::DWORD res(::WaitForSingleObject(data->Handle, THREAD_WAIT_TIMEOUT));
      if (WAIT_TIMEOUT == res)
      {
        assert(!"Failed to wait thread");
        CheckError(FALSE != ::TerminateThread(data->Handle, 0));
      }
      CheckError(FALSE != ::CloseHandle(data->Handle));
      data->Handle = INVALID_HANDLE_VALUE;
    }

    void Sleep(uint32_t milliseconds)
    {
      return ::Sleep(milliseconds);
    }
  }
}
