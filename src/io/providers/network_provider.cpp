/*
Abstract:
  Network provider

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "curl_api.h"
#include "enumerator.h"
//common includes
#include <contract.h>
#include <debug_log.h>
#include <error_tools.h>
#include <progress_callback.h>
#include <tools.h>
//library includes
#include <binary/container_factories.h>
#include <io/providers_parameters.h>
#include <l10n/api.h>
//std includes
#include <cstring>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <io/text/io.h>

#define FILE_TAG 18F46494

namespace
{
  const Debug::Stream Dbg("IO::Provider::Network");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("io");
}

namespace IO
{
  class NetworkProviderParameters
  {
  public:
    explicit NetworkProviderParameters(const Parameters::Accessor& accessor)
      : Accessor(accessor)
    {
    }

    String GetHttpUseragent() const
    {
      String res;
      Accessor.FindValue(Parameters::ZXTune::IO::Providers::Network::Http::USERAGENT, res);
      return res;
    }
  private:
    const Parameters::Accessor& Accessor;
  };

  const std::size_t INITIAL_SIZE = 1048576;

  class CurlObject
  {
  public:
    explicit CurlObject(Curl::Api::Ptr api)
      : Api(api)
      , Object(Api->curl_easy_init())
    {
      Dbg("Curl(%1%): created", Object);
    }

    ~CurlObject()
    {
      if (Object)
      {
        Api->curl_easy_cleanup(Object);
        Dbg("Curl(%1%): destroyed", Object);
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
        throw MakeFormattedError(loc, translate("Network error happends: %1%"), Api->curl_easy_strerror(code));
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
      : Object(api)
    {
      Object.SetOption(CURLOPT_DEBUGFUNCTION, reinterpret_cast<void*>(&DebugCallback), THIS_LINE);
      Object.SetOption(CURLOPT_VERBOSE, 1, THIS_LINE);
      Object.SetOption(CURLOPT_WRITEFUNCTION, reinterpret_cast<void*>(&WriteCallback), THIS_LINE);
    }

    void SetSource(const String& url)
    {
      Object.SetOption(CURLOPT_URL, url.c_str(), THIS_LINE);
    }

    void SetOptions(const NetworkProviderParameters& params)
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

    //TODO: pass callback to handle progress and other
    Binary::Container::Ptr Download()
    {
      std::auto_ptr<Dump> result(new Dump());
      result->reserve(INITIAL_SIZE);
      Object.SetOption<void*>(CURLOPT_WRITEDATA, result.get(), THIS_LINE);
      Object.Perform(THIS_LINE);
      long retCode = 0;
      Object.GetInfo(CURLINFO_RESPONSE_CODE, &retCode, THIS_LINE);
      if (IsHttpErrorCode(retCode))
      {
        throw MakeFormattedError(THIS_LINE, translate("Http error happends: %1%."), retCode);
      }
      return Binary::CreateContainer(result);
    }
  private:
    static int DebugCallback(CURL* obj, curl_infotype type, char* data, size_t size, void* /*param*/)
    {
      //size includes CR code
      switch (type)
      {
      case CURLINFO_TEXT:
        Dbg("Curl(%1%): %2%", obj, std::string(data, data + size - 1));
        break;
      case CURLINFO_HEADER_IN:
        Dbg("Curl(%1%): -> %2%", obj, std::string(data, data + size - 1));
        break;
      case CURLINFO_HEADER_OUT:
        Dbg("Curl(%1%): <- %2%", obj, std::string(data, data + size - 1));
        break;
      default:
        break;
      }
      return 0;
    }

    static size_t WriteCallback(const char* ptr, size_t size, size_t nmemb, Dump* result)
    {
      const std::size_t toSave = size * nmemb;
      const std::size_t prevSize = result->size();
      result->resize(prevSize + toSave);
      std::memcpy(&result->front() + prevSize, ptr, toSave);
      return toSave;
    }

    static int ProgressCallback(void* data, double dlTotal, double dlNow, double /*ulTotal*/, double /*ulNow*/)
    {
      if (dlTotal)//0 for source files with unknown size
      {
        Log::ProgressCallback* const cb = static_cast<Log::ProgressCallback*>(data);
        const uint_t progress = static_cast<uint_t>(dlNow * 100 / dlTotal);
        cb->OnProgress(progress);
      }
      return 0;
    }
  private:
    CurlObject Object;
  };

  // uri-related constants
  const Char SCHEME_SIGN[] = {':', '/', '/', 0};
  const Char SCHEME_HTTP[] = {'h', 't', 't', 'p', 0};
  const Char SCHEME_FTP[] = {'f', 't', 'p', 0};
  const Char SUBPATH_DELIMITER = '#';

  const Char* ALL_SCHEMES[] = 
  {
    SCHEME_HTTP,
    SCHEME_FTP,
  };

  class RemoteIdentifier : public Identifier
  {
  public:
    RemoteIdentifier(const String& scheme, const String& path, const String& subpath)
      : SchemeValue(scheme)
      , PathValue(path)
      , SubpathValue(subpath)
      , FullValue(Serialize())
    {
      Require(!SchemeValue.empty() && !PathValue.empty());
    }

    virtual String Full() const
    {
      return FullValue;
    }

    virtual String Scheme() const
    {
      return SchemeValue;
    }

    virtual String Path() const
    {
      return PathValue;
    }

    virtual String Filename() const
    {
      //filename usually is useless on remote schemes
      return String();
    }

    virtual String Extension() const
    {
      //filename usually is useless on remote schemes
      return String();
    }

    virtual String Subpath() const
    {
      return SubpathValue;
    }

    virtual Ptr WithSubpath(const String& subpath) const
    {
      return boost::make_shared<RemoteIdentifier>(SchemeValue, PathValue, subpath);
    }
  private:
    String Serialize() const
    {
      //do not place scheme
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

  const String ID = Text::IO_NETWORK_PROVIDER_ID;
  const char* const DESCRIPTION = L10n::translate("Network files access via different schemes support");

  class NetworkDataProvider : public DataProvider
  {
  public:
    explicit NetworkDataProvider(Curl::Api::Ptr api)
      : Api(api)
      , SupportedSchemes(ALL_SCHEMES, ArrayEnd(ALL_SCHEMES))
    {
    }

    virtual String Id() const
    {
      return ID;
    }

    virtual String Description() const
    {
      return translate(DESCRIPTION);
    }

    virtual Error Status() const
    {
      return Error();
    }

    virtual Strings::Set Schemes() const
    {
      return SupportedSchemes;
    }

    virtual Identifier::Ptr Resolve(const String& uri) const
    {
      const String::size_type schemePos = uri.find(SCHEME_SIGN);
      if (String::npos == schemePos)
      {
        //scheme is required
        return Identifier::Ptr();
      }
      const String::size_type hierPos = schemePos + ArraySize(SCHEME_SIGN) - 1;
      const String::size_type subPos = uri.find_first_of(SUBPATH_DELIMITER, hierPos);

      const String scheme = uri.substr(0, schemePos);
      const String hier = String::npos == subPos ? uri.substr(hierPos) : uri.substr(hierPos, subPos - hierPos);
      if (hier.empty() || !SupportedSchemes.count(scheme))
      {
        //scheme and hierarchy part is mandatory
        return Identifier::Ptr();
      }
      //Path should include scheme and all possible parameters
      const String path = String::npos == subPos ? uri : uri.substr(0, subPos);
      const String subpath = String::npos == subPos ? String() : uri.substr(subPos + 1);
      return boost::make_shared<RemoteIdentifier>(scheme, path, subpath);
    }

    virtual Binary::Container::Ptr Open(const String& path, const Parameters::Accessor& params, Log::ProgressCallback& cb) const
    {
      try
      {
        const NetworkProviderParameters options(params);
        RemoteResource resource(Api);
        resource.SetSource(path);
        resource.SetOptions(options);
        resource.SetProgressCallback(cb);
        return resource.Download();
      }
      catch (const Error& e)
      {
        throw MakeFormattedError(THIS_LINE, translate("Failed to open network resource '%1%'."), path).AddSuberror(e);
      }
    }
  private:
    const Curl::Api::Ptr Api;
    const Strings::Set SupportedSchemes;
  };
}

namespace IO
{
  DataProvider::Ptr CreateNetworkDataProvider(Curl::Api::Ptr api)
  {
    return boost::make_shared<NetworkDataProvider>(api);
  }

  void RegisterNetworkProvider(ProvidersEnumerator& enumerator)
  {
    try
    {
      const Curl::Api::Ptr api = Curl::LoadDynamicApi();
      Dbg("Detected CURL library %1%", api->curl_version());
      enumerator.RegisterProvider(CreateNetworkDataProvider(api));
    }
    catch (const Error& e)
    {
      enumerator.RegisterProvider(CreateUnavailableProviderStub(ID, DESCRIPTION, e));
    }
  }
}
