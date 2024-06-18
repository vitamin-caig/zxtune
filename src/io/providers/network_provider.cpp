/**
 *
 * @file
 *
 * @brief  Network provider implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "io/providers/network_provider.h"
#include "io/impl/l10n.h"
#include "io/providers/enumerator.h"
#include "io/providers/gates/curl_api.h"
// common includes
#include <contract.h>
#include <error_tools.h>
#include <make_ptr.h>
#include <progress_callback.h>
// library includes
#include <binary/container_factories.h>
#include <binary/data_builder.h>
#include <debug/log.h>
#include <io/providers_parameters.h>
#include <parameters/accessor.h>
// std includes
#include <cstring>
#include <utility>

namespace IO::Network
{
  const Debug::Stream Dbg("IO::Provider::Network");

  class ProviderParameters
  {
  public:
    explicit ProviderParameters(const Parameters::Accessor& accessor)
      : Accessor(accessor)
    {}

    String GetHttpUseragent() const
    {
      return Parameters::GetString(Accessor, Parameters::ZXTune::IO::Providers::Network::Http::USERAGENT);
    }

  private:
    const Parameters::Accessor& Accessor;
  };

  const std::size_t INITIAL_SIZE = 1048576;

  class CurlObject
  {
  public:
    explicit CurlObject(Curl::Api::Ptr api)
      : Api(std::move(api))
      , Object(Api->curl_easy_init())
    {
      Dbg("Curl({}): created", Object);
    }

    ~CurlObject()
    {
      if (Object)
      {
        Api->curl_easy_cleanup(Object);
        Dbg("Curl({}): destroyed", Object);
      }
    }

    template<class P>
    void SetOption(CURLoption opt, P param, Error::LocationRef loc)
    {
      CheckCurlResult(Api->curl_easy_setopt(Object, opt, param), loc);
    }

    void Perform(Error::LocationRef loc)
    {
      CheckCurlResult(Api->curl_easy_perform(Object), loc);
    }

    template<class P>
    void GetInfo(CURLINFO info, P param, Error::LocationRef loc)
    {
      CheckCurlResult(Api->curl_easy_getinfo(Object, info, param), loc);
    }

  private:
    void CheckCurlResult(CURLcode code, Error::LocationRef loc) const
    {
      if (code != CURLE_OK)
      {
        throw MakeFormattedError(loc, translate("Network error happends: {}"), Api->curl_easy_strerror(code));
      }
    }

  private:
    const Curl::Api::Ptr Api;
    CURL* Object;
  };

  bool IsHttpErrorCode(long code)
  {
    return 2 != (code / 100);
  }

  class RemoteResource
  {
  public:
    explicit RemoteResource(Curl::Api::Ptr api)
      : Object(std::move(api))
    {
      Object.SetOption(CURLOPT_DEBUGFUNCTION, reinterpret_cast<void*>(&DebugCallback), THIS_LINE);
      Object.SetOption(CURLOPT_VERBOSE, 1, THIS_LINE);
      Object.SetOption(CURLOPT_WRITEFUNCTION, reinterpret_cast<void*>(&WriteCallback), THIS_LINE);
    }

    void SetSource(StringView url)
    {
      Object.SetOption(CURLOPT_URL, String{url}.c_str(), THIS_LINE);
    }

    void SetOptions(const ProviderParameters& params)
    {
      const String useragent = params.GetHttpUseragent();
      if (!useragent.empty())
      {
        Object.SetOption(CURLOPT_USERAGENT, useragent.c_str(), THIS_LINE);
      }
      Object.SetOption(CURLOPT_FOLLOWLOCATION, 1, THIS_LINE);
    }

    void SetProgressCallback(Log::ProgressCallback& cb)
    {
      Object.SetOption(CURLOPT_PROGRESSFUNCTION, reinterpret_cast<void*>(&ProgressCallback), THIS_LINE);
      Object.SetOption(CURLOPT_PROGRESSDATA, static_cast<void*>(&cb), THIS_LINE);
      Object.SetOption(CURLOPT_NOPROGRESS, 0, THIS_LINE);
    }

    // TODO: pass callback to handle progress and other
    Binary::Container::Ptr Download()
    {
      Binary::DataBuilder result(INITIAL_SIZE);
      Object.SetOption<void*>(CURLOPT_WRITEDATA, &result, THIS_LINE);
      Object.Perform(THIS_LINE);
      long retCode = 0;
      Object.GetInfo(CURLINFO_RESPONSE_CODE, &retCode, THIS_LINE);
      if (IsHttpErrorCode(retCode))
      {
        throw MakeFormattedError(THIS_LINE, translate("Http error happends: {}."), retCode);
      }
      return result.CaptureResult();
    }

  private:
    static int DebugCallback(CURL* obj, curl_infotype type, char* data, size_t size, void* /*param*/)
    {
      static const char SPACES[] = "\n\r\t ";
      String str(data, data + size);
      const auto lastSym = str.find_last_not_of(SPACES);
      if (lastSym == str.npos)
      {
        return 0;
      }
      str.resize(lastSym + 1);
      switch (type)
      {
      case CURLINFO_TEXT:
        Dbg("Curl({}): {}", obj, str);
        break;
      case CURLINFO_HEADER_IN:
        Dbg("Curl({}): -> {}", obj, str);
        break;
      case CURLINFO_HEADER_OUT:
        Dbg("Curl({}): <- {}", obj, str);
        break;
      default:
        break;
      }
      return 0;
    }

    static size_t WriteCallback(const char* ptr, size_t size, size_t nmemb, Binary::DataBuilder* result)
    {
      const std::size_t toSave = size * nmemb;
      result->Add(Binary::View{ptr, toSave});
      return toSave;
    }

    static int ProgressCallback(void* data, double dlTotal, double dlNow, double /*ulTotal*/, double /*ulNow*/)
    {
      if (dlTotal != 0.0)  // 0 for source files with unknown size
      {
        auto* const cb = static_cast<Log::ProgressCallback*>(data);
        const auto progress = static_cast<uint_t>(dlNow * 100 / dlTotal);
        cb->OnProgress(progress);
      }
      return 0;
    }

  private:
    CurlObject Object;
  };

  // uri-related constants
  const auto SCHEME_SIGN = "://"_sv;
  const Char SCHEME_HTTP[] = {'h', 't', 't', 'p', 0};
  const Char SCHEME_HTTPS[] = {'h', 't', 't', 'p', 's', 0};
  const Char SCHEME_FTP[] = {'f', 't', 'p', 0};
  const Char SUBPATH_DELIMITER = '#';

  const Char* ALL_SCHEMES[] = {
      SCHEME_HTTP,
      SCHEME_HTTPS,
      SCHEME_FTP,
  };

  class RemoteIdentifier : public Identifier
  {
  public:
    RemoteIdentifier(StringView scheme, StringView path, StringView subpath)
      : SchemeValue(scheme)
      , PathValue(path)
      , SubpathValue(subpath)
      , FullValue(Serialize())
    {
      Require(!SchemeValue.empty() && !PathValue.empty());
    }

    String Full() const override
    {
      return FullValue;
    }

    String Scheme() const override
    {
      return SchemeValue;
    }

    String Path() const override
    {
      return PathValue;
    }

    String Filename() const override
    {
      // filename usually is useless on remote schemes
      return {};
    }

    String Extension() const override
    {
      // filename usually is useless on remote schemes
      return {};
    }

    String Subpath() const override
    {
      return SubpathValue;
    }

    Ptr WithSubpath(StringView subpath) const override
    {
      return MakePtr<RemoteIdentifier>(SchemeValue, PathValue, subpath);
    }

  private:
    String Serialize() const
    {
      // do not place scheme
      String res = PathValue;
      if (!SubpathValue.empty())
      {
        res += SUBPATH_DELIMITER;
        res += SubpathValue;
      }
      return res;
    }

  private:
    const String SchemeValue;
    const String PathValue;
    const String SubpathValue;
    const String FullValue;
  };

  class DataProvider : public IO::DataProvider
  {
  public:
    explicit DataProvider(Curl::Api::Ptr api)
      : Api(std::move(api))
      , SupportedSchemes(ALL_SCHEMES, std::end(ALL_SCHEMES))
    {}

    String Id() const override
    {
      return PROVIDER_IDENTIFIER;
    }

    String Description() const override
    {
      return translate(PROVIDER_DESCRIPTION);
    }

    Error Status() const override
    {
      return {};
    }

    Strings::Set Schemes() const override
    {
      return SupportedSchemes;
    }

    Identifier::Ptr Resolve(StringView uri) const override
    {
      const auto schemePos = uri.find(SCHEME_SIGN);
      if (uri.npos == schemePos)
      {
        // scheme is required
        return {};
      }
      const auto hierPos = schemePos + SCHEME_SIGN.size();
      const auto subPos = uri.find_first_of(SUBPATH_DELIMITER, hierPos);

      const auto scheme = uri.substr(0, schemePos);
      const auto hier = uri.npos == subPos ? uri.substr(hierPos) : uri.substr(hierPos, subPos - hierPos);
      if (hier.empty() || !SupportedSchemes.count(scheme))  // TODO
      {
        // scheme and hierarchy part is mandatory
        return {};
      }
      // Path should include scheme and all possible parameters
      const auto path = uri.npos == subPos ? uri : uri.substr(0, subPos);
      const auto subpath = uri.npos == subPos ? StringView() : uri.substr(subPos + 1);
      return MakePtr<RemoteIdentifier>(scheme, path, subpath);
    }

    Binary::Container::Ptr Open(StringView path, const Parameters::Accessor& params,
                                Log::ProgressCallback& cb) const override
    {
      try
      {
        const ProviderParameters options(params);
        RemoteResource resource(Api);
        resource.SetSource(path);
        resource.SetOptions(options);
        resource.SetProgressCallback(cb);
        return resource.Download();
      }
      catch (const Error& e)
      {
        throw MakeFormattedError(THIS_LINE, translate("Failed to open network resource '{}'."), path).AddSuberror(e);
      }
    }

    Binary::OutputStream::Ptr Create(StringView /*path*/, const Parameters::Accessor& /*params*/,
                                     Log::ProgressCallback& /*cb*/) const override
    {
      throw Error(THIS_LINE, translate("Not supported."));
    }

  private:
    const Curl::Api::Ptr Api;
    const Strings::Set SupportedSchemes;
  };
}  // namespace IO::Network

namespace IO
{
  DataProvider::Ptr CreateNetworkDataProvider(Curl::Api::Ptr api)
  {
    return MakePtr<Network::DataProvider>(std::move(api));
  }

  void RegisterNetworkProvider(ProvidersEnumerator& enumerator)
  {
    try
    {
      auto api = Curl::LoadDynamicApi();
      Network::Dbg("Detected CURL library {}", api->curl_version());
      enumerator.RegisterProvider(CreateNetworkDataProvider(std::move(api)));
    }
    catch (const Error& e)
    {
      enumerator.RegisterProvider(
          CreateUnavailableProviderStub(Network::PROVIDER_IDENTIFIER, Network::PROVIDER_DESCRIPTION, e));
    }
  }
}  // namespace IO
