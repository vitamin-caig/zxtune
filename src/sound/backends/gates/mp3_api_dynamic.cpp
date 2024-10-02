/**
 *
 * @file
 *
 * @brief  MP3 subsystem API gate implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound/backends/gates/mp3_api.h"
// common includes
#include <make_ptr.h>
// library includes
#include <debug/log.h>
#include <platform/shared_library_adapter.h>

namespace Sound::Mp3
{
  class LibraryName : public Platform::SharedLibrary::Name
  {
  public:
    LibraryName() = default;

    StringView Base() const override
    {
      return "mp3lame"sv;
    }

    std::vector<StringView> PosixAlternatives() const override
    {
      return {"libmp3lame.so.0"sv};
    }

    std::vector<StringView> WindowsAlternatives() const override
    {
      return {"libmp3lame.dll"sv, "libmp3lame-0.dll"sv};
    }
  };

  class DynamicApi : public Api
  {
  public:
    explicit DynamicApi(Platform::SharedLibrary::Ptr lib)
      : Lib(std::move(lib))
    {
      Debug::Log("Sound::Backend::Mp3", "Library loaded");
    }

    ~DynamicApi() override
    {
      Debug::Log("Sound::Backend::Mp3", "Library unloaded");
    }

    // clang-format off

    const char* get_lame_version() override
    {
      using FunctionType = decltype(&::get_lame_version);
      const auto func = Lib.GetSymbol<FunctionType>("get_lame_version");
      return func();
    }

    lame_t lame_init() override
    {
      using FunctionType = decltype(&::lame_init);
      const auto func = Lib.GetSymbol<FunctionType>("lame_init");
      return func();
    }

    int lame_close(lame_t ctx) override
    {
      using FunctionType = decltype(&::lame_close);
      const auto func = Lib.GetSymbol<FunctionType>("lame_close");
      return func(ctx);
    }

    int lame_set_in_samplerate(lame_t ctx, int rate) override
    {
      using FunctionType = decltype(&::lame_set_in_samplerate);
      const auto func = Lib.GetSymbol<FunctionType>("lame_set_in_samplerate");
      return func(ctx, rate);
    }

    int lame_set_out_samplerate(lame_t ctx, int rate) override
    {
      using FunctionType = decltype(&::lame_set_out_samplerate);
      const auto func = Lib.GetSymbol<FunctionType>("lame_set_out_samplerate");
      return func(ctx, rate);
    }

    int lame_set_bWriteVbrTag(lame_t ctx, int flag) override
    {
      using FunctionType = decltype(&::lame_set_bWriteVbrTag);
      const auto func = Lib.GetSymbol<FunctionType>("lame_set_bWriteVbrTag");
      return func(ctx, flag);
    }

    int lame_set_mode(lame_t ctx, MPEG_mode mode) override
    {
      using FunctionType = decltype(&::lame_set_mode);
      const auto func = Lib.GetSymbol<FunctionType>("lame_set_mode");
      return func(ctx, mode);
    }

    int lame_set_num_channels(lame_t ctx, int chans) override
    {
      using FunctionType = decltype(&::lame_set_num_channels);
      const auto func = Lib.GetSymbol<FunctionType>("lame_set_num_channels");
      return func(ctx, chans);
    }

    int lame_set_brate(lame_t ctx, int brate) override
    {
      using FunctionType = decltype(&::lame_set_brate);
      const auto func = Lib.GetSymbol<FunctionType>("lame_set_brate");
      return func(ctx, brate);
    }

    int lame_set_VBR(lame_t ctx, vbr_mode mode) override
    {
      using FunctionType = decltype(&::lame_set_VBR);
      const auto func = Lib.GetSymbol<FunctionType>("lame_set_VBR");
      return func(ctx, mode);
    }

    int lame_set_VBR_q(lame_t ctx, int quality) override
    {
      using FunctionType = decltype(&::lame_set_VBR_q);
      const auto func = Lib.GetSymbol<FunctionType>("lame_set_VBR_q");
      return func(ctx, quality);
    }

    int lame_set_VBR_mean_bitrate_kbps(lame_t ctx, int brate) override
    {
      using FunctionType = decltype(&::lame_set_VBR_mean_bitrate_kbps);
      const auto func = Lib.GetSymbol<FunctionType>("lame_set_VBR_mean_bitrate_kbps");
      return func(ctx, brate);
    }

    int lame_init_params(lame_t ctx) override
    {
      using FunctionType = decltype(&::lame_init_params);
      const auto func = Lib.GetSymbol<FunctionType>("lame_init_params");
      return func(ctx);
    }

    int lame_encode_buffer_interleaved(lame_t ctx, short int* pcm, int samples, unsigned char* dst, int dstSize) override
    {
      using FunctionType = decltype(&::lame_encode_buffer_interleaved);
      const auto func = Lib.GetSymbol<FunctionType>("lame_encode_buffer_interleaved");
      return func(ctx, pcm, samples, dst, dstSize);
    }

    int lame_encode_flush(lame_t ctx, unsigned char* dst, int dstSize) override
    {
      using FunctionType = decltype(&::lame_encode_flush);
      const auto func = Lib.GetSymbol<FunctionType>("lame_encode_flush");
      return func(ctx, dst, dstSize);
    }

    void id3tag_init(lame_t ctx) override
    {
      using FunctionType = decltype(&::id3tag_init);
      const auto func = Lib.GetSymbol<FunctionType>("id3tag_init");
      return func(ctx);
    }

    void id3tag_add_v2(lame_t ctx) override
    {
      using FunctionType = decltype(&::id3tag_add_v2);
      const auto func = Lib.GetSymbol<FunctionType>("id3tag_add_v2");
      return func(ctx);
    }

    void id3tag_set_title(lame_t ctx, const char* title) override
    {
      using FunctionType = decltype(&::id3tag_set_title);
      const auto func = Lib.GetSymbol<FunctionType>("id3tag_set_title");
      return func(ctx, title);
    }

    void id3tag_set_artist(lame_t ctx, const char* artist) override
    {
      using FunctionType = decltype(&::id3tag_set_artist);
      const auto func = Lib.GetSymbol<FunctionType>("id3tag_set_artist");
      return func(ctx, artist);
    }

    void id3tag_set_comment(lame_t ctx, const char* comment) override
    {
      using FunctionType = decltype(&::id3tag_set_comment);
      const auto func = Lib.GetSymbol<FunctionType>("id3tag_set_comment");
      return func(ctx, comment);
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
}  // namespace Sound::Mp3
