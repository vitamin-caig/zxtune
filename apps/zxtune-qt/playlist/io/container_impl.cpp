/**
 *
 * @file
 *
 * @brief Playlist container internal implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "container_impl.h"
// common includes
#include <contract.h>
#include <error.h>
#include <make_ptr.h>
// library includes
#include <debug/log.h>
#include <module/properties/path.h>
#include <parameters/merged_accessor.h>
// std includes
#include <utility>

namespace
{
  const Debug::Stream Dbg("Playlist::IO::Base");

  class CollectorStub : public Playlist::Item::DetectParameters
  {
  public:
    explicit CollectorStub(const Parameters::Accessor& params)
      : Params(params)
    {}

    Parameters::Container::Ptr CreateInitialAdjustedParameters() const override
    {
      return Parameters::Container::Clone(Params);
    }

    void ProcessItem(Playlist::Item::Data::Ptr item) override
    {
      assert(!Item);
      Item = item;
    }

    Log::ProgressCallback* GetProgress() const override
    {
      return nullptr;
    }

    Playlist::Item::Data::Ptr GetItem() const
    {
      return Item;
    }

  private:
    const Parameters::Accessor& Params;
    Playlist::Item::Data::Ptr Item;
  };

  class StubData : public Playlist::Item::Data
  {
  public:
    StubData(String path, const Parameters::Accessor& params, Error state)
      : Path(std::move(path))
      , Params(Parameters::Container::Create())
      , State(std::move(state))
    {
      params.Process(*Params);
    }

    bool IsLoaded() const override
    {
      return true;
    }

    // common
    Module::Holder::Ptr GetModule() const override
    {
      return Module::Holder::Ptr();
    }

    Binary::Data::Ptr GetModuleData() const override
    {
      return Binary::Data::Ptr();
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Params;
    }

    Parameters::Container::Ptr GetAdjustedParameters() const override
    {
      return Params;
    }

    Playlist::Item::Capabilities GetCapabilities() const override
    {
      return Playlist::Item::Capabilities(0);
    }

    // playlist-related
    Error GetState() const override
    {
      return State;
    }

    String GetFullPath() const override
    {
      return Path;
    }

    String GetFilePath() const override
    {
      return Path;
    }

    String GetType() const override
    {
      return String();
    }

    String GetDisplayName() const override
    {
      return Path;
    }

    Time::Milliseconds GetDuration() const override
    {
      return {};
    }

    String GetAuthor() const override
    {
      return String();
    }

    String GetTitle() const override
    {
      return String();
    }

    String GetComment() const override
    {
      return String();
    }

    uint32_t GetChecksum() const override
    {
      return 0;
    }

    uint32_t GetCoreChecksum() const override
    {
      return 0;
    }

    std::size_t GetSize() const override
    {
      return 0;
    }

  private:
    const String Path;
    const Parameters::Container::Ptr Params;
    const Error State;
  };

  class DelayLoadItemProvider
  {
  public:
    using Ptr = std::unique_ptr<const DelayLoadItemProvider>;

    DelayLoadItemProvider(Playlist::Item::DataProvider::Ptr provider, Parameters::Accessor::Ptr playlistParams,
                          const Playlist::IO::ContainerItem& item)
      : Provider(std::move(provider))
      , Params(Parameters::CreateMergedAccessor(Module::CreatePathProperties(item.Path), item.AdjustedParameters,
                                                std::move(playlistParams)))
      , Path(item.Path)
    {}

    Playlist::Item::Data::Ptr OpenItem() const
    {
      try
      {
        CollectorStub collector(*Params);
        Provider->OpenModule(Path, collector);
        return collector.GetItem();
      }
      catch (const Error& e)
      {
        return MakePtr<StubData>(Path, *Params, e);
      }
    }

    String GetPath() const
    {
      return Path;
    }

    Parameters::Container::Ptr GetParameters() const
    {
      auto res = Parameters::Container::Create();
      Params->Process(*res);
      return res;
    }

  private:
    const Playlist::Item::DataProvider::Ptr Provider;
    const Parameters::Accessor::Ptr Params;
    const String Path;
  };

  class DelayLoadItemData : public Playlist::Item::Data
  {
  public:
    explicit DelayLoadItemData(DelayLoadItemProvider::Ptr provider)
      : Provider(std::move(provider))
    {}

    bool IsLoaded() const override
    {
      return !Provider;
    }

    // common
    Module::Holder::Ptr GetModule() const override
    {
      return AcquireDelegate().GetModule();
    }

    Binary::Data::Ptr GetModuleData() const override
    {
      return AcquireDelegate().GetModuleData();
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return AcquireDelegate().GetModuleProperties();
    }

    Parameters::Container::Ptr GetAdjustedParameters() const override
    {
      return Provider.get() ? Provider->GetParameters() : Delegate->GetAdjustedParameters();
    }

    Playlist::Item::Capabilities GetCapabilities() const override
    {
      return AcquireDelegate().GetCapabilities();
    }

    // playlist-related
    Error GetState() const override
    {
      return AcquireDelegate().GetState();
    }

    String GetFullPath() const override
    {
      return Provider.get() ? Provider->GetPath() : Delegate->GetFullPath();
    }

    String GetFilePath() const override
    {
      return AcquireDelegate().GetFilePath();
    }

    String GetType() const override
    {
      return AcquireDelegate().GetType();
    }

    String GetDisplayName() const override
    {
      return AcquireDelegate().GetDisplayName();
    }

    Time::Milliseconds GetDuration() const override
    {
      return AcquireDelegate().GetDuration();
    }

    String GetAuthor() const override
    {
      return AcquireDelegate().GetAuthor();
    }

    String GetTitle() const override
    {
      return AcquireDelegate().GetTitle();
    }

    String GetComment() const override
    {
      return AcquireDelegate().GetComment();
    }

    uint32_t GetChecksum() const override
    {
      return AcquireDelegate().GetChecksum();
    }

    uint32_t GetCoreChecksum() const override
    {
      return AcquireDelegate().GetCoreChecksum();
    }

    std::size_t GetSize() const override
    {
      return AcquireDelegate().GetSize();
    }

  private:
    const Playlist::Item::Data& AcquireDelegate() const
    {
      if (!Delegate)
      {
        Delegate = Provider->OpenItem();
        Provider.reset();
      }
      return *Delegate;
    }

  private:
    mutable DelayLoadItemProvider::Ptr Provider;
    mutable Playlist::Item::Data::Ptr Delegate;
  };

  class DelayLoadItemsIterator : public Playlist::Item::Collection
  {
  public:
    DelayLoadItemsIterator(Playlist::Item::DataProvider::Ptr provider, Parameters::Accessor::Ptr properties,
                           Playlist::IO::ContainerItems::Ptr items)
      : Provider(std::move(provider))
      , Properties(std::move(properties))
      , Items(std::move(items))
      , Current(Items->begin())
    {}

    bool IsValid() const override
    {
      return Current != Items->end();
    }

    Playlist::Item::Data::Ptr Get() const override
    {
      Require(Current != Items->end());
      DelayLoadItemProvider::Ptr provider = MakePtr<DelayLoadItemProvider>(Provider, Properties, *Current);
      return MakePtr<DelayLoadItemData>(std::move(provider));
    }

    void Next() override
    {
      Require(Current != Items->end());
      ++Current;
    }

  private:
    const Playlist::Item::DataProvider::Ptr Provider;
    const Parameters::Accessor::Ptr Properties;
    const Playlist::IO::ContainerItems::Ptr Items;
    Playlist::IO::ContainerItems::const_iterator Current;
  };

  class ContainerImpl : public Playlist::IO::Container
  {
  public:
    ContainerImpl(Playlist::Item::DataProvider::Ptr provider, Parameters::Accessor::Ptr properties,
                  Playlist::IO::ContainerItems::Ptr items)
      : Provider(std::move(provider))
      , Properties(std::move(properties))
      , Items(std::move(items))
    {}

    Parameters::Accessor::Ptr GetProperties() const override
    {
      return Properties;
    }

    unsigned GetItemsCount() const override
    {
      return static_cast<unsigned>(Items->size());
    }

    Playlist::Item::Collection::Ptr GetItems() const override
    {
      return MakePtr<DelayLoadItemsIterator>(Provider, Properties, Items);
    }

  private:
    const Playlist::Item::DataProvider::Ptr Provider;
    const Parameters::Accessor::Ptr Properties;
    const Playlist::IO::ContainerItems::Ptr Items;
  };
}  // namespace

namespace Playlist::IO
{
  Container::Ptr CreateContainer(Item::DataProvider::Ptr provider, Parameters::Accessor::Ptr properties,
                                 ContainerItems::Ptr items)
  {
    return MakePtr<ContainerImpl>(std::move(provider), std::move(properties), std::move(items));
  }
}  // namespace Playlist::IO
