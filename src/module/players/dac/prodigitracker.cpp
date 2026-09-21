/**
 *
 * @file
 *
 * @brief  ProDigiTracker chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/digital/prodigitracker.h"

#include "devices/dac/sample_factories.h"
#include "module/players/dac/dac_properties_helper.h"
#include "module/players/dac/dac_simple.h"
#include "module/players/properties_meta.h"
#include "module/players/simple_orderlist.h"
#include "module/players/tracking.h"

#include "binary/container.h"
#include "parameters/container.h"

#include "make_ptr.h"

#include <array>

namespace Module::ProDigiTracker
{
  const uint_t CHANNELS_COUNT = 4;

  const uint64_t Z80_FREQ = 3500000;
  const uint_t TICKS_PER_CYCLE = 374;
  const uint_t C_1_STEP = 46;
  const uint_t SAMPLES_FREQ = Z80_FREQ * C_1_STEP / TICKS_PER_CYCLE / 256;

  using Formats::Chiptune::ProDigiTracker::Ornament;

  class ModuleData : public DAC::SimpleModuleData
  {
  public:
    using Ptr = std::shared_ptr<const ModuleData>;
    using RWPtr = std::shared_ptr<ModuleData>;

    ModuleData()
      : DAC::SimpleModuleData(CHANNELS_COUNT)
    {}

    SparsedObjectsStorage<Ornament> Ornaments;
  };

  class DataBuilder : public Formats::Chiptune::ProDigiTracker::Builder
  {
  public:
    explicit DataBuilder(DAC::PropertiesHelper& props)
      : Properties(props)
      , Meta(props)
      , Data(MakeRWPtr<ModuleData>())
    {
      Properties.SetSamplesFrequency(SAMPLES_FREQ);
    }

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetInitialTempo(uint_t tempo) override
    {
      Data->InitialTempo = tempo;
    }

    void SetSample(uint_t index, std::size_t loop, Binary::View sample) override
    {
      Data->Samples.Add(index, Devices::DAC::CreateU8Sample(sample, loop));
    }

    void SetOrnament(uint_t index, Ornament ornament) override
    {
      Data->Ornaments.Add(index, std::move(ornament));
    }

    void SetPositions(Formats::Chiptune::ProDigiTracker::Positions positions) override
    {
      Data->Order = MakePtr<SimpleOrderList>(positions.Loop, std::move(positions.Lines));
    }

    Formats::Chiptune::PatternBuilder& StartPattern(uint_t index) override
    {
      Patterns.SetPattern(index);
      return Patterns;
    }

    void StartChannel(uint_t index) override
    {
      Patterns.SetChannel(index);
    }

    void SetRest() override
    {
      Patterns.GetChannel().SetEnabled(false);
    }

    void SetNote(uint_t note) override
    {
      Patterns.GetChannel().SetEnabled(true);
      Patterns.GetChannel().SetNote(note);
    }

    void SetSample(uint_t sample) override
    {
      Patterns.GetChannel().SetSample(sample);
    }

    void SetOrnament(uint_t ornament) override
    {
      Patterns.GetChannel().SetOrnament(ornament);
    }

    ModuleData::Ptr CaptureResult()
    {
      Data->Patterns = Patterns.CaptureResult();
      return std::move(Data);
    }

  private:
    DAC::PropertiesHelper& Properties;
    MetaProperties Meta;
    PatternsBuilder Patterns;
    ModuleData::RWPtr Data;
  };

  struct OrnamentState
  {
    OrnamentState() = default;
    const Ornament* Object = nullptr;
    std::size_t Position = 0;

    int_t GetOffset() const
    {
      return Object ? Object->GetLine(Position) : 0;
    }

    void Update()
    {
      if (Object && Position++ >= Object->GetSize())
      {
        Position = Object->GetLoop();
      }
    }
  };

  class DataRenderer : public DAC::DataRenderer
  {
  public:
    explicit DataRenderer(ModuleData::Ptr data)
      : Data(std::move(data))
    {
      Reset();
    }

    void Reset() override
    {
      std::fill(Ornaments.begin(), Ornaments.end(), OrnamentState());
    }

    void SynthesizeData(const TrackState& state, DAC::TrackBuilder& track) override
    {
      SynthesizeChannelsData(track);
      if (0 == state.Quirk)
      {
        GetNewLineState(state, track);
      }
    }

  private:
    void SynthesizeChannelsData(DAC::TrackBuilder& track)
    {
      for (uint_t chan = 0; chan != CHANNELS_COUNT; ++chan)
      {
        OrnamentState& ornament = Ornaments[chan];
        ornament.Update();
        DAC::ChannelDataBuilder builder = track.GetChannel(chan);
        builder.SetNoteSlide(ornament.GetOffset());
      }
    }

    void GetNewLineState(const TrackState& state, DAC::TrackBuilder& track)
    {
      if (const auto* const line = Data->GetLine(state))
      {
        line->ForEachChannel(
            [&](auto chan, const auto& src) { GetNewChannelState(src, Ornaments[chan], track.GetChannel(chan)); });
      }
    }

    void GetNewChannelState(const Cell& src, OrnamentState& ornamentState, DAC::ChannelDataBuilder builder)
    {
      if (const bool* enabled = src.GetEnabled())
      {
        builder.SetEnabled(*enabled);
        if (!*enabled)
        {
          builder.SetPosInSample(0);
        }
      }

      if (const uint_t* note = src.GetNote())
      {
        if (const uint_t* ornament = src.GetOrnament())
        {
          ornamentState.Object = &Data->Ornaments.Get(*ornament);
          ornamentState.Position = 0;
          builder.SetNoteSlide(ornamentState.GetOffset());
        }
        if (const uint_t* sample = src.GetSample())
        {
          builder.SetSampleNum(*sample);
        }
        builder.SetNote(*note);
        builder.SetPosInSample(0);
      }
    }

  private:
    const ModuleData::Ptr Data;
    std::array<OrnamentState, CHANNELS_COUNT> Ornaments;
  };

}  // namespace Module::ProDigiTracker

namespace Module::DAC
{
  Chiptune::Ptr CreateProDigiTrackerChiptune(const Binary::Container& rawData, Parameters::Container::Ptr properties)
  {
    PropertiesHelper props(*properties, ProDigiTracker::CHANNELS_COUNT);
    ProDigiTracker::DataBuilder dataBuilder(props);
    if (const auto container = Formats::Chiptune::ProDigiTracker::Parse(rawData, dataBuilder))
    {
      props.SetSource(*container);
      return CreateTrackingChiptune<ProDigiTracker::DataRenderer>(dataBuilder.CaptureResult(), std::move(properties));
    }
    return {};
  }
}  // namespace Module::DAC
