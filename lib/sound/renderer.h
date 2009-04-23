#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <sound.h>

#include <cassert>

namespace ZXTune
{
  namespace Sound
  {
    /*
      Renderers. Data for single sample stored sequently.
      size in bytes = size in samples * Channels * sizeof(Sample)
    */
    typedef void (*ReadyCallback)(const void*, std::size_t, void*);

    /// Size in multisamples
    Receiver::Ptr CreateCallbackRenderer(std::size_t size, ReadyCallback callback, void* data = 0);

    Receiver::Ptr CreateBufferRenderer(Sample* buffer, std::size_t size);
  }
}

#endif //__RENDERER_H__
