/*
Abstract:
  Providers enumerator implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "enumerator.h"
#include "providers_list.h"
//common includes
#include <debug_log.h>
//library includes
#include <io/error_codes.h>
//std includes
#include <algorithm>
#include <list>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <io/text/io.h>

#define FILE_TAG 03113EE3

namespace
{
  using namespace ZXTune::IO;

  const Debug::Stream Dbg("IO::Enumerator");

  typedef std::vector<DataProvider::Ptr> ProvidersList;

  //implementation of IO providers enumerator
  class ProvidersEnumeratorImpl : public ProvidersEnumerator
  {
  public:
    ProvidersEnumeratorImpl()
    {
      RegisterProviders(*this);
    }

    virtual void RegisterProvider(DataProvider::Ptr provider)
    {
      Providers.push_back(provider);
      Dbg("Registered provider '%1%'", provider->Id());
      const StringSet& schemes = provider->Schemes();
      std::transform(schemes.begin(), schemes.end(), std::inserter(Schemes, Schemes.end()),
        boost::bind(&std::make_pair<String, DataProvider::Ptr>, _1, provider));
    }

    virtual Error OpenData(const String& path, const Parameters::Accessor& params, Log::ProgressCallback& cb, Binary::Container::Ptr& result) const
    {
      Dbg("Opening path '%1%'", path);
      if (Identifier::Ptr id = Resolve(path))
      {
        if (const DataProvider* provider = FindProvider(id->Scheme()))
        {
          Dbg(" Used provider '%1%'", provider->Id());
          return provider->Open(id->Path(), params, cb, result);
        }
      }
      Dbg(" No suitable provider found");
      return Error(THIS_LINE, ERROR_NOT_SUPPORTED, Text::IO_ERROR_NOT_SUPPORTED_URI);
    }

    virtual Error SplitUri(const String& uri, String& path, String& subpath) const
    {
      Dbg("Splitting uri '%1%'", uri);
      if (Identifier::Ptr id = Resolve(uri))
      {
        path = id->Path();
        subpath = id->Subpath();
        return Error();
      }
      Dbg(" No suitable provider found");
      return Error(THIS_LINE, ERROR_NOT_SUPPORTED, Text::IO_ERROR_NOT_SUPPORTED_URI);
    }

    virtual Error CombineUri(const String& path, const String& subpath, String& uri) const
    {
      Dbg("Combining path '%1%' and subpath '%2%'", path, subpath);
      if (Identifier::Ptr id = Resolve(path))
      {
        uri = id->WithSubpath(subpath)->Full();
        return Error();
      }
      Dbg(" No suitable provider found");
      return Error(THIS_LINE, ERROR_NOT_SUPPORTED, Text::IO_ERROR_NOT_SUPPORTED_URI);
    }

    virtual Provider::Iterator::Ptr Enumerate() const
    {
      return Provider::Iterator::Ptr(new RangedObjectIteratorAdapter<ProvidersList::const_iterator, Provider::Ptr>(Providers.begin(), Providers.end()));
    }
  private:
    Identifier::Ptr Resolve(const String& uri) const
    {
      for (ProvidersList::const_iterator it = Providers.begin(), lim = Providers.end(); it != lim; ++it)
      {
        if (const Identifier::Ptr res = (*it)->Resolve(uri))
        {
          return res;
        }
      }
      return Identifier::Ptr();
    }

    const DataProvider* FindProvider(const String& scheme) const
    {
      const SchemeToProviderMap::const_iterator it = Schemes.find(scheme);
      return it != Schemes.end()
        ? it->second.get()
        : 0;
    }
  private:
    ProvidersList Providers;
    typedef std::map<String, DataProvider::Ptr> SchemeToProviderMap;
    SchemeToProviderMap Schemes;
  };

  class UnavailableProvider : public DataProvider
  {
  public:
    UnavailableProvider(const String& id, const String& descr, const Error& status)
      : IdValue(id)
      , DescrValue(descr)
      , StatusValue(status)
    {
    }

    virtual String Id() const
    {
      return IdValue;
    }

    virtual String Description() const
    {
      return DescrValue;
    }

    virtual Error Status() const
    {
      return StatusValue;
    }

    virtual bool Check(const String&) const
    {
      return false;
    }

    virtual Error Split(const String&, String&, String&) const
    {
      return Error(THIS_LINE, ERROR_NOT_SUPPORTED, Text::IO_ERROR_NOT_SUPPORTED_URI);
    }

    virtual Error Combine(const String&, const String&, String&) const
    {
      return Error(THIS_LINE, ERROR_NOT_SUPPORTED, Text::IO_ERROR_NOT_SUPPORTED_URI);
    }

    virtual Error Open(const String&, const Parameters::Accessor&, Log::ProgressCallback&, Binary::Container::Ptr&) const
    {
      return Error(THIS_LINE, ERROR_NOT_SUPPORTED, Text::IO_ERROR_NOT_SUPPORTED_URI);
    }

    virtual StringSet Schemes() const
    {
      return StringSet();
    }

    virtual Identifier::Ptr Resolve(const String& /*uri*/) const
    {
      return Identifier::Ptr();
    }
  private:
    const String IdValue;
    const String DescrValue;
    const Error StatusValue;
  };
}

namespace ZXTune
{
  namespace IO
  {
    ProvidersEnumerator& ProvidersEnumerator::Instance()
    {
      static ProvidersEnumeratorImpl instance;
      return instance;
    }

    Error OpenData(const String& path, const Parameters::Accessor& params, Log::ProgressCallback& cb, Binary::Container::Ptr& data)
    {
      try
      {
        return ProvidersEnumerator::Instance().OpenData(path, params, cb, data);
      }
      catch (const std::bad_alloc&)
      {
        return Error(THIS_LINE, ERROR_NO_MEMORY, Text::IO_ERROR_NO_MEMORY);
      }
    }

    Error SplitUri(const String& uri, String& path, String& subpath)
    {
      return ProvidersEnumerator::Instance().SplitUri(uri, path, subpath);
    }

    Error CombineUri(const String& path, const String& subpath, String& uri)
    {
      return ProvidersEnumerator::Instance().CombineUri(path, subpath, uri);
    }

    Provider::Iterator::Ptr EnumerateProviders()
    {
      return ProvidersEnumerator::Instance().Enumerate();
    }

    DataProvider::Ptr CreateDisabledProviderStub(const String& id, const String& description)
    {
      return CreateUnavailableProviderStub(id, description, Error(THIS_LINE, ERROR_NOT_SUPPORTED, Text::IO_ERROR_DISABLED_PROVIDER));
    }

    DataProvider::Ptr CreateUnavailableProviderStub(const String& id, const String& description, const Error& status)
    {
      return boost::make_shared<UnavailableProvider>(id, description, status);
    }
  }
}
