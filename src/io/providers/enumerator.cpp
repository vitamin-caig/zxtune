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
      virtual String Id() const
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
      Log::Debug(THIS_MODULE, "Registered provider '%1%'", provider->Id());
    }

    virtual Error OpenData(const String& path, const Parameters::Accessor& params, const ProgressCallback& cb, Binary::Container::Ptr& result) const
    {
      Log::Debug(THIS_MODULE, "Opening path '%1%'", path);
      if (const DataProvider* provider = FindProvider(path))
      {
        Log::Debug(THIS_MODULE, " Used provider '%1%'", provider->Id());
        return provider->Open(path, params, cb, result);
      }
      Log::Debug(THIS_MODULE, " No suitable provider found");
      return Error(THIS_LINE, ERROR_NOT_SUPPORTED, Text::IO_ERROR_NOT_SUPPORTED_URI);
    }

    virtual Error SplitUri(const String& uri, String& path, String& subpath) const
    {
      Log::Debug(THIS_MODULE, "Splitting uri '%1%'", uri);
      if (const DataProvider* provider = FindProvider(uri))
      {
        Log::Debug(THIS_MODULE, " Used provider '%1%'", provider->Id());
        return provider->Split(uri, path, subpath);
      }
      Log::Debug(THIS_MODULE, " No suitable provider found");
      return Error(THIS_LINE, ERROR_NOT_SUPPORTED, Text::IO_ERROR_NOT_SUPPORTED_URI);
    }

    virtual Error CombineUri(const String& path, const String& subpath, String& uri) const
    {
      Log::Debug(THIS_MODULE, "Combining path '%1%' and subpath '%2%'", path, subpath);
      if (const DataProvider* provider = FindProvider(path))
      {
        Log::Debug(THIS_MODULE, " Used provider '%1%'", provider->Id());
        return provider->Combine(path, subpath, uri);
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

    Error OpenData(const String& path, const Parameters::Accessor& params, const ProgressCallback& cb, Binary::Container::Ptr& data)
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
  }
}
