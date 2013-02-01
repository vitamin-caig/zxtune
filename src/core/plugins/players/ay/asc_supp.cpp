/*
Abstract:
  ASC modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ay_base.h"
#include "core/plugins/registrator.h"
#include "core/plugins/utils.h"
#include "core/plugins/players/creation_result.h"
#include "core/plugins/players/module_properties.h"
//common includes
#include <error_tools.h>
#include <tools.h>
//library includes
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <core/conversion/aym.h>
#include <formats/chiptune/ascsoundmaster.h>
#include <math/numeric.h>

#define FILE_TAG 45B26E38

namespace ASCSoundMaster
{
  //supported commands and their parameters
  enum CmdType
  {
    //no parameters
    EMPTY,
    //r13,envL
    ENVELOPE,
    //no parameters
    ENVELOPE_ON,
    //no parameters
    ENVELOPE_OFF,
    //value
    NOISE,
    //no parameters
    CONT_SAMPLE,
    //no parameters
    CONT_ORNAMENT,
    //glissade value
    GLISS,
    //steps
    SLIDE,
    //steps,note
    SLIDE_NOTE,
    //period,delta
    AMPLITUDE_SLIDE,
    //no parameter
    BREAK_SAMPLE
  };

  struct Sample : public Formats::Chiptune::ASCSoundMaster::Sample
  {
    Sample()
      : Formats::Chiptune::ASCSoundMaster::Sample()
    {
    }

    Sample(const Formats::Chiptune::ASCSoundMaster::Sample& rh)
      : Formats::Chiptune::ASCSoundMaster::Sample(rh)
    {
    }

    uint_t GetLoop() const
    {
      return Loop;
    }

    uint_t GetLoopLimit() const
    {
      return LoopLimit;
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

  struct Ornament : public Formats::Chiptune::ASCSoundMaster::Ornament
  {
    Ornament()
      : Formats::Chiptune::ASCSoundMaster::Ornament()
    {
    }

    Ornament(const Formats::Chiptune::ASCSoundMaster::Ornament& rh)
      : Formats::Chiptune::ASCSoundMaster::Ornament(rh)
    {
    }

    uint_t GetLoop() const
    {
      return Loop;
    }

    uint_t GetLoopLimit() const
    {
      return LoopLimit;
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

  typedef ZXTune::Module::TrackingSupport<Devices::AYM::CHANNELS, CmdType, Sample, Ornament> Track;

  std::auto_ptr<Formats::Chiptune::ASCSoundMaster::Builder> CreateDataBuilder(Track::ModuleData::RWPtr data, ZXTune::Module::ModuleProperties::RWPtr props);
  ZXTune::Module::AYM::Chiptune::Ptr CreateChiptune(Track::ModuleData::Ptr data, ZXTune::Module::ModuleProperties::Ptr properties);
}

namespace ASCSoundMaster
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  class DataBuilder : public Formats::Chiptune::ASCSoundMaster::Builder
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
      Properties->SetFreqtable(TABLE_ASM);
    }

    virtual void SetTitle(const String& title)
    {
      Properties->SetTitle(OptimizeString(title));
    }

    virtual void SetAuthor(const String& author)
    {
      Properties->SetAuthor(OptimizeString(author));
    }

    virtual void SetInitialTempo(uint_t tempo)
    {
      Data->InitialTempo = tempo;
    }

    virtual void SetSample(uint_t index, const Formats::Chiptune::ASCSoundMaster::Sample& sample)
    {
      Data->Samples.resize(index + 1);
      Data->Samples[index] = Sample(sample);
    }

    virtual void SetOrnament(uint_t index, const Formats::Chiptune::ASCSoundMaster::Ornament& ornament)
    {
      Data->Ornaments.resize(index + 1);
      Data->Ornaments[index] = Ornament(ornament);
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
      Track::Line::Chan* const channel = Context.CurChannel;
      if (!channel->FindCommand(BREAK_SAMPLE))
      {
        channel->SetEnabled(true);
      }
      if (!channel->Commands.empty() &&
          SLIDE == channel->Commands.back().Type)
      {
        //set slide to note
        Track::Command& command = channel->Commands.back();
        command.Type = SLIDE_NOTE;
        command.Param2 = note;
      }
      else
      {
        channel->SetNote(note);
      }
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

    virtual void SetEnvelopeType(uint_t type)
    {
      Track::Line::Chan* const channel = Context.CurChannel;
      const Track::CommandsArray::iterator cmd = std::find(channel->Commands.begin(), channel->Commands.end(), ENVELOPE);
      if (cmd != channel->Commands.end())
      {
        cmd->Param1 = type;
      }
      else
      {
        channel->Commands.push_back(Track::Command(ENVELOPE, type, -1));
      }
    }

    virtual void SetEnvelopeTone(uint_t tone)
    {
      Track::Line::Chan* const channel = Context.CurChannel;
      const Track::CommandsArray::iterator cmd = std::find(channel->Commands.begin(), channel->Commands.end(), ENVELOPE);
      if (cmd != channel->Commands.end())
      {
        cmd->Param2 = tone;
      }
      else
      {
        //strange situation
        channel->Commands.push_back(Track::Command(ENVELOPE, -1, tone));
      }
    }

    virtual void SetEnvelope()
    {
      Context.CurChannel->Commands.push_back(Track::Command(ENVELOPE_ON));
    }

    virtual void SetNoEnvelope()
    {
      Context.CurChannel->Commands.push_back(Track::Command(ENVELOPE_OFF));
    }

    virtual void SetNoise(uint_t val)
    {
      Context.CurChannel->Commands.push_back(Track::Command(NOISE, val));
    }

    virtual void SetContinueSample()
    {
      Context.CurChannel->Commands.push_back(Track::Command(CONT_SAMPLE));
    }

    virtual void SetContinueOrnament()
    {
      Context.CurChannel->Commands.push_back(Track::Command(CONT_ORNAMENT));
    }

    virtual void SetGlissade(int_t val)
    {
      Context.CurChannel->Commands.push_back(Track::Command(GLISS, val));
    }

    virtual void SetSlide(int_t steps)
    {
      Context.CurChannel->Commands.push_back(Track::Command(SLIDE, steps));
    }

    virtual void SetVolumeSlide(uint_t period, int_t delta)
    {
      Context.CurChannel->Commands.push_back(Track::Command(AMPLITUDE_SLIDE, period, delta));
    }

    virtual void SetBreakSample()
    {
      Context.CurChannel->Commands.push_back(Track::Command(BREAK_SAMPLE));
    }
  private:
    const Track::ModuleData::RWPtr Data;
    const ModuleProperties::RWPtr Properties;

    Track::BuildContext Context;
  };

  const uint_t LIMITER(~uint_t(0));

  struct ChannelState
  {
    ChannelState()
      : Enabled(false), Envelope(false), BreakSample(false)
      , Volume(15), VolumeAddon(0), VolSlideDelay(0), VolSlideAddon(), VolSlideCounter(0)
      , BaseNoise(0), CurrentNoise(0)
      , Note(0), NoteAddon(0)
      , SampleNum(0), CurrentSampleNum(0), PosInSample(0)
      , OrnamentNum(0), CurrentOrnamentNum(0), PosInOrnament(0)
      , ToneDeviation(0)
      , SlidingSteps(0), Sliding(0), SlidingTargetNote(LIMITER), Glissade(0)
    {
    }
    bool Enabled;
    bool Envelope;
    bool BreakSample;
    uint_t Volume;
    int_t VolumeAddon;
    uint_t VolSlideDelay;
    int_t VolSlideAddon;
    uint_t VolSlideCounter;
    int_t BaseNoise;
    int_t CurrentNoise;
    uint_t Note;
    int_t NoteAddon;
    uint_t SampleNum;
    uint_t CurrentSampleNum;
    uint_t PosInSample;
    uint_t OrnamentNum;
    uint_t CurrentOrnamentNum;
    uint_t PosInOrnament;
    int_t ToneDeviation;
    int_t SlidingSteps;//may be infinite (negative)
    int_t Sliding;
    uint_t SlidingTargetNote;
    int_t Glissade;

    void ResetBaseNoise()
    {
      BaseNoise = 0;
    }
  };

  class DataRenderer : public AYM::DataRenderer
  {
  public:
    explicit DataRenderer(Track::ModuleData::Ptr data)
      : Data(data)
      , EnvelopeTone(0)
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
      if (0 == state.Line())
      {
        std::for_each(PlayerState.begin(), PlayerState.end(), std::mem_fun_ref(&ChannelState::ResetBaseNoise));
      }
      if (const Track::Line* line = Data->Patterns[state.Pattern()].GetLine(state.Line()))
      {
        for (uint_t chan = 0; chan != line->Channels.size(); ++chan)
        {
          const Track::Line::Chan& src = line->Channels[chan];
          if (src.Empty())
          {
            continue;
          }
          GetNewChannelState(src, PlayerState[chan], track);
        }
      }
    }

    void GetNewChannelState(const Track::Line::Chan& src, ChannelState& dst, AYM::TrackBuilder& track)
    {
      if (src.Enabled)
      {
        dst.Enabled = *src.Enabled;
      }
      dst.VolSlideCounter = 0;
      dst.SlidingSteps = 0;
      bool contSample = false, contOrnament = false;
      for (Track::CommandsArray::const_iterator it = src.Commands.begin(), lim = src.Commands.end();
        it != lim; ++it)
      {
        switch (it->Type)
        {
        case ENVELOPE:
          if (-1 != it->Param1)
          {
            track.SetEnvelopeType(it->Param1);
          }
          if (-1 != it->Param2)
          {
            EnvelopeTone = it->Param2;
            track.SetEnvelopeTone(EnvelopeTone);
          }
          break;
        case ENVELOPE_ON:
          dst.Envelope = true;
          break;
        case ENVELOPE_OFF:
          dst.Envelope = false;
          break;
        case NOISE:
          dst.BaseNoise = it->Param1;
          break;
        case CONT_SAMPLE:
          contSample = true;
          break;
        case CONT_ORNAMENT:
          contOrnament = true;
          break;
        case GLISS:
          dst.Glissade = it->Param1;
          dst.SlidingSteps = -1;//infinite sliding
          break;
        case SLIDE:
        {
          dst.SlidingSteps = it->Param1;
          const int_t newSliding = (dst.Sliding | 0xf) ^ 0xf;
          dst.Glissade = -newSliding / (dst.SlidingSteps ? dst.SlidingSteps : 1);
          dst.Sliding = dst.Glissade * dst.SlidingSteps;
          break;
        }
        case SLIDE_NOTE:
        {
          dst.SlidingSteps = it->Param1;
          dst.SlidingTargetNote = it->Param2;
          const int_t absoluteSliding = track.GetSlidingDifference(dst.Note, dst.SlidingTargetNote);
          const int_t newSliding = absoluteSliding - (contSample ? dst.Sliding / 16 : 0);
          dst.Glissade = 16 * newSliding / (dst.SlidingSteps ? dst.SlidingSteps : 1);
          break;
        }
        case AMPLITUDE_SLIDE:
          dst.VolSlideCounter = dst.VolSlideDelay = it->Param1;
          dst.VolSlideAddon = it->Param2;
          break;
        case BREAK_SAMPLE:
          dst.BreakSample = true;
          break;
        default:
          assert(!"Invalid cmd");
          break;
        }
      }
      if (src.OrnamentNum)
      {
        dst.OrnamentNum = *src.OrnamentNum;
      }
      if (src.SampleNum)
      {
        dst.SampleNum = *src.SampleNum;
      }
      if (src.Note)
      {
        dst.Note = *src.Note;
        dst.CurrentNoise = dst.BaseNoise;
        if (dst.SlidingSteps <= 0)
        {
          dst.Sliding = 0;
        }
        if (!contSample)
        {
          dst.CurrentSampleNum = dst.SampleNum;
          dst.PosInSample = 0;
          dst.VolumeAddon = 0;
          dst.ToneDeviation = 0;
          dst.BreakSample = false;
        }
        if (!contOrnament)
        {
          dst.CurrentOrnamentNum = dst.OrnamentNum;
          dst.PosInOrnament = 0;
          dst.NoteAddon = 0;
        }
      }
      if (src.Volume)
      {
        dst.Volume = *src.Volume;
      }
    }

    void SynthesizeChannelsData(AYM::TrackBuilder& track)
    {
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        AYM::ChannelBuilder channel = track.GetChannel(chan);
        SynthesizeChannel(PlayerState[chan], channel, track);
      }
    }

    void SynthesizeChannel(ChannelState& dst, AYM::ChannelBuilder& channel, AYM::TrackBuilder& track)
    {
      if (!dst.Enabled)
      {
        channel.SetLevel(0);
        return;
      }

      const Sample& curSample = Data->Samples[dst.CurrentSampleNum];
      const Sample::Line& curSampleLine = curSample.GetLine(dst.PosInSample);
      const Ornament& curOrnament = Data->Ornaments[dst.CurrentOrnamentNum];
      const Ornament::Line& curOrnamentLine = curOrnament.GetLine(dst.PosInOrnament);

      //calculate volume addon
      if (dst.VolSlideCounter >= 2)
      {
        dst.VolSlideCounter--;
      }
      else if (dst.VolSlideCounter)
      {
        dst.VolumeAddon += dst.VolSlideAddon;
        dst.VolSlideCounter = dst.VolSlideDelay;
      }
      dst.VolumeAddon += curSampleLine.VolSlide;
      dst.VolumeAddon = Math::Clamp<int_t>(dst.VolumeAddon, -15, 15);

      //calculate tone
      dst.ToneDeviation += curSampleLine.ToneDeviation;
      dst.NoteAddon += curOrnamentLine.NoteAddon;
      const int_t halfTone = int_t(dst.Note) + dst.NoteAddon;
      const int_t toneAddon = dst.ToneDeviation + dst.Sliding / 16;
      //apply tone
      channel.SetTone(halfTone, toneAddon);

      //apply level
      channel.SetLevel((dst.Volume + 1) * Math::Clamp<int_t>(dst.VolumeAddon + curSampleLine.Level, 0, 15) / 16);
      //apply envelope
      if (dst.Envelope && curSampleLine.EnableEnvelope)
      {
        channel.EnableEnvelope();
      }

      //calculate noise
      dst.CurrentNoise += curOrnamentLine.NoiseAddon;

      //mixer
      if (curSampleLine.ToneMask)
      {
        channel.DisableTone();
      }
      if (curSampleLine.NoiseMask && curSampleLine.EnableEnvelope)
      {
        EnvelopeTone += curSampleLine.Adding;
        track.SetEnvelopeTone(EnvelopeTone);
      }
      else
      {
        dst.CurrentNoise += curSampleLine.Adding;
      }

      if (!curSampleLine.NoiseMask)
      {
        track.SetNoise((dst.CurrentNoise + dst.Sliding / 256) & 0x1f);
      }
      else
      {
        channel.DisableNoise();
      }

      //recalc positions
      if (dst.SlidingSteps != 0)
      {
        if (dst.SlidingSteps > 0)
        {
          if (!--dst.SlidingSteps &&
              LIMITER != dst.SlidingTargetNote) //finish slide to note
          {
            dst.Note = dst.SlidingTargetNote;
            dst.SlidingTargetNote = LIMITER;
            dst.Sliding = dst.Glissade = 0;
          }
        }
        dst.Sliding += dst.Glissade;
      }

      if (dst.PosInSample++ >= curSample.GetLoopLimit())
      {
        if (!dst.BreakSample)
        {
          dst.PosInSample = curSample.GetLoop();
        }
        else if (dst.PosInSample >= curSample.GetSize())
        {
          dst.Enabled = false;
        }
      }
      if (dst.PosInOrnament++ >= curOrnament.GetLoopLimit())
      {
        dst.PosInOrnament = curOrnament.GetLoop();
      }
    }
  private:
    const Track::ModuleData::Ptr Data;
    uint_t EnvelopeTone;
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

namespace ASCSoundMaster
{
  std::auto_ptr<Formats::Chiptune::ASCSoundMaster::Builder> CreateDataBuilder(Track::ModuleData::RWPtr data, ModuleProperties::RWPtr props)
  {
    return std::auto_ptr<Formats::Chiptune::ASCSoundMaster::Builder>(new DataBuilder(data, props));
  }

  ZXTune::Module::AYM::Chiptune::Ptr CreateChiptune(Track::ModuleData::Ptr data, ZXTune::Module::ModuleProperties::Ptr properties)
  {
    return boost::make_shared<Chiptune>(data, properties);
  }
}

namespace ASC
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char ID_0[] = {'A', 'S', '0', 0};
  const Char ID_1[] = {'A', 'S', 'C', 0};
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | SupportedAYMFormatConvertors;

  class Factory : public ModulesFactory
  {
  public:
    explicit Factory(Formats::Chiptune::ASCSoundMaster::Decoder::Ptr decoder)
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
      const ::ASCSoundMaster::Track::ModuleData::RWPtr modData = ::ASCSoundMaster::Track::ModuleData::Create();
      const std::auto_ptr<Formats::Chiptune::ASCSoundMaster::Builder> dataBuilder = ::ASCSoundMaster::CreateDataBuilder(modData, properties);
      if (const Formats::Chiptune::Container::Ptr container = Decoder->Parse(*rawData, *dataBuilder))
      {
        usedSize = container->Size();
        properties->SetSource(container);
        const Module::AYM::Chiptune::Ptr chiptune = ::ASCSoundMaster::CreateChiptune(modData, properties);
        return AYM::CreateHolder(chiptune);
      }
      return Holder::Ptr();
    }
  private:
    const Formats::Chiptune::ASCSoundMaster::Decoder::Ptr Decoder;
  };
}

namespace ZXTune
{
  void RegisterASCSupport(PlayerPluginsRegistrator& registrator)
  {
    {
      const Formats::Chiptune::ASCSoundMaster::Decoder::Ptr decoder = Formats::Chiptune::ASCSoundMaster::Ver0::CreateDecoder();
      const ModulesFactory::Ptr factory = boost::make_shared<ASC::Factory>(decoder);
      const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ASC::ID_0, decoder->GetDescription(), ASC::CAPS, factory);
      registrator.RegisterPlugin(plugin);
    }
    {
      const Formats::Chiptune::ASCSoundMaster::Decoder::Ptr decoder = Formats::Chiptune::ASCSoundMaster::Ver1::CreateDecoder();
      const ModulesFactory::Ptr factory = boost::make_shared<ASC::Factory>(decoder);
      const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ASC::ID_1, decoder->GetDescription(), ASC::CAPS, factory);
      registrator.RegisterPlugin(plugin);
    }
  }
}
