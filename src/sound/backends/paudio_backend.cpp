/**
 *
 * @file
 *
 * @brief  PulseAudio backend implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "sound/backends/backend_impl.h"
#include "sound/backends/gates/paudio_api.h"
#include "sound/backends/l10n.h"
#include "sound/backends/paudio.h"
#include "sound/backends/storage.h"

#include "debug/log.h"
#include "l10n/api.h"
#include "module/attributes.h"
#include "module/holder.h"
#include "parameters/accessor.h"
#include "platform/version/api.h"
#include "sound/backend.h"
#include "sound/backend_attrs.h"
#include "sound/chunk.h"
#include "sound/render_params.h"

#include "byteorder.h"
#include "error_tools.h"
#include "make_ptr.h"
#include "string_type.h"
#include "string_view.h"

#include <pulse/def.h>
#include <pulse/sample.h>

#include <memory>
#include <utility>
#include <vector>

namespace Module
{
  class State;
}  // namespace Module

namespace Sound::PulseAudio
{
  const Debug::Stream Dbg("Sound::Backend::PulseAudio");

  const uint_t CAPABILITIES = CAP_TYPE_SYSTEM;

  class BackendWorker : public Sound::BackendWorker
  {
  public:
    BackendWorker(Api::Ptr api, Parameters::Accessor::Ptr params, StringView stream)
      : PaApi(std::move(api))
      , Params(std::move(params))
      , Client(Platform::Version::GetProgramTitle())  // TODO: think about another solution...
      , Stream(stream)
    {}

    void Startup() override
    {
      Dbg("Starting playback");

      Device = OpenDevice();
    }

    void Shutdown() override
    {
      Dbg("Shutdown");
      Device = {};
    }

    void Pause() override
    {
      Dbg("Pause");
      int error = 0;
      if (PaApi->pa_simple_flush(Device.get(), &error) < 0)
      {
        throw MakeError(error, THIS_LINE);
      }
    }

    void Resume() override
    {
      Dbg("Resume");
    }

    void FrameStart(const Module::State& /*state*/) override {}

    void FrameFinish(Chunk buffer) override
    {
      int error = 0;
      if (PaApi->pa_simple_write(Device.get(), buffer.data(), buffer.size() * sizeof(buffer.front()), &error) < 0)
      {
        throw MakeError(error, THIS_LINE);
      }
    }

    VolumeControl::Ptr GetVolumeControl() const override
    {
      return {};
    }

  private:
    std::shared_ptr<pa_simple> OpenDevice() const
    {
      const pa_sample_spec format = GetFormat();
      int error = 0;
      if (auto* result = PaApi->pa_simple_new(nullptr, Client.c_str(), PA_STREAM_PLAYBACK, nullptr, Stream.c_str(),
                                              &format, nullptr, nullptr, &error))
      {
        return {result, [api = PaApi](auto&& arg) { api->pa_simple_free(arg); }};
      }
      throw MakeError(error, THIS_LINE);
    }

    pa_sample_spec GetFormat() const
    {
      static_assert(Sample::BITS == 16 && Sample::MID == 0, "Incompatible sound sample type");

      pa_sample_spec format;
      format.channels = Sample::CHANNELS;
      format.format = isLE() ? PA_SAMPLE_S16LE : PA_SAMPLE_S16BE;

      const RenderParameters::Ptr sound = RenderParameters::Create(Params);
      format.rate = sound->SoundFreq();
      return format;
    }

    Error MakeError(int code, Error::LocationRef loc) const
    {
      if (const char* txt = PaApi->pa_strerror(code))
      {
        return MakeFormattedError(loc, translate("Error in PulseAudio backend: {}."), txt);
      }
      else
      {
        return {loc, translate("Unknown error in PulseAudio backend.")};
      }
    }

  private:
    const Api::Ptr PaApi;
    const Parameters::Accessor::Ptr Params;
    const String Client;
    const String Stream;
    std::shared_ptr<pa_simple> Device;
  };

  class BackendWorkerFactory : public Sound::BackendWorkerFactory
  {
  public:
    explicit BackendWorkerFactory(Api::Ptr api)
      : PaApi(std::move(api))
    {}

    BackendWorker::Ptr CreateWorker(Parameters::Accessor::Ptr params, Module::Holder::Ptr holder) const override
    {
      const auto& stream = GetStreamName(*holder);
      return MakePtr<BackendWorker>(PaApi, std::move(params), stream);
    }

  private:
    static String GetStreamName(const Module::Holder& holder)
    {
      const auto props = holder.GetModuleProperties();
      const auto author = Parameters::GetString(*props, Module::ATTR_AUTHOR);
      const auto title = Parameters::GetString(*props, Module::ATTR_TITLE);
      const bool hasAuthor = !author.empty();
      const bool hasTitle = !title.empty();
      return hasAuthor && hasTitle ? author + " - " + title : (hasTitle ? title : "");
    }

  private:
    const Api::Ptr PaApi;
  };
}  // namespace Sound::PulseAudio

namespace Sound
{
  void RegisterPulseAudioBackend(BackendsStorage& storage)
  {
    try
    {
      auto api = PulseAudio::LoadDynamicApi();
      const char* const version = api->pa_get_library_version();
      PulseAudio::Dbg("Detected PulseAudio v{}", version);
      auto factory = MakePtr<PulseAudio::BackendWorkerFactory>(std::move(api));
      storage.Register(PulseAudio::BACKEND_ID, PulseAudio::BACKEND_DESCRIPTION, PulseAudio::CAPABILITIES,
                       std::move(factory));
    }
    catch (const Error& e)
    {
      storage.Register(PulseAudio::BACKEND_ID, PulseAudio::BACKEND_DESCRIPTION, PulseAudio::CAPABILITIES, e);
    }
  }
}  // namespace Sound
