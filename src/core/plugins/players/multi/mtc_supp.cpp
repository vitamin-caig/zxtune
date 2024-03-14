/**
 *
 * @file
 *
 * @brief  MTC support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/multi/multi_base.h"
#include "core/plugins/players/plugin.h"
// common includes
#include <contract.h>
#include <error.h>
#include <make_ptr.h>
// library includes
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <formats/chiptune/multidevice/multitrackcontainer.h>
#include <module/attributes.h>
#include <module/players/properties_helper.h>
#include <parameters/convert.h>
#include <parameters/merged_accessor.h>
#include <parameters/merged_container.h>
// std includes
#include <algorithm>
#include <list>
#include <utility>

namespace Module::MTC
{
  const Debug::Stream Dbg("Core::MTCSupp");

  Parameters::Accessor::Ptr CombineProps(Parameters::Accessor::Ptr first, Parameters::Accessor::Ptr second)
  {
    return first ? (second ? Parameters::CreateMergedAccessor(std::move(first), std::move(second)) : first) : second;
  }

  Parameters::Accessor::Ptr CombineProps(Parameters::Accessor::Ptr first, Parameters::Accessor::Ptr second,
                                         Parameters::Accessor::Ptr third)
  {
    return first
               ? (second
                      ? (third ? Parameters::CreateMergedAccessor(std::move(first), std::move(second), std::move(third))
                               : CombineProps(std::move(first), std::move(second)))
                      : CombineProps(std::move(first), std::move(third)))
               : CombineProps(std::move(second), std::move(third));
  }

  class DataBuilder : public Formats::Chiptune::MultiTrackContainer::Builder
  {
  public:
    DataBuilder(const Parameters::Accessor& params, Parameters::Container::Ptr props)
      : Module(params, std::move(props))
      , CurEntity(&Module)
    {}

    void SetAuthor(StringView author) override
    {
      PropertiesHelper(GetCurrentProperties()).SetAuthor(author);
    }

    void SetTitle(StringView title) override
    {
      PropertiesHelper(GetCurrentProperties()).SetTitle(title);
    }

    void SetAnnotation(StringView annotation) override
    {
      PropertiesHelper(GetCurrentProperties()).SetComment(annotation);
    }

    void SetProperty(StringView name, StringView value) override
    {
      Parameters::IntType asInt = 0;
      Parameters::StringType asString;
      auto& out = GetCurrentProperties();
      if (Parameters::ConvertFromString(value, asInt))
      {
        out.SetValue(name, asInt);
      }
      else if (const auto asData = Parameters::ConvertDataFromString(value))
      {
        out.SetValue(name, *asData);
      }
      else if (Parameters::ConvertFromString(value, asString))
      {
        out.SetValue(name, asString);
      }
    }

    void StartTrack(uint_t idx) override
    {
      Dbg("Start track {}", idx);
      CurEntity = CurTrack = Module.AddTrack(idx);
    }

    void SetData(Binary::Container::Ptr data) override
    {
      Dbg("Set track data");
      CurEntity = CurStream = CurTrack->AddStream(std::move(data));
    }

    Module::Holder::Ptr GetResult() const
    {
      return Module.GetHolder();
    }

  private:
    Parameters::Modifier& GetCurrentProperties()
    {
      return CurEntity->CreateProperties();
    }

    class TrackEntity
    {
    public:
      using Ptr = std::shared_ptr<TrackEntity>;

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
      explicit Stream(const Parameters::Accessor& params, Binary::Container::Ptr data,
                      Parameters::Accessor::Ptr tuneProperties, Parameters::Accessor::Ptr trackProperties)
        : Params(params)
        , Data(std::move(data))
        , TuneProperties(std::move(tuneProperties))
        , TrackProperties(std::move(trackProperties))
      {}

      Module::Holder::Ptr GetHolder() const override
      {
        DelayedOpenModule();
        Require(IsValid());
        return Holder;
      }

      bool operator<(const Stream& rh) const
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
          Require(Parameters::FindValue(*GetHolder()->GetModuleProperties(), Module::ATTR_TYPE, Type));
        }
        return Type;
      }

    private:
      bool IsValid() const
      {
        DelayedOpenModule();
        return !!Holder;
      }

      uint_t GetPenalty() const
      {
        const auto& type = GetType();
        if (type == "STR")
        {
          // badly emulated
          return 2;
        }
        else if (type == "AY")
        {
          // too low quality
          return 1;
        }
        else
        {
          // all is ok
          return 0;
        }
      }

      void DelayedOpenModule() const
      {
        if (Data)
        {
          auto initialProperties = CombineProps(GetProperties(), std::move(TrackProperties), std::move(TuneProperties));
          Holder = OpenModule(*Data, std::move(initialProperties));
        }
      }

      Module::Holder::Ptr OpenModule(const Binary::Container& data, Parameters::Accessor::Ptr baseProperties) const
      {
        const auto initialProperties =
            Parameters::CreateMergedContainer(std::move(baseProperties), Parameters::Container::Create());
        for (const auto& plugin : ZXTune::PlayerPlugin::Enumerate())
        {
          if (auto result = plugin->TryOpen(Params, data, initialProperties))
          {
            return result;
          }
        }
        return {};
      }

    private:
      const Parameters::Accessor& Params;
      mutable Binary::Container::Ptr Data;
      mutable Parameters::Accessor::Ptr TuneProperties;
      mutable Parameters::Accessor::Ptr TrackProperties;
      mutable Module::Holder::Ptr Holder;
      mutable String Type;
    };

    class Track : public StaticPropertiesTrackEntity
    {
    public:
      Track(const Parameters::Accessor& params, Parameters::Accessor::Ptr tuneProperties)
        : Params(params)
        , TuneProperties(std::move(tuneProperties))
      {}

      Stream* AddStream(Binary::Container::Ptr data)
      {
        Streams.emplace_back(Params, std::move(data), TuneProperties, StaticPropertiesTrackEntity::GetProperties());
        SelectedStream = nullptr;
        return &Streams.back();
      }

      Module::Holder::Ptr GetHolder() const override
      {
        return SelectStream().GetHolder();
      }

      Parameters::Accessor::Ptr GetProperties() const override
      {
        return SelectStream().GetProperties();
      }

    private:
      const Stream& SelectStream() const
      {
        if (!SelectedStream)
        {
          Dbg("Select stream from {} candidates", Streams.size());
          Require(!Streams.empty());
          SelectedStream = &*std::min_element(Streams.begin(), Streams.end());
          Dbg(" selected {}", SelectedStream->GetType());
        }
        return *SelectedStream;
      }

    private:
      const Parameters::Accessor& Params;
      const Parameters::Accessor::Ptr TuneProperties;
      std::list<Stream> Streams;
      mutable const Stream* SelectedStream = nullptr;
    };

    class Tune : public TrackEntity
    {
    public:
      Tune(const Parameters::Accessor& params, Parameters::Container::Ptr tuneProperties)
        : Params(params)
        , TuneProperties(std::move(tuneProperties))
      {}

      Track* AddTrack(uint_t idx)
      {
        Require(idx == Tracks.size());
        Tracks.emplace_back(Params, TuneProperties);
        return &Tracks.back();
      }

      Module::Holder::Ptr GetHolder() const override
      {
        const std::size_t tracksCount = Tracks.size();
        Dbg("Merge {} tracks together", tracksCount);
        Require(tracksCount > 0);
        Module::Multi::HoldersArray holders(tracksCount);
        std::transform(Tracks.begin(), Tracks.end(), holders.begin(),
                       [](const TrackEntity& entity) { return entity.GetHolder(); });
        const auto longest = std::max_element(holders.begin(), holders.end(), &CompareByDuration);
        if (longest != holders.begin())
        {
          std::iter_swap(longest, holders.begin());
        }
        return Module::Multi::CreateHolder(TuneProperties, std::move(holders));
      }

      Parameters::Accessor::Ptr GetProperties() const override
      {
        return TuneProperties;
      }

      Parameters::Modifier& CreateProperties() override
      {
        return *TuneProperties;
      }

    private:
      static bool CompareByDuration(const Module::Holder::Ptr& lh, const Module::Holder::Ptr& rh)
      {
        return lh->GetModuleInformation()->Duration() < rh->GetModuleInformation()->Duration();
      }

    private:
      const Parameters::Accessor& Params;
      Parameters::Container::Ptr TuneProperties;
      std::list<Track> Tracks;
    };

  private:
    Tune Module;
    Track* CurTrack = nullptr;
    Stream* CurStream = nullptr;
    TrackEntity* CurEntity;
  };

  class Factory : public Module::Factory
  {
  public:
    Module::Holder::Ptr CreateModule(const Parameters::Accessor& params, const Binary::Container& rawData,
                                     Parameters::Container::Ptr properties) const override
    {
      try
      {
        DataBuilder dataBuilder(params, properties);
        if (const auto container = Formats::Chiptune::MultiTrackContainer::Parse(rawData, dataBuilder))
        {
          PropertiesHelper(*properties).SetSource(*container);
          return dataBuilder.GetResult();
        }
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create MTC: {}", e.what());
      }
      return {};
    }
  };
}  // namespace Module::MTC

namespace ZXTune
{
  void RegisterMTCSupport(PlayerPluginsRegistrator& registrator)
  {
    const auto ID = "MTC"_id;
    const uint_t CAPS = Capabilities::Module::Type::MULTI | Capabilities::Module::Device::MULTI;

    auto decoder = Formats::Chiptune::CreateMultiTrackContainerDecoder();
    auto factory = MakePtr<Module::MTC::Factory>();
    auto plugin = CreatePlayerPlugin(ID, CAPS, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
