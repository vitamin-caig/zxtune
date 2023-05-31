/**
 *
 * @file
 *
 * @brief  Vorbis subsystem API gate implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound/backends/gates/vorbis_api.h"
// common includes
#include <make_ptr.h>
// library includes
#include <debug/log.h>
#include <platform/shared_library_adapter.h>

namespace Sound::Vorbis
{
  class LibraryName : public Platform::SharedLibrary::Name
  {
  public:
    LibraryName() = default;

    StringView Base() const override
    {
      return "vorbis"_sv;
    }

    std::vector<StringView> PosixAlternatives() const override
    {
      return {"libvorbis.so.0"_sv};
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

    int vorbis_block_clear(vorbis_block *vb) override
    {
      using FunctionType = decltype(&::vorbis_block_clear);
      const auto func = Lib.GetSymbol<FunctionType>("vorbis_block_clear");
      return func(vb);
    }

    int vorbis_block_init(vorbis_dsp_state *v, vorbis_block *vb) override
    {
      using FunctionType = decltype(&::vorbis_block_init);
      const auto func = Lib.GetSymbol<FunctionType>("vorbis_block_init");
      return func(v, vb);
    }

    void vorbis_dsp_clear(vorbis_dsp_state *v) override
    {
      using FunctionType = decltype(&::vorbis_dsp_clear);
      const auto func = Lib.GetSymbol<FunctionType>("vorbis_dsp_clear");
      return func(v);
    }

    void vorbis_info_clear(vorbis_info *vi) override
    {
      using FunctionType = decltype(&::vorbis_info_clear);
      const auto func = Lib.GetSymbol<FunctionType>("vorbis_info_clear");
      return func(vi);
    }

    void vorbis_info_init(vorbis_info *vi) override
    {
      using FunctionType = decltype(&::vorbis_info_init);
      const auto func = Lib.GetSymbol<FunctionType>("vorbis_info_init");
      return func(vi);
    }

    const char *vorbis_version_string() override
    {
      using FunctionType = decltype(&::vorbis_version_string);
      const auto func = Lib.GetSymbol<FunctionType>("vorbis_version_string");
      return func();
    }

    int vorbis_analysis(vorbis_block *vb, ogg_packet *op) override
    {
      using FunctionType = decltype(&::vorbis_analysis);
      const auto func = Lib.GetSymbol<FunctionType>("vorbis_analysis");
      return func(vb, op);
    }

    int vorbis_analysis_blockout(vorbis_dsp_state *v, vorbis_block *vb) override
    {
      using FunctionType = decltype(&::vorbis_analysis_blockout);
      const auto func = Lib.GetSymbol<FunctionType>("vorbis_analysis_blockout");
      return func(v, vb);
    }

    float** vorbis_analysis_buffer(vorbis_dsp_state *v, int vals) override
    {
      using FunctionType = decltype(&::vorbis_analysis_buffer);
      const auto func = Lib.GetSymbol<FunctionType>("vorbis_analysis_buffer");
      return func(v, vals);
    }

    int vorbis_analysis_headerout(vorbis_dsp_state *v, vorbis_comment *vc, ogg_packet *op, ogg_packet *op_comm, ogg_packet *op_code) override
    {
      using FunctionType = decltype(&::vorbis_analysis_headerout);
      const auto func = Lib.GetSymbol<FunctionType>("vorbis_analysis_headerout");
      return func(v, vc, op, op_comm, op_code);
    }

    int vorbis_analysis_init(vorbis_dsp_state *v, vorbis_info *vi) override
    {
      using FunctionType = decltype(&::vorbis_analysis_init);
      const auto func = Lib.GetSymbol<FunctionType>("vorbis_analysis_init");
      return func(v, vi);
    }

    int vorbis_analysis_wrote(vorbis_dsp_state *v,int vals) override
    {
      using FunctionType = decltype(&::vorbis_analysis_wrote);
      const auto func = Lib.GetSymbol<FunctionType>("vorbis_analysis_wrote");
      return func(v, vals);
    }

    int vorbis_bitrate_addblock(vorbis_block *vb) override
    {
      using FunctionType = decltype(&::vorbis_bitrate_addblock);
      const auto func = Lib.GetSymbol<FunctionType>("vorbis_bitrate_addblock");
      return func(vb);
    }

    int vorbis_bitrate_flushpacket(vorbis_dsp_state *vd, ogg_packet *op) override
    {
      using FunctionType = decltype(&::vorbis_bitrate_flushpacket);
      const auto func = Lib.GetSymbol<FunctionType>("vorbis_bitrate_flushpacket");
      return func(vd, op);
    }

    void vorbis_comment_add_tag(vorbis_comment *vc, const char *tag, const char *contents) override
    {
      using FunctionType = decltype(&::vorbis_comment_add_tag);
      const auto func = Lib.GetSymbol<FunctionType>("vorbis_comment_add_tag");
      return func(vc, tag, contents);
    }

    void vorbis_comment_clear(vorbis_comment *vc) override
    {
      using FunctionType = decltype(&::vorbis_comment_clear);
      const auto func = Lib.GetSymbol<FunctionType>("vorbis_comment_clear");
      return func(vc);
    }

    void vorbis_comment_init(vorbis_comment *vc) override
    {
      using FunctionType = decltype(&::vorbis_comment_init);
      const auto func = Lib.GetSymbol<FunctionType>("vorbis_comment_init");
      return func(vc);
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
}  // namespace Sound::Vorbis
