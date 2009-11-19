#include "sound_backend_async.h"

#include <error.h>

#include <text/errors.h>

#define FILE_TAG 70F7A2C5

namespace ZXTune
{
  namespace Sound
  {
    AsyncBackendImpl::AsyncBackendImpl() : Stopping(true)
    {
    }

    void AsyncBackendImpl::OnParametersChanged(unsigned changedFields)
    {
      if (changedFields & DRIVER_FLAGS)
      {
        const unsigned depth(Params.DriverFlags & BUFFER_DEPTH_MASK);
        if (depth && (depth < MIN_BUFFER_DEPTH || depth > MAX_BUFFER_DEPTH))
        {
          throw Error(ERROR_DETAIL, 1, TEXT_ERROR_BACKEND_INVALID_BUFFER_DEPTH);//TODO: code
        }
      }
    }

    void AsyncBackendImpl::OnShutdown()
    {
      Stopping = true;
      FilledEvent.notify_one();
      PlayedEvent.notify_one();
      PlaybackThread.join();
    }

    void AsyncBackendImpl::OnPause()
    {
    }

    void AsyncBackendImpl::OnResume()
    {
    }

    //Simple async backend implementation
    SimpleAsyncBackend::SimpleAsyncBackend()
    {
    }

    void SimpleAsyncBackend::AllocateBuffer(Parent::Buffer& buf)
    {
      buf.Data.resize(OUTPUT_CHANNELS * Params.BufferInMultisamples());
    }

    void SimpleAsyncBackend::ReleaseBuffer(Parent::Buffer& buf)
    {
      std::vector<Sample> tmp;
      buf.Data.swap(tmp);
    }

    void SimpleAsyncBackend::FillBuffer(const void* data, std::size_t sizeInSamples, Parent::Buffer& buf)
    {
      assert(sizeInSamples <= buf.Data.size());
      std::memcpy(&buf.Data[0], data, (buf.Size = sizeInSamples) * sizeof(buf.Data.front()));
    }
  }
}
