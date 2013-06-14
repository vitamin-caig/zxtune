/*
Abstract:
  DMM modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "digital.h"
#include "core/plugins/enumerator.h"
#include "core/plugins/registrator.h"
#include "core/plugins/utils.h"
#include "core/plugins/players/creation_result.h"
#include "core/plugins/players/module_properties.h"
#include "core/plugins/players/simple_orderlist.h"
#include "core/plugins/players/tracking.h"
//common includes
#include <byteorder.h>
#include <error_tools.h>
#include <tools.h>
//library includes
#include <binary/container_factories.h>
#include <binary/typed_container.h>
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <devices/dac/sample_factories.h>
#include <math/numeric.h>
#include <sound/mixer_factory.h>
//std includes
#include <utility>
//boost includes
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>
//text includes
#include <formats/text/chiptune.h>

#define FILE_TAG 312C703E

namespace
{
  const Debug::Stream Dbg("Core::DMMSupp");
}

namespace DMM
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  const std::size_t MAX_POSITIONS_COUNT = 0x32;
  const std::size_t MAX_PATTERN_SIZE = 64;
  const std::size_t PATTERNS_COUNT = 24;
  const std::size_t CHANNELS_COUNT = 3;
  const std::size_t SAMPLES_COUNT = 16;//15 really

  const uint64_t Z80_FREQ = 3500000;
  //119+116+111+10=356 ticks/out cycle = 9831 outs/sec (AY)
  const uint_t TICKS_PER_CYCLE = 119 + 116 + 111 + 10;
  //C-1 step 44/256 32.7Hz = ~1689 samples/sec
  const uint_t C_1_STEP = 44;
  const uint_t SAMPLES_FREQ = Z80_FREQ * C_1_STEP / TICKS_PER_CYCLE / 256;
  const uint_t RENDERS_PER_SEC = Z80_FREQ / TICKS_PER_CYCLE;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct Pattern
  {
    PACK_PRE struct Line
    {
      PACK_PRE struct Channel
      {
        uint8_t NoteCommand;
        uint8_t SampleParam;
        uint8_t Effect;
      } PACK_POST;

      Channel Channels[CHANNELS_COUNT];
    } PACK_POST;

    Line Lines[1];//at least 1
  } PACK_POST;

  PACK_PRE struct SampleInfo
  {
    uint8_t Name[9];
    uint16_t Start;
    uint8_t Bank;
    uint16_t Limit;
    uint16_t Loop;
  } PACK_POST;

  PACK_PRE struct MixedLine
  {
    Pattern::Line::Channel Mixin;
    uint8_t Period;
  } PACK_POST;

  PACK_PRE struct Header
  {
    //+0
    boost::array<uint16_t, 6> EndOfBanks;
    //+0x0c
    uint8_t PatternSize;
    //+0x0d
    uint8_t Padding1;
    //+0x0e
    boost::array<uint8_t, 0x32> Positions;
    //+0x40
    uint8_t Tempo;
    //+0x41
    uint8_t Loop;
    //+0x42
    uint8_t Padding2;
    //+0x43
    uint8_t Length;
    //+0x44
    uint8_t HeaderSizeSectors;
    //+0x45
    MixedLine Mixings[5];
    //+0x59
    uint8_t Padding3;
    //+0x5a
    SampleInfo SampleDescriptions[SAMPLES_COUNT];
    //+0x15a
    uint8_t Padding4[4];
    //+0x15e
    //patterns starts here
  } PACK_POST;

#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(MixedLine) == 4);
  BOOST_STATIC_ASSERT(sizeof(SampleInfo) == 16);
  BOOST_STATIC_ASSERT(sizeof(Header) == 0x15e);
  BOOST_STATIC_ASSERT(sizeof(Pattern::Line) == 9);

  //supported tracking commands
  enum CmdType
  {
    //no parameters
    EMPTY_CMD,
    //2 param: direction, step
    FREQ_FLOAT,
    //3 params: isApply, step, period
    VIBRATO,
    //3 params: isApply, step, period
    ARPEGGIO,
    //3 param: direction, step, period
    TONE_SLIDE,
    //2 params: isApply, period
    DOUBLE_NOTE,
    //3 params: isApply, limit, period
    VOL_ATTACK,
    //3 params: isApply, limit, period
    VOL_DECAY,
    //1 param
    MIX_SAMPLE,
  };

  enum
  {
    NOTE_BASE = 1,
    NO_DATA = 70,
    REST_NOTE = 61,
    SET_TEMPO = 62,
    SET_FREQ_FLOAT = 63,
    SET_VIBRATO = 64,
    SET_ARPEGGIO = 65,
    SET_SLIDE = 66,
    SET_DOUBLE = 67,
    SET_ATTACK = 68,
    SET_DECAY = 69,

    FX_FLOAT_UP = 1,
    FX_FLOAT_DN = 2,
    FX_VIBRATO = 3,
    FX_ARPEGGIO = 4,
    FX_STEP_UP = 5,
    FX_STEP_DN = 6,
    FX_DOUBLE = 7,
    FX_ATTACK = 8,
    FX_DECAY = 9,
    FX_MIX = 10,
    FX_DISABLE = 15
  };

  const std::size_t SAMPLES_ADDR = 0xc000;

  class ModuleData : public DAC::ModuleData
  {
  public:
    typedef boost::shared_ptr<const ModuleData> Ptr;


    struct MixedChannel
    {
      MutableCell Mixin;
      uint_t Period;

      MixedChannel()
        : Period()
      {
      }
    };

    boost::array<MixedChannel, 64> Mixes;
  };

  void ParseChannel(const Pattern::Line::Channel& srcChan, MutableCell& dstChan)
  {
    const uint_t note = srcChan.NoteCommand;
    if (NO_DATA == note)
    {
      return;
    }
    if (note < SET_TEMPO)
    {
      if (note)
      {
        if (note != REST_NOTE)
        {
          dstChan.SetEnabled(true);
          dstChan.SetNote(note - NOTE_BASE);
        }
        else
        {
          dstChan.SetEnabled(false);
        }
      }
      const uint_t params = srcChan.SampleParam;
      if (const uint_t sample = params >> 4)
      {
        dstChan.SetSample(sample);
      }
      if (const uint_t volume = params & 15)
      {
        dstChan.SetVolume(volume);
      }
      switch (srcChan.Effect)
      {
      case 0:
        break;
      case FX_FLOAT_UP:
        dstChan.AddCommand(FREQ_FLOAT, 1);
        break;
      case FX_FLOAT_DN:
        dstChan.AddCommand(FREQ_FLOAT, -1);
        break;
      case FX_VIBRATO:
        dstChan.AddCommand(VIBRATO, true);
        break;
      case FX_ARPEGGIO:
        dstChan.AddCommand(ARPEGGIO, true);
        break;
      case FX_STEP_UP:
        dstChan.AddCommand(TONE_SLIDE, 1);
        break;
      case FX_STEP_DN:
        dstChan.AddCommand(TONE_SLIDE, -1);
        break;
      case FX_DOUBLE:
        dstChan.AddCommand(DOUBLE_NOTE, true);
        break;
      case FX_ATTACK:
        dstChan.AddCommand(VOL_ATTACK, true);
        break;
      case FX_DECAY:
        dstChan.AddCommand(VOL_DECAY, true);
        break;
      case FX_DISABLE:
        dstChan.AddCommand(EMPTY_CMD);
        break;
      default:
        {
          const uint_t mixNum = srcChan.Effect - FX_MIX;
          //according to player there can be up to 64 mixins (with enabled 4)
          dstChan.AddCommand(MIX_SAMPLE, mixNum % 64);
        }
        break; 
      }
    }
    else
    {
      switch (note)
      {
      case SET_TEMPO:
        break;
      case SET_FREQ_FLOAT:
        dstChan.AddCommand(FREQ_FLOAT, 0, srcChan.SampleParam);
        break;
      case SET_VIBRATO:
        dstChan.AddCommand(VIBRATO, false, srcChan.SampleParam, srcChan.Effect);
        break;
      case SET_ARPEGGIO:
        dstChan.AddCommand(ARPEGGIO, false, srcChan.SampleParam, srcChan.Effect);
        break;
      case SET_SLIDE:
        dstChan.AddCommand(TONE_SLIDE, 0, srcChan.SampleParam, srcChan.Effect);
        break;
      case SET_DOUBLE:
        dstChan.AddCommand(DOUBLE_NOTE, false, srcChan.SampleParam);
        break;
      case SET_ATTACK:
        dstChan.AddCommand(VOL_ATTACK, false, srcChan.SampleParam & 15, srcChan.Effect);
        break;
      case SET_DECAY:
        dstChan.AddCommand(VOL_DECAY, false, srcChan.SampleParam & 15, srcChan.Effect);
        break;
      }
    }
  }

  void ParsePattern(uint_t size, const Pattern& src, PatternsBuilder& builder)
  {
    for (uint_t lineNum = 0; lineNum != size; ++lineNum)
    {
      const Pattern::Line& srcLine = src.Lines[lineNum];
      builder.SetLine(lineNum);
      for (uint_t chanNum = 0; chanNum != CHANNELS_COUNT; ++chanNum)
      {
        builder.SetChannel(chanNum);
        const Pattern::Line::Channel& srcChan = srcLine.Channels[chanNum];
        ParseChannel(srcChan, builder.GetChannel());
        if (srcChan.NoteCommand == SET_TEMPO && srcChan.SampleParam)
        {
          builder.GetLine().SetTempo(srcChan.SampleParam);
        }
      }
    }
  }

  ModuleData::Ptr Parse(PropertiesBuilder& properties, Binary::Container::Ptr rawData)
  {
    const boost::shared_ptr<ModuleData> Data = boost::make_shared<ModuleData>();
    //assume data is correct
    const Binary::TypedContainer& data(*rawData);
    const Header& header = *data.GetField<Header>(0);

    //fill order
    const uint_t positionsCount = header.Length + 1;
    Data->Order = boost::make_shared<SimpleOrderList>(header.Loop, header.Positions.begin(), header.Positions.begin() + positionsCount);

    //fill patterns
    const std::size_t patternsCount = 1 + *std::max_element(header.Positions.begin(), header.Positions.begin() + positionsCount);
    const uint_t patternSize = header.PatternSize;
    {
      PatternsBuilder builder = PatternsBuilder::Create<CHANNELS_COUNT>();
      Data->Patterns = builder.GetResult();
      for (std::size_t patIdx = 0; patIdx < std::min(patternsCount, PATTERNS_COUNT); ++patIdx)
      {
        const Pattern* const src = safe_ptr_cast<const Pattern*>(&header + 1) + patIdx * patternSize;
        builder.SetPattern(patIdx);
        ParsePattern(patternSize, *src, builder);
      }
    }

    //big mixins amount support
    for (std::size_t mixIdx = 0; mixIdx != 64; ++mixIdx)
    {
      const MixedLine& src = header.Mixings[mixIdx];
      ModuleData::MixedChannel& dst = Data->Mixes[mixIdx];
      ParseChannel(src.Mixin, dst.Mixin);
      dst.Period = src.Period;
    }

    const bool is4bitSamples = true;//TODO: detect
    std::size_t lastData = 256 * header.HeaderSizeSectors;

    //bank => Data
    typedef std::map<std::size_t, Binary::Container::Ptr> Bank2Data;
    Bank2Data regions;
    for (std::size_t layIdx = 0; layIdx != header.EndOfBanks.size(); ++layIdx)
    {
      static const std::size_t BANKS[] = {0x50, 0x51, 0x53, 0x54, 0x56, 0x57};

      const uint_t bankNum = BANKS[layIdx];
      const std::size_t bankEnd = fromLE(header.EndOfBanks[layIdx]);
      if (bankEnd <= SAMPLES_ADDR)
      {
        Dbg("Skipping bank #%1$02x (end=#%2$04x)", bankNum, bankEnd);
        continue;
      }
      const std::size_t bankSize = bankEnd - SAMPLES_ADDR;
      const std::size_t alignedBankSize = Math::Align<std::size_t>(bankSize, 256);
      if (is4bitSamples)
      {
        const std::size_t realSize = 256 * (1 + alignedBankSize / 512);
        const Binary::Container::Ptr packedSample = rawData->GetSubcontainer(lastData, realSize);
        regions[bankNum] = packedSample;
        Dbg("Added unpacked bank #%1$02x (end=#%2$04x, size=#%3$04x) offset=#%4$05x", bankNum, bankEnd, realSize, lastData);
        lastData += realSize;
      }
      else
      {
        regions[bankNum] = rawData->GetSubcontainer(lastData, alignedBankSize);
        Dbg("Added bank #%1$02x (end=#%2$04x, size=#%3$04x) offset=#%4$05x", bankNum, bankEnd, alignedBankSize, lastData);
        lastData += alignedBankSize;
      }
    }

    for (uint_t samIdx = 1; samIdx != SAMPLES_COUNT; ++samIdx)
    {
      const SampleInfo& srcSample = header.SampleDescriptions[samIdx - 1];
      if (srcSample.Name[0] == '.')
      {
        Dbg("No sample %1%", samIdx);
        continue;
      }
      const std::size_t sampleStart = fromLE(srcSample.Start);
      const std::size_t sampleEnd = fromLE(srcSample.Limit);
      const std::size_t sampleLoop = fromLE(srcSample.Loop);
      Dbg("Processing sample %1% (bank #%2$02x #%3$04x..#%4$04x loop #%5$04x)", samIdx, uint_t(srcSample.Bank), sampleStart, sampleEnd, sampleLoop);
      if (sampleStart < SAMPLES_ADDR ||
          sampleStart > sampleEnd ||
          sampleStart > sampleLoop)
      {
        Dbg("Skipped due to invalid layout");
        continue;
      }
      if (!regions.count(srcSample.Bank))
      {
        Dbg("Skipped. No data");
        continue;
      }
      const Binary::Container::Ptr bankData = regions[srcSample.Bank];
      const std::size_t offsetInBank = sampleStart - SAMPLES_ADDR;
      const std::size_t limitInBank = sampleEnd - SAMPLES_ADDR;
      const std::size_t sampleSize = limitInBank - offsetInBank;
      const uint_t multiplier = is4bitSamples ? 2 : 1;
      if (limitInBank > multiplier * bankData->Size())
      {
        Dbg("Skipped. Not enough data");
        continue;
      }
      const std::size_t realSampleSize = sampleSize >= 12 ? (sampleSize - 12) : sampleSize;
      if (const Binary::Data::Ptr content = bankData->GetSubcontainer(offsetInBank / multiplier, realSampleSize / multiplier))
      {
        const std::size_t loop = sampleLoop - sampleStart;
        Data->Samples.Add(samIdx, is4bitSamples
          ? Devices::DAC::CreateU4PackedSample(content, loop)
          : Devices::DAC::CreateU8Sample(content, loop));
      }
    }
    Data->InitialTempo = header.Tempo;

    //meta properties
    {
      const ModuleRegion fixedRegion(sizeof(header), sizeof(Pattern::Line) * patternsCount * patternSize);
      properties.SetSource(rawData, lastData, fixedRegion);
    }
    properties.SetProgram(Text::DIGITALMUSICMAKER_DECODER_DESCRIPTION);
    properties.SetSamplesFreq(SAMPLES_FREQ);
    return Data;
  }

  class ChannelState
  {
  public:
    ChannelState()
      //values are from player' defaults
      : FreqSlideStep(1)
      , VibratoPeriod(4)
      , VibratoStep(3)
      , ArpeggioPeriod(1)
      , ArpeggioStep(18)
      , NoteSlidePeriod(2)
      , NoteSlideStep(12)
      , NoteDoublePeriod(3)
      , AttackPeriod(1)
      , AttackLimit(15)
      , DecayPeriod(1)
      , DecayLimit(1)
      , MixPeriod(3)
      , Counter(0)
      , Note(0)
      , NoteSlide(0)
      , FreqSlide(0)
      , Volume(15)
      , Sample(0)
      , Effect(&ChannelState::NoEffect)
    {
    }

    void OnFrame(DAC::ChannelDataBuilder& builder)
    {
      (this->*Effect)(builder);
    }

    void OnNote(const Cell& src, const ModuleData& data, DAC::ChannelDataBuilder& builder)
    {
      //if has new sample, start from it, else use previous sample
      const uint_t oldPos = src.GetSample() ? 0 : builder.GetState().PosInSample;
      ParseNote(src, builder);
      CommandsIterator it = src.GetCommands();
      if (!it)
      {
        return;
      }
      OldData = src;
      for (; it; ++it)
      {
        switch (it->Type)
        {
        case EMPTY_CMD:
          DisableEffect();
          break;
        case FREQ_FLOAT:
          if (it->Param1)
          {
            Effect = &ChannelState::FreqFloat;
            FreqSlideStep = it->Param1;
          }
          else
          {
            FreqSlideStep *= it->Param2;
          }
          break;
        case VIBRATO:
          if (it->Param1)
          {
            Effect = &ChannelState::Vibrato;
          }
          else
          {
            VibratoStep = it->Param2;
            VibratoPeriod = it->Param3;
          }
          break;
        case ARPEGGIO:
          if (it->Param1)
          {
            Effect = &ChannelState::Arpeggio;
          }
          else
          {
            ArpeggioStep = it->Param2;
            ArpeggioPeriod = it->Param3;
          }
          break;
        case TONE_SLIDE:
          if (it->Param1)
          {
            Effect = &ChannelState::NoteFloat;
            NoteSlideStep = it->Param1;
          }
          else
          {
            NoteSlideStep *= it->Param2;
            NoteSlidePeriod = it->Param3;
          }
          break;
        case DOUBLE_NOTE:
          if (it->Param1)
          {
            Effect = &ChannelState::DoubleNote;
          }
          else
          {
            NoteDoublePeriod = it->Param2;
          }
          break;
        case VOL_ATTACK:
          if (it->Param1)
          {
            Effect = &ChannelState::Attack;
          }
          else
          {
            AttackLimit = it->Param2;
            AttackPeriod = it->Param3;
          }
          break;
        case VOL_DECAY:
          if (it->Param1)
          {
            Effect = &ChannelState::Decay;
          }
          else
          {
            DecayLimit = it->Param2;
            DecayPeriod = it->Param3;
          }
          break;
        case MIX_SAMPLE:
          {
            DacState = builder.GetState();
            DacState.PosInSample = oldPos;
            const ModuleData::MixedChannel& mix = data.Mixes[it->Param1];
            ParseNote(mix.Mixin, builder);
            MixPeriod = mix.Period;
            Effect = &ChannelState::Mix;
          }
          break;
        }
      }
    }

    void GetState(DAC::ChannelDataBuilder& builder)
    {
      builder.SetNote(Note);
      builder.SetNoteSlide(NoteSlide);
      //FreqSlide in 1/256 steps
      //step 44 is C-1@3.5Mhz AY
      //C-1 is 32.7 Hz
      builder.SetFreqSlideHz(FreqSlide * 327 / 440);
      builder.SetSampleNum(Sample);
      builder.SetLevelInPercents(Volume * 100 / 15);
    }
  private:
    void ParseNote(const Cell& src, DAC::ChannelDataBuilder& builder)
    {
      if (const uint_t* note = src.GetNote())
      {
        Counter = 0;
        VibratoStep = ArpeggioStep = 0;
        Note = *note;
        NoteSlide = FreqSlide = 0;
      }
      if (const uint_t* volume = src.GetVolume())
      {
        Volume = *volume;
      }
      if (const bool* enabled = src.GetEnabled())
      {
        builder.SetEnabled(*enabled);
        if (!*enabled)
        {
          NoteSlide = FreqSlide = 0;
        }
      }
      if (const uint_t* sample = src.GetSample())
      {
        Sample = *sample;
        builder.SetPosInSample(0);
      }
      else
      {
        builder.DropPosInSample();
      }
    }

    void NoEffect(DAC::ChannelDataBuilder& /*builder*/)
    {
    }

    void FreqFloat(DAC::ChannelDataBuilder& /*builder*/)
    {
      SlideFreq(FreqSlideStep);
    }

    void Vibrato(DAC::ChannelDataBuilder& /*builder*/)
    {
      if (Step(VibratoPeriod))
      {
        VibratoStep = -VibratoStep;
        SlideFreq(VibratoStep);
      }
    }

    void Arpeggio(DAC::ChannelDataBuilder& /*builder*/)
    {
      if (Step(ArpeggioPeriod))
      {
        ArpeggioStep = -ArpeggioStep;
        NoteSlide += ArpeggioStep;
        FreqSlide = 0;
      }
    }

    void NoteFloat(DAC::ChannelDataBuilder& /*builder*/)
    {
      if (Step(NoteSlidePeriod))
      {
        NoteSlide += NoteSlideStep;
        FreqSlide = 0;
      }
    }

    void DoubleNote(DAC::ChannelDataBuilder& builder)
    {
      if (Step(NoteDoublePeriod))
      {
        ParseNote(OldData, builder);
        DisableEffect();
      }
    }

    void Attack(DAC::ChannelDataBuilder& /*builder*/)
    {
      if (Step(AttackPeriod))
      {
        ++Volume;
        if (AttackLimit < Volume)
        {
          --Volume;
          DisableEffect();
        }
      }
    }

    void Decay(DAC::ChannelDataBuilder& /*builder*/)
    {
      if (Step(DecayPeriod))
      {
        --Volume;
        if (DecayLimit > Volume)
        {
          ++Volume;
          DisableEffect();
        }
      }
    }

    void Mix(DAC::ChannelDataBuilder& builder)
    {
      if (Step(MixPeriod))
      {
        ParseNote(OldData, builder);
        //restore a6ll
        const uint_t prevStep = GetStep() + FreqSlide;
        const uint_t FPS = 50;//TODO
        const uint_t skipped = MixPeriod * prevStep * RENDERS_PER_SEC / FPS / 256;
        Devices::DAC::DataChunk::ChannelData& dst = builder.GetState();
        dst = DacState;
        builder.SetPosInSample(dst.PosInSample + skipped);

        DisableEffect();
      }
    }

    bool Step(uint_t period)
    {
      if (++Counter == period)
      {
        Counter = 0;
        return true;
      }
      return false;
    }

    void SlideFreq(int_t step)
    {
      const int_t nextStep = GetStep() + FreqSlide + step;
      if (nextStep <= 0 || nextStep >= 0x0c00)
      {
        DisableEffect();
      }
      else
      {
        FreqSlide += step;
        NoteSlide = 0;
      }
    }

    uint_t GetStep() const
    {
      static const uint_t STEPS[] =
      {
        44, 47, 50, 53, 56, 59, 63, 66, 70, 74, 79, 83,
        88, 94, 99, 105, 111, 118, 125, 133, 140, 149, 158, 167,
        177, 187, 199, 210, 223, 236, 250, 265, 281, 297, 315, 334,
        354, 375, 397, 421, 446, 472, 500, 530, 561, 595, 630, 668,
        707, 749, 794, 841, 891, 944, 1001, 1060, 1123, 1189, 1216, 1335
      };
      return STEPS[Note + NoteSlide];
    }

    void DisableEffect()
    {
      Effect = &ChannelState::NoEffect;
    }
  private:
    int_t FreqSlideStep;

    uint_t VibratoPeriod;//VBT_x
    int_t VibratoStep;//VBF_x * VBA1/VBA2
    
    uint_t ArpeggioPeriod;//APT_x
    int_t ArpeggioStep;//APF_x * APA1/APA2

    uint_t NoteSlidePeriod;//SUT_x/SDT_x
    int_t NoteSlideStep;

    uint_t NoteDoublePeriod;//DUT_x

    uint_t AttackPeriod;//ATT_x
    uint_t AttackLimit;//ATL_x

    uint_t DecayPeriod;//DYT_x
    uint_t DecayLimit;//DYL_x

    uint_t MixPeriod;

    uint_t Counter;//COUN_x
    uint_t Note;  //NOTN_x
    uint_t NoteSlide;
    uint_t FreqSlide;
    uint_t Volume;//pVOL_x
    uint_t Sample;

    Cell OldData;
    Devices::DAC::DataChunk::ChannelData DacState;

    typedef void (ChannelState::*EffectFunc)(DAC::ChannelDataBuilder&);
    EffectFunc Effect;
  };

  class DataRenderer : public DAC::DataRenderer
  {
  public:
    explicit DataRenderer(ModuleData::Ptr data)
      : Data(data)
    {
      Reset();
    }

    virtual void Reset()
    {
      std::fill(Chans.begin(), Chans.end(), ChannelState());
    }

    virtual void SynthesizeData(const TrackModelState& state, DAC::TrackBuilder& track)
    {
      const Line::Ptr line = 0 == state.Quirk() ? state.LineObject() : Line::Ptr();
      for (uint_t chan = 0; chan != CHANNELS_COUNT; ++chan)
      {
        DAC::ChannelDataBuilder builder = track.GetChannel(chan);

        ChannelState& chanState = Chans[chan];
        chanState.OnFrame(builder);
        //begin note
        if (line)
        {
          if (const Cell::Ptr src = line->GetChannel(chan))
          {
            chanState.OnNote(*src, *Data, builder);
          }
        }
        chanState.GetState(builder);
      }
    }
  private:
    const ModuleData::Ptr Data;
    boost::array<ChannelState, CHANNELS_COUNT> Chans;
  };

  class Chiptune : public DAC::Chiptune
  {
  public:
    Chiptune(ModuleData::Ptr data, Parameters::Accessor::Ptr properties)
      : Data(data)
      , Properties(properties)
      , Info(CreateTrackInfo(Data, CHANNELS_COUNT))
    {
    }

    virtual Information::Ptr GetInformation() const
    {
      return Info;
    }

    virtual Parameters::Accessor::Ptr GetProperties() const
    {
      return Properties;
    }

    virtual DAC::DataIterator::Ptr CreateDataIterator() const
    {
      const TrackStateIterator::Ptr iterator = CreateTrackStateIterator(Data);
      const DAC::DataRenderer::Ptr renderer = boost::make_shared<DataRenderer>(Data);
      return DAC::CreateDataIterator(iterator, renderer);
    }

    virtual void GetSamples(Devices::DAC::Chip::Ptr chip) const
    {
      for (uint_t idx = 0, lim = Data->Samples.Size(); idx != lim; ++idx)
      {
        chip->SetSample(idx, Data->Samples.Get(idx));
      }
    }
  private:
    const ModuleData::Ptr Data;
    const Parameters::Accessor::Ptr Properties;
    const Information::Ptr Info;
  };

  bool Check(const Binary::Container& data)
  {
    //check for header
    const std::size_t size(data.Size());
    if (sizeof(Header) > size)
    {
      return false;
    }
    const Header* const header(safe_ptr_cast<const Header*>(data.Start()));
    if (!(header->PatternSize == 64 || header->PatternSize == 48 || header->PatternSize == 32 || header->PatternSize == 24))
    {
      return false;
    }

    const bool is4bitSamples = true;//TODO: detect
    std::size_t lastData = 256 * header->HeaderSizeSectors;

    typedef std::map<std::size_t, std::pair<std::size_t, std::size_t> > Bank2OffsetAndSize;
    Bank2OffsetAndSize regions;
    for (std::size_t layIdx = 0; layIdx != header->EndOfBanks.size(); ++layIdx)
    {
      static const std::size_t BANKS[] = {0x50, 0x51, 0x53, 0x54, 0x56, 0x57};

      const std::size_t bankEnd = fromLE(header->EndOfBanks[layIdx]);
      if (bankEnd < SAMPLES_ADDR)
      {
        return false;
      }
      if (bankEnd == SAMPLES_ADDR)
      {
        continue;
      }
      const std::size_t bankSize = bankEnd - SAMPLES_ADDR;
      const std::size_t alignedBankSize = Math::Align<std::size_t>(bankSize, 256);
      const std::size_t realSize = is4bitSamples
        ? 256 * (1 + alignedBankSize / 512)
        : alignedBankSize;
      regions[BANKS[layIdx]] = std::make_pair(lastData, realSize);
      lastData += realSize;
    }
    if (lastData > size)
    {
      return false;
    }

    for (uint_t samIdx = 0; samIdx != SAMPLES_COUNT; ++samIdx)
    {
      const SampleInfo& srcSample = header->SampleDescriptions[samIdx];
      if (srcSample.Name[0] == '.')
      {
        continue;
      }
      const std::size_t sampleStart = fromLE(srcSample.Start);
      const std::size_t sampleEnd = fromLE(srcSample.Limit);
      const std::size_t sampleLoop = fromLE(srcSample.Loop);
      if (sampleStart < SAMPLES_ADDR ||
          sampleStart > sampleEnd ||
          sampleStart > sampleLoop)
      {
        return false;
      }
      if (!regions.count(srcSample.Bank))
      {
        return false;
      }
      const std::size_t offsetInBank = sampleStart - SAMPLES_ADDR;
      const std::size_t limitInBank = sampleEnd - SAMPLES_ADDR;
      const std::size_t sampleSize = limitInBank - offsetInBank;
      const std::size_t rawSampleSize = is4bitSamples ? sampleSize / 2 : sampleSize;
      if (rawSampleSize > regions[srcSample.Bank].second)
      {
        return false;
      }
    }
    return true;
  }
}

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char ID[] = {'D', 'M', 'M', 0};
  const Char* const INFO = Text::DIGITALMUSICMAKER_DECODER_DESCRIPTION;
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_3DAC | CAP_CONV_RAW;

  const std::string DMM_FORMAT(
    //bank ends
    "(?c0-ff){6}"
    //pat size: 64,48,32,24
    "%0xxxx000 ?"
    //positions
    "(00-17){50}"
    //tempo (3..30)
    "03-1e"
    //loop position
    "00-32 ?"
    //length
    "01-32"
    //base size
    "02-38"
  );

  class DMMModulesFactory : public ModulesFactory
  {
  public:
    DMMModulesFactory()
      : Format(Binary::Format::Create(DMM_FORMAT, sizeof(DMM::Header)))
    {
    }

    virtual bool Check(const Binary::Container& inputData) const
    {
      return Format->Match(inputData) && DMM::Check(inputData);
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Format;
    }

    virtual Holder::Ptr CreateModule(PropertiesBuilder& propBuilder, Binary::Container::Ptr data) const
    {
      try
      {
        const DMM::ModuleData::Ptr modData = DMM::Parse(propBuilder, data);
        const DAC::Chiptune::Ptr chiptune = boost::make_shared<DMM::Chiptune>(modData, propBuilder.GetResult());
        return DAC::CreateHolder(chiptune);
      }
      catch (const Error&/*e*/)
      {
        Dbg("Failed to create holder");
      }
      return Holder::Ptr();
    }
  private:
    const Binary::Format::Ptr Format;
  };
}

namespace ZXTune
{
  void RegisterDMMSupport(PlayerPluginsRegistrator& registrator)
  {
    const ModulesFactory::Ptr factory = boost::make_shared<DMMModulesFactory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, INFO, CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
