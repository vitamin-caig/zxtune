/*
Abstract:
  PSM modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ay_base.h"
#include "ay_conversion.h"
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
#include <formats/chiptune_decoders.h>
#include <formats/packed_decoders.h>
#include <formats/chiptune/prosoundmaker.h>
#include <math/numeric.h>

#define FILE_TAG B74E7B96

namespace ProSoundMaker
{
  //supported commands and parameters
  enum CmdType
  {
    //no parameters
    EMPTY,
    //r13,period,note delta
    ENVELOPE,
    //disable ornament
    NOORNAMENT,
  };

  struct Sample : public Formats::Chiptune::ProSoundMaker::Sample
  {
    Sample()
      : Formats::Chiptune::ProSoundMaker::Sample()
    {
    }

    Sample(const Formats::Chiptune::ProSoundMaker::Sample& rh)
      : Formats::Chiptune::ProSoundMaker::Sample(rh)
    {
    }

    const uint_t* GetLoop() const
    {
      return Loop ? &*Loop : 0;
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

  struct Ornament : public Formats::Chiptune::ProSoundMaker::Ornament
  {
    Ornament()
      : Formats::Chiptune::ProSoundMaker::Ornament()
    {
    }

    Ornament(const Formats::Chiptune::ProSoundMaker::Ornament& rh)
      : Formats::Chiptune::ProSoundMaker::Ornament(rh)
    {
    }

    const uint_t* GetLoop() const
    {
      return Loop ? &*Loop : 0;
    }

    uint_t GetSize() const
    {
      return static_cast<uint_t>(Lines.size());
    }

    int_t GetLine(uint_t idx) const
    {
      return Lines.size() > idx ? Lines[idx] : 0;
    }
  };

  typedef ZXTune::Module::TrackingSupport<Devices::AYM::CHANNELS, CmdType, Sample, Ornament> Track;

  class ModuleData : public Track::ModuleData
  {
  public:
    typedef boost::shared_ptr<ModuleData> RWPtr;
    typedef boost::shared_ptr<const ModuleData> Ptr;

    std::vector<int_t> Transpositions;
  };
}

namespace ProSoundMaker
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  class DataBuilder : public Formats::Chiptune::ProSoundMaker::Builder
  {
  public:
    DataBuilder(ModuleData::RWPtr data, ZXTune::Module::ModuleProperties::RWPtr props)
      : Data(data)
      , Properties(props)
      , Context(*Data)
    {
    }

    virtual void SetProgram(const String& program)
    {
      Properties->SetProgram(program);
      Properties->SetFreqtable(TABLE_PROSOUNDMAKER);
    }

    virtual void SetTitle(const String& title)
    {
      Properties->SetTitle(OptimizeString(title));
    }

    virtual void SetSample(uint_t index, const Formats::Chiptune::ProSoundMaker::Sample& sample)
    {
      Data->Samples.resize(index + 1);
      Data->Samples[index] = Sample(sample);
    }

    virtual void SetOrnament(uint_t index, const Formats::Chiptune::ProSoundMaker::Ornament& ornament)
    {
      Data->Ornaments.resize(index + 1);
      Data->Ornaments[index] = Ornament(ornament);
    }

    virtual void SetPositions(const std::vector<Formats::Chiptune::ProSoundMaker::PositionEntry>& positions, uint_t loop)
    {
      const std::size_t posCount = positions.size();
      Data->LoopPosition = loop;
      Data->Positions.resize(posCount);
      Data->Transpositions.resize(posCount);
      using namespace Formats::Chiptune::ProSoundMaker;
      std::transform(positions.begin(), positions.end(), Data->Positions.begin(), boost::mem_fn(&PositionEntry::PatternIndex));
      std::transform(positions.begin(), positions.end(), Data->Transpositions.begin(), boost::mem_fn(&PositionEntry::Transposition));
    }

    virtual void StartPattern(uint_t index)
    {
      Context.SetPattern(index);
    }

    virtual void FinishPattern(uint_t size)
    {
      Context.FinishPattern(size);
    }

    virtual void SetTempo(uint_t tempo)
    {
      Context.CurLine->SetTempo(tempo);
    }

    virtual void StartLine(uint_t index)
    {
      Context.SetLine(index);
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
      channel->SetEnabled(true);
      channel->SetNote(note);
    }

    virtual void SetSample(uint_t sample)
    {
      Context.CurChannel->SetSample(sample);
    }

    virtual void SetOrnament(uint_t ornament)
    {
      Context.CurChannel->SetOrnament(ornament);
    }

    virtual void SetVolume(uint_t volume)
    {
      Context.CurChannel->SetVolume(volume);
    }

    virtual void DisableOrnament()
    {
      Context.CurChannel->Commands.push_back(Track::Command(NOORNAMENT));
    }

    virtual void SetEnvelopeType(uint_t type)
    {
      Track::Line::Chan* const channel = Context.CurChannel;
      const Track::CommandsArray::iterator cmd = std::find(channel->Commands.begin(), channel->Commands.end(), ENVELOPE);
      if (cmd != channel->Commands.end())
      {
        cmd->Param1 = int_t(type);
      }
      else
      {
        channel->Commands.push_back(Track::Command(ENVELOPE, type, -1, -1));
      }
    }

    virtual void SetEnvelopeTone(uint_t tone)
    {
      Track::Line::Chan* const channel = Context.CurChannel;
      const Track::CommandsArray::iterator cmd = std::find(channel->Commands.begin(), channel->Commands.end(), ENVELOPE);
      if (cmd != channel->Commands.end())
      {
        cmd->Param2 = int_t(tone);
      }
      else
      {
        channel->Commands.push_back(Track::Command(ENVELOPE, -1, int_t(tone), -1));
      }
    }

    virtual void SetEnvelopeNote(uint_t note)
    {
      Track::Line::Chan* const channel = Context.CurChannel;
      const Track::CommandsArray::iterator cmd = std::find(channel->Commands.begin(), channel->Commands.end(), ENVELOPE);
      if (cmd != channel->Commands.end())
      {
        cmd->Param3 = int_t(note);
      }
      else
      {
        channel->Commands.push_back(Track::Command(ENVELOPE, -1, -1, int_t(note)));
      }
    }
  private:
    struct BuildContext
    {
      Track::ModuleData& Data;
      Track::Pattern* CurPattern;
      Track::Line* CurLine;
      Track::Line::Chan* CurChannel;

      explicit BuildContext(Track::ModuleData& data)
        : Data(data)
        , CurPattern()
        , CurLine()
        , CurChannel()
      {
      }

      void SetPattern(uint_t idx)
      {
        Data.Patterns.resize(std::max<std::size_t>(idx + 1, Data.Patterns.size()));
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

      void FinishPattern(uint_t size)
      {
        if (const std::size_t skipped = size - CurPattern->GetSize())
        {
          CurPattern->AddLines(skipped);
        }
        CurLine = 0;
        CurPattern = 0;
      }
    };
  private:
    const ModuleData::RWPtr Data;
    const ModuleProperties::RWPtr Properties;

    BuildContext Context;
  };

  struct SampleState
  {
    SampleState()
      : Current(0)
      , Position(0)
      , LoopsCount(0)
    {
    }

    const Sample* Current;
    uint_t Position;
    uint_t LoopsCount;
  };

  struct OrnamentState
  {
    OrnamentState()
      : Current(0)
      , Position(0)
      , Finished()
      , Disabled()
    {
    }

    const Ornament* Current;
    uint_t Position;
    bool Finished;
    bool Disabled;
  };

  struct ChannelState
  {
    ChannelState()
      : Enabled(false), Envelope(false)
      , Note()
      , VolumeDelta(), BaseVolumeDelta()
      , Slide()
    {
    }
    bool Enabled;
    bool Envelope;
    boost::optional<uint_t> EnvelopeNote;
    boost::optional<uint_t> Note;
    SampleState Smp;
    OrnamentState Orn;
    uint_t VolumeDelta;
    uint_t BaseVolumeDelta;
    int_t Slide;
  };

  class DataRenderer : public AYM::DataRenderer
  {
  public:
    explicit DataRenderer(ModuleData::Ptr data)
       : Data(data)
    {
      Reset();
    }

    virtual void Reset()
    {
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        ChannelState& state = PlayerState[chan];
        state = ChannelState();
        state.Smp.Current = &Data->Samples[0];
        state.Orn.Current = &Data->Ornaments[0];
      }
    }

    virtual void SynthesizeData(const TrackState& state, AYM::TrackBuilder& track)
    {
      if (0 == state.Quirk())
      {
        //start pattern
        if (0 == state.Line())
        {
          ResetNotes();
        }
        GetNewLineState(state, track);
      }
      SynthesizeChannelsData(state, track);
    }
  private:
    void ResetNotes()
    {
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        PlayerState[chan].Note.reset();
      }
    }

    void GetNewLineState(const TrackState& state, AYM::TrackBuilder& track)
    {
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
      if (src.SampleNum)
      {
        dst.Smp.Current = &Data->Samples[*src.SampleNum];
      }
      if (src.OrnamentNum)
      {
        if (*src.OrnamentNum != 0x20 &&
            *src.OrnamentNum != 0x21)
        {
          dst.Orn.Current = &Data->Ornaments[*src.OrnamentNum];
          dst.Orn.Position = 0;
          dst.Orn.Finished = dst.Orn.Disabled = false;
          dst.Envelope = false;
        }
        else
        {
          static const Ornament STUB_ORNAMENT;
          dst.Orn.Current = &STUB_ORNAMENT;
        }
      }
      if (src.Volume)
      {
        dst.BaseVolumeDelta = *src.Volume;
      }
      for (Track::CommandsArray::const_iterator it = src.Commands.begin(), lim = src.Commands.end(); it != lim; ++it)
      {
        switch (it->Type)
        {
        case ENVELOPE:
          if (it->Param1 != -1)
          {
            track.SetEnvelopeType(it->Param1);
          }
          if (it->Param2 != -1)
          {
            track.SetEnvelopeTone(it->Param2);
            dst.EnvelopeNote.reset();
          }
          else if (it->Param3 != -1)
          {
            dst.EnvelopeNote = it->Param3;
          }
          dst.Envelope = true;
          break;
        case NOORNAMENT:
          dst.Envelope = false;
          dst.Orn.Finished = dst.Orn.Disabled = true;
          break;
        default:
          assert(!"Invalid command");
        }
      }
      if (src.Note)
      {
        dst.Note = *src.Note;
        dst.VolumeDelta = dst.BaseVolumeDelta;
        dst.Smp.Position = 0;
        dst.Slide = 0;
        dst.Smp.LoopsCount = 1;
        dst.Orn.Position = 0;
        if (!dst.Orn.Disabled)
        {
          dst.Orn.Finished = false;
        }
        if (dst.Envelope && dst.EnvelopeNote)
        {
          uint_t envNote = *dst.Note + *dst.EnvelopeNote;
          if (envNote >= 0x60)
          {
            envNote -= 0x60;
          }
          else if (envNote >= 0x30)
          {
            envNote -= 0x30;
          }
          const uint_t envFreq = track.GetFrequency((envNote & 0x7f) + 0x30);
          track.SetEnvelopeTone(envFreq & 0xff);
        }
      }
    }

    void SynthesizeChannelsData(const TrackState& state, AYM::TrackBuilder& track)
    {
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        AYM::ChannelBuilder channel = track.GetChannel(chan);
        SynthesizeChannel(state, PlayerState[chan], channel, track);
      }
    }

    void SynthesizeChannel(const TrackState& state, ChannelState& dst, AYM::ChannelBuilder& channel, AYM::TrackBuilder& track)
    {
      const Ornament& curOrnament = *dst.Orn.Current;
      const int_t ornamentLine = (dst.Orn.Finished || dst.Envelope) ? 0 : curOrnament.GetLine(dst.Orn.Position);
      const Sample& curSample = *dst.Smp.Current;
      const Sample::Line& curSampleLine = curSample.GetLine(dst.Smp.Position);

      dst.Slide += curSampleLine.Gliss;
      const int_t curNote = dst.Note ? int_t(*dst.Note) + Data->Transpositions[state.Position()] : 0;
      const int_t halftones = Math::Clamp<int_t>(curNote + ornamentLine, 0, 95);
      const int_t tone = Math::Clamp<int_t>(track.GetFrequency(halftones) + dst.Slide, 0, 4095);
      channel.SetTone(tone);
      
      //emulate level construction due to possibility of envelope bit reset
      int_t level = int_t(curSampleLine.Level) + (dst.Envelope ? 16 : 0) + dst.VolumeDelta - 15;
      if (!dst.Enabled || level < 0)
      {
        level = 0;
      }
      channel.SetLevel(level & 15);
      if (level & 16)
      {
        channel.EnableEnvelope();
      }
      if (curSampleLine.ToneMask)
      {
        channel.DisableTone();
      }
      if (!curSampleLine.NoiseMask && dst.Enabled)
      {
        if (level)
        {
          track.SetNoise(curSampleLine.Noise);
        }
      }
      else
      {
        channel.DisableNoise();
      }

      if (++dst.Smp.Position >= curSample.GetSize())
      {
        if (const uint_t* loop = curSample.GetLoop())
        {
          dst.Smp.Position = *loop;
          if (!--dst.Smp.LoopsCount)
          {
            dst.Smp.LoopsCount = curSample.VolumeDeltaPeriod;
            dst.VolumeDelta = Math::Clamp<int_t>(int_t(dst.VolumeDelta) + curSample.VolumeDeltaValue, 0, 15);
          }
        }
        else
        {
          dst.Enabled = false;
        }
      }

      if (++dst.Orn.Position >= curOrnament.GetSize())
      {
        if (const uint_t* loop = curOrnament.GetLoop())
        {
          dst.Orn.Position = *loop;
        }
        else
        {
          dst.Orn.Finished = true;
        }
      }
    }
  private:
    const ModuleData::Ptr Data;
    boost::array<ChannelState, Devices::AYM::CHANNELS> PlayerState;
  };

  class Chiptune : public AYM::Chiptune
  {
  public:
    Chiptune(ModuleData::Ptr data, ModuleProperties::Ptr properties)
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
    const ModuleData::Ptr Data;
    const ModuleProperties::Ptr Properties;
    const Information::Ptr Info;
  };
}

namespace ProSoundMaker
{
  std::auto_ptr<Formats::Chiptune::ProSoundMaker::Builder> CreateDataBuilder(ModuleData::RWPtr data, ZXTune::Module::ModuleProperties::RWPtr props)
  {
    return std::auto_ptr<Formats::Chiptune::ProSoundMaker::Builder>(new DataBuilder(data, props));
  }

  ZXTune::Module::AYM::Chiptune::Ptr CreateChiptune(ModuleData::Ptr data, ZXTune::Module::ModuleProperties::Ptr properties)
  {
    return boost::make_shared<Chiptune>(data, properties);
  }
}

namespace PSM
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char ID[] = {'P', 'S', 'M', 0};
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | GetSupportedAYMFormatConvertors();

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
      const ::ProSoundMaker::ModuleData::RWPtr modData = boost::make_shared< ::ProSoundMaker::ModuleData>();
      const std::auto_ptr<Formats::Chiptune::ProSoundMaker::Builder> dataBuilder = ::ProSoundMaker::CreateDataBuilder(modData, properties);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::ProSoundMaker::ParseCompiled(*rawData, *dataBuilder))
      {
        usedSize = container->Size();
        properties->SetSource(container);
        const Module::AYM::Chiptune::Ptr chiptune = ::ProSoundMaker::CreateChiptune(modData, properties);
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
  void RegisterPSMSupport(PluginsRegistrator& registrator)
  {
    //direct modules
    {
      const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateProSoundMakerCompiledDecoder();
      const ModulesFactory::Ptr factory = boost::make_shared<PSM::Factory>(decoder);
      const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(PSM::ID, decoder->GetDescription(), PSM::CAPS, factory);
      registrator.RegisterPlugin(plugin);
    }
  }
}
