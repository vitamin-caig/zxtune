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

#include "../provider.h"

namespace ZXTune
{
  namespace IO
  {
    typedef bool (*ProviderCheckFunc)(const String&);
    typedef Error (*ProviderOpenFunc)(const String&, const OpenDataParameters&, DataContainer::Ptr&, String&);
    typedef Error (*ProviderSplitFunc)(const String&, String&, String&);
    typedef Error (*ProviderCombineFunc)(const String&, const String&, String&);
  
    class ProvidersEnumerator
    {
    public:
      virtual ~ProvidersEnumerator() {}
      //registration
      virtual void RegisterProvider(const ProviderInfo& info,
	ProviderCheckFunc detector, ProviderOpenFunc opener, ProviderSplitFunc splitter, ProviderCombineFunc combiner) = 0;
      
      virtual Error OpenUri(const String& uri, const OpenDataParameters& params, DataContainer::Ptr& result, String& subpath) const = 0;
      virtual Error SplitUri(const String& uri, String& baseUri, String& subpath) const = 0;
      virtual Error CombineUri(const String& baseUri, const String& subpath, String& uri) const = 0;
      
      virtual void Enumerate(std::vector<ProviderInfo>& infos) const = 0;
      
      static ProvidersEnumerator& Instance();
    };
  }
}

#endif //__IO_ENUMERATOR_H_DEFINED__
