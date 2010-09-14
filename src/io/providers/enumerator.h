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

//libary includes
#include <io/provider.h>  // for ProviderInfoArray

namespace ZXTune
{
  namespace IO
  {
    // Internal interface
    class DataProvider : public Provider
    {
    public:
      typedef boost::shared_ptr<const DataProvider> Ptr;

      // Check if provider supports that scheme
      virtual bool Check(const String& uri) const = 0;
      // Split uri
      virtual Error Split(const String& uri, String& basePath, String& subPath) const = 0;
      // Combine uri
      virtual Error Combine(const String& basePath, const String& subPath, String& uri) const = 0;
      // Open data
      virtual Error Open(const String& uri, const Parameters::Accessor& parameters, 
                         const ProgressCallback& callback, DataContainer::Ptr& result, String& subpath) const = 0;
    };

    // internal enumerator interface
    class ProvidersEnumerator
    {
    public:
      virtual ~ProvidersEnumerator() {}
      //registration
      virtual void RegisterProvider(DataProvider::Ptr provider) = 0;
      
      virtual Error OpenUri(const String& uri, const Parameters::Accessor& params, const ProgressCallback& cb,
                            DataContainer::Ptr& result, String& subpath) const = 0;
      virtual Error SplitUri(const String& uri, String& baseUri, String& subpath) const = 0;
      virtual Error CombineUri(const String& baseUri, const String& subpath, String& uri) const = 0;
      
      virtual Provider::Iterator::Ptr Enumerate() const = 0;
      
      static ProvidersEnumerator& Instance();
    };
  }
}

#endif //__IO_ENUMERATOR_H_DEFINED__
