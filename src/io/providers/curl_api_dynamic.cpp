/*
Abstract:
  Curl api dynamic gate implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "curl_api.h"
//common includes
#include <debug_log.h>
#include <tools.h>
//library includes
#include <platform/shared_library_adapter.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  using namespace IO::Curl;

  class CurlName : public Platform::SharedLibrary::Name
  {
  public:
    virtual std::string Base() const
    {
      return "curl";
    }
    
    virtual std::vector<std::string> PosixAlternatives() const
    {
      static const std::string ALTERNATIVES[] =
      {
        "libcurl.so.3",
        "libcurl.so.4",
      };
      return std::vector<std::string>(ALTERNATIVES, ArrayEnd(ALTERNATIVES));
    }
    
    virtual std::vector<std::string> WindowsAlternatives() const
    {
      static const std::string ALTERNATIVES[] =
      {
        "libcurl.dll",
      };
      return std::vector<std::string>(ALTERNATIVES, ArrayEnd(ALTERNATIVES));
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

    virtual ~DynamicApi()
    {
      Dbg("Library unloaded");
    }

    
    virtual char* curl_version()
    {
      static const char NAME[] = "curl_version";
      typedef char* ( *FunctionType)();
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func();
    }
    
    virtual CURL *curl_easy_init()
    {
      static const char NAME[] = "curl_easy_init";
      typedef CURL *( *FunctionType)();
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func();
    }
    
    virtual void curl_easy_cleanup(CURL *curl)
    {
      static const char NAME[] = "curl_easy_cleanup";
      typedef void ( *FunctionType)(CURL *);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(curl);
    }
    
    virtual CURLcode curl_easy_perform(CURL *curl)
    {
      static const char NAME[] = "curl_easy_perform";
      typedef CURLcode ( *FunctionType)(CURL *);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(curl);
    }
    
    virtual const char *curl_easy_strerror(CURLcode errornum)
    {
      static const char NAME[] = "curl_easy_strerror";
      typedef const char *( *FunctionType)(CURLcode);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(errornum);
    }
    
    virtual CURLcode curl_easy_setopt(CURL *curl, CURLoption option, int intParam)
    {
      static const char NAME[] = "curl_easy_setopt";
      typedef CURLcode ( *FunctionType)(CURL *, CURLoption, int);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(curl, option, intParam);
    }
    
    virtual CURLcode curl_easy_setopt(CURL *curl, CURLoption option, const char* strParam)
    {
      static const char NAME[] = "curl_easy_setopt";
      typedef CURLcode ( *FunctionType)(CURL *, CURLoption, const char*);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(curl, option, strParam);
    }
    
    virtual CURLcode curl_easy_setopt(CURL *curl, CURLoption option, void* opaqueParam)
    {
      static const char NAME[] = "curl_easy_setopt";
      typedef CURLcode ( *FunctionType)(CURL *, CURLoption, void*);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(curl, option, opaqueParam);
    }
    
    virtual CURLcode curl_easy_getinfo(CURL *curl, CURLINFO info, void* opaqueResult)
    {
      static const char NAME[] = "curl_easy_getinfo";
      typedef CURLcode ( *FunctionType)(CURL *, CURLINFO, void*);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(curl, info, opaqueResult);
    }
    
  private:
    const Platform::SharedLibraryAdapter Lib;
  };

}

namespace IO
{
  namespace Curl
  {
    Api::Ptr LoadDynamicApi()
    {
      static const CurlName NAME;
      const Platform::SharedLibrary::Ptr lib = Platform::SharedLibrary::Load(NAME);
      return boost::make_shared<DynamicApi>(lib);
    }
  }
}
