/*
Abstract:
  Network provider

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//include first due to strange problems with curl includes
#include <io/error_codes.h>

//local includes
#include "curl_api.h"
#include "enumerator.h"
//common includes
#include <error_tools.h>
#include <logging.h>
#include <tools.h>
//library includes
#include <io/fs_tools.h>
#include <io/providers_parameters.h>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <io/text/io.h>

#define FILE_TAG 18F46494

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::IO;

  const std::string THIS_MODULE("IO::Provider::Network");

  class NetworkProviderParameters
  {
  public:
    explicit NetworkProviderParameters(const Parameters::Accessor& accessor)
      : Accessor(accessor)
    {
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
      Log::Debug(THIS_MODULE, "Curl(%1%): created", Object);
    }

    ~CurlObject()
    {
      if (Object)
      {
        Api->curl_easy_cleanup(Object);
        Log::Debug(THIS_MODULE, "Curl(%1%): destroyed", Object);
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
        throw MakeFormattedError(loc, ERROR_IO_ERROR, Text::IO_ERROR_NETWORK_ERROR, Api->curl_easy_strerror(code));
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
    RemoteResource(Curl::Api::Ptr api, const String& url)
      : Object(api)
    {
      Object.SetOption(CURLOPT_URL, IO::ConvertToFilename(url).c_str(), THIS_LINE);
      Object.SetOption(CURLOPT_DEBUGFUNCTION, reinterpret_cast<void*>(&DebugCallback), THIS_LINE);
      Object.SetOption(CURLOPT_WRITEFUNCTION, reinterpret_cast<void*>(&WriteCallback), THIS_LINE);
      Object.SetOption(CURLOPT_FOLLOWLOCATION, 1, THIS_LINE);
      Object.SetOption(CURLOPT_VERBOSE, 1, THIS_LINE);
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
        throw MakeFormattedError(THIS_LINE, ERROR_IO_ERROR, Text::IO_ERROR_NETWORK_ERROR, retCode);
      }
      return Binary::CreateContainer(result);
    }
  private:
    static int DebugCallback(CURL* obj, curl_infotype type, char* data, size_t size, void* /*param*/)
    {
      switch (type)
      {
      case CURLINFO_TEXT:
        Log::Debug(THIS_MODULE, "Curl(%1%): %2%", obj, std::string(data, data + size - 1));//initial CR
        break;
      case CURLINFO_HEADER_IN:
        Log::Debug(THIS_MODULE, "Curl(%1%): header in, %2% bytes", obj, size);
        break;
      case CURLINFO_HEADER_OUT:
        Log::Debug(THIS_MODULE, "Curl(%1%): header out, %2% bytes", obj, size);
        break;
      case CURLINFO_DATA_IN:
        Log::Debug(THIS_MODULE, "Curl(%1%): data in, %2% bytes", obj, size);
        break;
      case CURLINFO_DATA_OUT:
        Log::Debug(THIS_MODULE, "Curl(%1%): data out, %2% bytes", obj, size);
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
  private:
    CurlObject Object;
  };

  // uri-related constants
  const Char SCHEME_SIGN[] = {':', '/', '/', 0};
  const Char SCHEME_HTTP[] = {'h', 't', 't', 'p', 0};
  const Char SCHEME_HTTPS[] = {'h', 't', 't', 'p', 's', 0};
  const Char SCHEME_FTP[] = {'f', 't', 'p', 0};
  const Char SUBPATH_DELIMITER = '#';

  class NetworkDataProvider : public DataProvider
  {
  public:
    explicit NetworkDataProvider(Curl::Api::Ptr api)
      : Api(api)
    {
    }

    virtual String Id() const
    {
      return Text::IO_NETWORK_PROVIDER_ID;
    }

    virtual String Description() const
    {
      return Text::IO_NETWORK_PROVIDER_DESCRIPTION;
    }

    virtual bool Check(const String& uri) const
    {
      // TODO: extract and use common scheme-working code
      const String::size_type schemePos = uri.find(SCHEME_SIGN);
      if (schemePos == String::npos)
      {
        return false;//scheme is mandatory
      }
      const String scheme = uri.substr(0, schemePos);
      return scheme == SCHEME_HTTP
          || scheme == SCHEME_HTTPS
          || scheme == SCHEME_FTP
      ;
    }
  
    virtual Error Split(const String& uri, String& path, String& subpath) const
    {
      if (!Check(uri))
      {
        return Error(THIS_LINE, ERROR_NOT_SUPPORTED, Text::IO_ERROR_NOT_SUPPORTED_URI);
      }
      const String::size_type subPos = uri.find_first_of(SUBPATH_DELIMITER);
      if (String::npos != subPos)
      {
        path = uri.substr(0, subPos);
        subpath = uri.substr(subPos + 1);
      }
      else
      {
        path = uri;
        subpath = String();
      }
      return Error();
    }
  
    virtual Error Combine(const String& path, const String& subpath, String& uri) const
    {
      String base, sub;
      if (const Error& e = Split(path, base, sub))
      {
        return e;
      }
      uri = base;
      if (!subpath.empty())
      {
        uri += SUBPATH_DELIMITER;
        uri += subpath;
      }
      return Error();
    }
  
    //no callback
    virtual Error Open(const String& path, const Parameters::Accessor& params, const ProgressCallback& /*cb*/,
      Binary::Container::Ptr& result) const
    {
      try
      {
        RemoteResource resource(Api, path);
        result = resource.Download();
        return Error();
      }
      catch (const Error& e)
      {
        return MakeFormattedError(THIS_LINE, ERROR_NOT_OPENED, Text::IO_ERROR_NOT_OPENED, path).AddSuberror(e);
      }
    }
  private:
    const Curl::Api::Ptr Api;
  };
}

namespace ZXTune
{
  namespace IO
  {
    void RegisterNetworkProvider(ProvidersEnumerator& enumerator)
    {
      try
      {
        const Curl::Api::Ptr api = Curl::LoadDynamicApi();
        const DataProvider::Ptr provider = boost::make_shared<NetworkDataProvider>(api);
        enumerator.RegisterProvider(provider);
      }
      catch (const Error& e)
      {
        Log::Debug(THIS_MODULE, "%1%", Error::ToString(e));
      }
    }
  }
}
