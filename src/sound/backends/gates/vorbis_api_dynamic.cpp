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

namespace Sound
{
  namespace Vorbis
  {
    class LibraryName : public Platform::SharedLibrary::Name
    {
    public:
      LibraryName() {}

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

      
      int vorbis_block_clear(vorbis_block *vb) override
      {
        static const char NAME[] = "vorbis_block_clear";
        typedef int ( *FunctionType)(vorbis_block *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(vb);
      }
      
      int vorbis_block_init(vorbis_dsp_state *v, vorbis_block *vb) override
      {
        static const char NAME[] = "vorbis_block_init";
        typedef int ( *FunctionType)(vorbis_dsp_state *, vorbis_block *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(v, vb);
      }
      
      void vorbis_dsp_clear(vorbis_dsp_state *v) override
      {
        static const char NAME[] = "vorbis_dsp_clear";
        typedef void ( *FunctionType)(vorbis_dsp_state *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(v);
      }
      
      void vorbis_info_clear(vorbis_info *vi) override
      {
        static const char NAME[] = "vorbis_info_clear";
        typedef void ( *FunctionType)(vorbis_info *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(vi);
      }
      
      void vorbis_info_init(vorbis_info *vi) override
      {
        static const char NAME[] = "vorbis_info_init";
        typedef void ( *FunctionType)(vorbis_info *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(vi);
      }
      
      const char *vorbis_version_string(void) override
      {
        static const char NAME[] = "vorbis_version_string";
        typedef const char *( *FunctionType)();
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func();
      }
      
      int vorbis_analysis(vorbis_block *vb, ogg_packet *op) override
      {
        static const char NAME[] = "vorbis_analysis";
        typedef int ( *FunctionType)(vorbis_block *, ogg_packet *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(vb, op);
      }
      
      int vorbis_analysis_blockout(vorbis_dsp_state *v, vorbis_block *vb) override
      {
        static const char NAME[] = "vorbis_analysis_blockout";
        typedef int ( *FunctionType)(vorbis_dsp_state *, vorbis_block *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(v, vb);
      }
      
      float** vorbis_analysis_buffer(vorbis_dsp_state *v, int vals) override
      {
        static const char NAME[] = "vorbis_analysis_buffer";
        typedef float** ( *FunctionType)(vorbis_dsp_state *, int);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(v, vals);
      }
      
      int vorbis_analysis_headerout(vorbis_dsp_state *v, vorbis_comment *vc, ogg_packet *op, ogg_packet *op_comm, ogg_packet *op_code) override
      {
        static const char NAME[] = "vorbis_analysis_headerout";
        typedef int ( *FunctionType)(vorbis_dsp_state *, vorbis_comment *, ogg_packet *, ogg_packet *, ogg_packet *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(v, vc, op, op_comm, op_code);
      }
      
      int vorbis_analysis_init(vorbis_dsp_state *v, vorbis_info *vi) override
      {
        static const char NAME[] = "vorbis_analysis_init";
        typedef int ( *FunctionType)(vorbis_dsp_state *, vorbis_info *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(v, vi);
      }
      
      int vorbis_analysis_wrote(vorbis_dsp_state *v,int vals) override
      {
        static const char NAME[] = "vorbis_analysis_wrote";
        typedef int ( *FunctionType)(vorbis_dsp_state *, int);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(v, vals);
      }
      
      int vorbis_bitrate_addblock(vorbis_block *vb) override
      {
        static const char NAME[] = "vorbis_bitrate_addblock";
        typedef int ( *FunctionType)(vorbis_block *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(vb);
      }
      
      int vorbis_bitrate_flushpacket(vorbis_dsp_state *vd, ogg_packet *op) override
      {
        static const char NAME[] = "vorbis_bitrate_flushpacket";
        typedef int ( *FunctionType)(vorbis_dsp_state *, ogg_packet *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(vd, op);
      }
      
      void vorbis_comment_add_tag(vorbis_comment *vc, const char *tag, const char *contents) override
      {
        static const char NAME[] = "vorbis_comment_add_tag";
        typedef void ( *FunctionType)(vorbis_comment *, const char *, const char *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(vc, tag, contents);
      }
      
      void vorbis_comment_clear(vorbis_comment *vc) override
      {
        static const char NAME[] = "vorbis_comment_clear";
        typedef void ( *FunctionType)(vorbis_comment *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(vc);
      }
      
      void vorbis_comment_init(vorbis_comment *vc) override
      {
        static const char NAME[] = "vorbis_comment_init";
        typedef void ( *FunctionType)(vorbis_comment *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(vc);
      }
      
    private:
      const Platform::SharedLibraryAdapter Lib;
    };


    Api::Ptr LoadDynamicApi()
    {
      static const LibraryName NAME;
      auto lib = Platform::SharedLibrary::Load(NAME);
      return MakePtr<DynamicApi>(std::move(lib));
    }
  }  // namespace Vorbis
}  // namespace Sound
