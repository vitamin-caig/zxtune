#include "plugin_enumerator.h"

#include "tracking_supp.h"
#include "../devices/data_source.h"
#include "../devices/aym/aym.h"

#include "../io/container.h"
#include "../io/warnings_collector.h"

#include <error.h>
#include <tools.h>

#include <player_attrs.h>

#include <boost/static_assert.hpp>

#include <cassert>
#include <valarray>

#include <text/plugins.h>
#include <text/warnings.h>

#define FILE_TAG BD5A4CD5

namespace
{
  using namespace ZXTune;

  const String TEXT_STP_VERSION(FromChar("Revision: $Rev$"));

  //hints
  const std::size_t MAX_MODULE_SIZE = 16384;
  const std::size_t MAX_SAMPLES_COUNT = 15;
  const std::size_t MAX_ORNAMENTS_COUNT = 16;
  const std::size_t MAX_PATTERN_SIZE = 64;
  //const std::size_t MAX_PATTERN_COUNT = 32;//TODO

  typedef IO::FastDump<uint8_t> FastDump;

  const uint16_t FreqTable[96] = {
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
  PACK_PRE struct STPHeader
  {
    uint8_t Tempo;
    uint16_t PositionsOffset;
    uint16_t PatternsOffset;
    uint16_t OrnamentsOffset;
    uint16_t SamplesOffset;
    uint8_t FixesCount;
  } PACK_POST;

  const uint8_t STP_ID[] = {
    'K', 'S', 'A', ' ', 'S', 'O', 'F', 'T', 'W', 'A', 'R', 'E', ' ',
    'C', 'O', 'M', 'P', 'I', 'L', 'A', 'T', 'I', 'O', 'N', ' ', 'O', 'F', ' '
  };

  PACK_PRE struct STPId
  {
    uint8_t Id[sizeof(STP_ID)];
    uint8_t Title[25];

    operator bool () const
    {
      return 0 == std::memcmp(Id, STP_ID, sizeof(Id));
    }
  } PACK_POST;

  PACK_PRE struct STPPositions
  {
    uint8_t Lenght;
    uint8_t Loop;
    PACK_PRE struct STPPosEntry
    {
      uint8_t PatternOffset;//*6
      int8_t PatternHeight;
    } PACK_POST;
    STPPosEntry Data[1];
  } PACK_POST;

  PACK_PRE struct STPPattern
  {
    uint16_t Offsets[3];
  } PACK_POST;

  PACK_PRE struct STPOrnament
  {
    uint8_t Loop;
    uint8_t Size;
    int8_t Data[1];
  } PACK_POST;

  PACK_PRE struct STPOrnaments
  {
    uint16_t Offsets[MAX_ORNAMENTS_COUNT];
  } PACK_POST;

  PACK_PRE struct STPSample
  {
    int8_t Loop;
    uint8_t Size;
    PACK_PRE struct Line
    {
      uint8_t Level : 4;
      uint8_t ToneMask : 1;
      uint8_t Padding : 2;
      uint8_t NoiseMask : 1;
      uint8_t EnvelopeMask : 1;
      uint8_t Noise : 5;
      int16_t Vibrato;
    } PACK_POST;
    Line Data[1];
  } PACK_POST;

  PACK_PRE struct STPSamples
  {
    uint16_t Offsets[MAX_SAMPLES_COUNT];
  } PACK_POST;

#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(STPHeader) == 10);
  BOOST_STATIC_ASSERT(sizeof(STPPositions) == 4);
  BOOST_STATIC_ASSERT(sizeof(STPPattern) == 6);
  BOOST_STATIC_ASSERT(sizeof(STPOrnament) == 3);
  BOOST_STATIC_ASSERT(sizeof(STPOrnaments) == 32);
  BOOST_STATIC_ASSERT(sizeof(STPSample) == 6);
  BOOST_STATIC_ASSERT(sizeof(STPSamples) == 30);

  struct Sample
  {
    Sample() : Loop(), Data()
    {
    }

    explicit Sample(const STPSample& sample)
    : Loop(sample.Loop), Data(sample.Data, sample.Data + sample.Size)
    {
    }

    signed Loop;

    struct Line
    {
      /*explicit*/Line(const STPSample::Line& line)
      : Level(line.Level), Noise(line.Noise)
      , ToneMask(line.ToneMask), NoiseMask(line.NoiseMask), EnvelopeMask(line.EnvelopeMask)
      , Vibrato(fromLE(line.Vibrato))
      {
      }
      unsigned Level;//0-15
      unsigned Noise;//0-31
      bool ToneMask;
      bool NoiseMask;
      bool EnvelopeMask;
      signed Vibrato;
    };
    std::vector<Line> Data;
  };

  enum CmdType
  {
    EMPTY,
    ENVELOPE,     //2p
    NOENVELOPE,   //0p
    GLISS,        //1p
  };

  void Describing(ModulePlayer::Info& info);

  typedef Log::WarningsCollector::AutoPrefixParam<std::size_t> IndexPrefix;

  class STPPlayer : public Tracking::TrackPlayer<3, Sample>
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
        IndexPrefix pfx(warner, TEXT_CHANNEL_WARN_PREFIX, chan);
        for (;;)
        {
          const uint8_t cmd(data[offsets[chan]++]);
          Line::Chan& channel(line.Channels[chan]);
          if (cmd >= 1 && cmd <= 0x60)//note
          {
            warner.Assert(!channel.Note, TEXT_WARNING_DUPLICATE_NOTE);
            channel.Note = cmd - 1;
            channel.Enabled = true;
            break;
          }
          else if (cmd >= 0x61 && cmd <= 0x6f)//sample
          {
            warner.Assert(!channel.SampleNum, TEXT_WARNING_DUPLICATE_SAMPLE);
            channel.SampleNum = cmd - 0x61;
          }
          else if (cmd >= 0x70 && cmd <= 0x7f)//ornament
          {
            warner.Assert(!channel.FindCommand(ENVELOPE), TEXT_WARNING_DUPLICATE_ENVELOPE);
            warner.Assert(!channel.OrnamentNum, TEXT_WARNING_DUPLICATE_ORNAMENT);
            channel.OrnamentNum = cmd - 0x70;
            channel.Commands.push_back(Parent::Command(NOENVELOPE));
            warner.Assert(!channel.FindCommand(GLISS), "duplicate gliss");
            channel.Commands.push_back(Parent::Command(GLISS, 0));
          }
          else if (cmd >= 0x80 && cmd <= 0xbf)
          {
            periods[chan] = cmd - 0x80;
          }
          else if (cmd >= 0xc0 && cmd <= 0xcf)
          {
            warner.Assert(!channel.FindCommand(NOENVELOPE), TEXT_WARNING_DUPLICATE_ENVELOPE);
            channel.Commands.push_back(Parent::Command(ENVELOPE, cmd - 0xc0, cmd != 0xc0 ? data[offsets[chan]++] : 0));
            warner.Assert(!channel.OrnamentNum, TEXT_WARNING_DUPLICATE_ORNAMENT);
            channel.OrnamentNum = 0;
            warner.Assert(!channel.FindCommand(GLISS), "duplicate gliss");
            channel.Commands.push_back(Parent::Command(GLISS, 0));
          }
          else if (cmd >= 0xd0 && cmd <= 0xdf)//reset
          {
            warner.Assert(!channel.Enabled, TEXT_WARNING_DUPLICATE_STATE);
            channel.Enabled = false;
            break;
          }
          else if (cmd >= 0xe0 && cmd <= 0xef)//empty
          {
            break;
          }
          else if (cmd == 0xf0) //glissade
          {
            warner.Assert(!channel.FindCommand(GLISS), "duplicate gliss");
            channel.Commands.push_back(Parent::Command(GLISS, static_cast<int8_t>(data[offsets[chan]++])));
          }
          else if (cmd >= 0xf1 && cmd <= 0xff) //volume
          {
            warner.Assert(!channel.Volume, TEXT_WARNING_DUPLICATE_VOLUME);
            channel.Volume = cmd - 0xf1;
          }
        }
        counters[chan] = periods[chan];
      }
    }
  private:
    struct ChannelState
    {
      ChannelState()
        : Enabled(false), Envelope(false), Volume(0)
        , Note(0), SampleNum(0), PosInSample(0)
        , OrnamentNum(0), PosInOrnament(0) 
        , TonSlide(0), Glissade(0)
      {
      }
      bool Enabled;
      bool Envelope;
      std::size_t Volume;
      std::size_t Note;
      std::size_t SampleNum;
      std::size_t PosInSample;
      std::size_t OrnamentNum;
      std::size_t PosInOrnament;
      signed TonSlide;
      signed Glissade;
    };
  public:
    STPPlayer(const String& filename, const FastDump& src)
      : Parent(filename)
      , Device(AYM::CreateChip())
    {
      //assume that data is ok
      const uint8_t* const data(&src[0]);
      const STPHeader* const header(safe_ptr_cast<const STPHeader*>(data));

      const STPPositions* const positions(safe_ptr_cast<const STPPositions*>(data + fromLE(header->PositionsOffset)));
      const STPPattern* const patterns(safe_ptr_cast<const STPPattern*>(data + fromLE(header->PatternsOffset)));
      const STPOrnaments* const ornaments(safe_ptr_cast<const STPOrnaments*>(data + fromLE(header->OrnamentsOffset)));
      const STPSamples* const samples(safe_ptr_cast<const STPSamples*>(data + fromLE(header->SamplesOffset)));

      Information.Statistic.Tempo = header->Tempo;
      Information.Loop = positions->Loop;//not supported

      Log::WarningsCollector warner;
      //parse positions
      Data.Positions.reserve(positions->Lenght + 1);
      Transpositions.reserve(positions->Lenght + 1);
      const STPPositions::STPPosEntry* posEntry(positions->Data);
      for (unsigned pos = 0; pos != positions->Lenght; ++pos, ++posEntry)
      {
        Data.Positions.push_back(posEntry->PatternOffset / 6);
        Transpositions.push_back(posEntry->PatternHeight);
      }

      //parse samples
      Data.Samples.reserve(MAX_SAMPLES_COUNT);
      std::size_t index = 0;
      for (const uint16_t* pSample = samples->Offsets; pSample != ArrayEnd(samples->Offsets);
        ++pSample, ++index)
      {
        assert(*pSample && fromLE(*pSample) < src.Size());
        const STPSample* const sample(safe_ptr_cast<const STPSample*>(&data[fromLE(*pSample)]));
        Data.Samples.push_back(Sample(*sample));
        const Sample& smp(Data.Samples.back());
        IndexPrefix pfx(warner, TEXT_SAMPLE_WARN_PREFIX, index);
        warner.Assert(smp.Loop < signed(smp.Data.size()), TEXT_WARNING_LOOP_OUT_BOUND);
      }
      const std::size_t rawSize(safe_ptr_cast<const uint8_t*>(samples + 1) - data);

      Data.Ornaments.reserve(MAX_ORNAMENTS_COUNT);
      index = 0;
      for (const uint16_t* pOrnament = ornaments->Offsets; pOrnament != ArrayEnd(ornaments->Offsets);
        ++pOrnament, ++index)
      {
        assert(*pOrnament && fromLE(*pOrnament) < src.Size());
        const STPOrnament* const ornament(safe_ptr_cast<const STPOrnament*>(&data[fromLE(*pOrnament)]));
        Data.Ornaments.push_back(Parent::Ornament(0, ornament->Loop));
        Ornament& orn(Data.Ornaments.back());
        orn.Data.assign(ornament->Data, ornament->Data + ornament->Size);
        IndexPrefix pfx(warner, TEXT_ORNAMENT_WARN_PREFIX, index);
        warner.Assert(orn.Loop <= orn.Data.size(), TEXT_WARNING_LOOP_OUT_LIMIT);
      }

      //parse patterns
      const unsigned fixOffset(0);//TODO
      for (const STPPattern* pattern = patterns; 
        static_cast<const void*>(pattern) < static_cast<const void*>(ornaments); ++pattern)
      {
        Data.Patterns.push_back(Pattern());
        IndexPrefix patPfx(warner, TEXT_PATTERN_WARN_PREFIX, Data.Patterns.size());
        Pattern& pat(Data.Patterns.back());
        std::vector<std::size_t> offsets(ArraySize(pattern->Offsets));
        std::valarray<std::size_t> periods(std::size_t(0), ArraySize(pattern->Offsets));
        std::valarray<std::size_t> counters(std::size_t(0), ArraySize(pattern->Offsets));
        std::transform(pattern->Offsets, ArrayEnd(pattern->Offsets), offsets.begin(), 
          boost::bind(std::minus<unsigned>(), boost::bind(&fromLE<uint16_t>, _1), fixOffset));
        pat.reserve(MAX_PATTERN_SIZE);
        do
        {
          IndexPrefix linePfx(warner, TEXT_LINE_WARN_PREFIX, pat.size());
          pat.push_back(Line());
          Line& line(pat.back());
          ParsePattern(src, offsets, line, periods, counters, warner);
          //skip lines
          if (const std::size_t linesToSkip = counters.min())
          {
            counters -= linesToSkip;
            pat.resize(pat.size() + linesToSkip);//add dummies
          }
        }
        while (0x00 != data[offsets[0]] || counters[0]);
        warner.Assert(0 == counters.max(), TEXT_WARNING_PERIODS);
        warner.Assert(pat.size() <= MAX_PATTERN_SIZE, TEXT_WARNING_INVALID_PATTERN_SIZE);
      }
      Information.Statistic.Position = Data.Positions.size();
      Information.Statistic.Pattern = Data.Patterns.size();
      Information.Statistic.Channels = 3;

      const String& warnings(warner.GetWarnings());
      if (!warnings.empty())
      {
        Information.Properties.insert(StringMap::value_type(Module::ATTR_WARNINGS, warnings));
      }

      const STPId* const id(safe_ptr_cast<const STPId*>(header + 1));
      FillProperties(TEXT_STP_EDITOR, String(), *id ? String(id->Title, ArrayEnd(id->Title)) : String(), 
        &data[0], rawSize);
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
            dst.Enabled = *src.Enabled;
          }
          if (src.Note)
          {
            dst.Note = *src.Note;
            dst.PosInSample = 0;
            dst.PosInOrnament = 0;
            dst.TonSlide = 0;
          }
          if (src.SampleNum)
          {
            dst.SampleNum = *src.SampleNum;
            dst.PosInSample = 0;
          }
          if (src.OrnamentNum)
          {
            dst.OrnamentNum = *src.OrnamentNum;
            dst.PosInOrnament = 0;
          }
          if (src.Volume)
          {
            dst.Volume = *src.Volume;
          }
          for (CommandsArray::const_iterator it = src.Commands.begin(), lim = src.Commands.end(); it != lim; ++it)
          {
            switch (it->Type)
            {
            case ENVELOPE:
              if (it->Param1)
              {
                chunk.Data[AYM::DataChunk::REG_ENV] = uint8_t(it->Param1);
                chunk.Data[AYM::DataChunk::REG_TONEE_L] = uint8_t(it->Param2 & 0xff);
                chunk.Mask |= (1 << AYM::DataChunk::REG_ENV) | (1 << AYM::DataChunk::REG_TONEE_L);
              }
              dst.Envelope = true;
              break;
            case NOENVELOPE:
              dst.Envelope = false;
              break;
            case GLISS:
              dst.Glissade = it->Param1;
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
          dst->TonSlide += dst->Glissade;
          const std::size_t halfTone = static_cast<std::size_t>(clamp<int>(
            signed(dst->Note) + Transpositions[CurrentState.Position.Position] +
            (dst->Envelope ? 0 : curOrnament.Data[dst->PosInOrnament]), 0, 95));
          const uint16_t tone = static_cast<uint16_t>(clamp(FreqTable[halfTone] + dst->TonSlide + 
            curSampleLine.Vibrato, 0, 0xffff));

          chunk.Data[toneReg] = uint8_t(tone & 0xff);
          chunk.Data[toneReg + 1] = uint8_t(tone >> 8);
          chunk.Mask |= 3 << toneReg;
          //calculate level
          chunk.Data[volReg] = clamp<int>(int(curSampleLine.Level) - int(dst->Volume), 0, 15) |
            uint8_t(curSampleLine.EnvelopeMask && dst->Envelope ? AYM::DataChunk::MASK_ENV : 0);
          //mixer
          if (curSampleLine.ToneMask)
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

          if (++dst->PosInOrnament >= curOrnament.Data.size())
          {
            dst->PosInOrnament = curOrnament.Loop;
          }

          if (++dst->PosInSample >= curSample.Data.size())
          {
            if (curSample.Loop >= 0)
            {
              dst->PosInSample = curSample.Loop;
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
    info.Capabilities = CAP_DEV_AYM | CAP_CONV_RAW;
    info.Properties.clear();
    info.Properties.insert(StringMap::value_type(ATTR_DESCRIPTION, TEXT_STP_INFO));
    info.Properties.insert(StringMap::value_type(ATTR_VERSION, TEXT_STP_VERSION));
  }

  bool Checking(const String& /*filename*/, const IO::DataContainer& source, uint32_t /*capFilter*/)
  {
    const std::size_t limit(std::min(source.Size(), MAX_MODULE_SIZE));
    if (limit < sizeof(STPHeader))
    {
      return false;
    }
    const uint8_t* const data(static_cast<const uint8_t*>(source.Data()));
    const STPHeader* const header(safe_ptr_cast<const STPHeader*>(data));

    const STPId* const id(safe_ptr_cast<const STPId*>(header + 1));
    if (limit > sizeof(*header) + sizeof(*id) && *id)
    {
      return true;
    }

    const std::size_t positionsOffset(fromLE(header->PositionsOffset));
    const std::size_t patternsOffset(fromLE(header->PatternsOffset));
    const std::size_t ornamentsOffset(fromLE(header->OrnamentsOffset));
    const std::size_t samplesOffset(fromLE(header->SamplesOffset));
    if (header->FixesCount == 0 || //TODO: process with == 0
      positionsOffset >= patternsOffset || patternsOffset >= ornamentsOffset || ornamentsOffset >= samplesOffset)
    {
      return false;
    }
    boost::function<bool(std::size_t)> checker = !boost::bind(&in_range<std::size_t>, _1, sizeof(*header), limit - 1);
    boost::function<bool(uint16_t)> leChecker = boost::bind(checker, boost::bind(&fromLE<uint16_t>, _1));
    if (!header->Tempo ||
      checker(positionsOffset) || checker(positionsOffset + sizeof(STPPositions)) ||
      checker(patternsOffset) || checker(patternsOffset + sizeof(STPPattern)) ||
      checker(ornamentsOffset) || checker(ornamentsOffset + sizeof(STPOrnaments)) ||
      checker(samplesOffset) || checker(samplesOffset + sizeof(STPSamples) - 1) ||
      0 != (ornamentsOffset - patternsOffset) % sizeof(STPPattern) ||
      sizeof(STPOrnaments) != samplesOffset - ornamentsOffset
      )
    {
      return false;
    }
    const STPPositions* const positions(safe_ptr_cast<const STPPositions*>(data + positionsOffset));
    if (!positions->Lenght ||
        checker(positionsOffset + sizeof(STPPositions::STPPosEntry) * (positions->Lenght - 1)) ||
        positions->Data + positions->Lenght != std::find_if(positions->Data, positions->Data + positions->Lenght,
          boost::bind<uint8_t>(std::modulus<uint8_t>(), boost::bind(&STPPositions::STPPosEntry::PatternOffset, _1), 6))
       )
    {
      return false;
    }
    const STPOrnaments* const ornaments(safe_ptr_cast<const STPOrnaments*>(data + ornamentsOffset));
    if (ArrayEnd(ornaments->Offsets) !=
      std::find_if(ornaments->Offsets, ArrayEnd(ornaments->Offsets), leChecker))
    {
      return false;
    }
    const STPSamples* const samples(safe_ptr_cast<const STPSamples*>(data + samplesOffset));
    if (ArrayEnd(samples->Offsets) !=
      std::find_if(samples->Offsets, ArrayEnd(samples->Offsets), leChecker))
    {
      return false;
    }
    return true;
  }

  ModulePlayer::Ptr Creating(const String& filename, const IO::DataContainer& data, uint32_t /*capFilter*/)
  {
    assert(Checking(filename, data, 0) || !"Attempt to create stp player on invalid data");
    return ModulePlayer::Ptr(new STPPlayer(filename, FastDump(data)));
  }

  PluginAutoRegistrator registrator(Checking, Creating, Describing);
}
