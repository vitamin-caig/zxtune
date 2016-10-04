/**
*
* @file
*
* @brief  Providers enumerator
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//libary includes
#include <io/api.h>
#include <io/provider.h>  // for ProviderInfoArray
#include <strings/set.h>

namespace IO
{
  // Internal interface
  class DataProvider : public Provider
  {
  public:
    typedef std::shared_ptr<const DataProvider> Ptr;

    virtual Strings::Set Schemes() const = 0;
    virtual Identifier::Ptr Resolve(const String& uri) const = 0;
    virtual Binary::Container::Ptr Open(const String& path, const Parameters::Accessor& parameters, Log::ProgressCallback& callback) const = 0;
    virtual Binary::OutputStream::Ptr Create(const String& path, const Parameters::Accessor& params, Log::ProgressCallback& callback) const = 0;
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

    virtual Binary::OutputStream::Ptr CreateStream(const String& path, const Parameters::Accessor& params, Log::ProgressCallback& cb) const = 0;

    virtual Provider::Iterator::Ptr Enumerate() const = 0;

    static ProvidersEnumerator& Instance();
  };

  DataProvider::Ptr CreateDisabledProviderStub(const String& id, const char* description);
  DataProvider::Ptr CreateUnavailableProviderStub(const String& id, const char* description, const Error& status);
}
