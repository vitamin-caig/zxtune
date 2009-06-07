#include "sound_backend_impl.h"
#include "sound_backend_types.h"

#include <tools.h>

#include <boost/bind.hpp>
#include <boost/thread.hpp>

namespace ZXTune
{
  namespace Sound
  {
    template<class BufferType>
    class AsyncBackend : public BackendImpl
    {
    public:
      typedef BufferType Buffer;
      AsyncBackend() : Buffers(), FillPtr(), PlayPtr(), Stopping(true)
      {
      }

    protected:
      virtual void OnParametersChanged(unsigned changedFields)
      {
        if (changedFields & DRIVER_FLAGS)
        {
          const unsigned depth(Params.DriverFlags & BUFFER_DEPTH_MASK);
          if (depth && (depth < MIN_BUFFER_DEPTH || depth > MAX_BUFFER_DEPTH))
          {
            throw 1;//TODO
          }
        }
      }
      virtual void OnStartup()
      {
        const unsigned depth(Params.DriverFlags & BUFFER_DEPTH_MASK);
        Buffers.assign(depth ? depth : MIN_BUFFER_DEPTH, BufferEntry());
        //impossible to use boost::bind
        for (typename BufferEntryArray::iterator it = Buffers.begin(), lim = Buffers.end(); it != lim; ++it)
        {
          AllocateBuffer(it->Data);
        }
        PlayPtr = FillPtr = cycled_iterator<BufferEntry*>(&Buffers[0], &Buffers[Buffers.size()]);
        Stopping = false;
        PlaybackThread = boost::thread(std::mem_fun(&AsyncBackend<BufferType>::PlaybackFunc), this);
      }
      virtual void OnShutdown()
      {
        Stopping = true;
        FilledEvent.notify_one();
        PlayedEvent.notify_one();
        PlaybackThread.join();
        //impossible to use boost::bind
        for (typename BufferEntryArray::iterator it = Buffers.begin(), lim = Buffers.end(); it != lim; ++it)
        {
          ReleaseBuffer(it->Data);
        }
        Buffers.clear();
      }
      virtual void OnBufferReady(const void* data, std::size_t sizeInBytes)
      {
        if (!Stopping)
        {
          while (FillPtr->IsFilled)
          {
            //overrun
            Locker lock(QueueLock);
            PlayedEvent.wait(lock);
            if (Stopping)
            {
              return;
            }
          }
          assert(0 == sizeInBytes % sizeof(Sample));
          FillBuffer(data, sizeInBytes / sizeof(Sample), FillPtr->Data);
          FillPtr->IsFilled = true;
          ++FillPtr;
          FilledEvent.notify_one();
        }
      }
    private:
      void PlaybackFunc()
      {
        while (!Stopping)
        {
          while (!PlayPtr->IsFilled)
          {
            //underrun
            Locker lock(QueueLock);
            FilledEvent.wait(lock);
            if (Stopping)
            {
              return;
            }
          }
          PlayBuffer(PlayPtr->Data);
          PlayPtr->IsFilled = false;
          ++PlayPtr;
          PlayedEvent.notify_one();
        }
      }
    protected:
      virtual void AllocateBuffer(BufferType& buf) = 0;
      virtual void ReleaseBuffer(BufferType& buf) = 0;
      virtual void FillBuffer(const void* data, std::size_t sizeInBytes, BufferType& buf) = 0;
      virtual void PlayBuffer(const BufferType& buf) = 0;
    private:
      struct BufferEntry
      {
        BufferEntry() : IsFilled(false), Data()
        {
        }
        volatile bool IsFilled;
        BufferType Data;
      };
      typedef std::vector<BufferEntry> BufferEntryArray;
      BufferEntryArray Buffers;
      cycled_iterator<BufferEntry*> FillPtr;
      cycled_iterator<BufferEntry*> PlayPtr;
      //sync
      volatile bool Stopping;
      typedef boost::unique_lock<boost::mutex> Locker;
      boost::mutex QueueLock;
      boost::condition_variable FilledEvent;
      boost::condition_variable PlayedEvent;
      boost::thread PlaybackThread;
    };

    struct SimpleBuffer
    {
      SimpleBuffer() : Data(), Size()
      {
      }
      std::vector<Sample> Data;
      std::size_t Size;
    };

    class SimpleAsyncBackend : public AsyncBackend<SimpleBuffer>
    {
    public:
      typedef AsyncBackend<SimpleBuffer> Parent;
      SimpleAsyncBackend()
      {
      }

      virtual void OnPause()
      {
      }

      virtual void OnResume()
      {
      }
    protected:
      virtual void AllocateBuffer(Parent::Buffer& buf)
      {
        buf.Data.resize(OUTPUT_CHANNELS * Params.BufferInMultisamples());
      }
      virtual void ReleaseBuffer(Parent::Buffer& buf)
      {
        std::vector<Sample> tmp;
        buf.Data.swap(tmp);
      }
      virtual void FillBuffer(const void* data, std::size_t sizeInSamples, Parent::Buffer& buf)
      {
        assert(sizeInSamples <= buf.Data.size());
        std::memcpy(&buf.Data[0], data, (buf.Size = sizeInSamples) * sizeof(buf.Data.front()));
      }
    };
  }
}
