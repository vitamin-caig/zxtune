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
#include <boost/noncopyable.hpp>
//text includes
#include <sound/text/backends.h>
#include <sound/text/sound.h>

#define FILE_TAG 9A6FD87F

namespace
{
  using namespace ZXTune::Sound;

  const Char BACKEND_ID[] = {'n', 'u', 'l', 'l', 0};
  const String BACKEND_VERSION(FromStdString("$Rev$"));

  // backend description
  void DescribeBackend(BackendInformation& info)
  {
    info.Id = BACKEND_ID;
    info.Description = Text::NULL_BACKEND_DESCRIPTION;
    info.Version = BACKEND_VERSION;
    info.Capabilities = CAP_TYPE_STUB;
  }

  // dummy backend with no functionality except informational
  class NullBackend : public BackendImpl, private boost::noncopyable
  {
  public:
    NullBackend()
    {
    }

    virtual void GetInformation(BackendInformation& info) const
    {
      DescribeBackend(info);
    }

    virtual VolumeControl::Ptr GetVolumeControl() const
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
    
    virtual void OnParametersChanged(const Parameters::Map& /*updates*/)
    {
    }
    
    virtual void OnBufferReady(std::vector<MultiSample>& /*buffer*/)
    {
    }
  };

  Backend::Ptr NullBackendCreator(const Parameters::Map& params)
  {
    return Backend::Ptr(new SafeBackendWrapper<NullBackend>(params));
  }
}

namespace ZXTune
{
  namespace Sound
  {
    void RegisterNullBackend(BackendsEnumerator& enumerator)
    {
      BackendInformation info;
      DescribeBackend(info);
      enumerator.RegisterBackend(info, &NullBackendCreator);
    }
  }
}
