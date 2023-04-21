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

namespace Sound
{
  namespace Mp3
  {
    class LibraryName : public Platform::SharedLibrary::Name
    {
    public:
      LibraryName() {}

      StringView Base() const override
      {
        return "mp3lame"_sv;
      }

      std::vector<StringView> PosixAlternatives() const override
      {
        return {"libmp3lame.so.0"_sv};
      }

      std::vector<StringView> WindowsAlternatives() const override
      {
        return {"libmp3lame.dll"_sv};
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

      
      const char* get_lame_version() override
      {
        static const char NAME[] = "get_lame_version";
        typedef const char* ( *FunctionType)();
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func();
      }
      
      lame_t lame_init() override
      {
        static const char NAME[] = "lame_init";
        typedef lame_t ( *FunctionType)();
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func();
      }
      
      int lame_close(lame_t ctx) override
      {
        static const char NAME[] = "lame_close";
        typedef int ( *FunctionType)(lame_t);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(ctx);
      }
      
      int lame_set_in_samplerate(lame_t ctx, int rate) override
      {
        static const char NAME[] = "lame_set_in_samplerate";
        typedef int ( *FunctionType)(lame_t, int);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(ctx, rate);
      }
      
      int lame_set_out_samplerate(lame_t ctx, int rate) override
      {
        static const char NAME[] = "lame_set_out_samplerate";
        typedef int ( *FunctionType)(lame_t, int);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(ctx, rate);
      }
      
      int lame_set_bWriteVbrTag(lame_t ctx, int flag) override
      {
        static const char NAME[] = "lame_set_bWriteVbrTag";
        typedef int ( *FunctionType)(lame_t, int);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(ctx, flag);
      }
      
      int lame_set_mode(lame_t ctx, MPEG_mode mode) override
      {
        static const char NAME[] = "lame_set_mode";
        typedef int ( *FunctionType)(lame_t, MPEG_mode);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(ctx, mode);
      }
      
      int lame_set_num_channels(lame_t ctx, int chans) override
      {
        static const char NAME[] = "lame_set_num_channels";
        typedef int ( *FunctionType)(lame_t, int);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(ctx, chans);
      }
      
      int lame_set_brate(lame_t ctx, int brate) override
      {
        static const char NAME[] = "lame_set_brate";
        typedef int ( *FunctionType)(lame_t, int);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(ctx, brate);
      }
      
      int lame_set_VBR(lame_t ctx, vbr_mode mode) override
      {
        static const char NAME[] = "lame_set_VBR";
        typedef int ( *FunctionType)(lame_t, vbr_mode);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(ctx, mode);
      }
      
      int lame_set_VBR_q(lame_t ctx, int quality) override
      {
        static const char NAME[] = "lame_set_VBR_q";
        typedef int ( *FunctionType)(lame_t, int);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(ctx, quality);
      }
      
      int lame_set_VBR_mean_bitrate_kbps(lame_t ctx, int brate) override
      {
        static const char NAME[] = "lame_set_VBR_mean_bitrate_kbps";
        typedef int ( *FunctionType)(lame_t, int);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(ctx, brate);
      }
      
      int lame_init_params(lame_t ctx) override
      {
        static const char NAME[] = "lame_init_params";
        typedef int ( *FunctionType)(lame_t);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(ctx);
      }
      
      int lame_encode_buffer_interleaved(lame_t ctx, short int* pcm, int samples, unsigned char* dst, int dstSize) override
      {
        static const char NAME[] = "lame_encode_buffer_interleaved";
        typedef int ( *FunctionType)(lame_t, short int*, int, unsigned char*, int);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(ctx, pcm, samples, dst, dstSize);
      }
      
      int lame_encode_flush(lame_t ctx, unsigned char* dst, int dstSize) override
      {
        static const char NAME[] = "lame_encode_flush";
        typedef int ( *FunctionType)(lame_t, unsigned char*, int);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(ctx, dst, dstSize);
      }
      
      void id3tag_init(lame_t ctx) override
      {
        static const char NAME[] = "id3tag_init";
        typedef void ( *FunctionType)(lame_t);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(ctx);
      }
      
      void id3tag_add_v2(lame_t ctx) override
      {
        static const char NAME[] = "id3tag_add_v2";
        typedef void ( *FunctionType)(lame_t);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(ctx);
      }
      
      void id3tag_set_title(lame_t ctx, const char* title) override
      {
        static const char NAME[] = "id3tag_set_title";
        typedef void ( *FunctionType)(lame_t, const char*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(ctx, title);
      }
      
      void id3tag_set_artist(lame_t ctx, const char* artist) override
      {
        static const char NAME[] = "id3tag_set_artist";
        typedef void ( *FunctionType)(lame_t, const char*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(ctx, artist);
      }
      
      void id3tag_set_comment(lame_t ctx, const char* comment) override
      {
        static const char NAME[] = "id3tag_set_comment";
        typedef void ( *FunctionType)(lame_t, const char*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(ctx, comment);
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
  }  // namespace Mp3
}  // namespace Sound
