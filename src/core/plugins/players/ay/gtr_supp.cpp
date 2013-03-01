/*
Abstract:
  GTR modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ay_base.h"
#include "core/plugins/utils.h"
#include "core/plugins/registrator.h"
#include "core/plugins/archives/archive_supp_common.h"
#include "core/plugins/players/creation_result.h"
#include "core/plugins/players/module_properties.h"
//common includes
#include <byteorder.h>
#include <error_tools.h>
#include <range_checker.h>
#include <tools.h>
//library includes
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <core/conversion/aym.h>
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/globaltracker.h>
#include <math/numeric.h>

#define FILE_TAG 1F7CDD7C

namespace GlobalTracker
{
  enum CmdType
  {
    EMPTY,
    ENVELOPE,     //2p
    NOENVELOPE,   //0p
  };

  struct Sample : public Formats::Chiptune::GlobalTracker::Sample
  {
    Sample()
      : Formats::Chiptune::GlobalTracker::Sample()
    {
    }

    Sample(const Formats::Chiptune::GlobalTracker::Sample& rh)
      : Formats::Chiptune::GlobalTracker::Sample(rh)
    {
    }

    uint_t GetLoop() const
    {
      return Loop;
    }

    uint_t GetSize() const
    {
      return static_cast<uint_t>(Lines.size());
    }

    const Line& GetLine(uint_t idx) const
    {
      static const Line STUB;
      return Lines.size() > idx ? Lines[idx] : STUB;
    }
  };

  typedef ZXTune::Module::TrackingSupport<Devices::AYM::CHANNELS, Sample> Track;

  std::auto_ptr<Formats::Chiptune::GlobalTracker::Builder> CreateDataBuilder(Track::ModuleData::RWPtr data, ZXTune::Module::ModuleProperties::RWPtr props);
  ZXTune::Module::AYM::Chiptune::Ptr CreateChiptune(Track::ModuleData::Ptr data, ZXTune::Module::ModuleProperties::Ptr properties);
}

namespace GlobalTracker
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  class DataBuilder : public Formats::Chiptune::GlobalTracker::Builder
  {
  public:
    DataBuilder(Track::ModuleData::RWPtr data, ZXTune::Module::ModuleProperties::RWPtr props)
      : Data(data)
      , Properties(props)
      , Context(*Data)
    {
    }

    virtual void SetProgram(const String& program)
    {
      Properties->SetProgram(program);
      Properties->SetFreqtable(TABLE_PROTRACKER3_ST);
    }

    virtual void SetTitle(const String& title)
    {
      Properties->SetTitle(OptimizeString(title));
    }

    virtual void SetInitialTempo(uint_t tempo)
    {
      Data->InitialTempo = tempo;
    }

    virtual void SetSample(uint_t index, const Formats::Chiptune::GlobalTracker::Sample& sample)
    {
      Data->Samples.resize(index + 1);
      Data->Samples[index] = Sample(sample);
    }

    virtual void SetOrnament(uint_t index, const Formats::Chiptune::GlobalTracker::Ornament& ornament)
    {
      Data->Ornaments.resize(index + 1);
      Data->Ornaments[index] = Track::Ornament(ornament.Loop, ornament.Lines.begin(), ornament.Lines.end());
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

    virtual void FinishPattern(uint_t size)
    {
      Context.FinishPattern(size);
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

    virtual void SetVolume(uint_t vol)
    {
      Context.CurChannel->SetVolume(vol);
    }

    virtual void SetEnvelope(uint_t type, uint_t value)
    {
      Context.CurChannel->AddCommand(ENVELOPE, type, value);
    }

    virtual void SetNoEnvelope()
    {
      Context.CurChannel->AddCommand(NOENVELOPE);
    }
  private:
    const Track::ModuleData::RWPtr Data;
    const ModuleProperties::RWPtr Properties;

    Track::BuildContext Context;
  };

  struct ChannelState
  {
    ChannelState()
      : Enabled(false), Envelope(false)
      , Note(), SampleNum(0), PosInSample(0)
      , OrnamentNum(0), PosInOrnament(0)
      , Volume(0)
    {
    }
    bool Enabled;
    bool Envelope;
    uint_t Note;
    uint_t SampleNum;
    uint_t PosInSample;
    uint_t OrnamentNum;
    uint_t PosInOrnament;
    uint_t Volume;
  };

  class DataRenderer : public AYM::DataRenderer
  {
  public:
    explicit DataRenderer(Track::ModuleData::Ptr data)
       : Data(data)
    {
    }

    virtual void Reset()
    {
      std::fill(PlayerState.begin(), PlayerState.end(), ChannelState());
    }

    virtual void SynthesizeData(const TrackState& state, AYM::TrackBuilder& track)
    {
      if (0 == state.Quirk())
      {
        GetNewLineState(state, track);
      }
      SynthesizeChannelsData(track);
    }
  private:
    void GetNewLineState(const TrackState& state, AYM::TrackBuilder& track)
    {
      if (const Track::Line* line = Data->Patterns[state.Pattern()].GetLine(state.Line()))
      {
        for (uint_t chan = 0; chan != line->Channels.size(); ++chan)
        {
          const Cell& src = line->Channels[chan];
          if (src.Empty())
          {
            continue;
          }
          GetNewChannelState(src, PlayerState[chan], track);
        }
      }
    }

    void GetNewChannelState(const Cell& src, ChannelState& dst, AYM::TrackBuilder& track)
    {
      if (const bool* enabled = src.GetEnabled())
      {
        dst.Enabled = *enabled;
      }
      if (const uint_t* note = src.GetNote())
      {
        assert(src.GetEnabled());
        dst.Note = *note;
        dst.PosInSample = dst.PosInOrnament = 0;
      }
      if (const uint_t* sample = src.GetSample())
      {
        dst.SampleNum = *sample;
      }
      if (const uint_t* ornament = src.GetOrnament())
      {
        dst.OrnamentNum = *ornament;
        dst.PosInOrnament = 0;
      }
      if (const uint_t* volume = src.GetVolume())
      {
        dst.Volume = *volume;
      }
      for (CommandsIterator it = src.GetCommands(); it; ++it)
      {
        switch (it->Type)
        {
        case ENVELOPE:
          track.SetEnvelopeType(it->Param1);
          track.SetEnvelopeTone(it->Param2);
          dst.Envelope = true;
          break;
        case NOENVELOPE:
          dst.Envelope = false;
          break;
        default:
          assert(!"Invalid command");
        }
      }
    }

    void SynthesizeChannelsData(AYM::TrackBuilder& track)
    {
      uint_t noise = 0;
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        AYM::ChannelBuilder channel = track.GetChannel(chan);
        SynthesizeChannel(PlayerState[chan], channel, track, noise);
      }
      track.SetNoise(noise);
    }

    void SynthesizeChannel(ChannelState& dst, AYM::ChannelBuilder& channel, AYM::TrackBuilder& track, uint_t& noise)
    {
      if (!dst.Enabled)
      {
        channel.SetLevel(0);
        channel.DisableTone();
        channel.DisableNoise();
        return;
      }

      const Sample& curSample = Data->Samples[dst.SampleNum];
      const Sample::Line& curSampleLine = curSample.GetLine(dst.PosInSample);
      const Track::Ornament& curOrnament = Data->Ornaments[dst.OrnamentNum];

      //apply tone
      const int_t halftones = Math::Clamp<int_t>(int_t(dst.Note) + curOrnament.GetLine(dst.PosInOrnament), 0, 95);
      const uint_t freq = (track.GetFrequency(halftones) + curSampleLine.Vibrato) & 0xfff;
      channel.SetTone(freq);

      if (curSampleLine.ToneMask)
      {
        channel.DisableTone();
      }
      const int_t level = Math::Clamp<int_t>(int_t(curSampleLine.Level) - dst.Volume, 0, 255);
      //apply level
      channel.SetLevel(level & 0xf);
      //apply envelope
      if (dst.Envelope && curSampleLine.EnvelopeMask)
      {
        channel.EnableEnvelope();
      }
      //apply noise
      noise |= curSampleLine.Noise;
      if (curSampleLine.NoiseMask)
      {
        channel.DisableNoise();
      }

      if (++dst.PosInSample >= curSample.GetSize())
      {
        dst.PosInSample = curSample.GetLoop();
      }
      if (++dst.PosInOrnament >= curOrnament.GetSize())
      {
        dst.PosInOrnament = curOrnament.GetLoop();
      }
    }
  private:
    const Track::ModuleData::Ptr Data;
    boost::array<ChannelState, Devices::AYM::CHANNELS> PlayerState;
  };

  class Chiptune : public AYM::Chiptune
  {
  public:
    Chiptune(Track::ModuleData::Ptr data, ModuleProperties::Ptr properties)
      : Data(data)
      , Properties(properties)
      , Info(CreateTrackInfo(Data, Devices::AYM::CHANNELS))
    {
    }

    virtual Information::Ptr GetInformation() const
    {
      return Info;
    }

    virtual ModuleProperties::Ptr GetProperties() const
    {
      return Properties;
    }

    virtual AYM::DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr trackParams) const
    {
      const StateIterator::Ptr iterator = CreateTrackStateIterator(Info, Data);
      const AYM::DataRenderer::Ptr renderer = boost::make_shared<DataRenderer>(Data);
      return AYM::CreateDataIterator(trackParams, iterator, renderer);
    }
  private:
    const Track::ModuleData::Ptr Data;
    const ModuleProperties::Ptr Properties;
    const Information::Ptr Info;
  };
}

namespace GlobalTracker
{
  std::auto_ptr<Formats::Chiptune::GlobalTracker::Builder> CreateDataBuilder(Track::ModuleData::RWPtr data, ZXTune::Module::ModuleProperties::RWPtr props)
  {
    return std::auto_ptr<Formats::Chiptune::GlobalTracker::Builder>(new DataBuilder(data, props));
  }

  ZXTune::Module::AYM::Chiptune::Ptr CreateChiptune(Track::ModuleData::Ptr data, ZXTune::Module::ModuleProperties::Ptr properties)
  {
    return boost::make_shared<Chiptune>(data, properties);
  }
}

namespace GTR
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char ID[] = {'G', 'T', 'R', 0};
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | Module::SupportedAYMFormatConvertors;

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

    virtual Holder::Ptr CreateModule(ModuleProperties::RWPtr properties, Binary::Container::Ptr rawData, std::size_t& usedSize) const
    {
      const ::GlobalTracker::Track::ModuleData::RWPtr modData = boost::make_shared< ::GlobalTracker::Track::ModuleData>();
      const std::auto_ptr<Formats::Chiptune::GlobalTracker::Builder> dataBuilder = ::GlobalTracker::CreateDataBuilder(modData, properties);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::GlobalTracker::Parse(*rawData, *dataBuilder))
      {
        usedSize = container->Size();
        properties->SetSource(container);
        const Module::AYM::Chiptune::Ptr chiptune = ::GlobalTracker::CreateChiptune(modData, properties);
        return AYM::CreateHolder(chiptune);
      }
      return Holder::Ptr();
    }
  private:
    const Formats::Chiptune::Decoder::Ptr Decoder;
  };
}

namespace ZXTune
{
  void RegisterGTRSupport(PlayerPluginsRegistrator& registrator)
  {
    //direct modules
    {
      const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateGlobalTrackerDecoder();
      const ModulesFactory::Ptr factory = boost::make_shared<GTR::Factory>(decoder);
      const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(GTR::ID, decoder->GetDescription(), GTR::CAPS, factory);
      registrator.RegisterPlugin(plugin);
    }
  }
}
