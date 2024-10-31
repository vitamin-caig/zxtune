/**
 *
 * @file
 *
 * @brief  Curl api dynamic gate implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "io/providers/gates/curl_api.h"

#include "debug/log.h"
#include "platform/shared_library_adapter.h"

#include "make_ptr.h"
#include "string_view.h"

namespace IO::Curl
{
  class LibraryName : public Platform::SharedLibrary::Name
  {
  public:
    LibraryName() = default;

    StringView Base() const override
    {
      return "curl"sv;
    }

    std::vector<StringView> PosixAlternatives() const override
    {
      return {"libcurl.so.3"sv, "libcurl.so.4"sv};
    }

    std::vector<StringView> WindowsAlternatives() const override
    {
      return {"libcurl.dll"sv};
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

    // clang-format off

    char* curl_version() override
    {
      using FunctionType = decltype(&::curl_version);
      const auto func = Lib.GetSymbol<FunctionType>("curl_version");
      return func();
    }

    CURL *curl_easy_init() override
    {
      using FunctionType = decltype(&::curl_easy_init);
      const auto func = Lib.GetSymbol<FunctionType>("curl_easy_init");
      return func();
    }

    void curl_easy_cleanup(CURL *curl) override
    {
      using FunctionType = decltype(&::curl_easy_cleanup);
      const auto func = Lib.GetSymbol<FunctionType>("curl_easy_cleanup");
      return func(curl);
    }

    CURLcode curl_easy_perform(CURL *curl) override
    {
      using FunctionType = decltype(&::curl_easy_perform);
      const auto func = Lib.GetSymbol<FunctionType>("curl_easy_perform");
      return func(curl);
    }

    const char *curl_easy_strerror(CURLcode errornum) override
    {
      using FunctionType = decltype(&::curl_easy_strerror);
      const auto func = Lib.GetSymbol<FunctionType>("curl_easy_strerror");
      return func(errornum);
    }

    CURLcode curl_easy_setopt(CURL *curl, CURLoption option, int intParam) override
    {
      using FunctionType = decltype(&::curl_easy_setopt);
      const auto func = Lib.GetSymbol<FunctionType>("curl_easy_setopt");
      return func(curl, option, intParam);
    }

    CURLcode curl_easy_setopt(CURL *curl, CURLoption option, const char* strParam) override
    {
      using FunctionType = decltype(&::curl_easy_setopt);
      const auto func = Lib.GetSymbol<FunctionType>("curl_easy_setopt");
      return func(curl, option, strParam);
    }

    CURLcode curl_easy_setopt(CURL *curl, CURLoption option, void* opaqueParam) override
    {
      using FunctionType = decltype(&::curl_easy_setopt);
      const auto func = Lib.GetSymbol<FunctionType>("curl_easy_setopt");
      return func(curl, option, opaqueParam);
    }

    CURLcode curl_easy_getinfo(CURL *curl, CURLINFO info, void* opaqueResult) override
    {
      using FunctionType = decltype(&::curl_easy_getinfo);
      const auto func = Lib.GetSymbol<FunctionType>("curl_easy_getinfo");
      return func(curl, info, opaqueResult);
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
}  // namespace IO::Curl
