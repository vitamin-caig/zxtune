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
#include <byteorder.h>
#include <error_tools.h>
#include <tools.h>
//library includes
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <devices/dac_sample_factories.h>
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/chiptracker.h>
#include <sound/mixer_factory.h>

#define FILE_TAG AB8BEC8B

namespace ChipTracker
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  const std::size_t CHANNELS_COUNT = 4;
  const std::size_t BASE_FREQ = 4000;

  enum CmdType
  {
    EMPTY,
    //offset in bytes
    SAMPLE_OFFSET,
    //step
    SLIDE
  };

  //stub for ornament
  struct VoidType {};

  typedef TrackingSupport<CHANNELS_COUNT, CmdType, Devices::DAC::Sample::Ptr, VoidType> CHITrack;

  std::auto_ptr<Formats::Chiptune::ChipTracker::Builder> CreateDataBuilder(CHITrack::ModuleData::RWPtr data, ModuleProperties::RWPtr props);
}

namespace ChipTracker
{
  class Builder : public Formats::Chiptune::ChipTracker::Builder
  {
  public:
    Builder(CHITrack::ModuleData::RWPtr data, ModuleProperties::RWPtr props)
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

    virtual void SetVersion(uint_t major, uint_t minor)
    {
      Properties->SetVersion(major, minor);
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

    virtual void SetSlide(int_t step)
    {
      Context.CurChannel->Commands.push_back(CHITrack::Command(SLIDE, step));
    }

    virtual void SetSampleOffset(uint_t offset)
    {
      Context.CurChannel->Commands.push_back(CHITrack::Command(SAMPLE_OFFSET, offset));
    }
  private:
    const CHITrack::ModuleData::RWPtr Data;
    const ModuleProperties::RWPtr Properties;

    CHITrack::BuildContext Context;
  };

  // perform module 'playback' right after creating (debug purposes)
  #ifndef NDEBUG
  #define SELF_TEST
  #endif

  Renderer::Ptr CreateCHIRenderer(Parameters::Accessor::Ptr params, Information::Ptr info, CHITrack::ModuleData::Ptr data, Devices::DAC::Chip::Ptr device);

  class CHIHolder : public Holder
  {
  public:
    CHIHolder(CHITrack::ModuleData::Ptr data, ModuleProperties::Ptr properties)
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
      const Devices::DAC::Receiver::Ptr receiver = DAC::CreateReceiver<CHANNELS_COUNT>(mixer);
      const Devices::DAC::ChipParameters::Ptr chipParams = DAC::CreateChipParameters(params);
      const Devices::DAC::Chip::Ptr chip(Devices::DAC::CreateChip(CHANNELS_COUNT, BASE_FREQ, chipParams, receiver));
      for (uint_t idx = 0, lim = Data->Samples.size(); idx != lim; ++idx)
      {
        chip->SetSample(idx, Data->Samples[idx]);
      }
      return CreateCHIRenderer(params, Info, Data, chip);
    }
  private:
    const CHITrack::ModuleData::Ptr Data;
    const ModuleProperties::Ptr Properties;
    const Information::Ptr Info;
  };

  class CHIRenderer : public Renderer
  {
    struct GlissData
    {
      GlissData() : Sliding(), Glissade()
      {
      }
      int_t Sliding;
      int_t Glissade;

      void Update()
      {
        Sliding += Glissade;
      }
    };
  public:
    CHIRenderer(Parameters::Accessor::Ptr params, Information::Ptr info, CHITrack::ModuleData::Ptr data, Devices::DAC::Chip::Ptr device)
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
      std::fill(Gliss.begin(), Gliss.end(), GlissData());
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
      const CHITrack::Line* const line = Data->Patterns[state->Pattern()].GetLine(state->Line());
      const bool newLine = 0 == state->Quirk();
      for (uint_t chan = 0; chan != CHANNELS_COUNT; ++chan)
      {
        GlissData& gliss(Gliss[chan]);
        Devices::DAC::DataChunk::ChannelData dst;
        dst.Channel = chan;
        gliss.Update();
        //begin note
        if (newLine)
        {
          gliss = GlissData();
        }
        if (line && newLine)
        {
          const CHITrack::Line::Chan& src = line->Channels[chan];
          if (src.Enabled)
          {
            if (!(dst.Enabled = *src.Enabled))
            {
              dst.PosInSample = 0;
            }
          }
          if (src.Note)
          {
            //start from octave 2
            dst.Note = *src.Note + 12;
            dst.PosInSample = 0;
          }
          if (src.SampleNum)
          {
            dst.SampleNum = *src.SampleNum;
            dst.PosInSample = 0;
          }
          for (CHITrack::CommandsArray::const_iterator it = src.Commands.begin(), lim = src.Commands.end(); it != lim; ++it)
          {
            switch (it->Type)
            {
            case SAMPLE_OFFSET:
              dst.PosInSample = it->Param1;
              break;
            case SLIDE:
              gliss.Glissade = it->Param1;
              break;
            default:
              assert(!"Invalid command");
            }
          }
        }
        //step 72 is C-1@3.5MHz for SounDrive player
        //C-1 is 65.41Hz (real C-2)
        dst.FreqSlideHz = gliss.Sliding * 6541 / 7200;
        //store if smth new
        if (dst.Enabled || dst.Note || dst.NoteSlide || dst.FreqSlideHz || dst.SampleNum || dst.PosInSample)
        {
          res.push_back(dst);
        }
      }
      chunk.Channels.swap(res);
    }
  private:
    const CHITrack::ModuleData::Ptr Data;
    const DAC::TrackParameters::Ptr Params;
    const Devices::DAC::Chip::Ptr Device;
    const StateIterator::Ptr Iterator;
    boost::array<GlissData, CHANNELS_COUNT> Gliss;
    Time::Microseconds LastRenderTime;
  };

  Renderer::Ptr CreateCHIRenderer(Parameters::Accessor::Ptr params, Information::Ptr info, CHITrack::ModuleData::Ptr data, Devices::DAC::Chip::Ptr device)
  {
    return Renderer::Ptr(new CHIRenderer(params, info, data, device));
  }
}

namespace ChipTracker
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char ID[] = {'C', 'H', 'I', 0};
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_4DAC | CAP_CONV_RAW;

  class CHIModulesFactory : public ModulesFactory
  {
  public:
    explicit CHIModulesFactory(Formats::Chiptune::Decoder::Ptr decoder)
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
      const CHITrack::ModuleData::RWPtr modData = CHITrack::ModuleData::Create();
      Builder builder(modData, properties);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::ChipTracker::Parse(*data, builder))
      {
        usedSize = container->Size();
        properties->SetSource(container);
        return boost::make_shared<CHIHolder>(modData, properties);
      }
      return Holder::Ptr();
    }
  private:
    const Formats::Chiptune::Decoder::Ptr Decoder;
  };
}

namespace ZXTune
{
  void RegisterCHISupport(PlayerPluginsRegistrator& registrator)
  {
    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateChipTrackerDecoder();
    const ModulesFactory::Ptr factory = boost::make_shared<ChipTracker::CHIModulesFactory>(decoder);
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ChipTracker::ID, decoder->GetDescription(), ChipTracker::CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
