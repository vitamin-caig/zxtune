#include "plugin_enumerator.h"

#include "tracking_supp.h"
#include "../devices/data_source.h"
#include "../devices/aym/aym.h"

#include "../io/container.h"
#include "../io/warnings_collector.h"

#include <tools.h>

#include <player_attrs.h>

#include <boost/static_assert.hpp>

#include <cassert>
#include <valarray>

namespace
{
  using namespace ZXTune;

  const String TEXT_STC_INFO("SoundTracker modules support");
  const String TEXT_STC_VERSION("0.1");
  const String TEXT_STC_EDITOR("SoundTracker");

  //hints
  const std::size_t MAX_MODULE_SIZE = 16384;
  const std::size_t MAX_SAMPLES_COUNT = 16;
  const std::size_t MAX_ORNAMENTS_COUNT = 16;
  const std::size_t MAX_PATTERN_SIZE = 64;
  const std::size_t MAX_PATTERN_COUNT = 32;//TODO

  typedef IO::FastDump<uint8_t> FastDump;

  const uint16_t FreqTable[96] = {//TODO
    0xef8, 0xe10, 0xd60, 0xc80, 0xbd8, 0xb28, 0xa88, 0x9f0, 0x960, 0x8e0, 0x858, 0x7e0,
    0x77c, 0x708, 0x6b0, 0x640, 0x5ec, 0x594, 0x544, 0x4f8, 0x4b0, 0x470, 0x42c, 0x3f0,
    0x3be, 0x384, 0x358, 0x320, 0x2f6, 0x2ca, 0x2a2, 0x27c, 0x258, 0x238, 0x216, 0x1f8,
    0x1df, 0x1c2, 0x1ac, 0x190, 0x17b, 0x165, 0x151, 0x13e, 0x12c, 0x11c, 0x10b, 0x0fc,
    0x0ef, 0x0e1, 0x0d6, 0x0c8, 0x0bd, 0x0b2, 0x0a8, 0x09f, 0x096, 0x08e, 0x085, 0x07e,
    0x077, 0x070, 0x06b, 0x064, 0x05e, 0x059, 0x054, 0x04f, 0x04b, 0x047, 0x042, 0x03f,
    0x03b, 0x038, 0x035, 0x032, 0x02f, 0x02c, 0x02a, 0x027, 0x025, 0x023, 0x021, 0x01f,
    0x01d, 0x01c, 0x01a, 0x019, 0x017, 0x016, 0x015, 0x013, 0x012, 0x011, 0x010, 0x00f
  };
  //////////////////////////////////////////////////////////////////////////
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct STCHeader
  {
    uint8_t Tempo;
    uint16_t PositionsOffset;
    uint16_t OrnamentsOffset;
    uint16_t PatternsOffset;
    uint8_t Identifier[18];
    uint16_t Size;
  } PACK_POST;

  PACK_PRE struct STCPositions
  {
    uint8_t Lenght;
    PACK_PRE struct STCPosEntry
    {
      uint8_t PatternNum;
      int8_t PatternHeight;
    } PACK_POST;
    STCPosEntry Data[1];
  } PACK_POST;

  PACK_PRE struct STCPattern
  {
    uint8_t Number;
    uint16_t Offsets[3];

    operator bool () const
    {
      return 0xff != Number;
    }
  } PACK_POST;

  PACK_PRE struct STCOrnament
  {
    uint8_t Number;
    int8_t Data[32];
  } PACK_POST;

  PACK_PRE struct STCSample
  {
    uint8_t Number;
    PACK_PRE struct Line
    {
      uint8_t Level : 4;
      uint8_t EffHi : 4;
      uint8_t Noise : 5;
      uint8_t EffSign : 1;
      uint8_t EnvelopeMask : 1;
      uint8_t NoiseMask : 1;
      uint8_t EffLo;
    } PACK_POST;
    Line Data[32];
    uint8_t Loop;
    uint8_t LoopSize;
  } PACK_POST;

#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(STCHeader) == 27);
  BOOST_STATIC_ASSERT(sizeof(STCPositions) == 3);
  BOOST_STATIC_ASSERT(sizeof(STCPattern) == 7);
  BOOST_STATIC_ASSERT(sizeof(STCOrnament) == 33);
  BOOST_STATIC_ASSERT(sizeof(STCSample) == 99);

  struct Sample
  {
    Sample() : Loop(), LoopLimit(), Data()
    {
    }

    explicit Sample(const STCSample& sample)
    : Loop(sample.Loop), LoopLimit(sample.Loop + sample.LoopSize + 1), Data(sample.Data, ArrayEnd(sample.Data))
    {
      assert(Loop <= LoopLimit);
      assert(LoopLimit <= Data.size());
    }

    std::size_t Loop;
    std::size_t LoopLimit;

    struct Line
    {
      /*explicit*/Line(const STCSample::Line& line)
      : Level(line.Level), Noise(line.Noise), NoiseMask(line.NoiseMask), EnvelopeMask(line.EnvelopeMask)
        , Effect(line.EffLo + (line.EffHi << 8))
      {
        if (!line.EffSign)
        {
          Effect = -Effect;
        }
      }
      unsigned Level;//0-15
      unsigned Noise;//0-31
      bool NoiseMask;
      bool EnvelopeMask;
      signed Effect;
    };
    std::vector<Line> Data;
  };

  enum CmdType
  {
    EMPTY,
    ENVELOPE,     //2p
    NOENVELOPE,   //0p
  };

  void Describing(ModulePlayer::Info& info);

  typedef Log::WarningsCollector::AutoPrefixParam<std::size_t> IndexPrefix;

  class PlayerImpl : public Tracking::TrackPlayer<3, Sample>
  {
    typedef Tracking::TrackPlayer<3, Sample> Parent;

    static void ParsePattern(const FastDump& data, std::vector<std::size_t>& offsets, Parent::Line& line,
      std::valarray<std::size_t>& periods,
      std::valarray<std::size_t>& counters,
      Log::WarningsCollector& warner)
    {
      for (std::size_t chan = 0; chan != line.Channels.size(); ++chan)
      {
        if (counters[chan]--)
        {
          continue;//has to skip
        }
        IndexPrefix pfx(warner, "Channel %1%: ", chan);
        for (;;)
        {
          const uint8_t cmd(data[offsets[chan]++]);
          Line::Chan& channel(line.Channels[chan]);
          if (cmd <= 0x5f)//note
          {
            warner.Assert(!channel.Note, "duplicated note");
            channel.Note = cmd;
            channel.Enabled = true;
            break;
          }
          else if (cmd >= 0x60 && cmd <= 0x6f)//sample
          {
            warner.Assert(!channel.SampleNum, "duplicated sample");
            channel.SampleNum = cmd - 0x60;
          }
          else if (cmd >= 0x70 && cmd <= 0x7f)//ornament
          {
            warner.Assert(channel.Commands.end() == std::find(channel.Commands.begin(), channel.Commands.end(), ENVELOPE),
              "duplicated envelope command");
            warner.Assert(!channel.OrnamentNum, "duplicated ornament");
            channel.OrnamentNum = cmd - 0x70;
            channel.Commands.push_back(Parent::Command(NOENVELOPE));
          }
          else if (cmd == 0x80)//reset
          {
            warner.Assert(!channel.Enabled, "duplicated channel state");
            channel.Enabled = false;
            break;
          }
          else if (cmd == 0x81)//empty
          {
            break;
          }
          else if (cmd == 0x82)//orn 0
          {
            warner.Assert(channel.Commands.end() == std::find(channel.Commands.begin(), channel.Commands.end(), ENVELOPE),
              "duplicated envelope command");
            warner.Assert(!channel.OrnamentNum, "duplicated ornament");
            channel.OrnamentNum = 0;
            channel.Commands.push_back(Parent::Command(NOENVELOPE));
          }
          else if (cmd >= 0x83 && cmd <= 0x8e)//orn 0 with envelope
          {
            warner.Assert(channel.Commands.end() == std::find(channel.Commands.begin(), channel.Commands.end(), NOENVELOPE),
              "duplicated envelope command");
            warner.Assert(!channel.OrnamentNum, "duplicated ornament");
            channel.Commands.push_back(Parent::Command(ENVELOPE, cmd - 0x80, data[offsets[chan]++]));
            channel.OrnamentNum = 0;
          }
          else //skip
          {
            periods[chan] = cmd - 0xa1;
          }
        }
        counters[chan] = periods[chan];
      }
    }
  private:
    struct ChannelState
    {
      ChannelState()
        : Enabled(false), Envelope(false)
        , Note(), SampleNum(0), PosInSample(0)
        , OrnamentNum(0), LoopedInSample(false)
      {
      }
      bool Enabled;
      bool Envelope;
      std::size_t Note;
      std::size_t SampleNum;
      std::size_t PosInSample;
      std::size_t OrnamentNum;
      bool LoopedInSample;
    };
  public:
    PlayerImpl(const String& filename, const FastDump& data)
      : Device(AYM::CreateChip())//TODO: put out
    {
      //assume that data is ok
      const STCHeader* const header(safe_ptr_cast<const STCHeader*>(&data[0]));
      const STCSample* const samples(safe_ptr_cast<const STCSample*>(header + 1));
      const STCPositions* const positions(safe_ptr_cast<const STCPositions*>(&data[fromLE(header->PositionsOffset)]));
      const STCOrnament* const ornaments(safe_ptr_cast<const STCOrnament*>(&data[fromLE(header->OrnamentsOffset)]));
      const STCPattern* const patterns(safe_ptr_cast<const STCPattern*>(&data[fromLE(header->PatternsOffset)]));

      Information.Statistic.Tempo = header->Tempo;
      Information.Loop = 0;//not supported
      Information.Properties.insert(StringMap::value_type(Module::ATTR_FILENAME, filename));
      Information.Properties.insert(StringMap::value_type(Module::ATTR_TRACKER,
        String(header->Identifier, ArrayEnd(header->Identifier))));
      Information.Properties.insert(StringMap::value_type(Module::ATTR_PROGRAM, TEXT_STC_EDITOR));

      Log::WarningsCollector warner;
      //parse positions
      Data.Positions.reserve(positions->Lenght + 1);
      Transpositions.reserve(positions->Lenght + 1);
      for (const STCPositions::STCPosEntry* posEntry(positions->Data);
        static_cast<const void*>(posEntry) < static_cast<const void*>(ornaments); ++posEntry)
      {
        Data.Positions.push_back(posEntry->PatternNum - 1);
        Transpositions.push_back(posEntry->PatternHeight);
      }
      warner.Assert(Data.Positions.size() == std::size_t(positions->Lenght) + 1, "header lenght mismatch");

      //parse samples
      Data.Samples.resize(MAX_SAMPLES_COUNT);
      for (const STCSample* sample = samples; static_cast<const void*>(sample) < static_cast<const void*>(positions); ++sample)
      {
        if (sample->Number >= Data.Samples.size())
        {
          warner.Warning("invalid sample number");
          continue;
        }
        Data.Samples[sample->Number] = Sample(*sample);
      }
      warner.Assert(Data.Samples.size() <= MAX_SAMPLES_COUNT, "invalid samples count");

      //parse ornaments
      Data.Ornaments.resize(MAX_ORNAMENTS_COUNT);
      for (const STCOrnament* ornament = ornaments; static_cast<const void*>(ornament) < static_cast<const void*>(patterns); ++ornament)
      {
        if (ornament->Number >= Data.Ornaments.size())
        {
          warner.Warning("invalid ornament number");
          continue;
        }
        Data.Ornaments[ornament->Number] = Parent::Ornament(ArraySize(ornament->Data), 0);
        Data.Ornaments[ornament->Number].Data.assign(ornament->Data, ArrayEnd(ornament->Data));
      }
      warner.Assert(Data.Ornaments.size() <= MAX_ORNAMENTS_COUNT, "invalid ornaments count");

      //parse patterns
      Data.Patterns.resize(MAX_PATTERN_COUNT);
      for (const STCPattern* pattern = patterns; *pattern; ++pattern)
      {
        IndexPrefix patPfx(warner, "Pattern %1%: ", pattern->Number - 1);
        if (!pattern->Number || pattern->Number >= Data.Patterns.size())
        {
          warner.Warning("invalid pattern number");
          continue;
        }
        Pattern& pat(Data.Patterns[pattern->Number - 1]);
        std::vector<std::size_t> offsets(ArraySize(pattern->Offsets));
        std::valarray<std::size_t> periods(std::size_t(0), ArraySize(pattern->Offsets));
        std::valarray<std::size_t> counters(std::size_t(0), ArraySize(pattern->Offsets));
        std::transform(pattern->Offsets, ArrayEnd(pattern->Offsets), offsets.begin(), &fromLE<uint16_t>);
        pat.reserve(MAX_PATTERN_SIZE);
        do
        {
          IndexPrefix linePfx(warner, "Line %1%: ", pat.size());
          pat.push_back(Line());
          Line& line(pat.back());
          ParsePattern(data, offsets, line, periods, counters, warner);
          //skip lines
          if (const std::size_t linesToSkip = counters.min())
          {
            counters -= linesToSkip;
            pat.resize(pat.size() + linesToSkip);//add dummies
          }
        }
        while (0xff != data[offsets[0]] || counters[0]);
        warner.Assert(0 == counters.max(), "not all channels counters reached");
        warner.Assert(pat.size() <= MAX_PATTERN_SIZE, "invalid pattern size");
      }
      warner.Assert(Data.Patterns.size() <= MAX_PATTERN_COUNT, "invalid patterns count");
      Information.Statistic.Position = Data.Positions.size();
      Information.Statistic.Pattern = Data.Patterns.size();
      Information.Statistic.Channels = 3;

      const String& warnings(warner.GetWarnings());
      if (!warnings.empty())
      {
        Information.Properties.insert(StringMap::value_type(Module::ATTR_WARNINGS, warnings));
      }
      InitTime();
    }

    virtual void GetInfo(Info& info) const
    {
      Describing(info);
    }

    /// Retrieving current state of sound
    virtual State GetSoundState(Sound::Analyze::ChannelsState& state) const
    {
      assert(Device.get());
      Device->GetState(state);
      return PlaybackState;
    }

    /// Rendering frame
    virtual State RenderFrame(const Sound::Parameters& params, Sound::Receiver& receiver)
    {
      AYM::DataChunk chunk;
      chunk.Tick = (CurrentState.Tick += params.ClocksPerFrame());
      RenderData(chunk);

      Device->RenderData(params, chunk, receiver);

      return Parent::RenderFrame(params, receiver);
    }

    virtual State Reset()
    {
      Device->Reset();
      return Parent::Reset();
    }

  private:
    void RenderData(AYM::DataChunk& chunk)
    {
      const Line& line(Data.Patterns[CurrentState.Position.Pattern][CurrentState.Position.Note]);
      if (0 == CurrentState.Position.Frame)//begin note
      {
        for (std::size_t chan = 0; chan != line.Channels.size(); ++chan)
        {
          const Line::Chan& src(line.Channels[chan]);
          ChannelState& dst(Channels[chan]);
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
            dst.LoopedInSample = false;
          }
          if (src.SampleNum)
          {
            dst.SampleNum = *src.SampleNum;
            assert(0 == dst.PosInSample);
          }
          if (src.OrnamentNum)
          {
            dst.OrnamentNum = *src.OrnamentNum;
            assert(0 == dst.PosInSample);
          }
          for (CommandsArray::const_iterator it = src.Commands.begin(), lim = src.Commands.end(); it != lim; ++it)
          {
            switch (it->Type)
            {
            case ENVELOPE:
              chunk.Data[AYM::DataChunk::REG_ENV] = uint8_t(it->Param1);
              chunk.Data[AYM::DataChunk::REG_TONEE_L] = uint8_t(it->Param2 & 0xff);
              chunk.Mask |= (1 << AYM::DataChunk::REG_ENV) | (1 << AYM::DataChunk::REG_TONEE_L);
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
      }
      //permanent registers
      chunk.Data[AYM::DataChunk::REG_MIXER] = 0;
      chunk.Mask |= (1 << AYM::DataChunk::REG_MIXER) |
        (1 << AYM::DataChunk::REG_VOLA) | (1 << AYM::DataChunk::REG_VOLB) | (1 << AYM::DataChunk::REG_VOLC);
      std::size_t toneReg = AYM::DataChunk::REG_TONEA_L;
      std::size_t volReg = AYM::DataChunk::REG_VOLA;
      uint8_t toneMsk = AYM::DataChunk::MASK_TONEA;
      uint8_t noiseMsk = AYM::DataChunk::MASK_NOISEA;
      for (ChannelState* dst = Channels; dst != ArrayEnd(Channels);
        ++dst, toneReg += 2, ++volReg, toneMsk <<= 1, noiseMsk <<= 1)
      {
        if (dst->Enabled)
        {
          const Sample& curSample(Data.Samples[dst->SampleNum]);
          const Sample::Line& curSampleLine(curSample.Data[dst->PosInSample]);
          const Ornament& curOrnament(Data.Ornaments[dst->OrnamentNum]);

          //calculate tone
          const std::size_t halfTone = static_cast<std::size_t>(clamp<int>(
            signed(dst->Note) + curOrnament.Data[dst->PosInSample] + Transpositions[CurrentState.Position.Position], 0, 95));
          const uint16_t tone = static_cast<uint16_t>(clamp(FreqTable[halfTone] + curSampleLine.Effect, 0, 0xffff));

          chunk.Data[toneReg] = uint8_t(tone & 0xff);
          chunk.Data[toneReg + 1] = uint8_t(tone >> 8);
          chunk.Mask |= 3 << toneReg;
          //calculate level
          chunk.Data[volReg] = curSampleLine.Level | uint8_t(dst->Envelope ? AYM::DataChunk::MASK_ENV : 0);
          //mixer
          if (curSampleLine.EnvelopeMask)
          {
            chunk.Data[AYM::DataChunk::REG_MIXER] |= toneMsk;
          }
          if (curSampleLine.NoiseMask)
          {
            chunk.Data[AYM::DataChunk::REG_MIXER] |= noiseMsk;
          }
          else
          {
            chunk.Data[AYM::DataChunk::REG_TONEN] = curSampleLine.Noise;
            chunk.Mask |= 1 << AYM::DataChunk::REG_TONEN;
          }

          if (++dst->PosInSample >= (dst->LoopedInSample ? curSample.LoopLimit : curSample.Data.size()))
          {
            if (curSample.Loop)
            {
              dst->PosInSample = curSample.Loop;
              dst->LoopedInSample = true;
            }
            else
            {
              dst->Enabled = false;
            }
          }
        }
        else
        {
          chunk.Data[volReg] = 0;
          //????
          chunk.Data[AYM::DataChunk::REG_MIXER] |= toneMsk | noiseMsk;
        }
      }
      CurrentState.Position.Channels = std::count_if(Channels, ArrayEnd(Channels),
        boost::mem_fn(&ChannelState::Enabled));
    }
  private:
    AYM::Chip::Ptr Device;
    std::vector<signed> Transpositions;
    ChannelState Channels[3];
  };
  //////////////////////////////////////////////////////////////////////////
  void Describing(ModulePlayer::Info& info)
  {
    info.Capabilities = CAP_AYM;
    info.Properties.clear();
    info.Properties.insert(StringMap::value_type(ATTR_DESCRIPTION, TEXT_STC_INFO));
    info.Properties.insert(StringMap::value_type(ATTR_VERSION, TEXT_STC_VERSION));
  }

  bool Checking(const String& /*filename*/, const IO::DataContainer& source, uint32_t /*capFilter*/)
  {
    const std::size_t limit(source.Size());
    if (sizeof(STCHeader) > limit || limit > MAX_MODULE_SIZE)
    {
      return false;
    }
    const uint8_t* const data(static_cast<const uint8_t*>(source.Data()));
    const STCHeader* const header(safe_ptr_cast<const STCHeader*>(data));
    const std::size_t samOff(sizeof(STCHeader));
    const std::size_t posOff(fromLE(header->PositionsOffset));
    const STCPositions* const positions(safe_ptr_cast<const STCPositions*>(data + posOff));
    const std::size_t ornOff(fromLE(header->OrnamentsOffset));
    const STCOrnament* const ornaments(safe_ptr_cast<const STCOrnament*>(data + ornOff));
    const std::size_t patOff(fromLE(header->PatternsOffset));
    //check offsets
    if (posOff > limit || ornOff > limit || patOff > limit)
    {
      return false;
    }
    //check order
    if (samOff >= posOff || posOff >= ornOff || ornOff >= patOff)
    {
      return false;
    }
    //check align
    if (0 != (posOff - samOff) % sizeof(STCSample) ||
        static_cast<const void*>(positions->Data + positions->Lenght + 1) != static_cast<const void*>(ornaments) ||
        0 != (patOff - ornOff) % sizeof(STCOrnament)
        )
    {
      return false;
    }
    return true;
  }

  ModulePlayer::Ptr Creating(const String& filename, const IO::DataContainer& data, uint32_t /*capFilter*/)
  {
    assert(Checking(filename, data, 0) || !"Attempt to create stc player on invalid data");
    return ModulePlayer::Ptr(new PlayerImpl(filename, FastDump(data)));
  }

  PluginAutoRegistrator registrator(Checking, Creating, Describing);
}
