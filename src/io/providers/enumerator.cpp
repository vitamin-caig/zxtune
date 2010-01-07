/*
Abstract:
  Providers enumerator implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "enumerator.h"
#include "providers_list.h"

#include <logging.h>
#include <io/error_codes.h>

#include <algorithm>
#include <list>
#include <boost/bind.hpp>
#include <boost/bind/apply.hpp>

#include <text/io.h>

#define FILE_TAG 03113EE3

namespace
{
  using namespace ZXTune::IO;

  const std::string THIS_MODULE("IO::Enumerator");
  
  class ProvidersEnumeratorImpl : public ProvidersEnumerator
  {
    struct ProviderEntry
    {
      ProviderEntry() : Checker(), Opener(), Splitter(), Combiner()
      {
      }
      
      ProviderEntry(const ProviderInfo& info,
        ProviderCheckFunc checker, ProviderOpenFunc opener, ProviderSplitFunc splitter, ProviderCombineFunc combiner)
        : Info(info), Checker(checker), Opener(opener), Splitter(splitter), Combiner(combiner)
      {
      }
      ProviderInfo Info;
      ProviderCheckFunc Checker;
      ProviderOpenFunc Opener;
      ProviderSplitFunc Splitter;
      ProviderCombineFunc Combiner;
    };
    typedef std::list<ProviderEntry> ProvidersList;
  public:
    ProvidersEnumeratorImpl()
    {
      RegisterProviders(*this);
    }
    
    virtual void RegisterProvider(const ProviderInfo& info,
      const ProviderCheckFunc& detector, const ProviderOpenFunc& opener,
      const ProviderSplitFunc& splitter, const ProviderCombineFunc& combiner)
    {
      assert(detector && opener && splitter && combiner);
      assert(Providers.end() == std::find_if(Providers.begin(), Providers.end(),
        boost::bind(&ProviderInfo::Name, boost::bind<ProviderInfo>(&ProviderEntry::Info, _1)) == info.Name));
      Providers.push_back(ProviderEntry(info, detector, opener, splitter, combiner));
      Log::Debug(THIS_MODULE, "Registered provider '%1%'", info.Name);
    }
     
    virtual Error OpenUri(const String& uri, const Parameters::Map& params, const ProgressCallback& cb, DataContainer::Ptr& result, String& subpath) const
    {
      Log::Debug(THIS_MODULE, "Opening uri '%1%'", uri);
      const ProvidersList::const_iterator it = FindProvider(uri);
      if (it != Providers.end())
      {
        Log::Debug(THIS_MODULE, " Used provider '%1%'", it->Info.Name);
        return it->Opener(uri, params, cb, result, subpath);
      }
      Log::Debug(THIS_MODULE, " No suitable provider found");
      return Error(THIS_LINE, NOT_SUPPORTED, TEXT_IO_ERROR_NOT_SUPPORTED_URI);
    }
    
    virtual Error SplitUri(const String& uri, String& baseUri, String& subpath) const
    {
      Log::Debug(THIS_MODULE, "Splitting uri '%1%'", uri);
      const ProvidersList::const_iterator it = FindProvider(uri);
      if (it != Providers.end())
      {
        Log::Debug(THIS_MODULE, " Used provider '%1%'", it->Info.Name);
        return it->Splitter(uri, baseUri, subpath);
      }
      Log::Debug(THIS_MODULE, " No suitable provider found");
      return Error(THIS_LINE, NOT_SUPPORTED, TEXT_IO_ERROR_NOT_SUPPORTED_URI);
    }
    
    virtual Error CombineUri(const String& baseUri, const String& subpath, String& uri) const
    {
      Log::Debug(THIS_MODULE, "Combining uri '%1%' and subpath '%2%'", baseUri, subpath);
      const ProvidersList::const_iterator it = FindProvider(baseUri);
      if (it != Providers.end())
      {
        Log::Debug(THIS_MODULE, " Used provider '%1%'", it->Info.Name);
        return it->Combiner(baseUri, subpath, uri);
      }
      Log::Debug(THIS_MODULE, " No suitable provider found");
      return Error(THIS_LINE, NOT_SUPPORTED, TEXT_IO_ERROR_NOT_SUPPORTED_URI);
    }
     
    virtual void Enumerate(ProviderInfoArray& infos) const
    {
      ProviderInfoArray result(Providers.size());
      std::transform(Providers.begin(), Providers.end(), result.begin(), boost::bind<ProviderInfo>(&ProviderEntry::Info, _1));
      infos.swap(result);
    }
  private:
    ProvidersList::const_iterator FindProvider(const String& uri) const
    {
      return std::find_if(Providers.begin(), Providers.end(),
        boost::bind(boost::apply<bool>(), boost::bind(&ProviderEntry::Checker, _1), uri));
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
    
    Error OpenData(const String& uri, const Parameters::Map& params, const ProgressCallback& cb, DataContainer::Ptr& data, String& subpath)
    {
      return ProvidersEnumerator::Instance().OpenUri(uri, params, cb, data, subpath);
    }
    
    Error SplitUri(const String& uri, String& baseUri, String& subpath)
    {
      return ProvidersEnumerator::Instance().SplitUri(uri, baseUri, subpath);
    }
    
    Error CombineUri(const String& baseUri, const String& subpath, String& uri)
    {
      return ProvidersEnumerator::Instance().CombineUri(baseUri, subpath, uri);
    }
    
    void GetSupportedProviders(ProviderInfoArray& infos)
    {
      return ProvidersEnumerator::Instance().Enumerate(infos);
    }
  }
}
