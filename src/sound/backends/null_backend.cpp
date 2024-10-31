/**
 *
 * @file
 *
 * @brief  Null backend implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "sound/backends/backend_impl.h"
#include "sound/backends/storage.h"

#include "l10n/markup.h"
#include "sound/backend_attrs.h"

#include "make_ptr.h"

namespace Sound::Null
{
  const auto BACKEND_ID = "null"_id;
  const char* const DESCRIPTION = L10n::translate("Null output backend");

  class BackendWorker : public Sound::BackendWorker
  {
  public:
    void Startup() override {}

    void Shutdown() override {}

    void Pause() override {}

    void Resume() override {}

    void FrameStart(const Module::State& /*state*/) override {}

    void FrameFinish(Chunk /*buffer*/) override {}

    VolumeControl::Ptr GetVolumeControl() const override
    {
      return {};
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
}  // namespace Sound::Null

namespace Sound
{
  void RegisterNullBackend(BackendsStorage& storage)
  {
    auto factory = MakePtr<Null::BackendWorkerFactory>();
    storage.Register(Null::BACKEND_ID, Null::DESCRIPTION, CAP_TYPE_STUB, std::move(factory));
  }
}  // namespace Sound
