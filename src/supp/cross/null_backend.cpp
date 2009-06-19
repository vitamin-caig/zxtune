#include "../backend_enumerator.h"
#include "../sound_backend_impl.h"
#include "../sound_backend_types.h"

#include <sound_attrs.h>

#include <boost/noncopyable.hpp>

namespace
{
  using namespace ZXTune::Sound;
  //TODO
  const String::value_type TEXT_NULL_BACKEND_DESCRIPTON[] = "Null output backend";
  const String::value_type NULL_BACKEND_KEY[] = {'n', 'u', 'l', 'l', 0};

  void Descriptor(Backend::Info& info);

  class NullBackend : public BackendImpl, private boost::noncopyable
  {
  public:
    NullBackend()
    {
    }

    virtual void GetInfo(Info& info) const
    {
      return Descriptor(info);
    }

    virtual void OnParametersChanged(unsigned /*changedFields*/)
    {
      //loop is disabled
      Params.SoundParameters.Flags &= ~MOD_LOOP;
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

    virtual void OnBufferReady(const void* /*data*/, std::size_t /*sizeInBytes*/)
    {
    }
  };

  void Descriptor(Backend::Info& info)
  {
    info.Description = TEXT_NULL_BACKEND_DESCRIPTON;
    info.Key = NULL_BACKEND_KEY;
  }

  Backend::Ptr Creator()
  {
    return Backend::Ptr(new NullBackend);
  }

  BackendAutoRegistrator registrator(Creator, Descriptor);
}
