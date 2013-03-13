/*
Abstract:
  PT2 modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ay_base.h"
#include "core/plugins/utils.h"
#include "core/plugins/registrator.h"
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
#include <formats/chiptune/protracker2.h>

#define FILE_TAG 077C8579

namespace ProTracker2
{
  //supported commands and parameters
  enum CmdType
  {
    //no parameters
    EMPTY,
    //r13,period
    ENVELOPE,
    //no parameters
    NOENVELOPE,
    //glissade
    GLISS,
    //glissade,target node
    GLISS_NOTE,
    //no parameters
    NOGLISS,
    //noise addon
    NOISE_ADD
  };

  struct Sample : public Formats::Chiptune::ProTracker2::Sample
  {
    Sample()
      : Formats::Chiptune::ProTracker2::Sample()
    {
    }

    Sample(const Formats::Chiptune::ProTracker2::Sample& rh)
      : Formats::Chiptune::ProTracker2::Sample(rh)
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

  using namespace ZXTune;
  using namespace ZXTune::Module;

  //SimpleOrnament::Loop is not used
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
    SparsedObjectsStorage<Sample> Samples;
    SparsedObjectsStorage<Ornament> Ornaments;
  };

  std::auto_ptr<Formats::Chiptune::ProTracker2::Builder> CreateDataBuilder(ModuleData::RWPtr data, ModuleProperties::RWPtr props);
  AYM::Chiptune::Ptr CreateChiptune(ModuleData::Ptr data, ModuleProperties::Ptr properties);
}

namespace ProTracker2
{
  class DataBuilder : public Formats::Chiptune::ProTracker2::Builder
  {
  public:
    DataBuilder(ModuleData::RWPtr data, ModuleProperties::RWPtr props)
      : Data(data)
      , Properties(props)
      , Builder(PatternsBuilder::Create<Devices::AYM::CHANNELS>())
    {
      Data->Patterns = Builder.GetPatterns();
    }

    virtual void SetProgram(const String& program)
    {
      Properties->SetProgram(program);
      Properties->SetFreqtable(TABLE_PROTRACKER2);
    }

    virtual void SetTitle(const String& title)
    {
      Properties->SetTitle(OptimizeString(title));
    }

    virtual void SetInitialTempo(uint_t tempo)
    {
      Data->InitialTempo = tempo;
    }

    virtual void SetSample(uint_t index, const Formats::Chiptune::ProTracker2::Sample& sample)
    {
      Data->Samples.Add(index, Sample(sample));
    }

    virtual void SetOrnament(uint_t index, const Formats::Chiptune::ProTracker2::Ornament& ornament)
    {
      Data->Ornaments.Add(index, Ornament(ornament.Loop, ornament.Lines.begin(), ornament.Lines.end()));
    }

    virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop)
    {
      Data->Order = boost::make_shared<SimpleOrderList>(positions.begin(), positions.end(), loop);
    }

    virtual void StartPattern(uint_t index)
    {
      Builder.SetPattern(index);
    }

    virtual void FinishPattern(uint_t size)
    {
      Builder.FinishPattern(size);
    }

    virtual void StartLine(uint_t index)
    {
      Builder.SetLine(index);
    }

    virtual void SetTempo(uint_t tempo)
    {
      Builder.GetLine().SetTempo(tempo);
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
      MutableCell& channel = Builder.GetChannel();
      channel.SetEnabled(true);
      if (Command* cmd = channel.FindCommand(GLISS_NOTE))
      {
        cmd->Param2 = int_t(note);
      }
      else
      {
        channel.SetNote(note);
      }
    }

    virtual void SetSample(uint_t sample)
    {
      Builder.GetChannel().SetSample(sample);
    }

    virtual void SetOrnament(uint_t ornament)
    {
      Builder.GetChannel().SetOrnament(ornament);
    }

    virtual void SetVolume(uint_t vol)
    {
      Builder.GetChannel().SetVolume(vol);
    }

    virtual void SetGlissade(int_t val)
    {
      Builder.GetChannel().AddCommand(GLISS, val);
    }

    virtual void SetNoteGliss(int_t val, uint_t limit)
    {
      Builder.GetChannel().AddCommand(GLISS_NOTE, val, limit);
    }

    virtual void SetNoGliss()
    {
      Builder.GetChannel().AddCommand(NOGLISS);
    }

    virtual void SetEnvelope(uint_t type, uint_t value)
    {
      Builder.GetChannel().AddCommand(ENVELOPE, type, value);
    }

    virtual void SetNoEnvelope()
    {
      Builder.GetChannel().AddCommand(NOENVELOPE);
    }

    virtual void SetNoiseAddon(int_t val)
    {
      Builder.GetChannel().AddCommand(NOISE_ADD, val);
    }
  private:
    const ModuleData::RWPtr Data;
    const ModuleProperties::RWPtr Properties;
    PatternsBuilder Builder;
  };

  const uint_t LIMITER = ~uint_t(0);

  inline uint_t GetVolume(uint_t volume, uint_t level)
  {
    return (volume * 17 + (volume > 7 ? 1 : 0)) * level / 256;
  }

  struct ChannelState
  {
    ChannelState()
      : Enabled(false), Envelope(false)
      , Note(), SampleNum(0), PosInSample(0)
      , OrnamentNum(0), PosInOrnament(0)
      , Volume(15), NoiseAdd(0)
      , Sliding(0), SlidingTargetNote(LIMITER), Glissade(0)
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
    int_t NoiseAdd;
    int_t Sliding;
    uint_t SlidingTargetNote;
    int_t Glissade;
  };

  class DataRenderer : public AYM::DataRenderer
  {
  public:
    explicit DataRenderer(ModuleData::Ptr data)
       : Data(data)
    {
    }

    virtual void Reset()
    {
      std::fill(PlayerState.begin(), PlayerState.end(), ChannelState());
    }

    virtual void SynthesizeData(const TrackModelState& state, AYM::TrackBuilder& track)
    {
      if (0 == state.Quirk())
      {
        GetNewLineState(state, track);
      }
      SynthesizeChannelsData(track);
    }
  private:
    void GetNewLineState(const TrackModelState& state, AYM::TrackBuilder& track)
    {
      if (const Line::Ptr line = state.LineObject())
      {
        for (uint_t chan = 0; chan != Devices::AYM::CHANNELS; ++chan)
        {
          if (const Cell::Ptr src = line->GetChannel(chan))
          {
            GetNewChannelState(*src, PlayerState[chan], track);
          }
        }
      }
    }

    void GetNewChannelState(const Cell& src, ChannelState& dst, AYM::TrackBuilder& track)
    {
      if (const bool* enabled = src.GetEnabled())
      {
        dst.Enabled = *enabled;
        if (!dst.Enabled)
        {
          dst.Sliding = dst.Glissade = 0;
          dst.SlidingTargetNote = LIMITER;
        }
        dst.PosInSample = dst.PosInOrnament = 0;
      }
      if (const uint_t* note = src.GetNote())
      {
        assert(src.GetEnabled());
        dst.Note = *note;
        dst.Sliding = dst.Glissade = 0;
        dst.SlidingTargetNote = LIMITER;
      }
      if (const uint_t* sample = src.GetSample())
      {
        dst.SampleNum = *sample;
        dst.PosInSample = 0;
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
        case NOISE_ADD:
          dst.NoiseAdd = it->Param1;
          break;
        case GLISS_NOTE:
          dst.Sliding = 0;
          dst.Glissade = it->Param1;
          dst.SlidingTargetNote = it->Param2;
          break;
        case GLISS:
          dst.Glissade = it->Param1;
          dst.SlidingTargetNote = LIMITER;
          break;
        case NOGLISS:
          dst.Glissade = 0;
          break;
        default:
          assert(!"Invalid command");
        }
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

      const Sample& curSample = Data->Samples.Get(dst.SampleNum);
      const Sample::Line& curSampleLine = curSample.GetLine(dst.PosInSample);
      const Ornament& curOrnament = Data->Ornaments.Get(dst.OrnamentNum);

      //apply tone
      const int_t halftones = int_t(dst.Note) + curOrnament.GetLine(dst.PosInOrnament);
      channel.SetTone(halftones, dst.Sliding + curSampleLine.Vibrato);
      if (curSampleLine.ToneMask)
      {
        channel.DisableTone();
      }
      //apply level
      channel.SetLevel(GetVolume(dst.Volume, curSampleLine.Level));
      //apply envelope
      if (dst.Envelope)
      {
        channel.EnableEnvelope();
      }
      //apply noise
      if (!curSampleLine.NoiseMask)
      {
        track.SetNoise(curSampleLine.Noise + dst.NoiseAdd);
      }
      else
      {
        channel.DisableNoise();
      }

      //recalculate gliss
      if (dst.SlidingTargetNote != LIMITER)
      {
        const int_t absoluteSlidingRange = track.GetSlidingDifference(dst.Note, dst.SlidingTargetNote);
        const int_t realSlidingRange = absoluteSlidingRange - (dst.Sliding + dst.Glissade);

        if ((dst.Glissade > 0 && realSlidingRange <= 0) ||
            (dst.Glissade < 0 && realSlidingRange >= 0))
        {
          dst.Note = dst.SlidingTargetNote;
          dst.SlidingTargetNote = LIMITER;
          dst.Sliding = dst.Glissade = 0;
        }
      }
      dst.Sliding += dst.Glissade;

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
      const TrackStateIterator::Ptr iterator = CreateTrackStateIterator(Data);
      const AYM::DataRenderer::Ptr renderer = boost::make_shared<DataRenderer>(Data);
      return AYM::CreateDataIterator(trackParams, iterator, renderer);
    }
  private:
    const ModuleData::Ptr Data;
    const ModuleProperties::Ptr Properties;
    const Information::Ptr Info;
  };
}

namespace ProTracker2
{
  std::auto_ptr<Formats::Chiptune::ProTracker2::Builder> CreateDataBuilder(ModuleData::RWPtr data, ModuleProperties::RWPtr props)
  {
    return std::auto_ptr<Formats::Chiptune::ProTracker2::Builder>(new DataBuilder(data, props));
  }

  AYM::Chiptune::Ptr CreateChiptune(ModuleData::Ptr data, ModuleProperties::Ptr properties)
  {
    return boost::make_shared<Chiptune>(data, properties);
  }
}

namespace PT2
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char ID[] = {'P', 'T', '2', 0};
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | SupportedAYMFormatConvertors;

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
      const ::ProTracker2::ModuleData::RWPtr modData = boost::make_shared< ::ProTracker2::ModuleData>();
      const std::auto_ptr<Formats::Chiptune::ProTracker2::Builder> dataBuilder = ::ProTracker2::CreateDataBuilder(modData, properties);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::ProTracker2::Parse(*rawData, *dataBuilder))
      {
        usedSize = container->Size();
        properties->SetSource(container);
        const AYM::Chiptune::Ptr chiptune = ::ProTracker2::CreateChiptune(modData, properties);
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
  void RegisterPT2Support(PlayerPluginsRegistrator& registrator)
  {
    //direct modules
    {
      const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateProTracker2Decoder();
      const ModulesFactory::Ptr factory = boost::make_shared<PT2::Factory>(decoder);
      const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(PT2::ID, decoder->GetDescription(), PT2::CAPS, factory);
      registrator.RegisterPlugin(plugin);
    }
  }
}
