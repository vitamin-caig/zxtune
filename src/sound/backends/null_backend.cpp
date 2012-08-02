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
#include "enumerator.h"
//common includes
#include <error_tools.h>
//library includes
#include <sound/backend_attrs.h>
#include <sound/error_codes.h>
#include <sound/render_params.h>
//text includes
#include <sound/text/backends.h>
#include <sound/text/sound.h>

#define FILE_TAG 9A6FD87F

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  class NullBackendWorker : public BackendWorker
  {
  public:
    virtual void Test()
    {
    }

    virtual void OnStartup(const Module::Holder& /*module*/)
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

    virtual void OnFrame(const Module::TrackState& /*state*/)
    {
    }

    virtual void OnBufferReady(Chunk& /*buffer*/)
    {
    }

    VolumeControl::Ptr GetVolumeControl() const
    {
      return VolumeControl::Ptr();
    }
  };

  class NullBackendCreator : public BackendCreator
  {
  public:
    virtual String Id() const
    {
      return Text::NULL_BACKEND_ID;
    }

    virtual String Description() const
    {
      return Text::NULL_BACKEND_DESCRIPTION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_TYPE_STUB;
    }

    virtual Error Status() const
    {
      return Error();
    }

    virtual Error CreateBackend(CreateBackendParameters::Ptr params, Backend::Ptr& result) const
    {
      try
      {
        const Parameters::Accessor::Ptr allParams = params->GetParameters();
        const BackendWorker::Ptr worker(new NullBackendWorker());
        result = Sound::CreateBackend(params, worker);
        return Error();
      }
      catch (const Error& e)
      {
        return MakeFormattedError(THIS_LINE, BACKEND_FAILED_CREATE,
          Text::SOUND_ERROR_BACKEND_FAILED, Id()).AddSuberror(e);
      }
      catch (const std::bad_alloc&)
      {
        return Error(THIS_LINE, BACKEND_NO_MEMORY, Text::SOUND_ERROR_BACKEND_NO_MEMORY);
      }
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
