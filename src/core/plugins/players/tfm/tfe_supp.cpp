/*
Abstract:
  TFE modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "tfm_base.h"
#include "core/plugins/registrator.h"
#include "core/plugins/players/creation_result.h"
#include "core/plugins/players/module_properties.h"
#include "core/plugins/players/simple_orderlist.h"
//common includes
#include <contract.h>
#include <tools.h>
//library includes
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/fm/tfmmusicmaker.h>
#include <math/fixedpoint.h>
//boost includes
#include <boost/make_shared.hpp>

namespace TFMMusicMaker
{
  enum CmdType
  {
    EMPTY,
    //add1, add2
    ARPEGGIO,
    //step
    TONESLIDE,
    //step
    PORTAMENTO,
    //speed, depth
    VIBRATO,
    //op, value
    LEVEL,
    //speedup, speeddown
    VOLSLIDE,
    //on/off
    SPECMODE,
    //op, offset
    TONEOFFSET,
    //op, value
    MULTIPLE,
    //mask
    MIXING,
    //val
    PANE,
    //period
    NOTERETRIG,
    //pos
    NOTECUT,
    //pos
    NOTEDELAY,
    //
    DROPEFFECTS,
    //val
    FEEDBACK
  };

  typedef Formats::Chiptune::TFMMusicMaker::Instrument Instrument;

  using namespace ZXTune;
  using namespace ZXTune::Module;

  class ModuleData : public TrackModel
  {
  public:
    typedef boost::shared_ptr<ModuleData> RWPtr;
    typedef boost::shared_ptr<const ModuleData> Ptr;

    ModuleData()
      : EvenInitialTempo()
      , OddInitialTempo()
      , InitialTempoInterleave()
    {
    }

    virtual uint_t GetInitialTempo() const
    {
      return EvenInitialTempo;
    }

    virtual const OrderList& GetOrder() const
    {
      return *Order;
    }

    virtual const PatternsSet& GetPatterns() const
    {
      return *Patterns;
    }

    uint_t EvenInitialTempo;
    uint_t OddInitialTempo;
    uint_t InitialTempoInterleave;
    OrderList::Ptr Order;
    PatternsSet::Ptr Patterns;
    SparsedObjectsStorage<Instrument> Instruments;
  };

  std::auto_ptr<Formats::Chiptune::TFMMusicMaker::Builder> CreateDataBuilder(ModuleData::RWPtr data, ModuleProperties::RWPtr props);
  TFM::Chiptune::Ptr CreateChiptune(ModuleData::Ptr data, ModuleProperties::Ptr properties);
}

namespace TFMMusicMaker
{
  class DataBuilder : public Formats::Chiptune::TFMMusicMaker::Builder
  {
  public:
    DataBuilder(ModuleData::RWPtr data, ModuleProperties::RWPtr props)
      : Data(data)
      , Properties(props)
      , Builder(PatternsBuilder::Create<TFM::TRACK_CHANNELS>())
    {
      Data->Patterns = Builder.GetPatterns();
    }

    virtual Formats::Chiptune::MetaBuilder& GetMetaBuilder()
    {
      return *Properties;
    }

    virtual void SetTempo(uint_t evenTempo, uint_t oddTempo, uint_t interleavePeriod)
    {
      Require(evenTempo == oddTempo);
      Data->EvenInitialTempo = evenTempo;
      Data->OddInitialTempo = oddTempo;
      Data->InitialTempoInterleave = interleavePeriod;
    }

    virtual void SetDate(const Formats::Chiptune::TFMMusicMaker::Date& /*created*/, const Formats::Chiptune::TFMMusicMaker::Date& /*saved*/)
    {
    }

    virtual void SetComment(const String& comment)
    {
      Properties->SetComment(comment);
    }

    virtual void SetInstrument(uint_t index, const Formats::Chiptune::TFMMusicMaker::Instrument& instrument)
    {
      Data->Instruments.Add(index, instrument);
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

    virtual void SetKeyOff()
    {
      Builder.GetChannel().SetEnabled(false);
    }

    virtual void SetNote(uint_t note)
    {
      Builder.GetChannel().SetNote(note);
    }

    virtual void SetVolume(uint_t vol)
    {
      Builder.GetChannel().SetVolume(vol);
    }

    virtual void SetInstrument(uint_t ins)
    {
      Builder.GetChannel().SetSample(ins);
    }

    virtual void SetArpeggio(uint_t add1, uint_t add2)
    {
      Require(false);
      Builder.GetChannel().AddCommand(ARPEGGIO, add1, add2);
    }

    virtual void SetSlide(int_t step)
    {
      Require(false);
      Builder.GetChannel().AddCommand(TONESLIDE, step);
    }

    virtual void SetPortamento(int_t step)
    {
      Require(false);
      Builder.GetChannel().AddCommand(PORTAMENTO, step);
    }

    virtual void SetVibrato(uint_t speed, uint_t depth)
    {
      Require(false);
      Builder.GetChannel().AddCommand(VIBRATO, speed, depth);
    }

    virtual void SetTotalLevel(uint_t op, uint_t value)
    {
      Require(false);
      Builder.GetChannel().AddCommand(LEVEL, op, value);
    }

    virtual void SetVolumeSlide(uint_t up, uint_t down)
    {
      Require(false);
      Builder.GetChannel().AddCommand(VOLSLIDE, up, down);
    }

    virtual void SetSpecialMode(bool on)
    {
      Require(false);
      Builder.GetChannel().AddCommand(SPECMODE, on);
    }

    virtual void SetToneOffset(uint_t op, uint_t offset)
    {
      Require(false);
      Builder.GetChannel().AddCommand(TONEOFFSET, op, offset);
    }

    virtual void SetMultiple(uint_t op, uint_t val)
    {
      Require(false);
      Builder.GetChannel().AddCommand(MULTIPLE, op, val);
    }

    virtual void SetOperatorsMixing(uint_t mask)
    {
      Require(false);
      Builder.GetChannel().AddCommand(MIXING, mask);
    }

    virtual void SetLoopStart()
    {
      Require(false);
    }

    virtual void SetLoopEnd(uint_t additionalCount)
    {
      Require(false);
    }

    virtual void SetPane(uint_t pane)
    {
      Require(false);
      Builder.GetChannel().AddCommand(PANE, pane);
    }

    virtual void SetNoteRetrig(uint_t period)
    {
      Require(false);
      Builder.GetChannel().AddCommand(NOTERETRIG, period);
    }

    virtual void SetNoteCut(uint_t quirk)
    {
      Require(false);
      Builder.GetChannel().AddCommand(NOTECUT, quirk);
    }

    virtual void SetNoteDelay(uint_t quirk)
    {
      Require(false);
      Builder.GetChannel().AddCommand(NOTEDELAY, quirk);
    }

    virtual void SetDropEffects()
    {
      Require(false);
      Builder.GetChannel().AddCommand(DROPEFFECTS);
    }

    virtual void SetFeedback(uint_t val)
    {
      Require(false);
      Builder.GetChannel().AddCommand(FEEDBACK, val);
    }

    virtual void SetTempoInterleave(uint_t val)
    {
    }

    virtual void SetTempoValues(uint_t even, uint_t odd)
    {
      Require(even == odd);
      Builder.GetLine().SetTempo(even);
    }
  private:
    const ModuleData::RWPtr Data;
    const ModuleProperties::RWPtr Properties;
    PatternsBuilder Builder;
  };

  typedef Math::FixedPoint<int_t, 32> NoteType;
  typedef Math::FixedPoint<int_t, 8> VolumeType;

  struct ChannelState
  {
    ChannelState()
      : CurInstrument(0)
      , Algorithm(-1)
      , TotalLevel()
      , Note(-1)
      , Volume(31)
      , HasToneChange(false)
      , HasVolumeChange(false)
      , HasEffBx(false)
      , PortamentoTarget(-1)
    {
    }

    const Instrument* CurInstrument;
    int_t Algorithm;
    uint_t TotalLevel[4];
    NoteType Note;
    VolumeType Volume;
    bool HasToneChange;
    bool HasVolumeChange;
    bool HasEffBx;

    NoteType PortamentoTarget;
  };

  class DataRenderer : public TFM::DataRenderer
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

    virtual void SynthesizeData(const TrackModelState& state, TFM::TrackBuilder& track)
    {
      if (0 == state.Quirk())
      {
        GetNewLineState(state, track);
      }
      SynthesizeChannelsData(track);
    }
  private:
    void GetNewLineState(const TrackModelState& state, TFM::TrackBuilder& track)
    {
      if (const Line::Ptr line = state.LineObject())
      {
        for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
        {
          if (const Cell::Ptr src = line->GetChannel(chan))
          {
            TFM::ChannelBuilder channel = track.GetChannel(chan);
            GetNewChannelState(*src, PlayerState[chan], channel);
          }
        }
      }
    }

    void GetNewChannelState(const Cell& src, ChannelState& dst, TFM::ChannelBuilder& channel)
    {
      int_t multiplies[4] = {-1, -1, -1, -1};
      bool dropEffects = false;
      bool hasNoteDelay = false;
      bool hasPortamento = false;
      bool hasOpMixer = false;
      for (CommandsIterator it = src.GetCommands(); it; ++it)
      {
        switch (it->Type)
        {
        case PORTAMENTO:
          hasPortamento = true;
          break;
        case MULTIPLE:
          multiplies[it->Param1] = it->Param2;
          break;
        case MIXING:
          hasOpMixer = true;
          break;
        case NOTEDELAY:
          hasNoteDelay = true;
          break;
        }
      }

      if (const bool* enabled = src.GetEnabled())
      {
        if (!*enabled)
        {
          channel.KeyOff();
        }
      }
      if (const uint_t* note = src.GetNote())
      {
        if (!hasPortamento)
        {
          channel.KeyOff();
        }
        const Instrument* newInstrument = GetNewInstrument(src);
        if (!dst.CurInstrument && !newInstrument)
        {
          newInstrument = Data->Instruments.Find(1);
        }
        if (dropEffects && dst.CurInstrument && !newInstrument)
        {
          newInstrument = dst.CurInstrument;
        }
        if ((newInstrument && newInstrument != dst.CurInstrument) || dropEffects)
        {
          if (const uint_t* volume = src.GetVolume())
          {
            dst.Volume = *volume;
          }
          dst.CurInstrument = newInstrument;
          LoadInstrument(multiplies, dst, channel);
          dst.HasVolumeChange = true;
        }
        if (hasPortamento)
        {
          dst.PortamentoTarget = *note;
        }
        else
        {
          dst.Note = *note;
          //...
          dst.HasToneChange = true;
          if (!hasNoteDelay && !hasOpMixer)
          {
            channel.KeyOn();
          }
        }
      }
      if (const uint_t* volume = src.GetVolume())
      {
        const VolumeType newVol = VolumeType(*volume);
        if (newVol != dst.Volume)
        {
          dst.Volume = newVol;
          dst.HasVolumeChange = true;
        }
      }
    }

    const Instrument* GetNewInstrument(const Cell& src) const
    {
      if (const uint_t* instrument = src.GetSample())
      {
        return Data->Instruments.Find(*instrument);
      }
      else
      {
        return 0;
      }
    }

    void LoadInstrument(const int_t multiplies[], ChannelState& dst, TFM::ChannelBuilder& channel)
    {
      const Instrument& ins = *dst.CurInstrument;
      channel.SetupConnection(dst.Algorithm = ins.Algorithm, ins.Feedback);
      for (uint_t opIdx = 0; opIdx != 4; ++opIdx)
      {
        const Instrument::Operator& op = ins.Operators[opIdx];
        dst.TotalLevel[opIdx] = op.TotalLevel;
        const uint_t multiple = multiplies[opIdx] != -1 ? multiplies[opIdx] : op.Multiple;
        channel.SetDetuneMultiple(opIdx, op.Detune, multiple);
        channel.SetRateScalingAttackRate(opIdx, op.RateScaling, op.Attack);
        channel.SetDecay(opIdx, op.Decay);
        channel.SetSustain(opIdx, op.Sustain);
        channel.SetSustainLevelReleaseRate(opIdx, op.SustainLevel, op.Release);
        channel.SetEnvelopeType(opIdx, op.EnvelopeType);
      }
    }

    void SynthesizeChannelsData(TFM::TrackBuilder& track)
    {
      //TODO: CHANNELS
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        TFM::ChannelBuilder channel = track.GetChannel(chan);
        SynthesizeChannel(chan, channel);
      }
    }

    void SynthesizeChannel(uint_t idx, TFM::ChannelBuilder& channel)
    {
      ChannelState& state = PlayerState[idx];

      //TODO: update effects
      if (state.HasToneChange)
      {
        SetTone(idx, state, channel);
        state.HasToneChange = false;
      }
      if (state.HasVolumeChange)
      {
        SetLevel(state, channel);
        state.HasVolumeChange = false;
      }
    }

    void SetTone(uint_t idx, const ChannelState& state, TFM::ChannelBuilder& channel) const
    {
      static const uint_t FREQS[] =
      {
        707, 749, 793, 840, 890, 943, 999, 1059, 1122, 1189, 1259, 1334, 1413, 1497
      };
      const NoteType note = state.Note;
      const uint_t totalHalftones = note.Integer();
      const uint_t octave = totalHalftones / 12;
      const uint_t halftone = totalHalftones % 12;
      const uint_t val = FREQS[halftone] + ((FREQS[halftone + 1] - FREQS[halftone]) * note.Fraction() + note.PRECISION / 2) / note.PRECISION;
      if (idx != 2 || !state.HasEffBx)
      {
        channel.SetTone(octave, val);
      }
    }

    void SetLevel(const ChannelState& state, TFM::ChannelBuilder& channel) const
    {
      static const uint_t MIXER_TABLE[8] =
      {
        0x8, 0x8, 0x8, 0x8, 0x0c, 0xe, 0xe, 0x0f
      };
      static const uint_t LEVELS_TABLE[32] =
      {
        0x00, 0x00, 0x58, 0x5a, 0x5b, 0x5d, 0x5f, 0x60,
        0x61, 0x62, 0x64, 0x66, 0x68, 0x6a, 0x6b, 0x6d,
        0x6e, 0x70, 0x71, 0x72, 0x73, 0x74, 0x76, 0x77,
        0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f
      };
      if (state.Algorithm < 0)
      {
        return;
      }
      const uint_t mix = MIXER_TABLE[state.Algorithm];
      const uint_t level = LEVELS_TABLE[state.Volume.Integer()];
      for (uint_t op = 0; op != 4; ++op)
      {
        const uint_t out = 0 != (mix & (1 << op)) ? ScaleTL(state.TotalLevel[op], level) : state.TotalLevel[op];
        channel.SetTotalLevel(op, out);
      }
    }

    static uint_t ScaleTL(uint_t tl, uint_t scale)
    {
      return 0x7f - ((0x7f - tl) * scale / 127);
    }
  private:
    const ModuleData::Ptr Data;
    boost::array<ChannelState, TFM::TRACK_CHANNELS> PlayerState;
  };

  class Chiptune : public TFM::Chiptune
  {
  public:
    Chiptune(ModuleData::Ptr data, ModuleProperties::Ptr properties)
      : Data(data)
      , Properties(properties)
      , Info(CreateTrackInfo(Data, TFM::TRACK_CHANNELS))
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

    virtual TFM::DataIterator::Ptr CreateDataIterator(TFM::TrackParameters::Ptr trackParams) const
    {
      const TrackStateIterator::Ptr iterator = CreateTrackStateIterator(Data);
      const TFM::DataRenderer::Ptr renderer = boost::make_shared<DataRenderer>(Data);
      return TFM::CreateDataIterator(trackParams, iterator, renderer);
    }
  private:
    const ModuleData::Ptr Data;
    const ModuleProperties::Ptr Properties;
    const Information::Ptr Info;
  };
}

namespace TFMMusicMaker
{
  std::auto_ptr<Formats::Chiptune::TFMMusicMaker::Builder> CreateDataBuilder(ModuleData::RWPtr data, ModuleProperties::RWPtr props)
  {
    return std::auto_ptr<Formats::Chiptune::TFMMusicMaker::Builder>(new DataBuilder(data, props));
  }

  TFM::Chiptune::Ptr CreateChiptune(ModuleData::Ptr data, ModuleProperties::Ptr properties)
  {
    return boost::make_shared<Chiptune>(data, properties);
  }
}

namespace TFE
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char ID_05[] = {'T', 'F', '0', 0};
  const Char ID_13[] = {'T', 'F', 'E', 0};
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_FM | CAP_CONV_RAW;

  class Factory : public ModulesFactory
  {
  public:
    explicit Factory(Formats::Chiptune::TFMMusicMaker::Decoder::Ptr decoder)
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
      const ::TFMMusicMaker::ModuleData::RWPtr modData = boost::make_shared< ::TFMMusicMaker::ModuleData>();
      const std::auto_ptr<Formats::Chiptune::TFMMusicMaker::Builder> dataBuilder = ::TFMMusicMaker::CreateDataBuilder(modData, properties);
      if (const Formats::Chiptune::Container::Ptr container = Decoder->Parse(*data, *dataBuilder))
      {
        usedSize = container->Size();
        properties->SetSource(container);
        const TFM::Chiptune::Ptr chiptune = ::TFMMusicMaker::CreateChiptune(modData, properties);
        return TFM::CreateHolder(chiptune);
      }
      return Holder::Ptr();
    }
  private:
    const Formats::Chiptune::TFMMusicMaker::Decoder::Ptr Decoder;
  };
}

namespace ZXTune
{
  void RegisterTFESupport(PlayerPluginsRegistrator& registrator)
  {
    {
      const Formats::Chiptune::TFMMusicMaker::Decoder::Ptr decoder = Formats::Chiptune::TFMMusicMaker::Ver05::CreateDecoder();
      const ModulesFactory::Ptr factory = boost::make_shared<TFE::Factory>(decoder);
      const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(TFE::ID_05, decoder->GetDescription(), TFE::CAPS, factory);
      registrator.RegisterPlugin(plugin);
    }
    {
      const Formats::Chiptune::TFMMusicMaker::Decoder::Ptr decoder = Formats::Chiptune::TFMMusicMaker::Ver13::CreateDecoder();
      const ModulesFactory::Ptr factory = boost::make_shared<TFE::Factory>(decoder);
      const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(TFE::ID_13, decoder->GetDescription(), TFE::CAPS, factory);
      registrator.RegisterPlugin(plugin);
    }
  }
}
