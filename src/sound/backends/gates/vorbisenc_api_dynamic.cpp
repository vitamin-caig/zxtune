/**
*
* @file
*
* @brief  VorbisEnc subsystem API gate implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "sound/backends/gates/vorbisenc_api.h"
//common includes
#include <make_ptr.h>
//library includes
#include <debug/log.h>
#include <platform/shared_library_adapter.h>

namespace Sound
{
  namespace VorbisEnc
  {
    class LibraryName : public Platform::SharedLibrary::Name
    {
    public:
      LibraryName()
      {
      }

      std::string Base() const override
      {
        return "vorbisenc";
      }
      
      std::vector<std::string> PosixAlternatives() const override
      {
        static const std::string ALTERNATIVES[] =
        {
          "libvorbisenc.so.2",
        };
        return std::vector<std::string>(ALTERNATIVES, std::end(ALTERNATIVES));
      }
      
      std::vector<std::string> WindowsAlternatives() const override
      {
        return std::vector<std::string>();
      }
    };


    class DynamicApi : public Api
    {
    public:
      explicit DynamicApi(Platform::SharedLibrary::Ptr lib)
        : Lib(lib)
      {
        Debug::Log("Sound::Backend::Ogg", "Library loaded");
      }

      ~DynamicApi() override
      {
        Debug::Log("Sound::Backend::Ogg", "Library unloaded");
      }

      
      int vorbis_encode_init(vorbis_info *vi, long channels, long rate, long max_bitrate, long nominal_bitrate, long min_bitrate) override
      {
        static const char NAME[] = "vorbis_encode_init";
        typedef int ( *FunctionType)(vorbis_info *, long, long, long, long, long);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(vi, channels, rate, max_bitrate, nominal_bitrate, min_bitrate);
      }
      
      int vorbis_encode_init_vbr(vorbis_info *vi, long channels, long rate, float base_quality) override
      {
        static const char NAME[] = "vorbis_encode_init_vbr";
        typedef int ( *FunctionType)(vorbis_info *, long, long, float);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(vi, channels, rate, base_quality);
      }
      
    private:
      const Platform::SharedLibraryAdapter Lib;
    };


    Api::Ptr LoadDynamicApi()
    {
      static const LibraryName NAME;
      const Platform::SharedLibrary::Ptr lib = Platform::SharedLibrary::Load(NAME);
      return MakePtr<DynamicApi>(lib);
    }
  }
}
