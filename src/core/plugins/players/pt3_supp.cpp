/*
Abstract:
  PT3 modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "convert_helpers.h"
#include "utils.h"
#include "vortex_base.h"
#include "../detector.h"
#include "../enumerator.h"

#include <byteorder.h>
#include <error_tools.h>
#include <logging.h>
#include <messages_collector.h>
#include <tools.h>
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <core/error_codes.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <io/container.h>
#include <sound/render_params.h>
#include <core/devices/aym_parameters_helper.h>

#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <text/core.h>
#include <text/plugins.h>
#include <text/warnings.h>

#define FILE_TAG 3CBC0BBC

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  const Char PT3_PLUGIN_ID[] = {'P', 'T', '3', 0};
  const String TEXT_PT3_VERSION(FromStdString("$Rev$"));

  const std::size_t MAX_MODULE_SIZE = 16384;
  const uint_t MAX_PATTERNS_COUNT = 48;
  const uint_t MAX_PATTERN_SIZE = 64;
  const uint_t MAX_SAMPLES_COUNT = 32;
  const uint_t MAX_SAMPLE_SIZE = 64;
  const uint_t MAX_ORNAMENTS_COUNT = 16;
  const uint_t MAX_ORNAMENT_SIZE = 64;

  //checkers
  static const DetectFormatChain DETECTORS[] = {
    //PT3x
    {
      "21??"    // ld hl,xxxx
      "18?"     // jr xx
      "c3??"    // jp xxxx
      "c3+35+"  // jp xx:ds 33
      "f3"      // di
      "22??"    // ld (xxxx),hl
      "22??"
      "22??"
      "22??"
      "016400"  // ld bc,0x64
      "09"      // add hl,bc
      ,
      0xe21
    },
    //PT3.5x
    {
      "21??"    // ld hl,xxxx
      "18?"     // jr xx
      "c3??"    // jp xxxx
      "c3+37+"  // jp xx:ds 35
      "f3"      // di
      "ed73??"  // ld (xxxx),sp
      "22??"    // ld (xxxx),hl
      "22??"
      "22??"
      "22??"
      "016400"  // ld bc,0x64
      "09"      // add hl,bc
      ,
      0x30f
    },
    //Vortex1
    {
      "21??"    // ld hl,xxxx
      "18?"     // jr xx
      "c3??"    // jp xxxx
      "18+25+"  // jr xx:ds 24
      "21??"    // ld hl,xxxx
      "cbfe"    // set 7,(hl)
      "cb46"    // bit 0,(hl)
      "c8"      // ret z
      "e1"      // pop hl
      "21??"    // ld hl,xxxx
      "34"      // inc (hl)
      "21??"    // ld hl,xxxx
      "34"      // inc (hl)
      "af"      // xor a
      "67"      // ld h,a
      "6f"      // ld l,a
      "32??"    // ld (xxxx),a
      "22??"    // ld (xxxx),hl
      "c3"      // jp xxxx
      ,
      0x86e
    },
    //Vortex2
    {
      "21??"    // ld hl,xxxx
      "18?"     // jr xx
      "c3??"    // jp xxxx
      "18?"     // jr xx
      "?"       // db xx
      "f3"      // di
      "ed73??"  // ld (xxxx),sp
      "22??"    // ld (xxxx),hl
      "44"      // ld b,h
      "4d"      // ld c,l
      "11??"    // ld de,xxxx
      "19"      // add hl,de
      "7e"      // ld a,(hl)
      "23"      // inc hl
      "32??"    // ld (xxxx),a
      "f9"      // ld sp,hl
      "19"      // add hl,de
      "22??"    // ld (xxxx),hl
      "f1"      // pop af
      "5f"      // ld e,a
      "19"      // add hl,de
      "22??"    // ld (xxxx),hl
      "e1"      // pop hl
      "09"      // add hl,bc
      ,
      0xc00
    },
    //Another Vortex
    {
      "21??"    // ld hl,xxxx
      "18?"     // jr xx
      "c3??"    // jp xxxx
      "18?"     // jr xx
      "f3"      // di
      "ed73??"  // ld (xxxx),sp
      "22??"    // ld (xxxx),hl
      "44"      // ld b,h
      "4d"      // ld c,l
      "11??"    // ld de,xxxx
      "19"      // add hl,de
      "7e"      // ld a,(hl)
      "23"      // inc hl
      "32??"    // ld (xxxx),hl
      "f9"      // ld sp,hl
      "19"      // add hl,de
      "22??"    // ld (xxxx),hl
      "f1"      // pop af
      "5f"      // ld e,a
      "19"      // add hl,de
      "22??"    // ld (xxxx),hl
      "e1"      // pop hl
      "09"      // add hl,bc
      ,
      0xc89
    }
  };
  //////////////////////////////////////////////////////////////////////////
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct PT3Header
  {
    char Id[13];        //'ProTracker 3.'
    uint8_t Subversion;
    uint8_t Optional1[16]; //' compilation of '
    char TrackName[32];
    uint8_t Optional2[4]; //' by '
    char TrackAuthor[32];
    uint8_t Optional3;
    uint8_t FreqTableNum;
    uint8_t Tempo;
    uint8_t Lenght;
    uint8_t Loop;
    uint16_t PatternsOffset;
    boost::array<uint16_t, MAX_SAMPLES_COUNT> SamplesOffsets;
    boost::array<uint16_t, MAX_ORNAMENTS_COUNT> OrnamentsOffsets;
    uint8_t Positions[1]; //finished by marker
  } PACK_POST;

  PACK_PRE struct PT3Pattern
  {
    boost::array<uint16_t, 3> Offsets;

    bool Check() const
    {
      return Offsets[0] && Offsets[1] && Offsets[2];
    }
  } PACK_POST;

  PACK_PRE struct PT3Sample
  {
    uint8_t Loop;
    uint8_t Size;
    
    uint_t GetSize() const
    {
      return sizeof(*this) + (Size - 1) * sizeof(Data[0]);
    }
    
    PACK_PRE struct Line
    {
      //SUoooooE
      //NkKTLLLL
      //OOOOOOOO
      //OOOOOOOO
      // S - vol slide
      // U - vol slide up
      // o - noise or env offset
      // E - env mask
      // L - level
      // T - tone mask
      // K - keep noise or env offset
      // k - keep tone offset
      // N - noise mask
      // O - tone offset
      uint8_t VolSlideEnv;
      uint8_t LevelKeepers;
      int16_t ToneOffset;
      
      bool GetEnvelopeMask() const
      {
        return 0 != (VolSlideEnv & 1);
      }
      
      int_t GetNoiseOrEnvelopeOffset() const
      {
        const uint8_t noeoff = (VolSlideEnv & 62) >> 1;
        return static_cast<int8_t>(noeoff & 16 ? noeoff | 0xf0 : noeoff);
      }
      
      int_t GetVolSlide() const
      {
        return (VolSlideEnv & 128) ? ((VolSlideEnv & 64) ? +1 : -1) : 0;
      }
      
      uint_t GetLevel() const
      {
        return LevelKeepers & 15;
      }
      
      bool GetToneMask() const
      {
        return 0 != (LevelKeepers & 16);
      }
      
      bool GetKeepNoiseOrEnvelopeOffset() const
      {
        return 0 != (LevelKeepers & 32);
      }
      
      bool GetKeepToneOffset() const
      {
        return 0 != (LevelKeepers & 64);
      }
      
      bool GetNoiseMask() const
      {
        return 0 != (LevelKeepers & 128);
      }
      
      int_t GetToneOffset() const
      {
        return ToneOffset;
      }
    } PACK_POST;
    Line Data[1];
  } PACK_POST;

  PACK_PRE struct PT3Ornament
  {
    uint8_t Loop;
    uint8_t Size;
    int8_t Data[1];
    
    uint_t GetSize() const
    {
      return sizeof(*this) + (Size - 1) * sizeof(Data[0]);
    }
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(PT3Header) == 202);
  BOOST_STATIC_ASSERT(sizeof(PT3Pattern) == 6);
  BOOST_STATIC_ASSERT(sizeof(PT3Sample) == 6);
  BOOST_STATIC_ASSERT(sizeof(PT3Ornament) == 3);

  inline VortexSample::Line ParseSampleLine(const PT3Sample::Line& line)
  {
    VortexSample::Line res;
    res.Level = line.GetLevel();
    res.VolumeSlideAddon = line.GetVolSlide();
    res.ToneMask = line.GetToneMask();
    res.ToneOffset = line.GetToneOffset();
    res.KeepToneOffset = line.GetKeepToneOffset();
    res.NoiseMask = line.GetNoiseMask();
    res.EnvMask = line.GetEnvelopeMask();
    res.NoiseOrEnvelopeOffset = line.GetNoiseOrEnvelopeOffset();
    res.KeepNoiseOrEnvelopeOffset = line.GetKeepNoiseOrEnvelopeOffset();
    return res;
  }
  
  inline VortexSample ParseSample(const IO::FastDump& data, uint16_t offset, std::size_t& rawSize)
  {
    const uint_t off(fromLE(offset));
    const PT3Sample* const sample(safe_ptr_cast<const PT3Sample*>(&data[off]));
    if (0 == offset || !sample->Size)
    {
      return VortexSample(1, 0);//safe
    }
    VortexSample tmp(sample->Size, sample->Loop);
    std::transform(sample->Data, sample->Data + sample->Size, tmp.Data.begin(), ParseSampleLine);
    rawSize = std::max<std::size_t>(rawSize, off + sample->GetSize());
    return tmp;
  }

  inline SimpleOrnament ParseOrnament(const IO::FastDump& data, uint16_t offset, std::size_t& rawSize)
  {
    const uint_t off(fromLE(offset));
    const PT3Ornament* const ornament(safe_ptr_cast<const PT3Ornament*>(&data[off]));
    if (0 == offset || !ornament->Size)
    {
      return SimpleOrnament(1, 0);//safe version
    }
    rawSize = std::max(rawSize, off + ornament->GetSize());
    return SimpleOrnament(ornament->Data, ornament->Data + ornament->Size, ornament->Loop);
  }
  
  void DescribePT3Plugin(PluginInformation& info)
  {
    info.Id = PT3_PLUGIN_ID;
    info.Description = TEXT_PT3_INFO;
    info.Version = TEXT_PT3_VERSION;
    info.Capabilities = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | CAP_CONV_PSG;
  }

  class PT3Holder : public Holder, public boost::enable_shared_from_this<PT3Holder>
  {
    typedef boost::array<PatternCursor, AYM::CHANNELS> PatternCursors;
    
    void ParsePattern(const IO::FastDump& data
      , PatternCursors& cursors
      , VortexTrack::Line& line
      , Log::MessagesCollector& warner
      , uint_t& noiseBase
      )
    {
      bool wasEnvelope(false), wasNoisebase(false);
      assert(line.Channels.size() == cursors.size());
      VortexTrack::Line::ChannelsArray::iterator channel(line.Channels.begin());
      for (PatternCursors::iterator cur = cursors.begin(); cur != cursors.end(); ++cur, ++channel)
      {
        if (cur->Counter--)
        {
          continue;//has to skip
        }

        Log::ParamPrefixedCollector channelWarner(warner, TEXT_CHANNEL_WARN_PREFIX, std::distance(line.Channels.begin(), channel));
        for (;;)
        {
          const uint_t cmd(data[cur->Offset++]);
          const std::size_t restbytes = data.Size() - cur->Offset;
          if (cmd == 1)//gliss
          {
            channel->Commands.push_back(VortexTrack::Command(GLISS));
          }
          else if (cmd == 2)//portamento
          {
            channel->Commands.push_back(VortexTrack::Command(GLISS_NOTE));
          }
          else if (cmd == 3)//sample offset
          {
            channel->Commands.push_back(VortexTrack::Command(SAMPLEOFFSET, -1));
          }
          else if (cmd == 4)//ornament offset
          {
            channel->Commands.push_back(VortexTrack::Command(ORNAMENTOFFSET));
          }
          else if (cmd == 5)//vibrate
          {
            channel->Commands.push_back(VortexTrack::Command(VIBRATE));
          }
          else if (cmd == 8)//slide envelope
          {
            channel->Commands.push_back(VortexTrack::Command(SLIDEENV));
          }
          else if (cmd == 9)//tempo
          {
            channel->Commands.push_back(VortexTrack::Command(TEMPO));
          }
          else if ((cmd >= 0x10 && cmd <= 0x1f) || 
                   (cmd >= 0xb2 && cmd <= 0xbf) ||
                    cmd >= 0xf0)
          {
            const bool hasEnv(cmd >= 0x11 && cmd <= 0xbf);
            const bool hasOrn(cmd >= 0xf0);
            const bool hasSmp(cmd < 0xb2 || cmd > 0xbf);
            
            if (restbytes < std::size_t(2 * hasEnv + hasSmp))
            {
              throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
            }
            
            if (hasEnv) //has envelope command
            {
              const uint_t envPeriod(data[cur->Offset + 1] + (uint_t(data[cur->Offset]) << 8));
              cur->Offset += 2;
              Log::Assert(channelWarner, !wasEnvelope, TEXT_WARNING_DUPLICATE_ENVELOPE);
              channel->Commands.push_back(VortexTrack::Command(ENVELOPE, cmd - (cmd >= 0xb2 ? 0xb1 : 0x10), envPeriod));
              wasEnvelope = true;
            }
            else
            {
              channel->Commands.push_back(VortexTrack::Command(NOENVELOPE));
            }
           
            if (hasOrn) //has ornament command
            {
              const uint_t num(cmd - 0xf0);
              Log::Assert(channelWarner, !(num && Data.Ornaments[num].Data.empty()), TEXT_WARNING_INVALID_ORNAMENT);
              Log::Assert(channelWarner, !channel->OrnamentNum, TEXT_WARNING_DUPLICATE_ORNAMENT);
              channel->OrnamentNum = num;
            }
            
            if (hasSmp)
            {
              const uint_t doubleSampNum(data[cur->Offset++]);
              const bool sampValid(doubleSampNum < MAX_SAMPLES_COUNT * 2 && 0 == (doubleSampNum & 1));
              Log::Assert(channelWarner, sampValid, TEXT_WARNING_INVALID_SAMPLE);
              Log::Assert(channelWarner, !channel->SampleNum, TEXT_WARNING_DUPLICATE_SAMPLE);
              channel->SampleNum = sampValid ? (doubleSampNum / 2) : 0;
            }
          }
          else if (cmd >= 0x20 && cmd <= 0x3f)
          {
            //noisebase should be in channel B
            //Log::Assert(channelWarner, chan == 1, TEXT_WARNING_INVALID_NOISE_BASE);
            Log::Assert(channelWarner, !wasNoisebase, TEXT_WARNING_DUPLICATE_NOISEBASE);
            noiseBase = cmd - 0x20;
            wasNoisebase = true;
          }
          else if (cmd >= 0x40 && cmd <= 0x4f)
          {
            const uint_t num(cmd - 0x40);
            Log::Assert(channelWarner, !(num && Data.Ornaments[num].Data.empty()), TEXT_WARNING_INVALID_ORNAMENT);
            Log::Assert(channelWarner, !channel->OrnamentNum, TEXT_WARNING_DUPLICATE_ORNAMENT);
            channel->OrnamentNum = num;
          }
          else if (cmd >= 0x50 && cmd <= 0xaf)
          {
            const uint_t note(cmd - 0x50);
            VortexTrack::CommandsArray::iterator it(std::find(channel->Commands.begin(), channel->Commands.end(), GLISS_NOTE));
            if (channel->Commands.end() != it)
            {
              it->Param3 = note;
            }
            else
            {
              Log::Assert(channelWarner, !channel->Note, TEXT_WARNING_DUPLICATE_NOTE);
              channel->Note = note;
            }
            Log::Assert(channelWarner, !channel->Enabled, TEXT_WARNING_DUPLICATE_STATE);
            channel->Enabled = true;
            break;
          }
          else if (cmd == 0xb0)
          {
            channel->Commands.push_back(VortexTrack::Command(NOENVELOPE));
          }
          else if (cmd == 0xb1)
          {
            if (restbytes < 1)
            {
              throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
            }
            cur->Period = data[cur->Offset++] - 1;
          }
          else if (cmd == 0xc0)
          {
            Log::Assert(channelWarner, !channel->Enabled, TEXT_WARNING_DUPLICATE_STATE);
            channel->Enabled = false;
            break;
          }
          else if (cmd >= 0xc1 && cmd <= 0xcf)
          {
            Log::Assert(channelWarner, !channel->Volume, TEXT_WARNING_DUPLICATE_VOLUME);
            channel->Volume = cmd - 0xc0;
          }
          else if (cmd == 0xd0)
          {
            break;
          }
          else if (cmd >= 0xd1 && cmd <= 0xef)
          {
            //TODO: check for empty sample
            Log::Assert(channelWarner, !channel->SampleNum, TEXT_WARNING_DUPLICATE_SAMPLE);
            channel->SampleNum = cmd - 0xd0;
          }
        }
        //parse parameters
        const std::size_t restbytes = data.Size() - cur->Offset;
        for (VortexTrack::CommandsArray::reverse_iterator it = channel->Commands.rbegin(), lim = channel->Commands.rend();
          it != lim; ++it)
        {
          switch (it->Type)
          {
          case TEMPO:
            if (restbytes < 1)
            {
              throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
            }
            Log::Assert(channelWarner, !line.Tempo, TEXT_WARNING_DUPLICATE_TEMPO);
            line.Tempo = data[cur->Offset++];
            break;
          case SLIDEENV:
          case GLISS:
            if (restbytes < 3)
            {
              throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
            }
            it->Param1 = data[cur->Offset++];
            it->Param2 = static_cast<int16_t>(256 * data[cur->Offset + 1] + data[cur->Offset]);
            cur->Offset += 2;
            break;
          case VIBRATE:
            if (restbytes < 2)
            {
              throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
            }
            it->Param1 = data[cur->Offset++];
            it->Param2 = data[cur->Offset++];
            break;
          case ORNAMENTOFFSET:
          {
            if (restbytes < 1)
            {
              throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
            }
            const uint_t offset(data[cur->Offset++]);
            const bool isValid(offset < (channel->OrnamentNum ?
              Data.Ornaments[*channel->OrnamentNum].Data.size() : MAX_ORNAMENT_SIZE));
            Log::Assert(channelWarner, isValid, TEXT_WARNING_INVALID_ORNAMENT_OFFSET);
            it->Param1 = isValid ? offset : 0;
            break;
          }
          case SAMPLEOFFSET:
            if (-1 == it->Param1)
            {
              if (restbytes < 1)
              {
                throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
              }
              const uint_t offset(data[cur->Offset++]);
              const bool isValid(offset < (channel->SampleNum ?
                Data.Samples[*channel->SampleNum].Data.size() : MAX_SAMPLE_SIZE));
              Log::Assert(channelWarner, isValid, TEXT_WARNING_INVALID_SAMPLE_OFFSET);
              it->Param1 = isValid ? offset : 0;
            }
            break;
          case GLISS_NOTE:
            if (restbytes < 5)
            {
              throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
            }
            it->Param1 = data[cur->Offset++];
            //skip limiter
            cur->Offset += 2;
            it->Param2 = static_cast<int16_t>(256 * data[cur->Offset + 1] + data[cur->Offset]);
            cur->Offset += 2;
            break;
          }
        }
        cur->Counter = cur->Period;
      }
      if (noiseBase)
      {
        //place to channel B
        line.Channels[1].Commands.push_back(VortexTrack::Command(NOISEBASE, noiseBase));
      }
    }
  public:
    PT3Holder(const MetaContainer& container, ModuleRegion& region)
    {
      //assume all data is correct
      const IO::FastDump& data = IO::FastDump(*container.Data, region.Offset);
      const PT3Header* const header(safe_ptr_cast<const PT3Header*>(&data[0]));

      Log::MessagesCollector::Ptr warner(Log::MessagesCollector::Create());

      std::size_t rawSize(0);
      //fill samples
      Data.Samples.reserve(header->SamplesOffsets.size());
      std::transform(header->SamplesOffsets.begin(), header->SamplesOffsets.end(),
        std::back_inserter(Data.Samples), boost::bind(&ParseSample, boost::cref(data), _1, boost::ref(rawSize)));
      //fill ornaments
      Data.Ornaments.reserve(header->OrnamentsOffsets.size());
      std::transform(header->OrnamentsOffsets.begin(), header->OrnamentsOffsets.end(),
        std::back_inserter(Data.Ornaments), boost::bind(&ParseOrnament, boost::cref(data), _1, boost::ref(rawSize)));

      //fill patterns
      Data.Patterns.resize(MAX_PATTERNS_COUNT);
      uint_t index(0);
      Data.Patterns.resize(1 + *std::max_element(header->Positions, header->Positions + header->Lenght) / 3);
      const PT3Pattern* patterns(safe_ptr_cast<const PT3Pattern*>(&data[fromLE(header->PatternsOffset)]));
      for (const PT3Pattern* pattern = patterns; pattern->Check() && index < Data.Patterns.size(); ++pattern, ++index)
      {
        Log::ParamPrefixedCollector patternWarner(*warner, TEXT_PATTERN_WARN_PREFIX, index);
        VortexTrack::Pattern& pat(Data.Patterns[index]);
        
        PatternCursors cursors;
        std::transform(pattern->Offsets.begin(), pattern->Offsets.end(), cursors.begin(), &fromLE<uint16_t>);
        pat.reserve(MAX_PATTERN_SIZE);
        uint_t noiseBase(0);
        do
        {
          Log::ParamPrefixedCollector patLineWarner(patternWarner, TEXT_LINE_WARN_PREFIX, pat.size());
          pat.push_back(VortexTrack::Line());
          VortexTrack::Line& line(pat.back());
          ParsePattern(data, cursors, line, patLineWarner, noiseBase);
          //skip lines
          if (const uint_t linesToSkip = std::min_element(cursors.begin(), cursors.end(), PatternCursor::CompareByCounter)->Counter)
          {
            std::for_each(cursors.begin(), cursors.end(), std::bind2nd(std::mem_fun_ref(&PatternCursor::SkipLines), linesToSkip));
            pat.resize(pat.size() + linesToSkip);//add dummies
          }
        }
        while (data[cursors.front().Offset] || cursors.front().Counter);
        //as warnings
        Log::Assert(patternWarner, 0 == std::max_element(cursors.begin(), cursors.end(), PatternCursor::CompareByCounter)->Counter,
          TEXT_WARNING_PERIODS);
        Log::Assert(patternWarner, pat.size() <= MAX_PATTERN_SIZE, TEXT_WARNING_INVALID_PATTERN_SIZE);
        rawSize = std::max(rawSize, std::max_element(cursors.begin(), cursors.end(), PatternCursor::CompareByOffset)->Offset + 1);
      }
      //fill order
      for (const uint8_t* curPos = header->Positions; curPos != header->Positions + header->Lenght; ++curPos)
      {
        if (0 == *curPos % 3 && !Data.Patterns[*curPos / 3].empty())
        {
          Data.Positions.push_back(*curPos / 3);
        }
      }
      Log::Assert(*warner, header->Lenght == Data.Positions.size(), TEXT_WARNING_INVALID_LENGTH);

      //fix samples and ornaments
      std::for_each(Data.Ornaments.begin(), Data.Ornaments.end(), std::mem_fun_ref(&VortexTrack::Ornament::Fix));
      std::for_each(Data.Samples.begin(), Data.Samples.end(), std::mem_fun_ref(&VortexTrack::Sample::Fix));
      
      //fill region
      region.Size = rawSize;
      
      //meta properties
      ExtractMetaProperties(PT3_PLUGIN_ID, container, region, Data.Info.Properties, RawData);
      const String& title(OptimizeString(FromStdString(header->TrackName)));
      if (!title.empty())
      {
        Data.Info.Properties.insert(Parameters::Map::value_type(Module::ATTR_TITLE, title));
      }
      const String& author(OptimizeString(FromStdString(header->TrackAuthor)));
      if (!author.empty())
      {
        Data.Info.Properties.insert(Parameters::Map::value_type(Module::ATTR_AUTHOR, author));
      }
      const String& prog(OptimizeString(FromStdString(header->Id)));
      if (!prog.empty())
      {
        Data.Info.Properties.insert(Parameters::Map::value_type(Module::ATTR_PROGRAM, prog.substr(0, prog.find(Char(' ')))));
      }
      
      //tracking properties
      Version = std::isdigit(header->Subversion) ? header->Subversion - '0' : 6;
      switch (header->FreqTableNum)
      {
      case 0://PT
        FreqTableName = Version <= 3 ? TABLE_PROTRACKER3_3 : TABLE_PROTRACKER3_4;
        break;
      case 1://ST
        FreqTableName = TABLE_SOUNDTRACKER;
        break;
      case 2://ASM
        FreqTableName = Version <= 3 ? TABLE_PROTRACKER3_3_ASM : TABLE_PROTRACKER3_4_ASM;
        break;
      default:
        FreqTableName = Version <= 3 ? TABLE_PROTRACKER3_3_REAL : TABLE_PROTRACKER3_4_REAL;
      }
      
      Data.Info.LoopPosition = header->Loop;
      Data.Info.PhysicalChannels = AYM::CHANNELS;
      Data.Info.Statistic.Tempo = header->Tempo;
      Data.Info.Statistic.Position = Data.Positions.size();
      Data.Info.Statistic.Pattern = std::count_if(Data.Patterns.begin(), Data.Patterns.end(),
        !boost::bind(&VortexTrack::Pattern::empty, _1));
      Data.Info.Statistic.Channels = AYM::CHANNELS;
      VortexTrack::CalculateTimings(Data, Data.Info.Statistic.Frame, Data.Info.LoopFrame);
      if (const uint_t msgs = warner->CountMessages())
      {
        Data.Info.Properties.insert(Parameters::Map::value_type(Module::ATTR_WARNINGS_COUNT, msgs));
        Data.Info.Properties.insert(Parameters::Map::value_type(Module::ATTR_WARNINGS, warner->GetMessages('\n')));
      }
    }
    virtual void GetPluginInformation(PluginInformation& info) const
    {
      DescribePT3Plugin(info);
    }

    virtual void GetModuleInformation(Information& info) const
    {
      info = Data.Info;
    }
    
    virtual void ModifyCustomAttributes(const Parameters::Map& attrs, bool replaceExisting)
    {
      return Parameters::MergeMaps(Data.Info.Properties, attrs, Data.Info.Properties, replaceExisting);
    }

    virtual Player::Ptr CreatePlayer() const
    {
      return CreateVortexPlayer(shared_from_this(), Data, Version, FreqTableName, AYM::CreateChip());
    }
    
    virtual Error Convert(const Conversion::Parameter& param, Dump& dst) const
    {
      using namespace Conversion;
      Error result;
      if (parameter_cast<RawConvertParam>(&param))
      {
        dst = RawData;
      }
      else if (!ConvertAYMFormat(boost::bind(&CreateVortexPlayer, shared_from_this(), boost::cref(Data), Version, FreqTableName, _1),
        param, dst, result))
      {
        return Error(THIS_LINE, ERROR_MODULE_CONVERT, TEXT_MODULE_ERROR_CONVERSION_UNSUPPORTED);
      }
      return result;
    }
  private:
    Dump RawData;
    VortexTrack::ModuleData Data;
    uint_t Version;
    String FreqTableName;
  };

  //////////////////////////////////////////////////
  bool Check(const uint8_t* data, std::size_t size, const MetaContainer& container,
    Holder::Ptr& holder, ModuleRegion& region)
  {
    const PT3Header* const header(safe_ptr_cast<const PT3Header*>(data));
    const std::size_t patOff(fromLE(header->PatternsOffset));
    if (!header->Lenght ||
      patOff >= size ||
      0xff != data[patOff - 1] ||
      &data[patOff - 1] != std::find_if(header->Positions, data + patOff - 1,
      std::bind2nd(std::modulus<uint8_t>(), 3)) ||
      &header->Positions[header->Lenght] != data + patOff - 1 ||
      fromLE(header->OrnamentsOffsets[0]) + sizeof(PT3Ornament) > size
      )
    {
      return false;
    }
    //TODO
    //try to create holder
    try
    {
      holder.reset(new PT3Holder(container, region));
#ifdef SELF_TEST
      holder->CreatePlayer();
#endif
      return true;
    }
    catch (const Error&/*e*/)
    {
      Log::Debug("PT3Supp", "Failed to create holder");
    }
    return false;
  }

  //////////////////////////////////////////////////////////////////////////
  bool CreatePT3Module(const Parameters::Map& /*commonParams*/, const MetaContainer& container,
    Holder::Ptr& holder, ModuleRegion& region)
  {
    const std::size_t limit(std::min<std::size_t>(container.Data->Size(), MAX_MODULE_SIZE));
    const uint8_t* const data(static_cast<const uint8_t*>(container.Data->Data()));

    ModuleRegion tmpRegion;
    //try to detect without player
    if (Check(data, limit, container, holder, tmpRegion))
    {
      region = tmpRegion;
      return true;
    }
    for (const DetectFormatChain* chain = DETECTORS; chain != ArrayEnd(DETECTORS); ++chain)
    {
      tmpRegion.Offset = chain->PlayerSize;
      if (DetectFormat(data, limit, chain->PlayerFP) &&
          Check(data + chain->PlayerSize, limit - region.Offset, container, holder, tmpRegion))
      {
        region = tmpRegion;
        return true;
      }
    }
    return false;
  }
}

namespace ZXTune
{
  void RegisterPT3Support(PluginsEnumerator& enumerator)
  {
    PluginInformation info;
    DescribePT3Plugin(info);
    enumerator.RegisterPlayerPlugin(info, CreatePT3Module);
  }
}
