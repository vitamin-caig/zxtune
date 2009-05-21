#include "../sound_backend_impl.h"
#include "../sound_backend_types.h"

#include <sound_attrs.h>

#include <boost/noncopyable.hpp>

namespace
{
  using namespace ZXTune::Sound;

  class NullBackend : public BackendImpl, private boost::noncopyable
  {
  public:
    NullBackend()
    {
    }

    virtual void OnParametersChanged(unsigned /*changedFields*/)
    {
    }

    virtual void OnStartup()
    {
    }

    virtual void OnShutdown()
    {
    }

    virtual void OnPause()
    {
    }

    virtual void OnResume()
    {
    }

    virtual void OnBufferReady(const void* data, std::size_t sizeInBytes)
    {
    }
  };
}

namespace ZXTune
{
  namespace Sound
  {
    Backend::Ptr CreateNullBackend()
    {
      return Backend::Ptr(new NullBackend);
    }
  }
}
