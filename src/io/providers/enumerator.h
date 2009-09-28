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
    namespace Provider
    {
      typedef bool (*CheckFunc)(const String&);
      typedef Error (*OpenFunc)(const String&, const OpenDataParameters&, DataContainer::Ptr&, String&);
      typedef Error (*SplitFunc)(const String&, String&, String&);
      typedef Error (*CombineFunc)(const String&, const String&, String&);
      
      struct AutoRegistrator
      {
        AutoRegistrator(const ProviderInfo&, CheckFunc, OpenFunc, SplitFunc, CombineFunc);
      };
    }
  
    class ProvidersEnumerator
    {
    public:
      virtual ~ProvidersEnumerator() {}
      //registration
      virtual void RegisterProvider(const ProviderInfo& info,
	Provider::CheckFunc detector, Provider::OpenFunc opener, Provider::SplitFunc splitter, Provider::CombineFunc combiner) = 0;
      
      virtual Error OpenUri(const String& uri, const OpenDataParameters& params, DataContainer::Ptr& result, String& subpath) const = 0;
      virtual Error SplitUri(const String& uri, String& baseUri, String& subpath) const = 0;
      virtual Error CombineUri(const String& baseUri, const String& subpath, String& uri) const = 0;
      
      virtual void Enumerate(std::vector<ProviderInfo>& infos) const = 0;
      
      static ProvidersEnumerator& Instance();
    };
  }
}

#endif //__IO_ENUMERATOR_H_DEFINED__
