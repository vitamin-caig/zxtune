/*
Abstract:
  PDT modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "dac_base.h"
#include "core/plugins/registrator.h"
#include "core/plugins/players/creation_result.h"
#include "core/plugins/players/module_properties.h"
#include "core/plugins/players/simple_orderlist.h"
#include "core/plugins/players/simple_ornament.h"
#include "core/plugins/players/tracking.h"
//common includes
#include <tools.h>
//library includes
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <devices/dac_sample_factories.h>
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/digital/prodigitracker.h>
#include <sound/mixer_factory.h>
//boost includes
#include <boost/bind.hpp>
//text includes
#include <formats/text/chiptune.h>

#define FILE_TAG 30E3A543

namespace ProDigiTracker
{
  const uint_t CHANNELS_COUNT = 4;
  //all samples has base freq at 2kHz (C-1)
  const uint_t BASE_FREQ = 2000;

  using namespace ZXTune;
  using namespace ZXTune::Module;

  typedef SimpleOrnament Ornament;

  class ModuleData : public TrackModel
  {
  public:
    typedef boost::shared_ptr<ModuleData> RWPtr;
    typedef boost::shared_ptr<const ModuleData> Ptr;

    ModuleData()
      : InitialTempo()
    {
    }

    virtual uint_t GetInitialTempo() const
    {
      return InitialTempo;
    }

    virtual const OrderList& GetOrder() const
    {
      return *Order;
    }

    virtual const PatternsSet& GetPatterns() const
    {
      return *Patterns;
    }

    uint_t InitialTempo;
    OrderList::Ptr Order;
    PatternsSet::Ptr Patterns;
    SparsedObjectsStorage<Devices::DAC::Sample::Ptr> Samples;
    SparsedObjectsStorage<Ornament> Ornaments;
  };

  std::auto_ptr<Formats::Chiptune::ProDigiTracker::Builder> CreateDataBuilder(ModuleData::RWPtr data, ModuleProperties::RWPtr props);
}

namespace ProDigiTracker
{
  class DataBuilder : public Formats::Chiptune::ProDigiTracker::Builder
  {
  public:
    DataBuilder(ModuleData::RWPtr data, ModuleProperties::RWPtr props)
      : Data(data)
      , Properties(props)
      , Builder(PatternsBuilder::Create<ProDigiTracker::CHANNELS_COUNT>())
    {
      Data->Patterns = Builder.GetPatterns();
    }

    virtual Formats::Chiptune::MetaBuilder& GetMetaBuilder()
    {
      return *Properties;
    }

    virtual void SetInitialTempo(uint_t tempo)
    {
      Data->InitialTempo = tempo;
    }

    virtual void SetSample(uint_t index, std::size_t loop, Binary::Data::Ptr sample)
    {
      Data->Samples.Add(index, Devices::DAC::CreateU8Sample(sample, loop));
    }

    virtual void SetOrnament(uint_t index, std::size_t loop, const std::vector<int_t>& ornament)
    {
      Data->Ornaments.Add(index, Ornament(loop, ornament.begin(), ornament.end()));
    }

    virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop)
    {
      Data->Order = boost::make_shared<SimpleOrderList>(loop, positions.begin(), positions.end());
    }

    virtual Formats::Chiptune::PatternBuilder& StartPattern(uint_t index)
    {
      Builder.SetPattern(index);
      return Builder;
    }

    virtual void StartChannel(uint_t index)
    {
      Builder.SetChannel(index);
    }

    virtual void SetRest()
    {
      Builder.GetChannel().SetEnabled(false);
    }

    virtual void SetNote(uint_t note)
    {
      Builder.GetChannel().SetEnabled(true);
      Builder.GetChannel().SetNote(note);
    }

    virtual void SetSample(uint_t sample)
    {
      Builder.GetChannel().SetSample(sample);
    }

    virtual void SetOrnament(uint_t ornament)
    {
      Builder.GetChannel().SetOrnament(ornament);
    }
  private:
    const ModuleData::RWPtr Data;
    const ModuleProperties::RWPtr Properties;
    PatternsBuilder Builder;
  };

  Renderer::Ptr CreateModuleRenderer(Parameters::Accessor::Ptr params, ModuleData::Ptr data, Devices::DAC::Chip::Ptr device);

  class ModuleHolder : public Holder
  {
  public:
    ModuleHolder(ModuleData::Ptr data, ModuleProperties::Ptr properties)
      : Data(data)
      , Properties(properties)
      , Info(CreateTrackInfo(Data, CHANNELS_COUNT))
    {   
    }

    virtual Information::Ptr GetModuleInformation() const
    {
      return Info;
    }

    virtual Parameters::Accessor::Ptr GetModuleProperties() const
    {
      return Properties;
    }

    virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const
    {
      const Sound::FourChannelsMixer::Ptr mixer = Sound::CreateFourChannelsMixer(params);
      mixer->SetTarget(target);
      const Devices::DAC::Receiver::Ptr receiver = DAC::CreateReceiver(mixer);
      const Devices::DAC::ChipParameters::Ptr chipParams = DAC::CreateChipParameters(params);
      const Devices::DAC::Chip::Ptr chip(Devices::DAC::CreateChip(CHANNELS_COUNT, BASE_FREQ, chipParams, receiver));
      for (uint_t idx = 0, lim = Data->Samples.Size(); idx != lim; ++idx)
      {
        chip->SetSample(idx, Data->Samples.Get(idx));
      }
      return CreateModuleRenderer(params, Data, chip);
    }
  private:
    const ModuleData::Ptr Data;
    const ModuleProperties::Ptr Properties;
    const Information::Ptr Info;
  };

  class ModuleRenderer : public Renderer
  {
    struct OrnamentState
    {
      OrnamentState() : Object(), Position()
      {
      }
      const Ornament* Object;
      std::size_t Position;

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

      void Reset()
      {
        Position = 0;
      }
    };
  public:
    ModuleRenderer(Parameters::Accessor::Ptr params, ModuleData::Ptr data, Devices::DAC::Chip::Ptr device)
      : Data(data)
      , Params(DAC::TrackParameters::Create(params))
      , Device(device)
      , Iterator(CreateTrackStateIterator(Data))
      , LastRenderTime(0)
    {
#ifndef NDEBUG
//perform self-test
      Devices::DAC::DataChunk chunk;
      while (Iterator->IsValid())
      {
        RenderData(chunk);
        Iterator->NextFrame(false);
      }
      Reset();
#endif
    }

    virtual TrackState::Ptr GetTrackState() const
    {
      return Iterator->GetStateObserver();
    }

    virtual Analyzer::Ptr GetAnalyzer() const
    {
      return DAC::CreateAnalyzer(Device);
    }

    virtual bool RenderFrame()
    {
      if (Iterator->IsValid())
      {
        LastRenderTime += Params->FrameDuration();
        Devices::DAC::DataChunk chunk;
        RenderData(chunk);
        chunk.TimeStamp = LastRenderTime;
        Device->RenderData(chunk);
        Device->Flush();
        Iterator->NextFrame(Params->Looped());
      }
      return Iterator->IsValid();
    }

    virtual void Reset()
    {
      Device->Reset();
      Iterator->Reset();
      std::for_each(Ornaments.begin(), Ornaments.end(), std::mem_fun_ref(&OrnamentState::Reset));
      LastRenderTime = Time::Microseconds();
    }

    virtual void SetPosition(uint_t frame)
    {
      const TrackState::Ptr state = Iterator->GetStateObserver();
      if (frame < state->Frame())
      {
        //reset to beginning in case of moving back
        Iterator->Reset();
      }
      //fast forward
      Devices::DAC::DataChunk chunk;
      while (state->Frame() < frame && Iterator->IsValid())
      {
        //do not update tick for proper rendering
        RenderData(chunk);
        Iterator->NextFrame(false);
      }
    }

  private:
    void RenderData(Devices::DAC::DataChunk& chunk)
    {
      const TrackModelState::Ptr state = Iterator->GetStateObserver();
      DAC::TrackBuilder track;
      SynthesizeData(*state, track);
      track.GetResult(chunk.Channels);
    }

    void SynthesizeData(const TrackModelState& state, DAC::TrackBuilder& track)
    {
      SynthesizeChannelsData(track);
      if (0 == state.Quirk())
      {
        GetNewLineState(state, track);
      }
    }  

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

    void GetNewLineState(const TrackModelState& state, DAC::TrackBuilder& track)
    {
      if (const Line::Ptr line = state.LineObject())
      {
        for (uint_t chan = 0; chan != CHANNELS_COUNT; ++chan)
        {
          if (const Cell::Ptr src = line->GetChannel(chan))
          {
            DAC::ChannelDataBuilder builder = track.GetChannel(chan);
            GetNewChannelState(*src, Ornaments[chan], builder);
          }
        }
      }
    }

    void GetNewChannelState(const Cell& src, OrnamentState& ornamentState, DAC::ChannelDataBuilder& builder)
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
          ornamentState.Object = Data->Ornaments.Find(*ornament);
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
    const Information::Ptr Info;
    const ModuleData::Ptr Data;
    const DAC::TrackParameters::Ptr Params;
    const Devices::DAC::Chip::Ptr Device;
    const TrackStateIterator::Ptr Iterator;
    boost::array<OrnamentState, CHANNELS_COUNT> Ornaments;
    Time::Microseconds LastRenderTime;
  };

  Renderer::Ptr CreateModuleRenderer(Parameters::Accessor::Ptr params, ModuleData::Ptr data, Devices::DAC::Chip::Ptr device)
  {
    return Renderer::Ptr(new ModuleRenderer(params, data, device));
  }
}

namespace PDT
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char ID[] = {'P', 'D', 'T', 0};
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_4DAC | CAP_CONV_RAW;

  class Factory : public ModulesFactory
  {
  public:
    explicit Factory(Formats::Chiptune::Decoder::Ptr decoder)
      : Decoder(decoder)
    {
    }

    
    virtual bool Check(const Binary::Container& data) const
    {
      return Decoder->Check(data);
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Decoder->GetFormat();
    }

    virtual Holder::Ptr CreateModule(ModuleProperties::RWPtr properties, Binary::Container::Ptr data, std::size_t& usedSize) const
    {
      const ::ProDigiTracker::ModuleData::RWPtr modData = boost::make_shared< ::ProDigiTracker::ModuleData>();
      ::ProDigiTracker::DataBuilder builder(modData, properties);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::ProDigiTracker::Parse(*data, builder))
      {
        usedSize = container->Size();
        properties->SetSource(container);
        return boost::make_shared< ::ProDigiTracker::ModuleHolder>(modData, properties);
      }
      return Holder::Ptr();
    }
  private:
    const Formats::Chiptune::Decoder::Ptr Decoder;
  };
}

namespace ZXTune
{
  void RegisterPDTSupport(PlayerPluginsRegistrator& registrator)
  {
    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateProDigiTrackerDecoder();
    const ModulesFactory::Ptr factory = boost::make_shared<PDT::Factory>(decoder);
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(PDT::ID, decoder->GetDescription(), PDT::CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
