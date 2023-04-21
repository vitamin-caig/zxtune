/**
 *
 * @file
 *
 * @brief  Curl api dynamic gate implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "io/providers/gates/curl_api.h"
// common includes
#include <make_ptr.h>
// library includes
#include <debug/log.h>
#include <platform/shared_library_adapter.h>

namespace IO
{
  namespace Curl
  {
    class LibraryName : public Platform::SharedLibrary::Name
    {
    public:
      LibraryName() {}

      StringView Base() const override
      {
        return "curl"_sv;
      }

      std::vector<StringView> PosixAlternatives() const override
      {
        return {"libcurl.so.3"_sv, "libcurl.so.4"_sv};
      }

      std::vector<StringView> WindowsAlternatives() const override
      {
        return {"libcurl.dll"_sv};
      }
    };


    class DynamicApi : public Api
    {
    public:
      explicit DynamicApi(Platform::SharedLibrary::Ptr lib)
        : Lib(std::move(lib))
      {
        Debug::Log("IO::Provider::Network", "Library loaded");
      }

      ~DynamicApi() override
      {
        Debug::Log("IO::Provider::Network", "Library unloaded");
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
      auto lib = Platform::SharedLibrary::Load(NAME);
      return MakePtr<DynamicApi>(std::move(lib));
    }
  }  // namespace Curl
}  // namespace IO
