/*
Abstract:
  Providers enumerator

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __IO_ENUMERATOR_H_DEFINED__
#define __IO_ENUMERATOR_H_DEFINED__

#include <io/provider.h>

#include <boost/function.hpp>

namespace ZXTune
{
  namespace IO
  {
    typedef boost::function<bool(const String&)> ProviderCheckFunc;
    typedef boost::function<Error(const String&, const Parameters::Map&, const ProgressCallback&, DataContainer::Ptr&, String&)> ProviderOpenFunc;
    typedef boost::function<Error(const String&, String&, String&)> ProviderSplitFunc;
    typedef boost::function<Error(const String&, const String&, String&)> ProviderCombineFunc;
  
    class ProvidersEnumerator
    {
    public:
      virtual ~ProvidersEnumerator() {}
      //registration
      virtual void RegisterProvider(const ProviderInfo& info,
	                                  const ProviderCheckFunc& detector, const ProviderOpenFunc& opener,
                                    const ProviderSplitFunc& splitter, const ProviderCombineFunc& combiner) = 0;
      
      virtual Error OpenUri(const String& uri, const Parameters::Map& params, const ProgressCallback& cb, 
                            DataContainer::Ptr& result, String& subpath) const = 0;
      virtual Error SplitUri(const String& uri, String& baseUri, String& subpath) const = 0;
      virtual Error CombineUri(const String& baseUri, const String& subpath, String& uri) const = 0;
      
      virtual void Enumerate(std::vector<ProviderInfo>& infos) const = 0;
      
      static ProvidersEnumerator& Instance();
    };
  }
}

#endif //__IO_ENUMERATOR_H_DEFINED__
