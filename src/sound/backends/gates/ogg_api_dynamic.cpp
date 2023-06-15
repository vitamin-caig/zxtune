/**
 *
 * @file
 *
 * @brief  OGG subsystem API gate implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound/backends/gates/ogg_api.h"
// common includes
#include <make_ptr.h>
// library includes
#include <debug/log.h>
#include <platform/shared_library_adapter.h>

namespace Sound::Ogg
{
  class LibraryName : public Platform::SharedLibrary::Name
  {
  public:
    LibraryName() = default;

    StringView Base() const override
    {
      return "ogg"_sv;
    }

    std::vector<StringView> PosixAlternatives() const override
    {
      return {"libogg.so.0"_sv};
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
      Debug::Log("Sound::Backend::Ogg", "Library loaded");
    }

    ~DynamicApi() override
    {
      Debug::Log("Sound::Backend::Ogg", "Library unloaded");
    }

// clang-format off

    int ogg_stream_init(ogg_stream_state *os, int serialno) override
    {
      using FunctionType = decltype(&::ogg_stream_init);
      const auto func = Lib.GetSymbol<FunctionType>("ogg_stream_init");
      return func(os, serialno);
    }

    int ogg_stream_clear(ogg_stream_state *os) override
    {
      using FunctionType = decltype(&::ogg_stream_clear);
      const auto func = Lib.GetSymbol<FunctionType>("ogg_stream_clear");
      return func(os);
    }

    int ogg_stream_packetin(ogg_stream_state *os, ogg_packet *op) override
    {
      using FunctionType = decltype(&::ogg_stream_packetin);
      const auto func = Lib.GetSymbol<FunctionType>("ogg_stream_packetin");
      return func(os, op);
    }

    int ogg_stream_pageout(ogg_stream_state *os, ogg_page *og) override
    {
      using FunctionType = decltype(&::ogg_stream_pageout);
      const auto func = Lib.GetSymbol<FunctionType>("ogg_stream_pageout");
      return func(os, og);
    }

    int ogg_stream_flush(ogg_stream_state *os, ogg_page *og) override
    {
      using FunctionType = decltype(&::ogg_stream_flush);
      const auto func = Lib.GetSymbol<FunctionType>("ogg_stream_flush");
      return func(os, og);
    }

    int ogg_page_eos(const ogg_page *og) override
    {
      using FunctionType = decltype(&::ogg_page_eos);
      const auto func = Lib.GetSymbol<FunctionType>("ogg_page_eos");
      return func(og);
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
}  // namespace Sound::Ogg
