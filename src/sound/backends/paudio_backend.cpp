/**
*
* @file
*
* @brief  PulseAudio backend implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "backend_impl.h"
#include "storage.h"
#include "gates/paudio_api.h"
//common includes
#include <byteorder.h>
#include <error_tools.h>
#include <make_ptr.h>
//library includes
#include <core/module_attrs.h>
#include <debug/log.h>
#include <l10n/api.h>
#include <platform/version/api.h>
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/render_params.h>
//boost includes
#include <boost/bind.hpp>
//text includes
#include "text/backends.h"

#define FILE_TAG 181AC911

namespace
{
  const Debug::Stream Dbg("Sound::Backend::PulseAudio");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound_backends");
}

namespace Sound
{
namespace PulseAudio
{
  const String ID = Text::PAUDIO_BACKEND_ID;
  const char* const DESCRIPTION = L10n::translate("PulseAudio support backend");
  const uint_t CAPABILITIES = CAP_TYPE_SYSTEM;

  class BackendWorker : public Sound::BackendWorker
  {
  public:
    BackendWorker(Api::Ptr api, Parameters::Accessor::Ptr params, const String& stream)
      : PaApi(api)
      , Params(params)
      , Client(Platform::Version::GetProgramTitle())//TODO: think about another solution...
      , Stream(stream)
    {
    }

    virtual void Startup()
    {
      Dbg("Starting playback");

      Device = OpenDevice();
    }

    virtual void Shutdown()
    {
      Dbg("Shutdown");
      Device = boost::shared_ptr<pa_simple>();
    }

    virtual void Pause()
    {
      Dbg("Pause");
      int error = 0;
      if (PaApi->pa_simple_flush(Device.get(), &error) < 0)
      {
        throw MakeError(error, THIS_LINE);
      }
    }

    virtual void Resume()
    {
      Dbg("Resume");
    }

    virtual void FrameStart(const Module::TrackState& /*state*/)
    {
    }

    virtual void FrameFinish(Chunk::Ptr buffer)
    {
      int error = 0;
      if (PaApi->pa_simple_write(Device.get(), &buffer->front(), buffer->size() * sizeof(buffer->front()), &error) < 0)
      {
        throw MakeError(error, THIS_LINE);
      }
    }

    virtual VolumeControl::Ptr GetVolumeControl() const
    {
      return VolumeControl::Ptr();
    }
  private:
    boost::shared_ptr<pa_simple> OpenDevice() const
    {
      const pa_sample_spec format = GetFormat();
      int error = 0;
      if (pa_simple* result = PaApi->pa_simple_new(NULL, Client.c_str(), PA_STREAM_PLAYBACK, NULL, Stream.c_str(), &format, NULL, NULL, &error))
      {
        return boost::shared_ptr<pa_simple>(result, boost::bind(&Api::pa_simple_free, PaApi, _1));
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
        return MakeFormattedError(loc,
          translate("Error in PulseAudio backend: %1%."), FromStdString(txt));
      }
      else
      {
        return Error(loc, translate("Unknown error in PulseAudio backend."));
      }
    }
  private:
    const Api::Ptr PaApi;
    const Parameters::Accessor::Ptr Params;
    const String Client;
    const String Stream;
    boost::shared_ptr<pa_simple> Device;
  };

  class BackendWorkerFactory : public Sound::BackendWorkerFactory
  {
  public:
    explicit BackendWorkerFactory(Api::Ptr api)
      : PaApi(api)
    {
    }

    virtual BackendWorker::Ptr CreateWorker(Parameters::Accessor::Ptr params, Module::Holder::Ptr holder) const
    {
      const String& stream = GetStreamName(*holder);
      return MakePtr<BackendWorker>(PaApi, params, stream);
    }
  private:
    static String GetStreamName(const Module::Holder& holder)
    {
      const Parameters::Accessor::Ptr props = holder.GetModuleProperties();
      String author, title;
      props->FindValue(Module::ATTR_AUTHOR, author);
      props->FindValue(Module::ATTR_TITLE, title);
      const bool hasAuthor = !author.empty();
      const bool hasTitle = !title.empty();
      return hasAuthor && hasTitle
        ? author + " - " + title
        : (hasTitle ? title : "");
    }
  private:
    const Api::Ptr PaApi;
  };
}//PulseAudio
}//Sound

namespace Sound
{
  void RegisterPulseAudioBackend(BackendsStorage& storage)
  {
    try
    {
      const PulseAudio::Api::Ptr api = PulseAudio::LoadDynamicApi();
      const char* const version = api->pa_get_library_version();
      Dbg("Detected PulseAudio v%1%", version);
      const BackendWorkerFactory::Ptr factory = MakePtr<PulseAudio::BackendWorkerFactory>(api);
      storage.Register(PulseAudio::ID, PulseAudio::DESCRIPTION, PulseAudio::CAPABILITIES, factory);
    }
    catch (const Error& e)
    {
      storage.Register(PulseAudio::ID, PulseAudio::DESCRIPTION, PulseAudio::CAPABILITIES, e);
    }
  }
}
