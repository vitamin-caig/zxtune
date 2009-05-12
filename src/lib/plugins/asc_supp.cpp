#include "plugin_enumerator.h"

#include "tracking_supp.h"
#include "../devices/data_source.h"
#include "../devices/aym/aym.h"

#include <tools.h>

#include <player_attrs.h>

#include <boost/bind.hpp>
#include <boost/static_assert.hpp>

#include <cassert>
#include <valarray>

namespace
{
  using namespace ZXTune;

  const String TEXT_ASC_INFO("ASC modules support");
  const String TEXT_ASC_VERSION("0.1");
  const String TEXT_ASC_EDITOR("Asc Sound Master");

  //hints
  const std::size_t MAX_SAMPLES_COUNT = 32;
  const std::size_t MAX_SAMPLE_SIZE = 150;
  const std::size_t MAX_ORNAMENTS_COUNT = 32;
  const std::size_t MAX_ORNAMENT_SIZE = 30;
  const std::size_t MAX_PATTERN_SIZE = 64;//???
  const std::size_t MAX_PATTERNS_COUNT = 32;//TODO

  const uint16_t FreqTable[96] = {//TODO
    0xedc, 0xe07, 0xd3e, 0xc80, 0xbcc, 0xb22, 0xa82, 0x9ec, 0x95c, 0x8d6, 0x858, 0x7e0,
    0x76e, 0x704, 0x69f, 0x640, 0x5e6, 0x591, 0x541, 0x4f6, 0x4ae, 0x46b, 0x42c, 0x3f0,
    0x3b7, 0x382, 0x34f, 0x320, 0x2f3, 0x2c8, 0x2a1, 0x27b, 0x257, 0x236, 0x216, 0x1f8,
    0x1dc, 0x1c1, 0x1a8, 0x190, 0x179, 0x164, 0x150, 0x13d, 0x12c, 0x11b, 0x10b, 0x0fc,
    0x0ee, 0x0e0, 0x0d4, 0x0c8, 0x0bd, 0x0b2, 0x0a8, 0x09f, 0x096, 0x08d, 0x085, 0x07e,
    0x077, 0x070, 0x06a, 0x064, 0x05e, 0x059, 0x054, 0x050, 0x04b, 0x047, 0x043, 0x03f,
    0x03c, 0x038, 0x035, 0x032, 0x02f, 0x02d, 0x02a, 0x028, 0x026, 0x024, 0x022, 0x020,
    0x1e, 0x1c //??
  };
  //////////////////////////////////////////////////////////////////////////
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct ASCHeader
  {
    uint8_t Tempo;
    uint8_t Loop;
    uint16_t PatternsOffset;
    uint16_t SamplesOffset;
    uint16_t OrnamentsOffset;
    uint8_t Lenght;
    uint8_t Positions[1];
  } PACK_POST;

  const uint8_t ASC_ID_1[] = {'A', 'S', 'M', ' ', 'C', 'O', 'M', 'P', 'I', 'L', 'A', 'T', 'I', 'O', 'N', ' ',
    'O', 'F', ' '};
  const uint8_t ASC_ID_2[] = {' ', 'B', 'Y', ' '};

  PACK_PRE struct ASCID
  {
    uint8_t Identifier1[19];//'ASM COMPILATION OF '
    uint8_t Name[20];
    uint8_t Identifier2[4];//' BY '
    uint8_t Author[20];

    operator bool () const
    {
      BOOST_STATIC_ASSERT(sizeof(ASC_ID_1) == sizeof(Identifier1));
      BOOST_STATIC_ASSERT(sizeof(ASC_ID_2) == sizeof(Identifier2));
      return 0 == std::memcmp(Identifier1, ASC_ID_1, sizeof(Identifier1)) &&
             0 == std::memcmp(Identifier2, ASC_ID_2, sizeof(Identifier2));
    }
  } PACK_POST;

  PACK_PRE struct ASCPattern
  {
    uint16_t Offsets[3];//from start of patterns
  } PACK_POST;

  PACK_PRE struct ASCOrnaments
  {
    uint16_t Offsets[MAX_ORNAMENTS_COUNT];
  } PACK_POST;

  PACK_PRE struct ASCOrnament
  {
    PACK_PRE struct Line
    {
      int8_t NoiseOffset : 5;//?? signed
      uint8_t Finished : 1;//???????????
      uint8_t LoopEnd : 1;//64
      uint8_t LoopBegin : 1;//128
      int8_t NoteOffset;
    } PACK_POST;
    Line Data[1];
  } PACK_POST;

  PACK_PRE struct ASCSamples
  {
    uint16_t Offsets[MAX_SAMPLES_COUNT];
  } PACK_POST;

  PACK_PRE struct ASCSample
  {
    PACK_PRE struct Line
    {
      int8_t Adding : 5;//?? signed...
      uint8_t Finished : 1;
      uint8_t LoopEnd : 1;
      uint8_t LoopBegin : 1;
      int8_t ToneDeviation;
      uint8_t ToneMask : 1;
      uint8_t Command : 2;
      uint8_t NoiseMask : 1;
      uint8_t Level : 4;
    } PACK_POST;
    enum
    {
      EMPTY,
      ENVELOPE,
      DECVOLADD,
      INCVOLADD
    };
    Line Data[1];
  } PACK_POST;

#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(ASCHeader) == 10);
  BOOST_STATIC_ASSERT(sizeof(ASCPattern) == 6);
  BOOST_STATIC_ASSERT(sizeof(ASCOrnament) == 2);
  BOOST_STATIC_ASSERT(sizeof(ASCSample) == 3);

  struct Sample
  {
    explicit Sample(const ASCSample& sample)
      : Loop(), LoopLimit(), Data()
    {
      Data.reserve(MAX_SAMPLE_SIZE);
      const ASCSample::Line* line(sample.Data);
      for (std::size_t sline = 0; sline != MAX_SAMPLE_SIZE; ++sline, ++line)
      {
        Data.push_back(Line(*line));
        if (line->LoopBegin)
        {
          Loop = sline;
        }
        if (line->LoopEnd)
        {
          LoopLimit = Loop + sline;
        }
        if (line->Finished)
        {
          break;
        }
      }
      assert(Data.size() && line->Finished);
      assert(Loop <= LoopLimit);
    }

    std::size_t Loop;
    std::size_t LoopLimit;

    struct Line
    {
      explicit Line(const ASCSample::Line& line)
        : Level(line.Level), ToneDeviation(line.ToneDeviation)
        , ToneMask(line.ToneMask), NoiseMask(line.NoiseMask)
        , Adding(line.Adding), Command(line.Command)
      {
      }
      unsigned Level;//0-15
      signed ToneDeviation;
      bool ToneMask;
      bool NoiseMask;
      signed Adding;
      std::size_t Command;
    };
    std::vector<Line> Data;
  };

  struct Ornament
  {
    explicit Ornament(const ASCOrnament& ornament)
      : Loop(), LoopLimit(), Data()
    {
      Data.reserve(MAX_ORNAMENT_SIZE);
      const ASCOrnament::Line* line(ornament.Data);
      for (std::size_t sline = 0; sline != MAX_ORNAMENT_SIZE; ++sline, ++line)
      {
        Data.push_back(Line(*line));
        if (line->LoopBegin)
        {
          Loop = sline;
        }
        if (line->LoopEnd)
        {
          LoopLimit = Loop + sline;
        }
        if (line->Finished)
        {
          break;
        }
      }
      assert(Data.size() && line->Finished);
      assert(Loop <= LoopLimit);
    }
    std::size_t Loop;
    std::size_t LoopLimit;
    struct Line
    {
      explicit Line(const ASCOrnament::Line& line)
        : NoteAddon(line.NoteOffset)
        , NoiseAddon(line.NoiseOffset)
      {
      }
      signed NoteAddon;
      signed NoiseAddon;
    };
    std::vector<Line> Data;
  };

  enum CmdType
  {
    EMPTY,
    ENVELOPE,     //2p
    ENVELOPE_ON,   //0p
    ENVELOPE_OFF, //0p
    NOISE,        //1p
    CONT_SAMPLE,  //0p
    CONT_ORNAMENT,//0p
    SLIDE,        //1p
    SLIDE_NOTE,   //2p
    AMPLITUDE_DELAY,//1p
    BREAK_SAMPLE  //0p
  };

  class PlayerImpl : public Tracking::TrackPlayer<3, Sample, Ornament>
  {
    typedef Tracking::TrackPlayer<3, Sample, Ornament> Parent;

    static void ParsePattern(const Dump& data, std::vector<std::size_t>& offsets, Parent::Line& line,
      std::valarray<std::size_t>& periods,
      std::valarray<std::size_t>& counters,
      std::vector<bool>& envelopes)
    {
      for (std::size_t chan = 0; chan != line.Channels.size(); ++chan)
      {
        if (counters[chan]--)
        {
          continue;//has to skip
        }
        bool continueSample(false);
        for (;;)
        {
          const uint8_t cmd(data[offsets[chan]++]);
          Line::Chan& channel(line.Channels[chan]);
          if (cmd <= 0x55)//note
          {
            if (!continueSample)
            {
              channel.Enabled = true;
            }
            Parent::CommandsArray::iterator cmdIt(std::find(channel.Commands.begin(), channel.Commands.end(),
              SLIDE_NOTE));
            if (channel.Commands.end() != cmdIt)
            {
              cmdIt->Param2 = cmd;
            }
            else
            {
              channel.Note = cmd;
            }
            if (envelopes[chan])
            {
              //modify existing
              cmdIt = std::find(channel.Commands.begin(), channel.Commands.end(), ENVELOPE);
              if (channel.Commands.end() == cmdIt)
              {
                channel.Commands.push_back(Parent::Command(ENVELOPE, -1, data[offsets[chan]++]));
              }
              else
              {
                cmdIt->Param2 = data[offsets[chan]++];
              }
            }
            break;
          }
          else if (cmd >= 0x56 && cmd <= 0x5d) //stop
          {
            break;
          }
          else if (cmd == 0x5e) //break sample loop
          {
            channel.Commands.push_back(Parent::Command(BREAK_SAMPLE));
            break;
          }
          else if (cmd == 0x5f) //shut
          {
            channel.Enabled = false;
          }
          else if (cmd >= 0x60 && cmd <= 0x9f) //skip
          {
            periods[chan] = cmd - 0x60;
          }
          else if (cmd >= 0xa0 && cmd <= 0xbf) //sample
          {
            channel.SampleNum = cmd - 0xa0;
          }
          else if (cmd >= 0xc0 && cmd <= 0xdf) //ornament
          {
            channel.OrnamentNum = cmd - 0xc0;
          }
          else if (cmd == 0xe0) // envelope full vol
          {
            channel.Volume = 15;
            channel.Commands.push_back(Parent::Command(ENVELOPE_ON));
            envelopes[chan] = true;
          }
          else if (cmd >= 0xe1 && cmd <= 0xef) // noenvelope vol
          {
            channel.Volume = cmd - 0xe0;
            channel.Commands.push_back(Parent::Command(ENVELOPE_OFF));
            envelopes[chan] = false;
          }
          else if (cmd == 0xf0) //initial noise
          {
            channel.Commands.push_back(Parent::Command(NOISE, data[offsets[chan]++]));
          }
          else if (cmd == 0xf1 || cmd == 0xf2 || cmd == 0xf3)
          {
            if (cmd & 1)
            {
              continueSample = true;
              channel.Commands.push_back(Parent::Command(CONT_SAMPLE));
            }
            if (cmd & 2)
            {
              channel.Commands.push_back(Parent::Command(CONT_ORNAMENT));
            }
          }
          else if (cmd == 0xf4) //tempo
          {
            line.Tempo = data[offsets[chan]++];
          }
          else if (cmd == 0xf5 || cmd == 0xf6) //slide
          {
            channel.Commands.push_back(Parent::Command(SLIDE, ((cmd == 0xf5) ? -16 : 16) * static_cast<int8_t>(data[offsets[chan]++])));
          }
          else if (cmd == 0xf7 || cmd == 0xf9) //stepped slide
          {
            if (cmd == 0xf7)
            {
              channel.Commands.push_back(Parent::Command(CONT_SAMPLE));
            }
            channel.Commands.push_back(Parent::Command(SLIDE_NOTE, static_cast<int8_t>(data[offsets[chan]++])));
          }
          else if (cmd == 0xf8 || cmd == 0xfa || cmd == 0xfc || cmd == 0xfe) //envelope
          {
            Parent::CommandsArray::iterator cmdIt(std::find(channel.Commands.begin(), channel.Commands.end(),
              ENVELOPE));
            if (channel.Commands.end() == cmdIt)
            {
              channel.Commands.push_back(Parent::Command(ENVELOPE, cmd & 0xf, -1));
            }
            else
            {
              //strange situation...
              cmdIt->Param1 = cmd & 0xf;
            }
          }
          else if (cmd == 0xfb) //amplitude delay
          {
            channel.Commands.push_back(Parent::Command(AMPLITUDE_DELAY, cmd & 32 ? (((cmd << 3) ^ 0xf8) + 9) : (cmd << 3)));
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
        , Volume(15), AddToVolume(0), Noise(), Note()
        , SampleNum(0), PosInSample(0)
        , OrnamentNum(0), PosInOrnament(0)
      {
      }
      bool Enabled;
      bool Envelope;
      std::size_t Volume;
      signed AddToVolume;
      std::size_t Noise;
      std::size_t Note;
      std::size_t SampleNum;
      std::size_t PosInSample;
      std::size_t OrnamentNum;
      std::size_t PosInOrnament;
    };
  public:
    PlayerImpl(const String& filename, const Dump& data)
      : Device(AYM::CreateChip())//TODO: put out
    {
      //assume data is ok
      const ASCHeader* const header(safe_ptr_cast<const ASCHeader*>(&data[0]));

      Information.Statistic.Tempo = header->Tempo;
      Information.Loop = header->Loop;
      //copy positions
      Data.Positions.assign(header->Positions, header->Positions + header->Lenght);
      { //fill main and additional (if any) information
        Information.Properties.insert(StringMap::value_type(Module::ATTR_FILENAME, filename));
        Information.Properties.insert(StringMap::value_type(Module::ATTR_PROGRAM, TEXT_ASC_EDITOR));
        const ASCID* const id(safe_ptr_cast<const ASCID*>(header->Positions + header->Lenght));
        if (*id)
        {
          Information.Properties.insert(StringMap::value_type(Module::ATTR_TITLE,
            String(id->Name, ArrayEnd(id->Name))));
          Information.Properties.insert(StringMap::value_type(Module::ATTR_AUTHOR,
            String(id->Author, ArrayEnd(id->Author))));
        }
      }

      //parse samples
      const std::size_t samplesOff(fromLE(header->SamplesOffset));
      const ASCSamples* const samples(safe_ptr_cast<const ASCSamples*>(&data[samplesOff]));
      Data.Samples.reserve(MAX_SAMPLES_COUNT);
      for (const uint16_t* pSample = samples->Offsets; pSample != ArrayEnd(samples->Offsets);
        ++pSample)
      {
        assert(*pSample && *pSample < data.size());
        const ASCSample* const sample(safe_ptr_cast<const ASCSample*>(&data[samplesOff + fromLE(*pSample)]));
        Data.Samples.push_back(Sample(*sample));
      }

      //parse ornaments
      const std::size_t ornamentsOff(fromLE(header->OrnamentsOffset));
      const ASCOrnaments* const ornaments(safe_ptr_cast<const ASCOrnaments*>(&data[ornamentsOff]));
      Data.Ornaments.reserve(MAX_ORNAMENTS_COUNT);
      for (const uint16_t* pOrnament = ornaments->Offsets; pOrnament != ArrayEnd(ornaments->Offsets);
        ++pOrnament)
      {
        assert(*pOrnament && *pOrnament < data.size());
        const ASCOrnament* const ornament(safe_ptr_cast<const ASCOrnament*>(&data[ornamentsOff + fromLE(*pOrnament)]));
        Data.Ornaments.push_back(Parent::Ornament(*ornament));
      }

      //parse patterns
      const std::size_t patternsCount(1 + *std::max_element(Data.Positions.begin(), Data.Positions.end()));
      const uint16_t patternsOff(fromLE(header->PatternsOffset));
      const ASCPattern* pattern(safe_ptr_cast<const ASCPattern*>(&data[patternsOff]));
      assert(patternsCount <= MAX_PATTERNS_COUNT);
      Data.Patterns.resize(patternsCount);
      for (std::size_t patNum = 0; patNum < patternsCount; ++patNum, ++pattern)
      {
        Pattern& pat(Data.Patterns[patNum]);
        std::vector<std::size_t> offsets(ArraySize(pattern->Offsets));
        std::valarray<std::size_t> periods(std::size_t(0), ArraySize(pattern->Offsets));
        std::valarray<std::size_t> counters(std::size_t(0), ArraySize(pattern->Offsets));
        std::transform(pattern->Offsets, ArrayEnd(pattern->Offsets), offsets.begin(),
          boost::bind(std::plus<uint16_t>(), patternsOff,
            boost::bind(&fromLE<uint16_t>, _1)));
        assert(ArrayEnd(pattern->Offsets) == std::find(pattern->Offsets, ArrayEnd(pattern->Offsets), patternsOff));
        assert(ArrayEnd(pattern->Offsets) == std::find_if(pattern->Offsets, ArrayEnd(pattern->Offsets),
          std::bind2nd(std::greater<uint16_t>(), data.size())));
        std::vector<bool> envelopes(ArraySize(pattern->Offsets));
        pat.reserve(MAX_PATTERN_SIZE);
        do
        {
          pat.push_back(Line());
          Line& line(pat.back());
          ParsePattern(data, offsets, line, periods, counters, envelopes);
          //skip lines
          if (const std::size_t linesToSkip = counters.min())
          {
            counters -= linesToSkip;
            pat.resize(pat.size() + linesToSkip);//add dummies
          }
        }
        while (0xff != data[offsets[0]] || counters[0]);
        assert(0 == counters.max());
        assert(pat.size() <= MAX_PATTERN_SIZE);
      }
      Information.Statistic.Position = Data.Positions.size();
      Information.Statistic.Pattern = Data.Patterns.size();
      Information.Statistic.Channels = 3;
      InitTime();
    }

    virtual void GetInfo(Info& info) const
    {
      info.Capabilities = CAP_AYM;
      info.Properties.clear();
    }

    /// Retrieving current state of sound
    virtual State GetSoundState(Sound::Analyze::Volume& /*volState*/, Sound::Analyze::Spectrum& /*spectrumState*/) const
    {
      return PlaybackState;
    }

    /// Rendering frame
    virtual State RenderFrame(const Sound::Parameters& params, Sound::Receiver& receiver)
    {
      AYM::DataChunk chunk;
      chunk.Tick = (CurrentState.Tick += uint64_t(params.ClockFreq) * params.FrameDuration / 1000);
      RenderData(chunk);

      Device->RenderData(params, chunk, receiver);

      return Parent::RenderFrame(params, receiver);
    }

    virtual State Reset()
    {
      Device->Reset();
      return Parent::Reset();
    }

    virtual State SetPosition(const uint32_t& /*frame*/)
    {
      return PlaybackState;
    }

  private:
    void RenderData(AYM::DataChunk& chunk)
    {
      const Line& line(Data.Patterns[CurrentState.Position.Pattern][CurrentState.Position.Note]);
      if (0 == CurrentState.Position.Frame)//begin note
      {
        if (!line.Tempo.IsNull())
        {
          CurrentState.Position.Tempo = line.Tempo;
        }
        CurrentState.Position.Channels = 0;
        for (std::size_t chan = 0; chan != line.Channels.size(); ++chan)
        {
          const Line::Chan& src(line.Channels[chan]);
          ChannelState& dst(Channels[chan]);
          if (!src.Enabled.IsNull())
          {
            dst.Enabled = src.Enabled;
          }
          bool contSample(false), contOrnament(false);
          for (Parent::CommandsArray::const_iterator it = src.Commands.begin(), lim = src.Commands.end(); it != lim; ++it)
          {
            switch (it->Type)
            {
            case ENVELOPE:
              if (-1 != it->Param1)
              {
                chunk.Data[AYM::DataChunk::REG_ENV] = uint8_t(it->Param1);
                chunk.Mask |= 1 << AYM::DataChunk::REG_ENV;
              }
              if (-1 != it->Param2)
              {
                chunk.Data[AYM::DataChunk::REG_TONEE_L] = uint8_t(it->Param2);
                chunk.Mask |= 1 << AYM::DataChunk::REG_TONEE_L;
              }
              break;
            case ENVELOPE_ON:
              dst.Envelope = true;
              break;
            case ENVELOPE_OFF:
              dst.Envelope = false;
              break;
            case CONT_SAMPLE:
              contSample = true;
              break;
            case CONT_ORNAMENT:
              contOrnament = true;
              break;
            case NOISE:
              dst.Noise = it->Param1;
            default:
              break;
            }
          }
          if (!src.Note.IsNull())
          {
            dst.Note = src.Note;
            if (!contSample)
            {
              dst.PosInSample = 0;
            }
            if (!contOrnament)
            {
              dst.PosInOrnament = 0;
            }
          }
          if (!src.SampleNum.IsNull())
          {
            dst.SampleNum = src.SampleNum;
          }
          if (!src.OrnamentNum.IsNull())
          {
            dst.OrnamentNum = src.OrnamentNum;
          }
          if (!src.Volume.IsNull())
          {
            dst.Volume = src.Volume;
          }
          if (dst.Enabled)
          {
            ++CurrentState.Position.Channels;
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
          const Ornament::Line& curOrnamentLine(curOrnament.Data[dst->PosInOrnament]);

          //calculate tone
          const std::size_t halfTone(clamp(int(dst->Note) + curOrnamentLine.NoteAddon, 0, 95));
          const uint16_t baseFreq(FreqTable[halfTone]);
          const uint16_t tone(uint16_t(clamp(int(baseFreq) + curSampleLine.ToneDeviation, 0, 0xffff)));

          chunk.Data[toneReg] = uint8_t(tone & 0xff);
          chunk.Data[toneReg + 1] = uint8_t(tone >> 8);
          chunk.Mask |= 3 << toneReg;
          //calculate level
          const uint8_t level((dst->Volume + 1) * curSampleLine.Level >> 4);
          chunk.Data[volReg] = level | uint8_t((dst->Envelope && ASCSample::ENVELOPE == curSampleLine.Command) ? AYM::DataChunk::MASK_ENV : 0);

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
            chunk.Data[AYM::DataChunk::REG_TONEN] = dst->Noise;
          }

          //recalc positions
          ++dst->PosInSample;
          if (dst->PosInSample >= curSample.LoopLimit)
          {
            dst->PosInSample = curSample.Loop;
          }
          if (++dst->PosInOrnament >= curOrnament.LoopLimit)
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
    }
  private:
    AYM::Chip::Ptr Device;
    ChannelState Channels[3];
  };
  //////////////////////////////////////////////////////////////////////////
  void Information(ModulePlayer::Info& info)
  {
    info.Capabilities = CAP_AYM;
    info.Properties.clear();
    info.Properties.insert(StringMap::value_type(ATTR_DESCRIPTION, TEXT_ASC_INFO));
    info.Properties.insert(StringMap::value_type(ATTR_VERSION, TEXT_ASC_VERSION));
  }

  bool Checking(const String& /*filename*/, const Dump& data)
  {
    return false;
  }

  ModulePlayer::Ptr Creating(const String& filename, const Dump& data)
  {
    assert(Checking(filename, data) || !"Attempt to create pt2 player on invalid data");
    return ModulePlayer::Ptr(new PlayerImpl(filename, data));
  }

  PluginAutoRegistrator ascReg(Checking, Creating, Information);
}
