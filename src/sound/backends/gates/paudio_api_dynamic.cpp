/**
 *
 * @file
 *
 * @brief  PulseAudio subsystem API gate implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "sound/backends/gates/paudio_api.h"

#include "debug/log.h"
#include "platform/shared_library.h"
#include "platform/shared_library_adapter.h"

#include "make_ptr.h"
#include "string_view.h"

#include <pulse/channelmap.h>
#include <pulse/def.h>
#include <pulse/error.h>
#include <pulse/sample.h>
#include <pulse/version.h>
#include <stddef.h>

#include <utility>
#include <vector>

namespace Sound::PulseAudio
{
  class LibraryName : public Platform::SharedLibrary::Name
  {
  public:
    LibraryName() = default;

    StringView Base() const override
    {
      return "pulse-simple"sv;
    }

    std::vector<StringView> PosixAlternatives() const override
    {
      return {"libpulse-simple.so.0"sv, "libpulse-simple.so.0.1"sv, "libpulse-simple.so.0.1.0"sv};
    }

    std::vector<StringView> WindowsAlternatives() const override
    {
      return {};
    }
  };

  class DynamicApi : public Api
  {
  public:
    explicit DynamicApi(Platform::SharedLibrary::Ptr lib)
      : Lib(std::move(lib))
    {
      Debug::Log("Sound::Backend::PulseAudio", "Library loaded");
    }

    ~DynamicApi() override
    {
      Debug::Log("Sound::Backend::PulseAudio", "Library unloaded");
    }

    // clang-format off

    const char* pa_get_library_version() override
    {
      using FunctionType = decltype(&::pa_get_library_version);
      const auto func = Lib.GetSymbol<FunctionType>("pa_get_library_version");
      return func();
    }

    const char* pa_strerror(int error) override
    {
      using FunctionType = decltype(&::pa_strerror);
      const auto func = Lib.GetSymbol<FunctionType>("pa_strerror");
      return func(error);
    }

    pa_simple* pa_simple_new(const char* server, const char* name, pa_stream_direction_t dir, const char* dev, const char* stream, const pa_sample_spec* ss, const pa_channel_map* map, const pa_buffer_attr* attr, int* error) override
    {
      using FunctionType = decltype(&::pa_simple_new);
      const auto func = Lib.GetSymbol<FunctionType>("pa_simple_new");
      return func(server, name, dir, dev, stream, ss, map, attr, error);
    }

    int pa_simple_write(pa_simple* s, const void* data, size_t bytes, int* error) override
    {
      using FunctionType = decltype(&::pa_simple_write);
      const auto func = Lib.GetSymbol<FunctionType>("pa_simple_write");
      return func(s, data, bytes, error);
    }

    int pa_simple_flush(pa_simple* s, int* error) override
    {
      using FunctionType = decltype(&::pa_simple_flush);
      const auto func = Lib.GetSymbol<FunctionType>("pa_simple_flush");
      return func(s, error);
    }

    void pa_simple_free(pa_simple* s) override
    {
      using FunctionType = decltype(&::pa_simple_free);
      const auto func = Lib.GetSymbol<FunctionType>("pa_simple_free");
      return func(s);
    }

    // clang-format on
  private:
    const Platform::SharedLibraryAdapter Lib;
  };

  Api::Ptr LoadDynamicApi()
  {
    static const LibraryName NAME;
    auto lib = Platform::SharedLibrary::Load(NAME);
    return MakePtr<DynamicApi>(std::move(lib));
  }
}  // namespace Sound::PulseAudio
