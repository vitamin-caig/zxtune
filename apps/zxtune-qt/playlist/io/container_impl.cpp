/*
Abstract:
  Playlist container internal implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "container_impl.h"
#include <apps/base/playitem.h>
//common includes
#include <error.h>
#include <logging.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  const std::string THIS_MODULE("Playlist::IO::Base");

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

    virtual void ShowProgress(unsigned /*progress*/)
    {
    }

    virtual void ShowMessage(const String& /*message*/)
    {
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
    StubData(const String& path, const Parameters::Accessor& params)
      : Path(path)
      , Params(Parameters::Container::Create())
    {
      params.Process(*Params);
    }

    //common
    virtual ZXTune::Module::Holder::Ptr GetModule() const
    {
      return ZXTune::Module::Holder::Ptr();
    }

    virtual Parameters::Container::Ptr GetAdjustedParameters() const
    {
      return Params;
    }

    //playlist-related
    virtual bool IsValid() const
    {
      return false;
    }

    virtual String GetFullPath() const
    {
      return Path;
    }

    virtual String GetType() const
    {
      return String();
    }

    virtual String GetTitle() const
    {
      return Path;
    }

    virtual Time::Milliseconds GetDuration() const
    {
      return Time::Milliseconds();
    }

    virtual String GetDurationString() const
    {
      return String();
    }

    virtual String GetTooltip() const
    {
      return Path;
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
  };

  class DelayLoadItemProvider
  {
  public:
    typedef std::auto_ptr<const DelayLoadItemProvider> Ptr;

    DelayLoadItemProvider(Playlist::Item::DataProvider::Ptr provider, Parameters::Accessor::Ptr playlistParams, const Playlist::IO::ContainerItem& item)
      : Provider(provider)
      , Params(Parameters::CreateMergedAccessor(CreatePathProperties(item.Path), item.AdjustedParameters, playlistParams))
      , Path(item.Path)
    {
    }

    Playlist::Item::Data::Ptr OpenItem() const
    {
      CollectorStub collector(*Params);
      Provider->OpenModule(Path, collector);
      return collector.GetItem();
    }

    Playlist::Item::Data::Ptr OpenStub() const
    {
      return boost::make_shared<StubData>(Path, *Params);
    }

    String GetPath() const
    {
      return Path;
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
      , Valid(true)
    {
    }

    //common
    virtual ZXTune::Module::Holder::Ptr GetModule() const
    {
      AcquireDelegate();
      const ZXTune::Module::Holder::Ptr res = Delegate->GetModule();
      Valid = res;
      return res;
    }

    virtual Parameters::Container::Ptr GetAdjustedParameters() const
    {
      AcquireDelegate();
      return Delegate->GetAdjustedParameters();
    }

    //playlist-related
    virtual bool IsValid() const
    {
      return Valid;
    }

    virtual String GetFullPath() const
    {
      return Provider.get() ? Provider->GetPath() : Delegate->GetFullPath();
    }

    virtual String GetType() const
    {
      AcquireDelegate();
      return Delegate->GetType();
    }

    virtual String GetTitle() const
    {
      AcquireDelegate();
      return Delegate->GetTitle();
    }

    virtual Time::Milliseconds GetDuration() const
    {
      AcquireDelegate();
      return Delegate->GetDuration();
    }

    virtual String GetDurationString() const
    {
      AcquireDelegate();
      return Delegate->GetDurationString();
    }

    virtual String GetTooltip() const
    {
      AcquireDelegate();
      return Delegate->GetTooltip();
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
        const String& path = Provider->GetPath();
        if (const Playlist::Item::Data::Ptr realItem = Provider->OpenItem())
        {
          Log::Debug(THIS_MODULE, "Opened '%1%'", path);
          Delegate = realItem;
        }
        else
        {
          Log::Debug(THIS_MODULE, "Failed to open '%1%'", path);
          Delegate = Provider->OpenStub();
          Valid = false;
        }
        //release unneed
        Provider.reset();
      }
    }
  private:
    mutable DelayLoadItemProvider::Ptr Provider;
    mutable bool Valid;
    mutable Playlist::Item::Data::Ptr Delegate;
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

    virtual void ForAllItems(Playlist::Item::Callback& callback) const
    {
      for (Playlist::IO::ContainerItems::const_iterator it = Items->begin(), lim = Items->end(); it != lim; ++it)
      {
        DelayLoadItemProvider::Ptr provider(new DelayLoadItemProvider(Provider, Properties, *it));
        const Playlist::Item::Data::Ptr item = boost::make_shared<DelayLoadItemData>(boost::ref(provider));
        callback.OnItem(item);
      }
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
