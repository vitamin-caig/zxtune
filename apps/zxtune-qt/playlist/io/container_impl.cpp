/**
* 
* @file
*
* @brief Playlist container internal implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "container_impl.h"
//common includes
#include <contract.h>
#include <error.h>
//library includes
#include <core/properties/path.h>
#include <debug/log.h>
#include <parameters/merged_accessor.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/ref.hpp>

namespace
{
  const Debug::Stream Dbg("Playlist::IO::Base");

  class CollectorStub : public Playlist::Item::DetectParameters
  {
  public:
    explicit CollectorStub(const Parameters::Accessor& params)
      : Params(params)
    {
    }

    virtual Parameters::Container::Ptr CreateInitialAdjustedParameters() const
    {
      const Parameters::Container::Ptr res = Parameters::Container::Create();
      Params.Process(*res);
      return res;
    }

    virtual void ProcessItem(Playlist::Item::Data::Ptr item)
    {
      assert(!Item);
      Item = item;
    }

    virtual Log::ProgressCallback* GetProgress() const
    {
      return 0;
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
    StubData(const String& path, const Parameters::Accessor& params, const Error& state)
      : Path(path)
      , Params(Parameters::Container::Create())
      , State(state)
    {
      params.Process(*Params);
    }

    //common
    virtual Module::Holder::Ptr GetModule() const
    {
      return Module::Holder::Ptr();
    }

    virtual Parameters::Container::Ptr GetAdjustedParameters() const
    {
      return Params;
    }

    virtual Playlist::Item::Capabilities GetCapabilities() const
    {
      return Playlist::Item::Capabilities(0);
    }

    //playlist-related
    virtual Error GetState() const
    {
      return State;
    }

    virtual String GetFullPath() const
    {
      return Path;
    }

    virtual String GetFilePath() const
    {
      return Path;
    }

    virtual String GetType() const
    {
      return String();
    }

    virtual String GetDisplayName() const
    {
      return Path;
    }

    virtual Time::MillisecondsDuration GetDuration() const
    {
      return Time::MillisecondsDuration();
    }

    virtual String GetAuthor() const
    {
      return String();
    }

    virtual String GetTitle() const
    {
      return String();
    }
    
    virtual String GetComment() const
    {
      return String();
    }

    virtual uint32_t GetChecksum() const
    {
      return 0;
    }

    virtual uint32_t GetCoreChecksum() const
    {
      return 0;
    }

    virtual std::size_t GetSize() const
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
    typedef std::auto_ptr<const DelayLoadItemProvider> Ptr;

    DelayLoadItemProvider(Playlist::Item::DataProvider::Ptr provider, Parameters::Accessor::Ptr playlistParams, const Playlist::IO::ContainerItem& item)
      : Provider(provider)
      , Params(Parameters::CreateMergedAccessor(Module::CreatePathProperties(item.Path), item.AdjustedParameters, playlistParams))
      , Path(item.Path)
    {
    }

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
        return boost::make_shared<StubData>(Path, *Params, e);
      }
    }

    String GetPath() const
    {
      return Path;
    }

    Parameters::Container::Ptr GetParameters() const
    {
      const Parameters::Container::Ptr res = Parameters::Container::Create();
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
      : Provider(provider)
    {
    }

    //common
    virtual Module::Holder::Ptr GetModule() const
    {
      AcquireDelegate();
      return Delegate->GetModule();
    }

    virtual Parameters::Container::Ptr GetAdjustedParameters() const
    {
      return Provider.get() ? Provider->GetParameters() : Delegate->GetAdjustedParameters();
    }

    virtual Playlist::Item::Capabilities GetCapabilities() const
    {
      AcquireDelegate();
      return Delegate->GetCapabilities();
    }

    //playlist-related
    virtual Error GetState() const
    {
      AcquireDelegate();
      return Delegate->GetState();
    }

    virtual String GetFullPath() const
    {
      return Provider.get() ? Provider->GetPath() : Delegate->GetFullPath();
    }

    virtual String GetFilePath() const
    {
      AcquireDelegate();
      return Delegate->GetFilePath();
    }

    virtual String GetType() const
    {
      AcquireDelegate();
      return Delegate->GetType();
    }

    virtual String GetDisplayName() const
    {
      AcquireDelegate();
      return Delegate->GetDisplayName();
    }

    virtual Time::MillisecondsDuration GetDuration() const
    {
      AcquireDelegate();
      return Delegate->GetDuration();
    }

    virtual String GetAuthor() const
    {
      AcquireDelegate();
      return Delegate->GetAuthor();
    }

    virtual String GetTitle() const
    {
      AcquireDelegate();
      return Delegate->GetTitle();
    }

    virtual String GetComment() const
    {
      AcquireDelegate();
      return Delegate->GetComment();
    }

    virtual uint32_t GetChecksum() const
    {
      AcquireDelegate();
      return Delegate->GetChecksum();
    }

    virtual uint32_t GetCoreChecksum() const
    {
      AcquireDelegate();
      return Delegate->GetCoreChecksum();
    }

    virtual std::size_t GetSize() const
    {
      AcquireDelegate();
      return Delegate->GetSize();
    }
  private:
    void AcquireDelegate() const
    {
      if (!Delegate)
      {
        Delegate = Provider->OpenItem();
        Provider.reset();
      }
    }
  private:
    mutable DelayLoadItemProvider::Ptr Provider;
    mutable Playlist::Item::Data::Ptr Delegate;
  };

  class DelayLoadItemsIterator : public Playlist::Item::Collection
  {
  public:
    DelayLoadItemsIterator(Playlist::Item::DataProvider::Ptr provider,
      Parameters::Accessor::Ptr properties, Playlist::IO::ContainerItemsPtr items)
      : Provider(provider)
      , Properties(properties)
      , Items(items)
      , Current(Items->begin())
    {
    }

    virtual bool IsValid() const
    {
      return Current != Items->end();
    }

    virtual Playlist::Item::Data::Ptr Get() const
    {
      Require(Current != Items->end());
      DelayLoadItemProvider::Ptr provider(new DelayLoadItemProvider(Provider, Properties, *Current));
      return boost::make_shared<DelayLoadItemData>(boost::ref(provider));
    }

    virtual void Next()
    {
      Require(Current != Items->end());
      ++Current;
    }
  private:
    const Playlist::Item::DataProvider::Ptr Provider;
    const Parameters::Accessor::Ptr Properties;
    const Playlist::IO::ContainerItemsPtr Items;
    Playlist::IO::ContainerItems::const_iterator Current;
  };

  class ContainerImpl : public Playlist::IO::Container
  {
  public:
    ContainerImpl(Playlist::Item::DataProvider::Ptr provider,
      Parameters::Accessor::Ptr properties,
      Playlist::IO::ContainerItemsPtr items)
      : Provider(provider)
      , Properties(properties)
      , Items(items)
    {
    }

    virtual Parameters::Accessor::Ptr GetProperties() const
    {
      return Properties;
    }

    virtual unsigned GetItemsCount() const
    {
      return static_cast<unsigned>(Items->size());
    }

    virtual Playlist::Item::Collection::Ptr GetItems() const
    {
      return boost::make_shared<DelayLoadItemsIterator>(Provider, Properties, Items);
    }
  private:
    const Playlist::Item::DataProvider::Ptr Provider;
    const Parameters::Accessor::Ptr Properties;
    const Playlist::IO::ContainerItemsPtr Items;
  };
}

namespace Playlist
{
  namespace IO
  {
    Container::Ptr CreateContainer(Item::DataProvider::Ptr provider,
      Parameters::Accessor::Ptr properties,
      ContainerItemsPtr items)
    {
      return boost::make_shared<ContainerImpl>(provider, properties, items);
    }
  }
}
