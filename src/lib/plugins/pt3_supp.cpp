#include "plugin_enumerator.h"

#include "detector.h"
#include "vortex_base.h"
#include "../devices/data_source.h"

#include "../io/container.h"
#include "../io/warnings_collector.h"

#include <tools.h>

#include <player_attrs.h>
#include <convert_parameters.h>

#include <boost/static_assert.hpp>

#include <cassert>
#include <valarray>

#include <text/plugins.h>
#include <text/warnings.h>

namespace
{
  using namespace ZXTune;

  const String TEXT_PT3_VERSION(FromChar("Revision: $Rev$"));

  const std::size_t MAX_MODULE_SIZE = 1 << 16;
  const std::size_t MAX_PATTERNS_COUNT = 48;
  const std::size_t MAX_PATTERN_SIZE = 64;
  const std::size_t MAX_SAMPLES_COUNT = 32;
  const std::size_t MAX_SAMPLE_SIZE = 64;
  const std::size_t MAX_ORNAMENT_SIZE = 64;

  typedef IO::FastDump<uint8_t> FastDump;

  struct DetectChain
  {
    const std::string PlayerFP;
    const std::size_t PlayerSize;
  };

  DetectChain Players[] = {
    //PT3x
    {
      "21??18?c3??c3+35+f322??22??22??22??01640009",
      0xe21
    },
    //PT3.5x
    {
      "21??18?c3??c3+37+f3ed73??22??22??22??22??01640009",
      0x30f
    },
    /*Vortex1
    21 ??     ld hl,xxxx
    18 ?      jr xx
    c3 ??     jp xxxx
    18        jr xx
              db xx
    ?x25      dw xxxx
              ds 21
    21 ??     ld hl,xxxx
    cb fe     set 7,(hl)
    cb 46     bit 0,(hl)
    c8        ret z
    e1        pop hl
    21 ??     ld hl,xxxx
    34        inc (hl)
    21 ??     ld hl,xxxx
    34        inc (hl)
    af        xor a
    67        ld h,a
    6f        ld l,a
    32 ??     ld (xxxx),a
    22 ??     ld (xxxx),hl
    c3        jp xxxx
    */
    {
      "21??18?c3??18+25+21??cbfecb46c8e121??3421??34af676f32??22??c3",
      0x86e
    },
    /*Vortex2
    21 ??     ld hl,xxxx
    18 ?      jr xx
    c3 ??     jp xxxx
    18 ?      jr xx
    ?         db xx
    f3        di
    ed 73 ??  ld (..),sp
    22 ??     ld (..),hl
    44        ld b,h
    4d        ld c,l
    11 ??     ld de,xx
    19        add hl,de
    7e        ld a,(hl)
    23        inc hl
    32 ??     ld (..),a
    f9        ld sp,hl
    19        add hl,de
    22 ??     ld (..),hl
    f1        pop af
    5f        ld e,a
    19        add hl,de
    22 ??     ld (..),hl
    e1        pop hl
    09        add hl,bc
    22 ??     ld (..),hl
    */
    {
      "21??18?c3??18?f3ed73??22??444d11??197e2332??f91922??f15f1922??e109",
      0xc00
    },
    //Another Vortex
    {
      "21??18?c3??18?f3ed73??22??444d11??197e2332??f91922??f15f1922??e109",
      0xc89
    }
  };
  //////////////////////////////////////////////////////////////////////////
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct PT3Header
  {
    uint8_t Id[13];        //'ProTracker 3.'
    uint8_t Subversion;
    uint8_t Optional1[16]; //' compilation of '
    uint8_t TrackName[32];
    uint8_t Optional2[4]; //' by '
    uint8_t TrackAuthor[32];
    uint8_t Optional3;
    uint8_t FreqTableNum;
    uint8_t Tempo;
    uint8_t Lenght;
    uint8_t Loop;
    uint16_t PatternsOffset;
    uint16_t SamplesOffsets[32];
    uint16_t OrnamentsOffsets[16];
    uint8_t Positions[1]; //finished by marker
  } PACK_POST;

  PACK_PRE struct PT3Pattern
  {
    uint16_t Offsets[3];

    operator bool () const
    {
      return Offsets[0] && Offsets[1] && Offsets[2];
    }
  } PACK_POST;

  PACK_PRE struct PT3Sample
  {
    uint8_t Loop;
    uint8_t Size;
    PACK_PRE struct Line
    {
      uint8_t EnvMask : 1;
      uint8_t NoiseOrEnvOffset : 5;
      uint8_t VolSlideUp : 1;
      uint8_t VolSlide : 1;
      uint8_t Level : 4;
      uint8_t ToneMask : 1;
      uint8_t KeepNoiseOrEnvOffset : 1;
      uint8_t KeepToneOffset : 1;
      uint8_t NoiseMask : 1;
      int16_t ToneOffset;
    } PACK_POST;
    Line Data[1];
  } PACK_POST;

  PACK_PRE struct PT3Ornament
  {
    uint8_t Loop;
    uint8_t Size;
    int8_t Data[1];
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(PT3Header) == 202);
  BOOST_STATIC_ASSERT(sizeof(PT3Pattern) == 6);
  BOOST_STATIC_ASSERT(sizeof(PT3Sample) == 6);
  BOOST_STATIC_ASSERT(sizeof(PT3Ornament) == 3);

  void Describing(ModulePlayer::Info& info);

  typedef Log::WarningsCollector::AutoPrefixParam<std::size_t> IndexPrefix;

  class PT3Player : public Tracking::VortexPlayer
  {
    typedef Tracking::VortexPlayer Parent;

    class SampleCreator : public std::unary_function<uint16_t, Parent::Sample>
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
        const PT3Sample* const sample(safe_ptr_cast<const PT3Sample*>(&Data[fromLE(arg)]));
        if (0 == arg || !sample->Size)
        {
          return result_type(1, 0);//safe
        }
        result_type tmp(sample->Size, sample->Loop);
        for (std::size_t idx = 0; idx != sample->Size; ++idx)
        {
          const PT3Sample::Line& src(sample->Data[idx]);
          Sample::Line& dst(tmp.Data[idx]);
          dst.NoiseMask = src.NoiseMask;
          dst.ToneMask = src.ToneMask;
          dst.EnvMask = src.EnvMask;
          dst.KeepToneOffset = src.KeepToneOffset;
          dst.NEOffset =  static_cast<int8_t>(src.NoiseOrEnvOffset & 16 ? src.NoiseOrEnvOffset | 0xf0 : src.NoiseOrEnvOffset);
          dst.KeepNEOffset = src.KeepNoiseOrEnvOffset;

          dst.Level = src.Level;
          dst.VolSlideAddon = src.VolSlide ? (src.VolSlideUp ? +1 : -1) : 0;
          dst.ToneOffset = static_cast<int16_t>(fromLE(static_cast<uint16_t>(src.ToneOffset)));
        }
        return tmp;
      }
    private:
      const FastDump& Data;
    };

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
        if (0 == arg || arg >= Data.Size())
        {
          return result_type(1, 0);//safe version
        }
        const PT3Ornament* const ornament(safe_ptr_cast<const PT3Ornament*>(&Data[fromLE(arg)]));
        result_type tmp;
        tmp.Loop = ornament->Loop;
        tmp.Data.assign(ornament->Data, ornament->Data + (ornament->Size ? ornament->Size : 1));
        return tmp;
      }
    private:
      const FastDump& Data;
    };

    void ParsePattern(const FastDump& data, std::vector<std::size_t>& offsets, std::vector<std::size_t>& ornaments,
      Parent::Line& line,
      std::valarray<std::size_t>& periods,
      std::valarray<std::size_t>& counters,
      Log::WarningsCollector& warner)
    {
      const std::size_t maxSamples(Data.Samples.size());
      const std::size_t maxOrnaments(Data.Ornaments.size());
      bool wasEnvelope(false), wasNoisebase(false);
      for (std::size_t chan = 0; chan != line.Channels.size(); ++chan)
      {
        if (counters[chan]--)
        {
          continue;//has to skip
        }
        IndexPrefix pfx(warner, TEXT_CHANNEL_WARN_PREFIX, chan);
        Line::Chan& channel(line.Channels[chan]);
        for (;;)
        {
          const unsigned cmd(data[offsets[chan]++]);
          if (cmd == 1)//gliss
          {
            channel.Commands.push_back(Parent::Command(GLISS));
          }
          else if (cmd == 2)//portamento
          {
            channel.Commands.push_back(Parent::Command(GLISS_NOTE));
          }
          else if (cmd == 3)//sample offset
          {
            channel.Commands.push_back(Parent::Command(SAMPLEOFFSET, -1));
          }
          else if (cmd == 4)//ornament offset
          {
            channel.Commands.push_back(Parent::Command(ORNAMENTOFFSET));
          }
          else if (cmd == 5)//vibrate
          {
            channel.Commands.push_back(Parent::Command(VIBRATE));
          }
          else if (cmd == 8)//slide envelope
          {
            channel.Commands.push_back(Parent::Command(SLIDEENV));
          }
          else if (cmd == 9)//tempo
          {
            channel.Commands.push_back(Parent::Command(TEMPO));
          }
          else if (cmd == 0x10 || cmd >= 0xf0)
          {
            const uint8_t doubleSampNum(data[offsets[chan]++]);
            const bool sampValid(doubleSampNum < maxSamples * 2);
            warner.Assert(sampValid && 0 == (doubleSampNum & 1), TEXT_WARNING_INVALID_SAMPLE);
            warner.Assert(!channel.SampleNum, TEXT_WARNING_DUPLICATE_SAMPLE);
            channel.SampleNum = sampValid ? (doubleSampNum / 2) : 0;
            if (cmd != 0x10)
            {
              const bool ornValid(cmd - 0xf0 < maxOrnaments);
              warner.Assert(ornValid, TEXT_WARNING_INVALID_ORNAMENT);
              warner.Assert(!channel.OrnamentNum, TEXT_WARNING_DUPLICATE_ORNAMENT);
              channel.OrnamentNum = ornaments[chan] = ornValid ? (cmd - 0xf0) : 0;
            }
            else
            {
              channel.OrnamentNum = ornaments[chan];
            }
            channel.Commands.push_back(Parent::Command(NOENVELOPE));
          }
          else if ((cmd >= 0x11 && cmd <= 0x1f) || (cmd >= 0xb2 && cmd <= 0xbf))
          {
            const uint16_t envPeriod(data[offsets[chan] + 1] + (uint16_t(data[offsets[chan]]) << 8));
            offsets[chan] += 2;
            if (cmd >= 0x11 && cmd <= 0x1f)
            {
              warner.Assert(!wasEnvelope, TEXT_WARNING_DUPLICATE_ENVELOPE);
              channel.Commands.push_back(Parent::Command(ENVELOPE, cmd - 0x10, envPeriod));
              wasEnvelope = true;
              const uint8_t doubleSampNum(data[offsets[chan]++]);
              const bool sampValid(doubleSampNum < maxSamples * 2);
              warner.Assert(sampValid && 0 == (doubleSampNum & 1), TEXT_WARNING_INVALID_SAMPLE);
              warner.Assert(!channel.SampleNum, TEXT_WARNING_DUPLICATE_SAMPLE);
              channel.SampleNum = sampValid ? (doubleSampNum / 2) : 0;
            }
            else
            {
              warner.Assert(!wasEnvelope, TEXT_WARNING_DUPLICATE_ENVELOPE);
              channel.Commands.push_back(Parent::Command(ENVELOPE, cmd - 0xb1, envPeriod));
              wasEnvelope = true;
            }
            channel.OrnamentNum = ornaments[chan];
          }
          else if (cmd >= 0x20 && cmd <= 0x3f)
          {
            warner.Assert(!wasNoisebase, TEXT_WARNING_DUPLICATE_NOISEBASE);
            channel.Commands.push_back(Parent::Command(NOISEBASE, cmd - 0x20));
            wasNoisebase = true;
            warner.Assert(chan == 1, TEXT_WARNING_INVALID_NOISE_BASE);
          }
          else if (cmd >= 0x40 && cmd <= 0x4f)
          {
            const bool ornValid(cmd - 0x40 < maxOrnaments);
            warner.Assert(ornValid, TEXT_WARNING_INVALID_ORNAMENT);
            warner.Assert(!channel.OrnamentNum, TEXT_WARNING_DUPLICATE_ORNAMENT);
            channel.OrnamentNum = ornaments[chan] = ornValid ? (cmd - 0x40) : 0;
            //In despite of oficial documentation, editor cannot insert 0x40 command without
            //envelope switch off. But there's no 0xb0 command in stream... Dumn optimization...
            if (!*channel.OrnamentNum)
            {
              assert(channel.Commands.end() == std::find(channel.Commands.begin(), channel.Commands.end(), NOENVELOPE));
              channel.Commands.push_back(Parent::Command(NOENVELOPE));
            }
          }
          else if (cmd >= 0x50 && cmd <= 0xaf)
          {
            Parent::CommandsArray::iterator it(std::find(channel.Commands.begin(), channel.Commands.end(), GLISS_NOTE));
            if (channel.Commands.end() != it)
            {
              it->Param3 = cmd - 0x50;
            }
            else
            {
              warner.Assert(!channel.Note, TEXT_WARNING_DUPLICATE_NOTE);
              channel.Note = cmd - 0x50;
            }
            warner.Assert(!channel.Enabled, TEXT_WARNING_DUPLICATE_STATE);
            channel.Enabled = true;
            break;
          }
          else if (cmd == 0xb0)
          {
            channel.Commands.push_back(Parent::Command(NOENVELOPE));
            channel.OrnamentNum = ornaments[chan];
          }
          else if (cmd == 0xb1)
          {
            periods[chan] = data[offsets[chan]++] - 1;
          }
          else if (cmd == 0xc0)
          {
            warner.Assert(!channel.Enabled, TEXT_WARNING_DUPLICATE_STATE);
            channel.Enabled = false;
            break;
          }
          else if (cmd >= 0xc1 && cmd <= 0xcf)
          {
            warner.Assert(!channel.Volume, TEXT_WARNING_DUPLICATE_VOLUME);
            channel.Volume = cmd - 0xc0;
          }
          else if (cmd == 0xd0)
          {
            break;
          }
          else if (cmd >= 0xd1 && cmd <= 0xef)
          {
            const bool sampValid(cmd - 0xd0 < maxSamples);
            warner.Assert(sampValid, TEXT_WARNING_INVALID_SAMPLE);
            warner.Assert(!channel.SampleNum, TEXT_WARNING_DUPLICATE_SAMPLE);
            channel.SampleNum = sampValid ? (cmd - 0xd0) : 0;
          }
        }
        //parse parameters
        for (Parent::CommandsArray::reverse_iterator it = channel.Commands.rbegin(), lim = channel.Commands.rend();
          it != lim; ++it)
        {
          switch (it->Type)
          {
          case TEMPO:
            //warning
            warner.Assert(!line.Tempo, TEXT_WARNING_DUPLICATE_TEMPO);
            line.Tempo = data[offsets[chan]++];
            break;
          case SLIDEENV:
          case GLISS:
            it->Param1 = data[offsets[chan]++];
            it->Param2 = data[offsets[chan]] + (int16_t(static_cast<int8_t>(data[offsets[chan] + 1])) << 8);
            offsets[chan] += 2;
            break;
          case VIBRATE:
            it->Param1 = data[offsets[chan]++];
            it->Param2 = data[offsets[chan]++];
            break;
          case ORNAMENTOFFSET:
          {
            const uint8_t offset(data[offsets[chan]++]);
            const bool isValid(offset < (channel.OrnamentNum ?
              Data.Ornaments[*channel.OrnamentNum].Data.size() : MAX_ORNAMENT_SIZE));
            warner.Assert(isValid, TEXT_WARNING_INVALID_ORNAMENT_OFFSET);
            it->Param1 = isValid ? offset : 0;
            break;
          }
          case SAMPLEOFFSET:
            if (-1 == it->Param1)
            {
              const uint8_t offset(data[offsets[chan]++]);
              const bool isValid(offset < (channel.SampleNum ?
                Data.Samples[*channel.SampleNum].Data.size() : MAX_SAMPLE_SIZE));
              warner.Assert(isValid, TEXT_WARNING_INVALID_SAMPLE_OFFSET);
              it->Param1 = isValid ? offset : 0;
            }
            break;
          case GLISS_NOTE:
            it->Param1 = data[offsets[chan]++];
            it->Param2 = static_cast<int16_t>(fromLE(*safe_ptr_cast<const uint16_t*>(&data[offsets[chan] + 2])));
            offsets[chan] += 4;
            break;
          }
        }
        counters[chan] = periods[chan];
      }
    }

  public:
    PT3Player(const String& filename, const FastDump& data)
      : Parent(filename)
    {
      //assume all data is correct
      const PT3Header* const header(safe_ptr_cast<const PT3Header*>(&data[0]));
      Information.Statistic.Tempo = header->Tempo;
      Information.Statistic.Position = header->Lenght;
      Information.Loop = header->Loop;

      Log::WarningsCollector warner;

      //fill samples
      std::transform(header->SamplesOffsets, ArrayEnd(header->SamplesOffsets),
        std::back_inserter(Data.Samples), SampleCreator(data));
      //fill ornaments
      std::transform(header->OrnamentsOffsets, ArrayEnd(header->OrnamentsOffsets),
        std::back_inserter(Data.Ornaments), OrnamentCreator(data));
      //fill order
      Data.Positions.resize(header->Lenght);
      std::transform(header->Positions, header->Positions + header->Lenght,
        Data.Positions.begin(), std::bind2nd(std::divides<uint8_t>(), 3));

      std::size_t rawSize(0);

      //fill patterns
      Data.Patterns.resize(1 + *std::max_element(Data.Positions.begin(), Data.Positions.end()));
      const PT3Pattern* patPos(safe_ptr_cast<const PT3Pattern*>(&data[fromLE(header->PatternsOffset)]));
      std::size_t index(0);
      for (std::vector<Pattern>::iterator it = Data.Patterns.begin(), lim = Data.Patterns.end();
        it != lim;
        ++it, ++patPos, ++index)
      {
        IndexPrefix patPfx(warner, TEXT_PATTERN_WARN_PREFIX, index);
        Pattern& pat(*it);
        std::vector<std::size_t> offsets(ArraySize(patPos->Offsets));
        std::vector<std::size_t> ornaments(ArraySize(patPos->Offsets));
        std::valarray<std::size_t> periods(std::size_t(0), ArraySize(patPos->Offsets));
        std::valarray<std::size_t> counters(std::size_t(0), ArraySize(patPos->Offsets));
        std::transform(patPos->Offsets, ArrayEnd(patPos->Offsets), offsets.begin(), &fromLE<uint16_t>);
        pat.reserve(MAX_PATTERN_SIZE);
        do
        {
          IndexPrefix notePfx(warner, TEXT_LINE_WARN_PREFIX, pat.size());
          pat.push_back(Line());
          Line& line(pat.back());
          ParsePattern(data, offsets, ornaments, line, periods, counters, warner);
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
      assert(rawSize <= data.Size());

      Information.Statistic.Pattern = Data.Patterns.size();
      Information.Statistic.Channels = 3;

      const String& warnings(warner.GetWarnings());
      if (!warnings.empty())
      {
        Information.Properties.insert(StringMap::value_type(Module::ATTR_WARNINGS, warnings));
      }

      FillProperties(String(header->Id, 1 + ArrayEnd(header->Id)),
        String(header->TrackAuthor, ArrayEnd(header->TrackAuthor)),
        String(header->TrackName, ArrayEnd(header->TrackName)),
        &data[0], rawSize);

      Parent::Initialize(isdigit(header->Subversion) ? header->Subversion - '0' : 6,
        static_cast<Parent::NoteTable>(header->FreqTableNum));
    }

    virtual void GetInfo(Info& info) const
    {
      Describing(info);
    }
  };

  //////////////////////////////////////////////////////////////////////////
  void Describing(ModulePlayer::Info& info)
  {
    info.Capabilities = CAP_DEV_AYM | CAP_CONV_RAW | CAP_CONV_VORTEX;
    info.Properties.clear();
    info.Properties.insert(StringMap::value_type(ATTR_DESCRIPTION, TEXT_PT3_INFO));
    info.Properties.insert(StringMap::value_type(ATTR_VERSION, TEXT_PT3_VERSION));
  }

  bool Check(const uint8_t* data, std::size_t limit)
  {
    const PT3Header* const header(safe_ptr_cast<const PT3Header*>(data));
    const std::size_t patOff(fromLE(header->PatternsOffset));
    if (patOff >= limit ||
      0xff != data[patOff - 1] ||
      &data[patOff - 1] != std::find_if(header->Positions, data + patOff - 1,
      std::bind2nd(std::modulus<uint8_t>(), 3)) ||
      &header->Positions[header->Lenght] != data + patOff - 1 ||
      fromLE(header->OrnamentsOffsets[0]) + sizeof(PT3Ornament) > limit
      )
    {
      return false;
    }
    //TODO
    return true;
  }

  class Detector : public std::unary_function<DetectChain, bool>
  {
  public:
    Detector(const uint8_t* data, std::size_t limit) : Data(data), Limit(limit)
    {
    }

    result_type operator()(const argument_type& arg) const
    {
      return Module::Detect(Data, Limit, arg.PlayerFP) &&
        Check(Data + arg.PlayerSize, Limit - arg.PlayerSize);
    }
  private:
    const uint8_t* const Data;
    const std::size_t Limit;
  };

  bool Checking(const String& /*filename*/, const IO::DataContainer& source, uint32_t /*capFilter*/)
  {
    const std::size_t limit(std::min(source.Size(), MAX_MODULE_SIZE));
    if (limit < sizeof(PT3Header))
    {
      return false;
    }

    const uint8_t* const data(static_cast<const uint8_t*>(source.Data()));
    return Check(data, limit) ||
      ArrayEnd(Players) != std::find_if(Players, ArrayEnd(Players), Detector(data, limit));
  }

  ModulePlayer::Ptr Creating(const String& filename, const IO::DataContainer& data, uint32_t /*capFilter*/)
  {
    assert(Checking(filename, data, 0) || !"Attempt to create pt3 player on invalid data");
    const uint8_t* const buf(static_cast<const uint8_t*>(data.Data()));
    const DetectChain* const playerIt(std::find_if(Players, ArrayEnd(Players), Detector(buf, data.Size())));
    const std::size_t offset(ArrayEnd(Players) == playerIt ? 0 : playerIt->PlayerSize);
    return ModulePlayer::Ptr(new PT3Player(filename, FastDump(data, offset)));
  }

  PluginAutoRegistrator registrator(Checking, Creating, Describing);
}
