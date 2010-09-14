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
#include <logging.h>
//library includes
#include <io/error_codes.h>
//std includes
#include <algorithm>
#include <list>
//boost includes
#include <boost/bind.hpp>
#include <boost/bind/apply.hpp>
//text includes
#include <io/text/io.h>

#define FILE_TAG 03113EE3

namespace
{
  using namespace ZXTune::IO;

  const std::string THIS_MODULE("IO::Enumerator");

  typedef std::vector<DataProvider::Ptr> ProvidersList;

  class ProviderIteratorImpl : public Provider::Iterator
  {
    class ProviderStub : public Provider
    {
    public:
      virtual String Name() const
      {
        return String();
      }

      virtual String Description() const
      {
        return String();
      }

      virtual String Version() const
      {
        return String();
      }

      static void Deleter(Provider*)
      {
      }
    };
  public:
    ProviderIteratorImpl(ProvidersList::const_iterator from,
                         ProvidersList::const_iterator to)
      : Pos(from), Limit(to)
    {
    }

    virtual bool IsValid() const
    {
      return Pos != Limit;
    }

    virtual Provider::Ptr Get() const
    {
      //since this implementation is passed to external client, make it as safe as possible
      if (Pos != Limit)
      {
        return *Pos;
      }
      assert(!"Provider iterator is out of range");
      static ProviderStub stub;
      return Provider::Ptr(&stub, &ProviderStub::Deleter);
    }

    virtual void Next()
    {
      if (Pos != Limit)
      {
        ++Pos;
      }
      else
      {
        assert(!"Provider iterator is out of range");
      }
    }
  private:
    ProvidersList::const_iterator Pos;
    const ProvidersList::const_iterator Limit;
  };

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
      Log::Debug(THIS_MODULE, "Registered provider '%1%'", provider->Name());
    }

    virtual Error OpenUri(const String& uri, const Parameters::Accessor& params, const ProgressCallback& cb, DataContainer::Ptr& result, String& subpath) const
    {
      Log::Debug(THIS_MODULE, "Opening uri '%1%'", uri);
      if (const DataProvider* provider = FindProvider(uri))
      {
        Log::Debug(THIS_MODULE, " Used provider '%1%'", provider->Name());
        return provider->Open(uri, params, cb, result, subpath);
      }
      Log::Debug(THIS_MODULE, " No suitable provider found");
      return Error(THIS_LINE, ERROR_NOT_SUPPORTED, Text::IO_ERROR_NOT_SUPPORTED_URI);
    }

    virtual Error SplitUri(const String& uri, String& baseUri, String& subpath) const
    {
      Log::Debug(THIS_MODULE, "Splitting uri '%1%'", uri);
      if (const DataProvider* provider = FindProvider(baseUri))
      {
        Log::Debug(THIS_MODULE, " Used provider '%1%'", provider->Name());
        return provider->Split(uri, baseUri, subpath);
      }
      Log::Debug(THIS_MODULE, " No suitable provider found");
      return Error(THIS_LINE, ERROR_NOT_SUPPORTED, Text::IO_ERROR_NOT_SUPPORTED_URI);
    }

    virtual Error CombineUri(const String& baseUri, const String& subpath, String& uri) const
    {
      Log::Debug(THIS_MODULE, "Combining uri '%1%' and subpath '%2%'", baseUri, subpath);
      if (const DataProvider* provider = FindProvider(baseUri))
      {
        Log::Debug(THIS_MODULE, " Used provider '%1%'", provider->Name());
        return provider->Combine(baseUri, subpath, uri);
      }
      Log::Debug(THIS_MODULE, " No suitable provider found");
      return Error(THIS_LINE, ERROR_NOT_SUPPORTED, Text::IO_ERROR_NOT_SUPPORTED_URI);
    }

    virtual Provider::Iterator::Ptr Enumerate() const
    {
      return Provider::Iterator::Ptr(new ProviderIteratorImpl(Providers.begin(), Providers.end()));
    }
  private:
    const DataProvider* FindProvider(const String& uri) const
    {
      for (ProvidersList::const_iterator it = Providers.begin(), lim = Providers.end(); it != lim; ++it)
      {
        if ((*it)->Check(uri))
        {
          return it->get();
        }
      }
      return 0;
    }
  private:
    ProvidersList Providers;
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

    Error OpenData(const String& uri, const Parameters::Accessor& params, const ProgressCallback& cb, DataContainer::Ptr& data, String& subpath)
    {
      try
      {
        return ProvidersEnumerator::Instance().OpenUri(uri, params, cb, data, subpath);
      }
      catch (const std::bad_alloc&)
      {
        return Error(THIS_LINE, ERROR_NO_MEMORY, Text::IO_ERROR_NO_MEMORY);
      }
    }

    Error SplitUri(const String& uri, String& baseUri, String& subpath)
    {
      return ProvidersEnumerator::Instance().SplitUri(uri, baseUri, subpath);
    }

    Error CombineUri(const String& baseUri, const String& subpath, String& uri)
    {
      return ProvidersEnumerator::Instance().CombineUri(baseUri, subpath, uri);
    }

    Provider::Iterator::Ptr EnumerateProviders()
    {
      return ProvidersEnumerator::Instance().Enumerate();
    }
  }
}
