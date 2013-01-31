/*
Abstract:
  DST modules playback support

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
#include <error_tools.h>
#include <tools.h>
//library includes
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <devices/dac_sample_factories.h>
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/digitalstudio.h>
#include <sound/mixer_factory.h>
//std includes
#include <set>

#define FILE_TAG 3226C730

namespace DST
{
  const std::size_t CHANNELS_COUNT = 3;

  //all samples has base freq at 8kHz (C-1)
  const uint_t BASE_FREQ = 8000;
}

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //stub for ornament
  struct VoidType {};

  typedef TrackingSupport<DST::CHANNELS_COUNT, uint_t, Devices::DAC::Sample::Ptr, VoidType> DSTTrack;


  class Builder : public Formats::Chiptune::DigitalStudio::Builder
  {
  public:
    Builder(DSTTrack::ModuleData::RWPtr data, ModuleProperties::RWPtr props)
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

    virtual void SetSample(uint_t index, std::size_t loop, Binary::Data::Ptr content, bool is4Bit)
    {
      Data->Samples.resize(index + 1);
      Data->Samples[index] = is4Bit
        ? Devices::DAC::CreateU4Sample(content, loop)
        : Devices::DAC::CreateU8Sample(content, loop);
    }

    virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop)
    {
      Data->Positions.assign(positions.begin(), positions.end());
      Data->LoopPosition = loop;
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
  private:
    struct BuildContext
    {
      DSTTrack::ModuleData& Data;
      DSTTrack::Pattern* CurPattern;
      DSTTrack::Line* CurLine;
      DSTTrack::Line::Chan* CurChannel;

      explicit BuildContext(DSTTrack::ModuleData& data)
        : Data(data)
        , CurPattern()
        , CurLine()
        , CurChannel()
      {
      }

      void SetPattern(uint_t idx)
      {
        Data.Patterns.resize(idx + 1);
        CurPattern = &Data.Patterns[idx];
        CurLine = 0;
        CurChannel = 0;
      }

      void SetLine(uint_t idx)
      {
        if (const std::size_t skipped = idx - CurPattern->GetSize())
        {
          CurPattern->AddLines(skipped);
        }
        CurLine = &CurPattern->AddLine();
        CurChannel = 0;
      }

      void SetChannel(uint_t idx)
      {
        CurChannel = &CurLine->Channels[idx];
      }
    };
  private:
    const DSTTrack::ModuleData::RWPtr Data;
    const ModuleProperties::RWPtr Properties;
    std::size_t FourBitSamples;
    std::size_t EightBitSamples;

    BuildContext Context;
  };


  // perform module 'playback' right after creating (debug purposes)
  #ifndef NDEBUG
  #define SELF_TEST
  #endif

  Renderer::Ptr CreateDSTRenderer(Parameters::Accessor::Ptr params, Information::Ptr info, DSTTrack::ModuleData::Ptr data, Devices::DAC::Chip::Ptr device);

  class DSTHolder : public Holder
  {
  public:
    DSTHolder(DSTTrack::ModuleData::Ptr data, ModuleProperties::Ptr properties)
      : Data(data)
      , Properties(properties)
      , Info(CreateTrackInfo(Data, DST::CHANNELS_COUNT))
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
      const Sound::Mixer::Ptr mixer = Sound::CreatePollingMixer(DST::CHANNELS_COUNT, Properties);
      mixer->SetTarget(target);
      const Devices::DAC::Receiver::Ptr receiver = DAC::CreateReceiver(mixer, DST::CHANNELS_COUNT);
      const Devices::DAC::ChipParameters::Ptr chipParams = DAC::CreateChipParameters(params);
      const Devices::DAC::Chip::Ptr chip(Devices::DAC::CreateChip(DST::CHANNELS_COUNT, DST::BASE_FREQ, chipParams, receiver));
      for (uint_t idx = 0, lim = Data->Samples.size(); idx != lim; ++idx)
      {
        chip->SetSample(idx, Data->Samples[idx]);
      }
      return CreateDSTRenderer(params, Info, Data, chip);
    }
  private:
    const DSTTrack::ModuleData::Ptr Data;
    const ModuleProperties::Ptr Properties;
    const Information::Ptr Info;
  };

  class DSTRenderer : public Renderer
  {
  public:
    DSTRenderer(Parameters::Accessor::Ptr params, Information::Ptr info, DSTTrack::ModuleData::Ptr data, Devices::DAC::Chip::Ptr device)
      : Data(data)
      , Params(DAC::TrackParameters::Create(params))
      , Device(device)
      , Iterator(CreateTrackStateIterator(info, Data))
      , LastRenderTime(0)
    {
#ifdef SELF_TEST
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
      std::vector<Devices::DAC::DataChunk::ChannelData> res;
      const TrackState::Ptr state = Iterator->GetStateObserver();
      const DSTTrack::Line* const line = Data->Patterns[state->Pattern()].GetLine(state->Line());
      for (uint_t chan = 0; chan != DST::CHANNELS_COUNT; ++chan)
      {
        //begin note
        if (line && 0 == state->Quirk())
        {
          const DSTTrack::Line::Chan& src = line->Channels[chan];
          Devices::DAC::DataChunk::ChannelData dst;
          dst.Channel = chan;
          if (src.Enabled)
          {
            if (!(dst.Enabled = *src.Enabled))
            {
              dst.PosInSample = 0;
            }
          }
          if (src.Note)
          {
            dst.Note = *src.Note;
            dst.PosInSample = 0;
          }
          if (src.SampleNum)
          {
            dst.SampleNum = *src.SampleNum;
            dst.PosInSample = 0;
          }
          //store if smth new
          if (dst.Enabled || dst.Note || dst.SampleNum || dst.PosInSample)
          {
            res.push_back(dst);
          }
        }
      }
      chunk.Channels.swap(res);
    }
  private:
    const DSTTrack::ModuleData::Ptr Data;
    const DAC::TrackParameters::Ptr Params;
    const Devices::DAC::Chip::Ptr Device;
    const StateIterator::Ptr Iterator;
    Time::Microseconds LastRenderTime;
  };

  Renderer::Ptr CreateDSTRenderer(Parameters::Accessor::Ptr params, Information::Ptr info, DSTTrack::ModuleData::Ptr data, Devices::DAC::Chip::Ptr device)
  {
    return Renderer::Ptr(new DSTRenderer(params, info, data, device));
  }
}

namespace
{
  using namespace ZXTune;

  //plugin attributes
  const Char ID[] = {'D', 'S', 'T', 0};
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_3DAC | CAP_CONV_RAW;

  class DSTModulesFactory : public ModulesFactory
  {
  public:
    explicit DSTModulesFactory(Formats::Chiptune::Decoder::Ptr decoder)
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
      using namespace Formats::Chiptune;
      const DSTTrack::ModuleData::RWPtr modData = DSTTrack::ModuleData::Create();
      Builder builder(modData, properties);
      if (const Container::Ptr container = DigitalStudio::Parse(*data, builder))
      {
        usedSize = container->Size();
        properties->SetSource(container);
        return boost::make_shared<DSTHolder>(modData, properties);
      }
      return Holder::Ptr();
    }
  private:
    const Formats::Chiptune::Decoder::Ptr Decoder;
  };
}

namespace ZXTune
{
  void RegisterDSTSupport(PlayerPluginsRegistrator& registrator)
  {
    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateDigitalStudioDecoder();
    const ModulesFactory::Ptr factory = boost::make_shared<DSTModulesFactory>(decoder);
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, decoder->GetDescription(), CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
