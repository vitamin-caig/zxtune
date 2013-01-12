/*
Abstract:
  VorbisEnc api dynamic gate implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "vorbisenc_api.h"
//common includes
#include <tools.h>
//library includes
#include <debug/log.h>
#include <platform/shared_library_adapter.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  using namespace ZXTune::Sound::VorbisEnc;

  class VorbisEncName : public Platform::SharedLibrary::Name
  {
  public:
    virtual std::string Base() const
    {
      return "vorbisenc";
    }
    
    virtual std::vector<std::string> PosixAlternatives() const
    {
      static const std::string ALTERNATIVES[] =
      {
        "libvorbisenc.so.2",
      };
      return std::vector<std::string>(ALTERNATIVES, ArrayEnd(ALTERNATIVES));
    }
    
    virtual std::vector<std::string> WindowsAlternatives() const
    {
      return std::vector<std::string>();
    }
  };

  const Debug::Stream Dbg("Sound::Backend::Ogg");

  class DynamicApi : public Api
  {
  public:
    explicit DynamicApi(Platform::SharedLibrary::Ptr lib)
      : Lib(lib)
    {
      Dbg("Library loaded");
    }

    virtual ~DynamicApi()
    {
      Dbg("Library unloaded");
    }

    
    virtual int vorbis_encode_init(vorbis_info *vi, long channels, long rate, long max_bitrate, long nominal_bitrate, long min_bitrate)
    {
      static const char NAME[] = "vorbis_encode_init";
      typedef int ( *FunctionType)(vorbis_info *, long, long, long, long, long);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(vi, channels, rate, max_bitrate, nominal_bitrate, min_bitrate);
    }
    
    virtual int vorbis_encode_init_vbr(vorbis_info *vi, long channels, long rate, float base_quality)
    {
      static const char NAME[] = "vorbis_encode_init_vbr";
      typedef int ( *FunctionType)(vorbis_info *, long, long, float);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(vi, channels, rate, base_quality);
    }
    
  private:
    const Platform::SharedLibraryAdapter Lib;
  };

}

namespace ZXTune
{
  namespace Sound
  {
    namespace VorbisEnc
    {
      Api::Ptr LoadDynamicApi()
      {
        static const VorbisEncName NAME;
        const Platform::SharedLibrary::Ptr lib = Platform::SharedLibrary::Load(NAME);
        return boost::make_shared<DynamicApi>(lib);
      }
    }
  }
}
