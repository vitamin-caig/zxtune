/*
Abstract:
  Null backend implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "backend_impl.h"
#include "backend_wrapper.h"
#include "enumerator.h"
//library includes
#include <sound/backend_attrs.h>
#include <sound/error_codes.h>
//boost includes
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
//text includes
#include <sound/text/backends.h>
#include <sound/text/sound.h>

#define FILE_TAG 9A6FD87F

namespace
{
  using namespace ZXTune::Sound;

  const Char NULL_BACKEND_ID[] = {'n', 'u', 'l', 'l', 0};
  const String NULL_BACKEND_VERSION(FromStdString("$Rev$"));

  // dummy backend with no functionality except informational
  class NullBackend : public BackendImpl
                    , private boost::noncopyable
  {
  public:
    VolumeControl::Ptr GetVolumeControl() const
    {
      return VolumeControl::Ptr();
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
    
    virtual void OnParametersChanged(const Parameters::Accessor& /*updates*/)
    {
    }
    
    virtual void OnBufferReady(std::vector<MultiSample>& /*buffer*/)
    {
    }
  };

  class NullBackendCreator : public BackendCreator
                           , public boost::enable_shared_from_this<NullBackendCreator>
  {
  public:
    virtual String Id() const
    {
      return NULL_BACKEND_ID;
    }

    virtual String Description() const
    {
      return Text::NULL_BACKEND_DESCRIPTION;
    }

    virtual String Version() const
    {
      return NULL_BACKEND_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_TYPE_STUB;
    }

    virtual Error CreateBackend(Parameters::Accessor::Ptr params, Backend::Ptr& result) const
    {
      const BackendInformation::Ptr info = shared_from_this();
      return SafeBackendWrapper<NullBackend>::Create(info, params, result, THIS_LINE);
    }
  };
}

namespace ZXTune
{
  namespace Sound
  {
    void RegisterNullBackend(BackendsEnumerator& enumerator)
    {
      const BackendCreator::Ptr creator(new NullBackendCreator());
      enumerator.RegisterCreator(creator);
    }
  }
}
