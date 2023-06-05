/**
 *
 * @file
 *
 * @brief  Providers enumerator implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "io/providers/enumerator.h"
#include "io/impl/l10n.h"
#include "io/providers/providers_list.h"
// common includes
#include <error_tools.h>
#include <make_ptr.h>
// library includes
#include <debug/log.h>
#include <io/api.h>
#include <strings/map.h>
// std includes
#include <algorithm>
#include <list>

namespace IO
{
  const Debug::Stream Dbg("IO::Enumerator");

  using ProvidersList = std::vector<DataProvider::Ptr>;

  // implementation of IO providers enumerator
  class ProvidersEnumeratorImpl : public ProvidersEnumerator
  {
  public:
    ProvidersEnumeratorImpl()
    {
      RegisterProviders(*this);
    }

    void RegisterProvider(DataProvider::Ptr provider) override
    {
      Providers.push_back(provider);
      Dbg("Registered provider '{}'", provider->Id());
      const Strings::Set& schemes = provider->Schemes();
      for (const auto& scheme : schemes)
      {
        Schemes.insert(std::make_pair(scheme, provider));
      }
    }

    Identifier::Ptr ResolveUri(StringView uri) const override
    {
      Dbg("Resolving uri '{}'", uri);
      if (auto id = Resolve(uri))
      {
        return id;
      }
      Dbg(" No suitable provider found");
      throw MakeFormattedError(THIS_LINE, translate("Failed to resolve uri '{}'."), uri);
    }

    Binary::Container::Ptr OpenData(StringView path, const Parameters::Accessor& params,
                                    Log::ProgressCallback& cb) const override
    {
      Dbg("Opening path '{}'", path);
      if (const auto id = Resolve(path))
      {
        if (const DataProvider* provider = FindProvider(id->Scheme()))
        {
          Dbg(" Used provider '{}'", provider->Id());
          return provider->Open(id->Path(), params, cb);
        }
      }
      Dbg(" No suitable provider found");
      throw Error(THIS_LINE, translate("Specified uri scheme is not supported."));
    }

    Binary::OutputStream::Ptr CreateStream(StringView path, const Parameters::Accessor& params,
                                           Log::ProgressCallback& cb) const override
    {
      Dbg("Creating stream '{}'", path);
      if (const auto id = Resolve(path))
      {
        if (const DataProvider* provider = FindProvider(id->Scheme()))
        {
          Dbg(" Used provider '{}'", provider->Id());
          // pass nonchanged parameter to lower level
          return provider->Create(path, params, cb);
        }
      }
      Dbg(" No suitable provider found");
      throw Error(THIS_LINE, translate("Specified uri scheme is not supported."));
    }

    Provider::Iterator::Ptr Enumerate() const override
    {
      return MakePtr<RangedObjectIteratorAdapter<ProvidersList::const_iterator, Provider::Ptr> >(Providers.begin(),
                                                                                                 Providers.end());
    }

  private:
    Identifier::Ptr Resolve(StringView uri) const
    {
      for (const auto& provider : Providers)
      {
        if (auto res = provider->Resolve(uri))
        {
          return res;
        }
      }
      return {};
    }

    const DataProvider* FindProvider(StringView scheme) const
    {
      return Schemes.FindPtrValue(scheme);
    }

  private:
    ProvidersList Providers;
    Strings::ValueMap<DataProvider::Ptr> Schemes;
  };

  class UnavailableProvider : public DataProvider
  {
  public:
    UnavailableProvider(StringView id, const char* descr, Error status)
      : IdValue(id.to_string())
      , DescrValue(descr)
      , StatusValue(std::move(status))
    {}

    String Id() const override
    {
      return IdValue;
    }

    String Description() const override
    {
      return translate(DescrValue);
    }

    Error Status() const override
    {
      return StatusValue;
    }

    Binary::Container::Ptr Open(StringView, const Parameters::Accessor&, Log::ProgressCallback&) const override
    {
      throw Error(THIS_LINE, translate("Specified uri scheme is not supported."));
    }

    Binary::OutputStream::Ptr Create(StringView, const Parameters::Accessor&, Log::ProgressCallback&) const override
    {
      throw Error(THIS_LINE, translate("Specified uri scheme is not supported."));
    }

    Strings::Set Schemes() const override
    {
      return {};
    }

    Identifier::Ptr Resolve(StringView /*uri*/) const override
    {
      return {};
    }

  private:
    const String IdValue;
    const char* const DescrValue;
    const Error StatusValue;
  };
}  // namespace IO

namespace IO
{
  ProvidersEnumerator& ProvidersEnumerator::Instance()
  {
    static ProvidersEnumeratorImpl instance;
    return instance;
  }

  Identifier::Ptr ResolveUri(StringView uri)
  {
    return ProvidersEnumerator::Instance().ResolveUri(uri);
  }

  Binary::Container::Ptr OpenData(StringView path, const Parameters::Accessor& params, Log::ProgressCallback& cb)
  {
    return ProvidersEnumerator::Instance().OpenData(path, params, cb);
  }

  Binary::OutputStream::Ptr CreateStream(StringView path, const Parameters::Accessor& params, Log::ProgressCallback& cb)
  {
    return ProvidersEnumerator::Instance().CreateStream(path, params, cb);
  }

  Provider::Iterator::Ptr EnumerateProviders()
  {
    return ProvidersEnumerator::Instance().Enumerate();
  }

  DataProvider::Ptr CreateDisabledProviderStub(StringView id, const char* description)
  {
    return CreateUnavailableProviderStub(id, description,
                                         Error(THIS_LINE, translate("Not supported in current configuration")));
  }

  DataProvider::Ptr CreateUnavailableProviderStub(StringView id, const char* description, const Error& status)
  {
    return MakePtr<UnavailableProvider>(id, description, status);
  }
}  // namespace IO
