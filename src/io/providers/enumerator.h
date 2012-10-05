/*
Abstract:
  Providers enumerator

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __IO_ENUMERATOR_H_DEFINED__
#define __IO_ENUMERATOR_H_DEFINED__

//libary includes
#include <io/identifier.h>
#include <io/provider.h>  // for ProviderInfoArray
#include <strings/set.h>

namespace ZXTune
{
  namespace IO
  {
    // Internal interface
    class DataProvider : public Provider
    {
    public:
      typedef boost::shared_ptr<const DataProvider> Ptr;

      virtual Strings::Set Schemes() const = 0;
      virtual Identifier::Ptr Resolve(const String& uri) const = 0;
      virtual Binary::Container::Ptr Open(const String& path, const Parameters::Accessor& parameters, Log::ProgressCallback& callback) const = 0;
    };

    // internal enumerator interface
    class ProvidersEnumerator
    {
    public:
      virtual ~ProvidersEnumerator() {}
      //registration
      virtual void RegisterProvider(DataProvider::Ptr provider) = 0;

      virtual Identifier::Ptr ResolveUri(const String& uri) const = 0;

      virtual Binary::Container::Ptr OpenData(const String& path, const Parameters::Accessor& params, Log::ProgressCallback& cb) const = 0;

      virtual Provider::Iterator::Ptr Enumerate() const = 0;

      static ProvidersEnumerator& Instance();
    };

    DataProvider::Ptr CreateDisabledProviderStub(const String& id, const char* description);
    DataProvider::Ptr CreateUnavailableProviderStub(const String& id, const char* description, const Error& status);
  }
}

#endif //__IO_ENUMERATOR_H_DEFINED__
