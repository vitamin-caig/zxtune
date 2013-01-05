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
#include <l10n/api.h>
#include <sound/backend_attrs.h>
#include <sound/render_params.h>
//text includes
#include "text/backends.h"

#define FILE_TAG 9A6FD87F

namespace
{
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound_backends");
}

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
      return translate("Null output backend");
    }

    virtual uint_t Capabilities() const
    {
      return CAP_TYPE_STUB;
    }

    virtual Error Status() const
    {
      return Error();
    }

    virtual Backend::Ptr CreateBackend(CreateBackendParameters::Ptr params) const
    {
      try
      {
        const Parameters::Accessor::Ptr allParams = params->GetParameters();
        const BackendWorker::Ptr worker(new NullBackendWorker());
        return Sound::CreateBackend(params, worker);
      }
      catch (const Error& e)
      {
        throw MakeFormattedError(THIS_LINE,
          translate("Failed to create backend '%1%'."), Id()).AddSuberror(e);
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
