/**
*
* @file
*
* @brief  Curl api dynamic gate implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "curl_api.h"
//common includes
#include <make_ptr.h>
//library includes
#include <debug/log.h>
#include <platform/shared_library_adapter.h>

namespace IO
{
  namespace Curl
  {
    class LibraryName : public Platform::SharedLibrary::Name
    {
    public:
      LibraryName()
      {
      }

      std::string Base() const override
      {
        return "curl";
      }
      
      std::vector<std::string> PosixAlternatives() const override
      {
        static const std::string ALTERNATIVES[] =
        {
          "libcurl.so.3",
          "libcurl.so.4",
        };
        return std::vector<std::string>(ALTERNATIVES, std::end(ALTERNATIVES));
      }
      
      std::vector<std::string> WindowsAlternatives() const override
      {
        static const std::string ALTERNATIVES[] =
        {
          "libcurl.dll",
        };
        return std::vector<std::string>(ALTERNATIVES, std::end(ALTERNATIVES));
      }
    };

    const Debug::Stream Dbg("IO::Provider::Network");

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

      
      char* curl_version() override
      {
        static const char NAME[] = "curl_version";
        typedef char* ( *FunctionType)();
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func();
      }
      
      CURL *curl_easy_init() override
      {
        static const char NAME[] = "curl_easy_init";
        typedef CURL *( *FunctionType)();
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func();
      }
      
      void curl_easy_cleanup(CURL *curl) override
      {
        static const char NAME[] = "curl_easy_cleanup";
        typedef void ( *FunctionType)(CURL *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(curl);
      }
      
      CURLcode curl_easy_perform(CURL *curl) override
      {
        static const char NAME[] = "curl_easy_perform";
        typedef CURLcode ( *FunctionType)(CURL *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(curl);
      }
      
      const char *curl_easy_strerror(CURLcode errornum) override
      {
        static const char NAME[] = "curl_easy_strerror";
        typedef const char *( *FunctionType)(CURLcode);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(errornum);
      }
      
      CURLcode curl_easy_setopt(CURL *curl, CURLoption option, int intParam) override
      {
        static const char NAME[] = "curl_easy_setopt";
        typedef CURLcode ( *FunctionType)(CURL *, CURLoption, int);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(curl, option, intParam);
      }
      
      CURLcode curl_easy_setopt(CURL *curl, CURLoption option, const char* strParam) override
      {
        static const char NAME[] = "curl_easy_setopt";
        typedef CURLcode ( *FunctionType)(CURL *, CURLoption, const char*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(curl, option, strParam);
      }
      
      CURLcode curl_easy_setopt(CURL *curl, CURLoption option, void* opaqueParam) override
      {
        static const char NAME[] = "curl_easy_setopt";
        typedef CURLcode ( *FunctionType)(CURL *, CURLoption, void*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(curl, option, opaqueParam);
      }
      
      CURLcode curl_easy_getinfo(CURL *curl, CURLINFO info, void* opaqueResult) override
      {
        static const char NAME[] = "curl_easy_getinfo";
        typedef CURLcode ( *FunctionType)(CURL *, CURLINFO, void*);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(curl, info, opaqueResult);
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
