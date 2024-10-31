/**
 *
 * @file
 *
 * @brief  VorbisEnc subsystem API gate implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "sound/backends/gates/vorbisenc_api.h"

#include <debug/log.h>
#include <platform/shared_library_adapter.h>

#include <make_ptr.h>
#include <string_view.h>

namespace Sound::VorbisEnc
{
  class LibraryName : public Platform::SharedLibrary::Name
  {
  public:
    LibraryName() = default;

    StringView Base() const override
    {
      return "vorbisenc"sv;
    }

    std::vector<StringView> PosixAlternatives() const override
    {
      return {"libvorbisenc.so.2"sv};
    }

    std::vector<StringView> WindowsAlternatives() const override
    {
      return {"libvorbisenc-2.dll"sv};
    }
  };

  class DynamicApi : public Api
  {
  public:
    explicit DynamicApi(Platform::SharedLibrary::Ptr lib)
      : Lib(std::move(lib))
    {
      Debug::Log("Sound::Backend::Ogg", "Library loaded");
    }

    ~DynamicApi() override
    {
      Debug::Log("Sound::Backend::Ogg", "Library unloaded");
    }

    // clang-format off

    int vorbis_encode_init(vorbis_info *vi, long channels, long rate, long max_bitrate, long nominal_bitrate, long min_bitrate) override
    {
      using FunctionType = decltype(&::vorbis_encode_init);
      const auto func = Lib.GetSymbol<FunctionType>("vorbis_encode_init");
      return func(vi, channels, rate, max_bitrate, nominal_bitrate, min_bitrate);
    }

    int vorbis_encode_init_vbr(vorbis_info *vi, long channels, long rate, float base_quality) override
    {
      using FunctionType = decltype(&::vorbis_encode_init_vbr);
      const auto func = Lib.GetSymbol<FunctionType>("vorbis_encode_init_vbr");
      return func(vi, channels, rate, base_quality);
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
}  // namespace Sound::VorbisEnc
