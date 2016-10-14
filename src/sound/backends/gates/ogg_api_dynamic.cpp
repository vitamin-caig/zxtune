/**
*
* @file
*
* @brief  OGG subsystem API gate implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "ogg_api.h"
//common includes
#include <make_ptr.h>
//library includes
#include <debug/log.h>
#include <platform/shared_library_adapter.h>

namespace Sound
{
  namespace Ogg
  {
    class LibraryName : public Platform::SharedLibrary::Name
    {
    public:
      LibraryName()
      {
      }

      std::string Base() const override
      {
        return "ogg";
      }
      
      std::vector<std::string> PosixAlternatives() const override
      {
        static const std::string ALTERNATIVES[] =
        {
          "libogg.so.0",
        };
        return std::vector<std::string>(ALTERNATIVES, std::end(ALTERNATIVES));
      }
      
      std::vector<std::string> WindowsAlternatives() const override
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

      ~DynamicApi() override
      {
        Dbg("Library unloaded");
      }

      
      int ogg_stream_init(ogg_stream_state *os, int serialno) override
      {
        static const char NAME[] = "ogg_stream_init";
        typedef int ( *FunctionType)(ogg_stream_state *, int);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(os, serialno);
      }
      
      int ogg_stream_clear(ogg_stream_state *os) override
      {
        static const char NAME[] = "ogg_stream_clear";
        typedef int ( *FunctionType)(ogg_stream_state *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(os);
      }
      
      int ogg_stream_packetin(ogg_stream_state *os, ogg_packet *op) override
      {
        static const char NAME[] = "ogg_stream_packetin";
        typedef int ( *FunctionType)(ogg_stream_state *, ogg_packet *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(os, op);
      }
      
      int ogg_stream_pageout(ogg_stream_state *os, ogg_page *og) override
      {
        static const char NAME[] = "ogg_stream_pageout";
        typedef int ( *FunctionType)(ogg_stream_state *, ogg_page *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(os, og);
      }
      
      int ogg_stream_flush(ogg_stream_state *os, ogg_page *og) override
      {
        static const char NAME[] = "ogg_stream_flush";
        typedef int ( *FunctionType)(ogg_stream_state *, ogg_page *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(os, og);
      }
      
      int ogg_page_eos(const ogg_page *og) override
      {
        static const char NAME[] = "ogg_page_eos";
        typedef int ( *FunctionType)(const ogg_page *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(og);
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
