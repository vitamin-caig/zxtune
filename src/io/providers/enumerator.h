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

// libary includes
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
    virtual Identifier::Ptr Resolve(StringView uri) const = 0;
    virtual Binary::Container::Ptr Open(StringView path, const Parameters::Accessor& parameters,
                                        Log::ProgressCallback& callback) const = 0;
    virtual Binary::OutputStream::Ptr Create(StringView path, const Parameters::Accessor& params,
                                             Log::ProgressCallback& callback) const = 0;
  };

  // internal enumerator interface
  class ProvidersEnumerator
  {
  public:
    virtual ~ProvidersEnumerator() = default;
    // registration
    virtual void RegisterProvider(DataProvider::Ptr provider) = 0;

    virtual Identifier::Ptr ResolveUri(StringView uri) const = 0;

    virtual Binary::Container::Ptr OpenData(StringView path, const Parameters::Accessor& params,
                                            Log::ProgressCallback& cb) const = 0;

    virtual Binary::OutputStream::Ptr CreateStream(StringView path, const Parameters::Accessor& params,
                                                   Log::ProgressCallback& cb) const = 0;

    virtual Provider::Iterator::Ptr Enumerate() const = 0;

    static ProvidersEnumerator& Instance();
  };

  DataProvider::Ptr CreateDisabledProviderStub(StringView id, const char* description);
  DataProvider::Ptr CreateUnavailableProviderStub(StringView id, const char* description, const Error& status);
}  // namespace IO
