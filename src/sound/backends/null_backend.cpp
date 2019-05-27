/**
*
* @file
*
* @brief  Null backend implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "sound/backends/backend_impl.h"
#include "sound/backends/storage.h"
//common includes
#include <make_ptr.h>
//library includes
#include <l10n/api.h>
#include <sound/backend_attrs.h>
//text includes
#include <sound/backends/text/backends.h>

#define FILE_TAG 9A6FD87F

namespace Sound
{
namespace Null
{
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound_backends");

  const String ID = Text::NULL_BACKEND_ID;
  const char* const DESCRIPTION = L10n::translate("Null output backend");

  class BackendWorker : public Sound::BackendWorker
  {
  public:
    void Startup() override
    {
    }

    void Shutdown() override
    {
    }

    void Pause() override
    {
    }

    void Resume() override
    {
    }

    void FrameStart(const Module::State& /*state*/) override
    {
    }

    void FrameFinish(Chunk /*buffer*/) override
    {
    }

    VolumeControl::Ptr GetVolumeControl() const override
    {
      return VolumeControl::Ptr();
    }
  };

  class BackendWorkerFactory : public Sound::BackendWorkerFactory
  {
  public:
    BackendWorker::Ptr CreateWorker(Parameters::Accessor::Ptr /*params*/, Module::Holder::Ptr /*holder*/) const override
    {
      return MakePtr<BackendWorker>();
    }
  };
}//Null
}//Sound

namespace Sound
{
  void RegisterNullBackend(BackendsStorage& storage)
  {
    const BackendWorkerFactory::Ptr factory = MakePtr<Null::BackendWorkerFactory>();
    storage.Register(Null::ID, Null::DESCRIPTION, CAP_TYPE_STUB, factory);
  }
}

#undef FILE_TAG
