/*
Abstract:
  Ogg api dynamic gate implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ogg_api.h"
//common includes
#include <debug_log.h>
#include <tools.h>
//library includes
#include <platform/shared_library_adapter.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  using namespace ZXTune::Sound::Ogg;

  class OggName : public Platform::SharedLibrary::Name
  {
  public:
    virtual std::string Base() const
    {
      return "ogg";
    }
    
    virtual std::vector<std::string> PosixAlternatives() const
    {
      static const std::string ALTERNATIVES[] =
      {
        "libogg.so.0",
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

    
    virtual int ogg_stream_init(ogg_stream_state *os, int serialno)
    {
      static const char NAME[] = "ogg_stream_init";
      typedef int ( *FunctionType)(ogg_stream_state *, int);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(os, serialno);
    }
    
    virtual int ogg_stream_clear(ogg_stream_state *os)
    {
      static const char NAME[] = "ogg_stream_clear";
      typedef int ( *FunctionType)(ogg_stream_state *);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(os);
    }
    
    virtual int ogg_stream_packetin(ogg_stream_state *os, ogg_packet *op)
    {
      static const char NAME[] = "ogg_stream_packetin";
      typedef int ( *FunctionType)(ogg_stream_state *, ogg_packet *);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(os, op);
    }
    
    virtual int ogg_stream_pageout(ogg_stream_state *os, ogg_page *og)
    {
      static const char NAME[] = "ogg_stream_pageout";
      typedef int ( *FunctionType)(ogg_stream_state *, ogg_page *);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(os, og);
    }
    
    virtual int ogg_stream_flush(ogg_stream_state *os, ogg_page *og)
    {
      static const char NAME[] = "ogg_stream_flush";
      typedef int ( *FunctionType)(ogg_stream_state *, ogg_page *);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(os, og);
    }
    
    virtual int ogg_page_eos(const ogg_page *og)
    {
      static const char NAME[] = "ogg_page_eos";
      typedef int ( *FunctionType)(const ogg_page *);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(og);
    }
    
  private:
    const Platform::SharedLibraryAdapter Lib;
  };

}

namespace ZXTune
{
  namespace Sound
  {
    namespace Ogg
    {
      Api::Ptr LoadDynamicApi()
      {
        static const OggName NAME;
        const Platform::SharedLibrary::Ptr lib = Platform::SharedLibrary::Load(NAME);
        return boost::make_shared<DynamicApi>(lib);
      }
    }
  }
}
