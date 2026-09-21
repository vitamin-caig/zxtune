/**
 *
 * @file
 *
 * @brief  MTC support plugin implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/multitrack/mtc.h"

#include "formats/chiptune/multidevice/multitrackcontainer.h"
#include "module/players/multitrack/multi.h"
#include "module/players/properties_helper.h"

#include "debug/log.h"
#include "module/attributes.h"
#include "parameters/merged_accessor.h"
#include "parameters/merged_container.h"
#include "parameters/serialize.h"

#include "contract.h"
#include "error.h"
#include "make_ptr.h"
#include "string_view.h"

#include <algorithm>
#include <list>
#include <utility>

namespace Module::MTC
{
  const Debug::Stream Dbg("Module::MTC");

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
    DataBuilder(const Parameters::Accessor& params, Parameters::Container::Ptr props, const Factory& delegate)
      : Module(params, std::move(props), delegate)
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
      Parameters::Convert(name, value, GetCurrentProperties());
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

    Holder::Ptr GetResult() const
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
      virtual ~TrackEntity() = default;

      virtual Holder::Ptr GetHolder() const = 0;

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
      Stream(const Parameters::Accessor& params, Binary::Container::Ptr data, Parameters::Accessor::Ptr tuneProperties,
             Parameters::Accessor::Ptr trackProperties, const Factory& delegate)
        : Params(params)
        , Data(std::move(data))
        , TuneProperties(std::move(tuneProperties))
        , TrackProperties(std::move(trackProperties))
        , Delegate(delegate)
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
          Require(Parameters::FindValue(*GetHolder()->GetModuleProperties(), ATTR_TYPE, Type));
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
          auto openProperties =
              Parameters::CreateMergedContainer(std::move(initialProperties), Parameters::Container::Create());
          Holder = Delegate.CreateModule(Params, *Data, std::move(openProperties));
        }
      }

    private:
      const Parameters::Accessor& Params;
      mutable Binary::Container::Ptr Data;
      mutable Parameters::Accessor::Ptr TuneProperties;
      mutable Parameters::Accessor::Ptr TrackProperties;
      const Factory& Delegate;
      mutable Module::Holder::Ptr Holder;
      mutable String Type;
    };

    class Track : public StaticPropertiesTrackEntity
    {
    public:
      Track(const Parameters::Accessor& params, Parameters::Accessor::Ptr tuneProperties, const Factory& delegate)
        : Params(params)
        , TuneProperties(std::move(tuneProperties))
        , Delegate(delegate)
      {}

      Stream* AddStream(Binary::Container::Ptr data)
      {
        Streams.emplace_back(Params, std::move(data), TuneProperties, StaticPropertiesTrackEntity::GetProperties(),
                             Delegate);
        SelectedStream = nullptr;
        return &Streams.back();
      }

      Holder::Ptr GetHolder() const override
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
      const Factory& Delegate;
      std::list<Stream> Streams;
      mutable const Stream* SelectedStream = nullptr;
    };

    class Tune : public TrackEntity
    {
    public:
      Tune(const Parameters::Accessor& params, Parameters::Container::Ptr tuneProperties, const Factory& delegate)
        : Params(params)
        , TuneProperties(std::move(tuneProperties))
        , Delegate(delegate)
      {}

      Track* AddTrack(uint_t idx)
      {
        Require(idx == Tracks.size());
        Tracks.emplace_back(Params, TuneProperties, Delegate);
        return &Tracks.back();
      }

      Holder::Ptr GetHolder() const override
      {
        const std::size_t tracksCount = Tracks.size();
        Dbg("Merge {} tracks together", tracksCount);
        Require(tracksCount > 0);
        Multi::HoldersArray holders(tracksCount);
        std::transform(Tracks.begin(), Tracks.end(), holders.begin(),
                       [](const TrackEntity& entity) { return entity.GetHolder(); });
        const auto longest = std::max_element(holders.begin(), holders.end(), &CompareByDuration);
        if (longest != holders.begin())
        {
          std::iter_swap(longest, holders.begin());
        }
        return Multi::CreateHolder(TuneProperties, std::move(holders));
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
      static bool CompareByDuration(const Holder::Ptr& lh, const Holder::Ptr& rh)
      {
        return lh->GetModuleInformation().Duration < rh->GetModuleInformation().Duration;
      }

    private:
      const Parameters::Accessor& Params;
      Parameters::Container::Ptr TuneProperties;
      const Factory& Delegate;
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
    explicit Factory(Ptr delegate)
      : Delegate(std::move(delegate))
    {}

    Holder::Ptr CreateModule(const Parameters::Accessor& params, const Binary::Container& rawData,
                             Parameters::Container::Ptr properties) const override
    {
      try
      {
        DataBuilder dataBuilder(params, properties, *Delegate);
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

  private:
    const Ptr Delegate;
  };

  Factory::Ptr CreateFactory(Factory::Ptr delegate)
  {
    return MakePtr<Factory>(std::move(delegate));
  }
}  // namespace Module::MTC
