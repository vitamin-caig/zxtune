#include "plugin_enumerator.h"

#include "detector.h"
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

#define FILE_TAG 077C8579

namespace
{
  using namespace ZXTune;

  const String TEXT_PT2_VERSION(FromChar("Revision: $Rev$"));

  const std::size_t LIMITER(~std::size_t(0));

  //hints
  const std::size_t MAX_MODULE_SIZE = 16384;
  const std::size_t MAX_PATTERN_SIZE = 64;
  const std::size_t MAX_PATTERN_COUNT = 64;//TODO

  typedef IO::FastDump<uint8_t> FastDump;

  const uint16_t FreqTable[96] = {
    0xef8, 0xe10, 0xd60, 0xc80, 0xbd8, 0xb28, 0xa88, 0x9f0, 0x960, 0x8e0, 0x858, 0x7e0,
    0x77c, 0x708, 0x6b0, 0x640, 0x5ec, 0x594, 0x544, 0x4f8, 0x4b0, 0x470, 0x42c, 0x3fd,
    0x3be, 0x384, 0x358, 0x320, 0x2f6, 0x2ca, 0x2a2, 0x27c, 0x258, 0x238, 0x216, 0x1f8,
    0x1df, 0x1c2, 0x1ac, 0x190, 0x17b, 0x165, 0x151, 0x13e, 0x12c, 0x11c, 0x10a, 0x0fc,
    0x0ef, 0x0e1, 0x0d6, 0x0c8, 0x0bd, 0x0b2, 0x0a8, 0x09f, 0x096, 0x08e, 0x085, 0x07e,
    0x077, 0x070, 0x06b, 0x064, 0x05e, 0x059, 0x054, 0x04f, 0x04b, 0x047, 0x042, 0x03f,
    0x03b, 0x038, 0x035, 0x032, 0x02f, 0x02c, 0x02a, 0x027, 0x025, 0x023, 0x021, 0x01f,
    0x01d, 0x01c, 0x01a, 0x019, 0x017, 0x016, 0x015, 0x013, 0x012, 0x011, 0x010, 0x00f
  };

  //checkers

  static const std::string PT2_PLAYER();
  const std::size_t PLAYER_SIZE = 2629;

  Module::DetectChain Players[] = {
    /*PT20
    {
      "21??c3??c3+563+f3e522??e57e32",

    },
    */
    //PT21
    {
      "21??c3??c3+14+322e31",
      0xa2f
    },
    //
    /*
    PT24
    21 ? ?  ld hl,xx
    18 03   jr $+3
    c3 ? ?  jp xx
    f3      di
    e5      push hl
    7e      ld a,(hl)
    32 ? ?  ld (xx),a
    32 ? ?  ld (xx),a
    23      inc hl
    23      inc hl
    7e      ld a,(hl)
    23      inc hl
    */
    {
      "21??1803c3??f3e57e32??32??23237e23",
      2629
    }
  };

  //////////////////////////////////////////////////////////////////////////
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct PT2Header
  {
    uint8_t Tempo;
    uint8_t Length;
    uint8_t Loop;
    uint16_t SamplesOffsets[32];
    uint16_t OrnamentsOffsets[16];
    uint16_t PatternsOffset;
    char Name[30];
    uint8_t Positions[1];
  } PACK_POST;

  const uint8_t POS_END_MARKER = 0xff;

  PACK_PRE struct PT2Sample
  {
    uint8_t Size;
    uint8_t Loop;
    PACK_PRE struct Line
    {
      uint8_t NoiseOff : 1;
      uint8_t ToneOff : 1;
      uint8_t VibratoSign : 1;
      uint8_t Noise : 5;
      uint8_t VibratoHi : 4;
      uint8_t Level : 4;
      uint8_t VibratoLo;
    } PACK_POST;
    Line Data[1];
  } PACK_POST;

  PACK_PRE struct PT2Ornament
  {
    uint8_t Size;
    uint8_t Loop;
    int8_t Data[1];
  } PACK_POST;

  PACK_PRE struct PT2Pattern
  {
    uint16_t Offsets[3];

    operator bool () const
    {
      return Offsets[0] && Offsets[1] && Offsets[2];
    }
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(PT2Header) == 132);
  BOOST_STATIC_ASSERT(sizeof(PT2Sample) == 5);
  BOOST_STATIC_ASSERT(sizeof(PT2Ornament) == 3);

  struct Sample
  {
    Sample() : Loop(), Data()
    {
    }

    Sample(std::size_t size, std::size_t loop) : Loop(loop), Data(size)
    {
    }

    std::size_t Loop;

    struct Line
    {
      unsigned Level;//0-15
      unsigned Noise;//0-31
      bool ToneOff;
      bool NoiseOff;
      signed Vibrato;
    };
    std::vector<Line> Data;
  };

  class SampleCreator : public std::unary_function<uint16_t, Sample>
  {
  public:
    explicit SampleCreator(const FastDump& data) : Data(data)
    {
    }

    SampleCreator(const SampleCreator& rh) : Data(rh.Data)
    {
    }

    result_type operator () (const argument_type arg) const
    {
      const PT2Sample* const sample(safe_ptr_cast<const PT2Sample*>(&Data[fromLE(arg)]));
      if (0 == arg || !sample->Size)
      {
        return result_type(1, 0);//safe
      }
      result_type tmp(sample->Size, sample->Loop);
      for (std::size_t idx = 0; idx != sample->Size; ++idx)
      {
        const PT2Sample::Line& src(sample->Data[idx]);
        Sample::Line& dst(tmp.Data[idx]);
        dst.NoiseOff = src.NoiseOff;
        dst.ToneOff = src.ToneOff;
        dst.Noise = src.Noise;
        dst.Level = src.Level;
        dst.Vibrato = src.VibratoLo | (signed(src.VibratoHi) << 8);
        if (!src.VibratoSign)
        {
          dst.Vibrato = -dst.Vibrato;
        }
      }
      return tmp;
    }
  private:
    const FastDump& Data;
  };

  enum CmdType
  {
    EMPTY,
    ENVELOPE,     //2p
    NOENVELOPE,   //0p
    GLISS,        //1p
    GLISS_NOTE,   //2p
    NOGLISS,      //0p
    NOISE_ADD,    //1p
  };

  uint8_t GetVolume(std::size_t volume, std::size_t level)
  {
    return uint8_t((volume * 17 + (volume > 7 ? 1 : 0)) * level >> 8);
  }

  void Describing(ModulePlayer::Info& info);

  typedef Log::WarningsCollector::AutoPrefixParam<std::size_t> IndexPrefix;

  class PT2Player : public Tracking::TrackPlayer<3, Sample>
  {
    typedef Tracking::TrackPlayer<3, Sample> Parent;

    class OrnamentCreator : public std::unary_function<uint16_t, Parent::Ornament>
    {
    public:
      explicit OrnamentCreator(const FastDump& data) : Data(data)
      {
      }

      OrnamentCreator(const OrnamentCreator& rh) : Data(rh.Data)
      {
      }

      result_type operator () (const argument_type arg) const
      {
        const PT2Ornament* const ornament(safe_ptr_cast<const PT2Ornament*>(&Data[fromLE(arg)]));
        if (0 == arg || !ornament->Size)
        {
          return result_type(1, 0);//safe version
        }
        result_type tmp;
        tmp.Loop = ornament->Loop;
        tmp.Data.assign(ornament->Data, ornament->Data + (ornament->Size ? ornament->Size : 1));
        return tmp;
      }
    private:
      const FastDump& Data;
    };

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
          if (cmd >= 0xe1) //sample
          {
            warner.Assert(!channel.SampleNum, TEXT_WARNING_DUPLICATE_SAMPLE);
            channel.SampleNum = cmd - 0xe0;
          }
          else if (cmd == 0xe0) //sample 0 - shut up
          {
            warner.Assert(!channel.Enabled, TEXT_WARNING_DUPLICATE_STATE);
            channel.Enabled = false;
            break;
          }
          else if (cmd >= 0x80 && cmd <= 0xdf)//note
          {
            warner.Assert(!channel.Enabled, TEXT_WARNING_DUPLICATE_STATE);
            channel.Enabled = true;
            const std::size_t note(cmd - 0x80);
            //for note gliss calculate limit manually
            CommandsArray::iterator noteGlissCmd(std::find(channel.Commands.begin(), channel.Commands.end(), GLISS_NOTE));
            if (channel.Commands.end() != noteGlissCmd)
            {
              noteGlissCmd->Param2 = int(note);
            }
            else
            {
              warner.Assert(!channel.Note, TEXT_WARNING_DUPLICATE_NOTE);
              channel.Note = note;
            }
            break;
          }
          else if (cmd == 0x7f) //env off
          {
            channel.Commands.push_back(NOENVELOPE);
          }
          else if (cmd >= 0x71 && cmd <= 0x7e) //envelope
          {
            channel.Commands.push_back(
              Command(ENVELOPE, cmd - 0x70, data[offsets[chan]] | (unsigned(data[offsets[chan] + 1]) << 8)));
            offsets[chan] += 2;
          }
          else if (cmd == 0x70)//quit
          {
            break;
          }
          else if (cmd >= 0x60 && cmd <= 0x6f)//ornament
          {
            warner.Assert(!channel.OrnamentNum, TEXT_WARNING_DUPLICATE_ORNAMENT);
            channel.OrnamentNum = cmd - 0x60;
          }
          else if (cmd >= 0x20 && cmd <= 0x5f)//skip
          {
            periods[chan] = cmd - 0x20;
          }
          else if (cmd >= 0x10 && cmd <= 0x1f)//volume
          {
            warner.Assert(!channel.Volume, TEXT_WARNING_DUPLICATE_VOLUME);
            channel.Volume = cmd - 0x10;
          }
          else if (cmd == 0x0f)//new delay
          {
            warner.Assert(!line.Tempo, TEXT_WARNING_DUPLICATE_TEMPO);
            line.Tempo = data[offsets[chan]++];
          }
          else if (cmd == 0x0e)//gliss
          {
            channel.Commands.push_back(Command(GLISS, static_cast<int8_t>(data[offsets[chan]++])));
          }
          else if (cmd == 0x0d)//note gliss
          {
            //too late when note is filled
            warner.Assert(!channel.Note, TEXT_WARNING_INVALID_NOTE_GLISS);
            channel.Commands.push_back(Command(GLISS_NOTE, static_cast<int8_t>(data[offsets[chan]])));
            //ignore delta due to error
            offsets[chan] += 3;
          }
          else if (cmd == 0x0c) //gliss off
          {
            channel.Commands.push_back(NOGLISS);
          }
          else //noise add
          {
            channel.Commands.push_back(Command(NOISE_ADD, static_cast<int8_t>(data[offsets[chan]++])));
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
        , OrnamentNum(0), PosInOrnament(0)
        , Volume(15), NoiseAdd(0)
        , Sliding(0), SlidingTargetNote(LIMITER), Glissade(0)
      {
      }
      bool Enabled;
      bool Envelope;
      std::size_t Note;
      std::size_t SampleNum;
      std::size_t PosInSample;
      std::size_t OrnamentNum;
      std::size_t PosInOrnament;
      std::size_t Volume;
      signed NoiseAdd;
      signed Sliding;
      std::size_t SlidingTargetNote;
      signed Glissade;
    };
  public:
    PT2Player(const String& filename, const FastDump& data)
      : Parent(filename)
      , Device(AYM::CreateChip())//TODO: put out
    {
      //assume all data is correct
      const PT2Header* const header(safe_ptr_cast<const PT2Header*>(&data[0]));
      Information.Statistic.Tempo = header->Tempo;
      Information.Statistic.Position = header->Length;
      Information.Loop = header->Loop;

      Log::WarningsCollector warner;

      //fill samples
      std::transform(header->SamplesOffsets, ArrayEnd(header->SamplesOffsets),
        std::back_inserter(Data.Samples), SampleCreator(data));
      //fill ornaments
      std::transform(header->OrnamentsOffsets, ArrayEnd(header->OrnamentsOffsets),
        std::back_inserter(Data.Ornaments), OrnamentCreator(data));
      //fill order
      Data.Positions.assign(header->Positions,
        std::find(header->Positions, header->Positions + header->Length, POS_END_MARKER));
      warner.Assert(header->Length == Data.Positions.size(), TEXT_WARNING_INVALID_LENGTH);

      //fill patterns
      std::size_t rawSize(0);
      Data.Patterns.reserve(MAX_PATTERN_COUNT);
      std::size_t index(0);
      for (const PT2Pattern* patPos(safe_ptr_cast<const PT2Pattern*>(&data[fromLE(header->PatternsOffset)]));
        *patPos;
        ++patPos, ++index)
      {
        IndexPrefix pfx(warner, TEXT_PATTERN_WARN_PREFIX, index);
        Data.Patterns.push_back(Pattern());
        Pattern& pat(Data.Patterns.back());
        std::vector<std::size_t> offsets(ArraySize(patPos->Offsets));
        std::valarray<std::size_t> periods(std::size_t(0), ArraySize(patPos->Offsets));
        std::valarray<std::size_t> counters(std::size_t(0), ArraySize(patPos->Offsets));
        std::transform(patPos->Offsets, ArrayEnd(patPos->Offsets), offsets.begin(), &fromLE<uint16_t>);
        pat.reserve(MAX_PATTERN_SIZE);
        do
        {
          IndexPrefix pfx(warner, TEXT_LINE_WARN_PREFIX, pat.size());
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
        while (data[offsets[0]] || counters[0]);
        //as warnings
        warner.Assert(0 == counters.max(), TEXT_WARNING_PERIODS);
        warner.Assert(pat.size() <= MAX_PATTERN_SIZE, TEXT_WARNING_TOO_LONG);
        rawSize = std::max(rawSize, *std::max_element(offsets.begin(), offsets.end()));
      }
      Information.Statistic.Pattern = Data.Patterns.size();
      Information.Statistic.Channels = 3;
      const String& warnings(warner.GetWarnings());
      if (!warnings.empty())
      {
        Information.Properties.insert(StringMap::value_type(Module::ATTR_WARNINGS, warnings));
      }

      FillProperties(TEXT_PT2_EDITOR, String(), String(header->Name, ArrayEnd(header->Name)),
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
            if (!(dst.Enabled = *src.Enabled))
            {
              dst.Sliding = dst.Glissade = 0;
              dst.SlidingTargetNote = LIMITER;
              dst.PosInSample = dst.PosInOrnament = 0;
            }
          }
          if (src.Note)
          {
            dst.Note = *src.Note;
            dst.PosInSample = dst.PosInOrnament = 0;
            dst.Sliding = dst.Glissade = 0;
            dst.SlidingTargetNote = LIMITER;
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
              chunk.Data[AYM::DataChunk::REG_ENV] = uint8_t(it->Param1);
              chunk.Data[AYM::DataChunk::REG_TONEE_L] = uint8_t(it->Param2 & 0xff);
              chunk.Data[AYM::DataChunk::REG_TONEE_H] = uint8_t(it->Param2 >> 8);
              chunk.Mask |= (1 << AYM::DataChunk::REG_ENV) |
                (1 << AYM::DataChunk::REG_TONEE_L) | (1 << AYM::DataChunk::REG_TONEE_H);
              dst.Envelope = true;
              break;
            case NOENVELOPE:
              dst.Envelope = false;
              break;
            case NOISE_ADD:
              dst.NoiseAdd = it->Param1;
              break;
            case GLISS_NOTE:
              dst.Glissade = it->Param1;
              dst.SlidingTargetNote = it->Param2;
              break;
            case GLISS:
              dst.Glissade = it->Param1;
              break;
            case NOGLISS:
              dst.Glissade = 0;
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
        //sample0 - disable channel
        if (dst->Enabled && dst->SampleNum)
        {
          const Sample& curSample(Data.Samples[dst->SampleNum]);
          const Sample::Line& curSampleLine(curSample.Data[dst->PosInSample]);
          const Ornament& curOrnament(Data.Ornaments[dst->OrnamentNum]);

          assert(!curOrnament.Data.empty());
          //calculate tone
          const std::size_t halfTone(clamp<std::size_t>(dst->Note + curOrnament.Data[dst->PosInOrnament], 0, 95));
          const uint16_t tone = static_cast<uint16_t>(clamp(FreqTable[halfTone] + dst->Sliding + curSampleLine.Vibrato, 0, 0xffff));
          if (dst->SlidingTargetNote != LIMITER)
          {
            const unsigned nextTone(FreqTable[dst->Note] + dst->Sliding + dst->Glissade);
            const unsigned slidingTarget(FreqTable[dst->SlidingTargetNote]);
            if ((dst->Glissade > 0 && nextTone >= slidingTarget) ||
                (dst->Glissade < 0 && nextTone <= slidingTarget))
            {
              dst->Note = dst->SlidingTargetNote;
              dst->SlidingTargetNote = LIMITER;
              dst->Sliding = dst->Glissade = 0;
            }
          }
          dst->Sliding += dst->Glissade;
          chunk.Data[toneReg] = uint8_t(tone & 0xff);
          chunk.Data[toneReg + 1] = uint8_t(tone >> 8);
          chunk.Mask |= 3 << toneReg;
          //calculate level
          chunk.Data[volReg] = GetVolume(dst->Volume, curSampleLine.Level)
            | uint8_t(dst->Envelope ? AYM::DataChunk::MASK_ENV : 0);
          //mixer
          if (curSampleLine.ToneOff)
          {
            chunk.Data[AYM::DataChunk::REG_MIXER] |= toneMsk;
          }
          if (curSampleLine.NoiseOff)
          {
            chunk.Data[AYM::DataChunk::REG_MIXER] |= noiseMsk;
          }
          else
          {
            chunk.Data[AYM::DataChunk::REG_TONEN] = uint8_t(clamp<signed>(curSampleLine.Noise + dst->NoiseAdd, 0, 31));
            chunk.Mask |= 1 << AYM::DataChunk::REG_TONEN;
          }

          if (++dst->PosInSample >= curSample.Data.size())
          {
            dst->PosInSample = curSample.Loop;
          }
          if (++dst->PosInOrnament >= curOrnament.Data.size())
          {
            dst->PosInOrnament = curOrnament.Loop;
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
    ChannelState Channels[3];
  };

  //////////////////////////////////////////////////////////////////////////
  void Describing(ModulePlayer::Info& info)
  {
    info.Capabilities = CAP_DEV_AYM | CAP_CONV_RAW;
    info.Properties.clear();
    info.Properties.insert(StringMap::value_type(ATTR_DESCRIPTION, TEXT_PT2_INFO));
    info.Properties.insert(StringMap::value_type(ATTR_VERSION, TEXT_PT2_VERSION));
  }

  bool Check(const uint8_t* data, std::size_t size)
  {
    if (sizeof(PT2Header) > size/* || size > MAX_MODULE_SIZE*/)
    {
      return false;
    }

    const PT2Header* const header(safe_ptr_cast<const PT2Header*>(data));
    if (header->Length < 1 || header->Tempo < 2 || header->Loop >= header->Length)
    {
      return false;
    }
    const std::size_t lowlimit(1 + std::find(header->Positions, data + size, POS_END_MARKER) - data);
    if (lowlimit - sizeof(*header) != header->Length)//too big positions list
    {
      return false;
    }

    boost::function<bool(std::size_t)> checker = !boost::bind(&in_range<std::size_t>, _1, lowlimit, size - 1);

    //check offsets
    for (const uint16_t* sampOff = header->SamplesOffsets; sampOff != ArrayEnd(header->SamplesOffsets); ++sampOff)
    {
      const std::size_t offset(fromLE(*sampOff));
      if (offset && checker(offset))
      {
        return false;
      }
      const PT2Sample* const sample(safe_ptr_cast<const PT2Sample*>(data + offset));
      if (offset + sizeof(*sample) + (sample->Size - 1) * sizeof(PT2Sample::Line) > size)
      {
        return false;
      }
    }
    for (const uint16_t* ornOff = header->OrnamentsOffsets; ornOff != ArrayEnd(header->OrnamentsOffsets); ++ornOff)
    {
      const std::size_t offset(fromLE(*ornOff));
      if (offset && checker(offset))
      {
        return false;
      }
      const PT2Ornament* const ornament(safe_ptr_cast<const PT2Ornament*>(data + offset));
      if (offset + sizeof(*ornament) + ornament->Size - 1 > size)
      {
        return false;
      }
    }
    const std::size_t patOff(fromLE(header->PatternsOffset));
    if (checker(patOff))
    {
      return false;
    }
    //check patterns
    std::size_t patternsCount(0);
    for (const PT2Pattern* patPos(safe_ptr_cast<const PT2Pattern*>(data + patOff));
      *patPos;
      ++patPos, ++patternsCount)
    {
      if (ArrayEnd(patPos->Offsets) != std::find_if(patPos->Offsets, ArrayEnd(patPos->Offsets), checker))
      {
        return false;
      }
    }
    if (!patternsCount)
    {
      return false;
    }
    return header->Positions + header->Length == 
      std::find_if(header->Positions, header->Positions + header->Length, std::bind2nd(std::greater_equal<uint8_t>(),
        patternsCount));
  }

  bool Checking(const String& /*filename*/, const IO::DataContainer& source, uint32_t /*capFilter*/)
  {
    const uint8_t* data(static_cast<const uint8_t*>(source.Data()));
    const std::size_t size(source.Size());
    return Check(data, size) || 
      ArrayEnd(Players) != std::find_if(Players, ArrayEnd(Players), Module::Detector(Check, data, size));
  }

  ModulePlayer::Ptr Creating(const String& filename, const IO::DataContainer& data, uint32_t /*capFilter*/)
  {
    assert(Checking(filename, data, 0) || !"Attempt to create pt2 player on invalid data");
    const uint8_t* const buf(static_cast<const uint8_t*>(data.Data()));
    const Module::DetectChain* const playerIt(std::find_if(Players, ArrayEnd(Players), 
      Module::Detector(Check, buf, data.Size())));
    const std::size_t offset(ArrayEnd(Players) == playerIt ? 0 : playerIt->PlayerSize);
    return ModulePlayer::Ptr(new PT2Player(filename, FastDump(data, offset)));
  }

  PluginAutoRegistrator registrator(Checking, Creating, Describing);
}
