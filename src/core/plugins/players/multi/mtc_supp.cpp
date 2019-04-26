/**
* 
* @file
*
* @brief  MTC support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "multi_base.h"
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
//common includes
#include <contract.h>
#include <error.h>
#include <make_ptr.h>
//library includes
#include <core/module_open.h>
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <formats/chiptune/multidevice/multitrackcontainer.h>
#include <module/attributes.h>
#include <module/players/properties_helper.h>
#include <parameters/merged_accessor.h>
#include <parameters/serialize.h>
#include <parameters/tools.h>
//std includes
#include <algorithm>
#include <list>

namespace Module
{
namespace MTC
{
  const Debug::Stream Dbg("Core::MTCSupp");

  Parameters::Accessor::Ptr CombineProps(Parameters::Accessor::Ptr first, Parameters::Accessor::Ptr second)
  {
    return first
           ? (second
             ? Parameters::CreateMergedAccessor(first, second)
             : first)
           : second;
  }

  Parameters::Accessor::Ptr CombineProps(Parameters::Accessor::Ptr first, Parameters::Accessor::Ptr second, Parameters::Accessor::Ptr third)
  {
    return first
         ? (second
           ? (third
             ? Parameters::CreateMergedAccessor(first, second, third)
             : CombineProps(first, second))
           : CombineProps(first, third))
         : CombineProps(second, third);
  }
  
  
  class DataBuilder : public Formats::Chiptune::MultiTrackContainer::Builder
  {
  public:
    DataBuilder(const Parameters::Accessor& params, Parameters::Container::Ptr props)
      : Module(params, props)
      , CurTrack()
      , CurStream()
      , CurEntity(&Module)
    {
    }

    void SetAuthor(const String& author) override
    {
      PropertiesHelper(GetCurrentProperties()).SetAuthor(author);
    }
    
    void SetTitle(const String& title) override
    {
      PropertiesHelper(GetCurrentProperties()).SetTitle(title);
    }
    
    void SetAnnotation(const String& annotation) override
    {
      PropertiesHelper(GetCurrentProperties()).SetComment(annotation);
    }
    
    void SetProperty(const String& name, const String& value) override
    {
      Strings::Map props;
      props[name] = value;
      Parameters::Convert(props, GetCurrentProperties());
    }
    
    void StartTrack(uint_t idx) override
    {
      Dbg("Start track %1%", idx);
      CurEntity = CurTrack = Module.AddTrack(idx);
    }
    
    void SetData(Binary::Container::Ptr data) override
    {
      Dbg("Set track data");
      CurEntity = CurStream = CurTrack->AddStream(data);
    }

    Module::Holder::Ptr GetResult() const
    {
      return Module.GetHolder();
    }
  private:
    class TrackEntity
    {
    public:
      typedef std::shared_ptr<TrackEntity> Ptr;
      
      virtual ~TrackEntity() = default;
      
      virtual Module::Holder::Ptr GetHolder() const = 0;

      virtual Parameters::Accessor::Ptr GetProperties() const = 0;
      
      virtual Parameters::Modifier& CreateProperties() = 0;
    };
    
    class StaticPropertiesTrackEntity : public TrackEntity
    {
    public:
      Parameters::Accessor::Ptr GetProperties() const override
      {
        return Props;
      }
      
      Parameters::Modifier& CreateProperties() override
      {
        if (!Props)
        {
          Props = Parameters::Container::Create();
        }
        return *Props;
      }
    private:
      Parameters::Container::Ptr Props;
    };
    
    class Stream : public StaticPropertiesTrackEntity
    {
    public:
      explicit Stream(Module::Holder::Ptr holder)
        : Holder(std::move(holder))
      {
      }
      
      Module::Holder::Ptr GetHolder() const override
      {
        Require(IsValid());
        return Holder;
      }

      bool operator < (const Stream& rh) const
      {
        const bool isValid = IsValid();
        if (isValid != rh.IsValid())
        {
          return isValid;
        }
        else
        {
          return GetPenalty() < rh.GetPenalty();
        }
      }

      String GetType() const
      {
        if (Type.empty())
        {
          Require(Holder->GetModuleProperties()->FindValue(Module::ATTR_TYPE, Type));
        }
        return Type;
      }
    private:
      bool IsValid() const
      {
        return !!Holder;
      }

      uint_t GetPenalty() const
      {
        const String& type = GetType();
        if (type == "STR")
        {
          //badly emulated
          return 2;
        }
        else if (type == "AY")
        {
          //too low quality
          return 1;
        }
        else
        {
          //all is ok
          return 0;
        }
      }
    private:
      const Module::Holder::Ptr Holder;
      mutable String Type;
    };
    
    class Track : public StaticPropertiesTrackEntity
    {
    public:      
      explicit Track(const Parameters::Accessor& params)
        : Params(params)
        , SelectedStream()
      {
      }
      
      Stream* AddStream(Binary::Container::Ptr data)
      {
        Streams.push_back(Stream(OpenModule(data)));
        SelectedStream = nullptr;
        return &Streams.back();
      }
    
      Module::Holder::Ptr GetHolder() const override
      {
        //each track requires its own properties to create renderer (e.g. notetable)
        const Stream& stream = SelectStream();
        const Module::Holder::Ptr holder = stream.GetHolder();
        const Parameters::Accessor::Ptr props = CombineProps(holder->GetModuleProperties(), stream.GetProperties(), StaticPropertiesTrackEntity::GetProperties());
        return Module::CreateMixedPropertiesHolder(holder, props);
      }
      
      Parameters::Accessor::Ptr GetProperties() const override
      {
        const Stream& stream = SelectStream();
        return CombineProps(stream.GetProperties(), StaticPropertiesTrackEntity::GetProperties());
      }
    private:  
      Module::Holder::Ptr OpenModule(Binary::Container::Ptr data) const
      {
        try
        {
          return Module::Open(Params, *data);
        }
        catch (const Error&/*ignored*/)
        {
          return Module::Holder::Ptr();
        }
      }
      
      const Stream& SelectStream() const
      {
        if (!SelectedStream)
        {
          Dbg("Select stream from %1% candiates", Streams.size());
          Require(!Streams.empty());
          SelectedStream = &*std::min_element(Streams.begin(), Streams.end());
          Dbg(" selected %1%", SelectedStream->GetType());
        }
        return *SelectedStream;
      }
    private:
      const Parameters::Accessor& Params;
      std::list<Stream> Streams;
      mutable const Stream* SelectedStream;
    };
    
    class Tune : public TrackEntity
    {
    public:
      Tune(const Parameters::Accessor& params, Parameters::Container::Ptr properties)
        : Params(params)
        , Props(std::move(properties))
      {
      }
      
      Track* AddTrack(uint_t idx)
      {
        Require(idx == Tracks.size());
        Tracks.push_back(Track(Params));
        return &Tracks.back();
      }
      
      Module::Holder::Ptr GetHolder() const override
      {
        const std::size_t tracksCount = Tracks.size();
        Dbg("Merge %1% tracks together", tracksCount);
        Require(tracksCount > 0);
        Module::Multi::HoldersArray holders(tracksCount);
        std::transform(Tracks.begin(), Tracks.end(), holders.begin(),
            [](const TrackEntity& entity) {return entity.GetHolder();});
        const auto longest = std::max_element(holders.begin(), holders.end(), &CompareByDuration);
        MergeAbsentMetadata(*(*longest)->GetModuleProperties());
        if (longest != holders.begin())
        {
          std::iter_swap(longest, holders.begin());
        }
        return Module::Multi::CreateHolder(GetProperties(), holders);
      }

      Parameters::Accessor::Ptr GetProperties() const override
      {
        return Props;
      }
      
      Parameters::Modifier& CreateProperties() override
      {
        return *Props;
      }
    private:
      static bool CompareByDuration(Module::Holder::Ptr lh, Module::Holder::Ptr rh)
      {
        return lh->GetModuleInformation()->FramesCount() < rh->GetModuleInformation()->FramesCount();
      }
      
      void MergeAbsentMetadata(const Parameters::Accessor& toMerge) const
      {
        String title, author, comment;
        if (Props->FindValue(Module::ATTR_TITLE, title)
         || Props->FindValue(Module::ATTR_AUTHOR, author)
         || Props->FindValue(Module::ATTR_COMMENT, comment))
        {
          return;
        }
        Dbg("No existing metadata found. Merge from longest track.");
        Parameters::CopyExistingValue<Parameters::StringType>(toMerge, *Props, Module::ATTR_TITLE);
        Parameters::CopyExistingValue<Parameters::StringType>(toMerge, *Props, Module::ATTR_AUTHOR);
        Parameters::CopyExistingValue<Parameters::StringType>(toMerge, *Props, Module::ATTR_COMMENT);
      }
    private:
      const Parameters::Accessor& Params;
      Parameters::Container::Ptr Props;
      std::list<Track> Tracks;
    };
  private:
    Parameters::Modifier& GetCurrentProperties()
    {
      return CurEntity->CreateProperties();
    }
  private:
    Tune Module;
    Track* CurTrack;
    Stream* CurStream;
    TrackEntity* CurEntity;
  };
  
  class Factory : public Module::Factory
  {
  public:
    Module::Holder::Ptr CreateModule(const Parameters::Accessor& params, const Binary::Container& rawData, Parameters::Container::Ptr properties) const override
    {
      try
      {
        DataBuilder dataBuilder(params, properties);
        if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::MultiTrackContainer::Parse(rawData, dataBuilder))
        {
          PropertiesHelper(*properties).SetSource(*container);
          return dataBuilder.GetResult();
        }
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create MTC: %s", e.what());
      }
      return Module::Holder::Ptr();
    }
  };
}
}

namespace ZXTune
{
  void RegisterMTCSupport(PlayerPluginsRegistrator& registrator)
  {
    const Char ID[] = {'M', 'T', 'C', 0};
    const uint_t CAPS = Capabilities::Module::Type::MULTI | Capabilities::Module::Device::MULTI;

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateMultiTrackContainerDecoder();
    const Module::MTC::Factory::Ptr factory = MakePtr<Module::MTC::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
