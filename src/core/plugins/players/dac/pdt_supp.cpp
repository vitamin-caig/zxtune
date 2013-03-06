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
#include "core/plugins/utils.h"
#include "core/plugins/players/creation_result.h"
#include "core/plugins/players/module_properties.h"
#include "core/plugins/players/tracking.h"
//common includes
#include <tools.h>
//library includes
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <devices/dac_sample_factories.h>
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/prodigitracker.h>
#include <sound/mixer_factory.h>
//boost includes
#include <boost/bind.hpp>
//text includes
#include <formats/text/chiptune.h>

#define FILE_TAG 30E3A543

namespace ProDigiTracker
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  const uint_t CHANNELS_COUNT = 4;
  //all samples has base freq at 4kHz (C-1)
  const uint_t BASE_FREQ = 4000;

  struct Ornament
  {
    Ornament()
      : Loop()
    {
    }

    Ornament(uint_t loop, const std::vector<int_t>& data)
      : Loop(loop)
      , Data(data)
    {
    }

    std::size_t Loop;
    std::vector<int_t> Data;
  };

  typedef TrackingSupport<CHANNELS_COUNT, Devices::DAC::Sample::Ptr, Ornament> PDTTrack;

  std::auto_ptr<Formats::Chiptune::ProDigiTracker::Builder> CreateDataBuilder(PDTTrack::ModuleData::RWPtr data, ModuleProperties::RWPtr props);
}

namespace ProDigiTracker
{
  class Builder : public Formats::Chiptune::ProDigiTracker::Builder
  {
  public:
    Builder(PDTTrack::ModuleData::RWPtr data, ModuleProperties::RWPtr props)
      : Data(data)
      , Properties(props)
      , Context(*Data)
    {
    }

    virtual void SetTitle(const String& title)
    {
      Properties->SetTitle(OptimizeString(title));
    }

    virtual void SetProgram(const String& program)
    {
      Properties->SetProgram(program);
    }

    virtual void SetInitialTempo(uint_t tempo)
    {
      Data->InitialTempo = tempo;
    }

    virtual void SetSample(uint_t index, std::size_t loop, Binary::Data::Ptr sample)
    {
      Data->Samples.resize(index + 1);
      Data->Samples[index] = Devices::DAC::CreateU8Sample(sample, loop);
    }

    virtual void SetOrnament(uint_t index, std::size_t loop, const std::vector<int_t>& ornament)
    {
      Data->Ornaments.resize(index + 1);
      Data->Ornaments[index] = Ornament(loop, ornament);
    }

    virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop)
    {
      Data->Order = boost::make_shared<SimpleOrderList>(positions.begin(), positions.end(), loop);
    }

    virtual void StartPattern(uint_t index)
    {
      Context.SetPattern(index);
    }

    virtual void StartLine(uint_t index)
    {
      Context.SetLine(index);
    }

    virtual void SetTempo(uint_t tempo)
    {
      Context.CurLine->SetTempo(tempo);
    }

    virtual void StartChannel(uint_t index)
    {
      Context.SetChannel(index);
    }

    virtual void SetRest()
    {
      Context.CurChannel->SetEnabled(false);
    }

    virtual void SetNote(uint_t note)
    {
      Context.CurChannel->SetEnabled(true);
      Context.CurChannel->SetNote(note);
    }

    virtual void SetSample(uint_t sample)
    {
      Context.CurChannel->SetSample(sample);
    }

    virtual void SetOrnament(uint_t ornament)
    {
      Context.CurChannel->SetOrnament(ornament);
    }
  private:
    const PDTTrack::ModuleData::RWPtr Data;
    const ModuleProperties::RWPtr Properties;

    PDTTrack::BuildContext Context;
  };

  Renderer::Ptr CreatePDTRenderer(Parameters::Accessor::Ptr params, Information::Ptr info, PDTTrack::ModuleData::Ptr data, Devices::DAC::Chip::Ptr device);

  class PDTHolder : public Holder
  {
  public:
    PDTHolder(PDTTrack::ModuleData::Ptr data, ModuleProperties::Ptr properties)
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
      for (uint_t idx = 0, lim = Data->Samples.size(); idx != lim; ++idx)
      {
        chip->SetSample(idx, Data->Samples[idx]);
      }
      return CreatePDTRenderer(params, Info, Data, chip);
    }
  private:
    const PDTTrack::ModuleData::Ptr Data;
    const ModuleProperties::Ptr Properties;
    const Information::Ptr Info;
  };

  class PDTRenderer : public Renderer
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
        return Object && Object->Data.size() > Position ? Object->Data[Position] : 0;
      }

      void Update()
      {
        if (Object && Position++ >= Object->Data.size())
        {
          Position = Object->Loop;
        }
      }

      void Reset()
      {
        Position = 0;
      }
    };
  public:
    PDTRenderer(Parameters::Accessor::Ptr params, Information::Ptr info, PDTTrack::ModuleData::Ptr data, Devices::DAC::Chip::Ptr device)
      : Data(data)
      , Params(DAC::TrackParameters::Create(params))
      , Device(device)
      , Iterator(CreateTrackStateIterator(info, Data))
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
      const TrackState::Ptr state = Iterator->GetStateObserver();
      DAC::TrackBuilder track;
      SynthesizeData(*state, track);
      track.GetResult(chunk.Channels);
    }

    void SynthesizeData(const TrackState& state, DAC::TrackBuilder& track)
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

    void GetNewLineState(const TrackState& state, DAC::TrackBuilder& track)
    {
      if (const Line::Ptr line = Data->Patterns->Get(state.Pattern())->GetLine(state.Line()))
      {
        for (uint_t chan = 0; chan != PDTTrack::CHANNELS; ++chan)
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
          const uint_t ornIdx = *ornament < Data->Ornaments.size() ? *ornament : 0;
          ornamentState.Object = &Data->Ornaments[ornIdx];
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
    const PDTTrack::ModuleData::Ptr Data;
    const DAC::TrackParameters::Ptr Params;
    const Devices::DAC::Chip::Ptr Device;
    const StateIterator::Ptr Iterator;
    boost::array<OrnamentState, CHANNELS_COUNT> Ornaments;
    Time::Microseconds LastRenderTime;
  };

  Renderer::Ptr CreatePDTRenderer(Parameters::Accessor::Ptr params, Information::Ptr info, PDTTrack::ModuleData::Ptr data, Devices::DAC::Chip::Ptr device)
  {
    return Renderer::Ptr(new PDTRenderer(params, info, data, device));
  }
}

namespace ProDigiTracker
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char ID[] = {'P', 'D', 'T', 0};
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_4DAC | CAP_CONV_RAW;

  class PDTModulesFactory : public ModulesFactory
  {
  public:
    PDTModulesFactory(Formats::Chiptune::Decoder::Ptr decoder)
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
      const PDTTrack::ModuleData::RWPtr modData = PDTTrack::ModuleData::Create();
      Builder builder(modData, properties);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::ProDigiTracker::Parse(*data, builder))
      {
        usedSize = container->Size();
        properties->SetSource(container);
        return boost::make_shared<PDTHolder>(modData, properties);
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
    const ModulesFactory::Ptr factory = boost::make_shared<ProDigiTracker::PDTModulesFactory>(decoder);
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ProDigiTracker::ID, decoder->GetDescription(), ProDigiTracker::CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
