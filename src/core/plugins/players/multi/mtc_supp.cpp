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
#include "core/plugins/registrator.h"
#include "core/plugins/players/plugin.h"
//common includes
#include <contract.h>
#include <error.h>
//library includes
#include <core/module_attrs.h>
#include <core/module_open.h>
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/multi/multitrackcontainer.h>
#include <parameters/merged_accessor.h>
#include <parameters/serialize.h>
#include <parameters/tools.h>
//std includes
#include <list>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  const Debug::Stream Dbg("Core::MTCSupp");
}

namespace Module
{
namespace MTC
{
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
    explicit DataBuilder(PropertiesBuilder& propBuilder)
      : Module(propBuilder)
      , CurTrack()
      , CurStream()
      , CurEntity(&Module)
    {
    }

    virtual void SetAuthor(const String& author)
    {
      GetCurrentProperties().SetAuthor(author);
    }
    
    virtual void SetTitle(const String& title)
    {
      GetCurrentProperties().SetTitle(title);
    }
    
    virtual void SetAnnotation(const String& annotation)
    {
      GetCurrentProperties().SetComment(annotation);
    }
    
    virtual void SetProperty(const String& name, const String& value)
    {
      Strings::Map props;
      props[name] = value;
      Parameters::Convert(props, GetCurrentProperties());
    }
    
    virtual void StartTrack(uint_t idx)
    {
      Dbg("Start track %1%", idx);
      CurEntity = CurTrack = Module.AddTrack(idx);
    }
    
    virtual void SetData(Binary::Container::Ptr data)
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
      typedef boost::shared_ptr<TrackEntity> Ptr;
      
      virtual ~TrackEntity() {}
      
      virtual Module::Holder::Ptr GetHolder() const = 0;

      virtual Parameters::Accessor::Ptr GetProperties() const = 0;
      virtual Module::PropertiesBuilder& GetPropertiesBuilder() const = 0;
    };
    
    class StaticPropertiesTrackEntity : public TrackEntity
    {
    public:
      virtual Parameters::Accessor::Ptr GetProperties() const
      {
        return Props ? Props->GetResult() : Parameters::Accessor::Ptr();
      }
      
      virtual Module::PropertiesBuilder& GetPropertiesBuilder() const
      {
        if (!Props)
        {
          Props = boost::make_shared<Module::PropertiesBuilder>();
        }
        return *Props;
      }
    private:
      mutable boost::shared_ptr<Module::PropertiesBuilder> Props;
    };
    
    class Stream : public StaticPropertiesTrackEntity
    {
    public:
      explicit Stream(Module::Holder::Ptr holder)
        : Holder(holder)
      {
      }
      
      virtual Module::Holder::Ptr GetHolder() const
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
      Track()
        : SelectedStream()
      {
      }
      
      Stream* AddStream(Binary::Container::Ptr data)
      {
        Streams.push_back(Stream(OpenModule(data)));
        SelectedStream = 0;
        return &Streams.back();
      }
    
      virtual Module::Holder::Ptr GetHolder() const
      {
        //each track requires its own properties to create renderer (e.g. notetable)
        const Stream& stream = SelectStream();
        const Module::Holder::Ptr holder = stream.GetHolder();
        const Parameters::Accessor::Ptr props = CombineProps(holder->GetModuleProperties(), stream.GetProperties(), StaticPropertiesTrackEntity::GetProperties());
        return Module::CreateMixedPropertiesHolder(holder, props);
      }
      
      virtual Parameters::Accessor::Ptr GetProperties() const
      {
        const Stream& stream = SelectStream();
        return CombineProps(stream.GetProperties(), StaticPropertiesTrackEntity::GetProperties());
      }
    private:  
      static Module::Holder::Ptr OpenModule(Binary::Container::Ptr data)
      {
        try
        {
          return Module::Open(*data);
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
      std::list<Stream> Streams;
      mutable const Stream* SelectedStream;
    };
    
    class Tune : public TrackEntity
    {
    public:
      explicit Tune(Module::PropertiesBuilder& props)
        : Props(props)
      {
      }
      
      Track* AddTrack(uint_t idx)
      {
        Require(idx == Tracks.size());
        Tracks.push_back(Track());
        return &Tracks.back();
      }
      
      virtual Module::Holder::Ptr GetHolder() const
      {
        const std::size_t tracksCount = Tracks.size();
        Dbg("Merge %1% tracks together", tracksCount);
        Require(tracksCount > 0);
        Module::Multi::HoldersArray holders(tracksCount);
        std::transform(Tracks.begin(), Tracks.end(), holders.begin(), std::mem_fun_ref(&TrackEntity::GetHolder));
        const Module::Multi::HoldersArray::iterator longest = std::max_element(holders.begin(), holders.end(), &CompareByDuration);
        MergeAbsentMetadata(*(*longest)->GetModuleProperties());
        if (longest != holders.begin())
        {
          std::iter_swap(longest, holders.begin());
        }
        return Module::Multi::CreateHolder(GetProperties(), holders);
      }

      virtual Parameters::Accessor::Ptr GetProperties() const
      {
        return Props.GetResult();
      }
      
      virtual Module::PropertiesBuilder& GetPropertiesBuilder() const
      {
        return Props;
      }
    private:
      static bool CompareByDuration(Module::Holder::Ptr lh, Module::Holder::Ptr rh)
      {
        return lh->GetModuleInformation()->FramesCount() < rh->GetModuleInformation()->FramesCount();
      }
      
      void MergeAbsentMetadata(const Parameters::Accessor& toMerge) const
      {
        const Parameters::Accessor::Ptr cur = Props.GetResult();
        String title, author, comment;
        if (cur->FindValue(Module::ATTR_TITLE, title)
         || cur->FindValue(Module::ATTR_AUTHOR, author)
         || cur->FindValue(Module::ATTR_COMMENT, comment))
        {
          return;
        }
        Dbg("No existing metadata found. Merge from longest track.");
        Parameters::CopyExistingValue<Parameters::StringType>(toMerge, Props, Module::ATTR_TITLE);
        Parameters::CopyExistingValue<Parameters::StringType>(toMerge, Props, Module::ATTR_AUTHOR);
        Parameters::CopyExistingValue<Parameters::StringType>(toMerge, Props, Module::ATTR_COMMENT);
      }
    private:
      Module::PropertiesBuilder& Props;
      std::list<Track> Tracks;
    };
  private:
    Module::PropertiesBuilder& GetCurrentProperties()
    {
      return CurEntity->GetPropertiesBuilder();
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
    virtual Module::Holder::Ptr CreateModule(PropertiesBuilder& propBuilder, const Binary::Container& rawData) const
    {
      try
      {
        DataBuilder dataBuilder(propBuilder);
        if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::MultiTrackContainer::Parse(rawData, dataBuilder))
        {
          propBuilder.SetSource(*container);
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
    const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_MULTI | CAP_CONV_RAW;

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateMultiTrackContainerDecoder();
    const Module::MTC::Factory::Ptr factory = boost::make_shared<Module::MTC::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
