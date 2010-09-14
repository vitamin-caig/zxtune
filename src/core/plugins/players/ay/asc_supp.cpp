/*
Abstract:
  ASC modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ay_base.h"
#include "ay_conversion.h"
#include <core/plugins/detect_helper.h>
#include <core/plugins/utils.h>
#include <core/plugins/players/module_properties.h>
//common includes
#include <byteorder.h>
#include <error_tools.h>
#include <logging.h>
#include <messages_collector.h>
#include <range_checker.h>
#include <tools.h>
//library includes
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <io/container.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
//text includes
#include <core/text/plugins.h>
#include <core/text/warnings.h>

#define FILE_TAG 45B26E38

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char ASC_PLUGIN_ID[] = {'A', 'S', 'C', 0};
  const String ASC_PLUGIN_VERSION(FromStdString("$Rev$"));

  const uint_t LIMITER(~uint_t(0));

  //hints
  const std::size_t MAX_MODULE_SIZE = 16384;
  const uint_t SAMPLES_COUNT = 32;
  const uint_t MAX_SAMPLE_SIZE = 150;
  const uint_t ORNAMENTS_COUNT = 32;
  const uint_t MAX_ORNAMENT_SIZE = 30;
  const uint_t MAX_PATTERN_SIZE = 64;//???
  const uint_t MAX_PATTERNS_COUNT = 32;//TODO

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
    uint8_t Length;
    uint8_t Positions[1];
  } PACK_POST;

  const uint8_t ASC_ID_1[] =
  {
    'A', 'S', 'M', ' ', 'C', 'O', 'M', 'P', 'I', 'L', 'A', 'T', 'I', 'O', 'N', ' ', 'O', 'F', ' '
  };
  const uint8_t ASC_ID_2[] =
  {
    ' ', 'B', 'Y', ' '
  };

  PACK_PRE struct ASCID
  {
    uint8_t Identifier1[19];//'ASM COMPILATION OF '
    char Title[20];
    uint8_t Identifier2[4];//' BY '
    char Author[20];

    bool Check() const
    {
      BOOST_STATIC_ASSERT(sizeof(ASC_ID_1) == sizeof(Identifier1));
      BOOST_STATIC_ASSERT(sizeof(ASC_ID_2) == sizeof(Identifier2));
      return 0 == std::memcmp(Identifier1, ASC_ID_1, sizeof(Identifier1)) &&
             0 == std::memcmp(Identifier2, ASC_ID_2, sizeof(Identifier2));
    }
  } PACK_POST;

  PACK_PRE struct ASCPattern
  {
    boost::array<uint16_t, 3> Offsets;//from start of patterns
  } PACK_POST;

  PACK_PRE struct ASCOrnaments
  {
    boost::array<uint16_t, ORNAMENTS_COUNT> Offsets;
  } PACK_POST;

  PACK_PRE struct ASCOrnament
  {
    PACK_PRE struct Line
    {
      //BEFooooo
      //OOOOOOOO

      //o - noise offset (signed)
      //B - loop begin
      //E - loop end
      //F - finished
      //O - note offset (signed)
      uint8_t LoopAndNoiseOffset;
      int8_t NoteOffset;

      bool IsLoopBegin() const
      {
        return 0 != (LoopAndNoiseOffset & 128);
      }

      bool IsLoopEnd() const
      {
        return 0 != (LoopAndNoiseOffset & 64);
      }

      bool IsFinished() const
      {
        return 0 != (LoopAndNoiseOffset & 32);
      }

      int_t GetNoiseOffset() const
      {
        return static_cast<int8_t>(LoopAndNoiseOffset * 8) / 8;
      }
    } PACK_POST;
    Line Data[1];
  } PACK_POST;

  PACK_PRE struct ASCSamples
  {
    boost::array<uint16_t, SAMPLES_COUNT> Offsets;
  } PACK_POST;

  PACK_PRE struct ASCSample
  {
    PACK_PRE struct Line
    {
      //BEFaaaaa
      //TTTTTTTT
      //LLLLnCCt
      //a - adding
      //B - loop begin
      //E - loop end
      //F - finished
      //T - tone deviation
      //L - level
      //n - noise mask
      //C - command
      //t - tone mask

      uint8_t LoopAndAdding;
      int8_t ToneDeviation;
      uint8_t LevelAndMasks;

      bool IsLoopBegin() const
      {
        return 0 != (LoopAndAdding & 128);
      }

      bool IsLoopEnd() const
      {
        return 0 != (LoopAndAdding & 64);
      }

      bool IsFinished() const
      {
        return 0 != (LoopAndAdding & 32);
      }

      int_t GetAdding() const
      {
        return static_cast<int8_t>(LoopAndAdding * 8) / 8;
      }

      uint_t GetLevel() const
      {
        return LevelAndMasks >> 4;
      }

      bool GetNoiseMask() const
      {
        return 0 != (LevelAndMasks & 8);
      }

      uint_t GetCommand() const
      {
        return (LevelAndMasks & 6) >> 1;
      }

      bool GetToneMask() const
      {
        return 0 != (LevelAndMasks & 1);
      }
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
      : Loop(), LoopLimit(), Lines()
    {
      Lines.reserve(MAX_SAMPLE_SIZE);
      for (uint_t sline = 0; sline != MAX_SAMPLE_SIZE; ++sline)
      {
        const ASCSample::Line& line = sample.Data[sline];
        Lines.push_back(Line(line));
        if (line.IsLoopBegin())
        {
          Loop = sline;
        }
        if (line.IsLoopEnd())
        {
          LoopLimit = sline;
        }
        if (line.IsFinished())
        {
          break;
        }
      }
    }

    struct Line
    {
      Line()
        : Level(), ToneDeviation(), ToneMask(), NoiseMask(), Adding(), Command()
      {
      }

      explicit Line(const ASCSample::Line& line)
        : Level(line.GetLevel()), ToneDeviation(line.ToneDeviation)
        , ToneMask(line.GetToneMask()), NoiseMask(line.GetNoiseMask())
        , Adding(line.GetAdding()), Command(line.GetCommand())
      {
      }
      uint_t Level;//0-15
      int_t ToneDeviation;
      bool ToneMask;
      bool NoiseMask;
      int_t Adding;
      uint_t Command;
    };

    uint_t GetLoop() const
    {
      return Loop;
    }

    uint_t GetLoopLimit() const
    {
      return LoopLimit;
    }

    uint_t GetSize() const
    {
      return Lines.size();
    }

    const Line& GetLine(uint_t idx) const
    {
      static const Line STUB;
      return Lines.size() > idx ? Lines[idx] : STUB;
    }
  private:
    uint_t Loop;
    uint_t LoopLimit;
    std::vector<Line> Lines;
  };

  struct Ornament
  {
    explicit Ornament(const ASCOrnament& ornament)
      : Loop(), LoopLimit(), Lines()
    {
      Lines.reserve(MAX_ORNAMENT_SIZE);
      for (uint_t sline = 0; sline != MAX_ORNAMENT_SIZE; ++sline)
      {
        const ASCOrnament::Line& line = ornament.Data[sline];
        Lines.push_back(Line(line));
        if (line.IsLoopBegin())
        {
          Loop = sline;
        }
        if (line.IsLoopEnd())
        {
          LoopLimit = sline;
        }
        if (line.IsFinished())
        {
          break;
        }
      }
    }

    struct Line
    {
      Line()
        : NoteAddon(), NoiseAddon()
      {
      }

      explicit Line(const ASCOrnament::Line& line)
        : NoteAddon(line.NoteOffset)
        , NoiseAddon(line.GetNoiseOffset())
      {
      }
      int_t NoteAddon;
      int_t NoiseAddon;
    };

    uint_t GetLoop() const
    {
      return Loop;
    }

    uint_t GetLoopLimit() const
    {
      return LoopLimit;
    }

    uint_t GetSize() const
    {
      return Lines.size();
    }

    const Line& GetLine(uint_t idx) const
    {
      static const Line STUB;
      return Lines.size() > idx ? Lines[idx] : STUB;
    }
  private:
    uint_t Loop;
    uint_t LoopLimit;
    std::vector<Line> Lines;
  };

  //supported commands and their parameters
  enum CmdType
  {
    //no parameters
    EMPTY,
    //r13,envL
    ENVELOPE,
    //no parameters
    ENVELOPE_ON,
    //no parameters
    ENVELOPE_OFF,
    //value
    NOISE,
    //no parameters
    CONT_SAMPLE,
    //no parameters
    CONT_ORNAMENT,
    //glissade value
    GLISS,
    //steps
    SLIDE,
    //steps,note
    SLIDE_NOTE,
    //period,delta
    AMPLITUDE_SLIDE,
    //no parameter
    BREAK_SAMPLE
  };

  // tracker type
  typedef TrackingSupport<AYM::CHANNELS, Sample, Ornament> ASCTrack;

  Player::Ptr CreateASCPlayer(Information::Ptr info, ASCTrack::ModuleData::Ptr data, AYM::Chip::Ptr device);

  class ASCHolder : public Holder
  {
    typedef boost::array<PatternCursor, AYM::CHANNELS> PatternCursors;

    static void ParsePattern(const IO::FastDump& data
      , PatternCursors& cursors
      , ASCTrack::Line& line
      , Log::MessagesCollector& warner
      , uint_t& envelopes
      )
    {
      assert(line.Channels.size() == cursors.size());
      ASCTrack::Line::ChannelsArray::iterator channel = line.Channels.begin();
      uint_t envMask = 1;
      for (PatternCursors::iterator cur = cursors.begin(); cur != cursors.end(); ++cur, ++channel, envMask <<= 1)
      {
        if (cur->Counter--)
        {
          continue;//has to skip
        }

        const uint_t idx = std::distance(line.Channels.begin(), channel);
        Log::ParamPrefixedCollector channelWarner(warner, Text::CHANNEL_WARN_PREFIX, idx);
        bool continueSample = false;
        for (;;)
        {
          const uint_t cmd = data[cur->Offset++];
          const std::size_t restbytes = data.Size() - cur->Offset;
          if (cmd <= 0x55)//note
          {
            if (!continueSample)
            {
              Log::Assert(channelWarner, !channel->Enabled, Text::WARNING_DUPLICATE_STATE);
              channel->Enabled = true;
            }
            if (!channel->Commands.empty() &&
                SLIDE == channel->Commands.back().Type)
            {
              //set slide to note
              ASCTrack::Command& command = channel->Commands.back();
              command.Type = SLIDE_NOTE;
              command.Param2 = cmd;
            }
            else
            {
              Log::Assert(channelWarner, !channel->Note, Text::WARNING_DUPLICATE_NOTE);
              channel->Note = cmd;
            }
            if (envelopes & envMask)
            {
              //modify existing
              ASCTrack::CommandsArray::iterator cmdIt = std::find(channel->Commands.begin(),
                channel->Commands.end(), ENVELOPE);
              if (restbytes < 1)
              {
                throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
              }
              const uint_t param = data[cur->Offset++];
              if (channel->Commands.end() == cmdIt)
              {
                channel->Commands.push_back(ASCTrack::Command(ENVELOPE, -1, param));
              }
              else
              {
                cmdIt->Param2 = param;
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
            channel->Commands.push_back(ASCTrack::Command(BREAK_SAMPLE));
            break;
          }
          else if (cmd == 0x5f) //shut
          {
            Log::Assert(channelWarner, !channel->Enabled, Text::WARNING_DUPLICATE_STATE);
            channel->Enabled = false;
            break;
          }
          else if (cmd >= 0x60 && cmd <= 0x9f) //skip
          {
            cur->Period = cmd - 0x60;
          }
          else if (cmd >= 0xa0 && cmd <= 0xbf) //sample
          {
            Log::Assert(channelWarner, !channel->SampleNum, Text::WARNING_DUPLICATE_SAMPLE);
            channel->SampleNum = cmd - 0xa0;
          }
          else if (cmd >= 0xc0 && cmd <= 0xdf) //ornament
          {
            Log::Assert(channelWarner, !channel->OrnamentNum, Text::WARNING_DUPLICATE_ORNAMENT);
            channel->OrnamentNum = cmd - 0xc0;
          }
          else if (cmd == 0xe0) // envelope full vol
          {
            Log::Assert(channelWarner, !channel->Volume, Text::WARNING_DUPLICATE_VOLUME);
            channel->Volume = 15;
            channel->Commands.push_back(ASCTrack::Command(ENVELOPE_ON));
            envelopes |= envMask;
          }
          else if (cmd >= 0xe1 && cmd <= 0xef) // noenvelope vol
          {
            Log::Assert(channelWarner, !channel->Volume, Text::WARNING_DUPLICATE_VOLUME);
            channel->Volume = cmd - 0xe0;
            channel->Commands.push_back(ASCTrack::Command(ENVELOPE_OFF));
            envelopes &= ~envMask;
          }
          else if (cmd == 0xf0) //initial noise
          {
            if (restbytes < 1)
            {
              throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
            }
            channel->Commands.push_back(ASCTrack::Command(NOISE, data[cur->Offset++]));
          }
          else if ((cmd & 0xfc) == 0xf0) //0xf1, 0xf2, 0xf3- continue sample or ornament
          {
            if (cmd & 1)
            {
              continueSample = true;
              channel->Commands.push_back(ASCTrack::Command(CONT_SAMPLE));
            }
            if (cmd & 2)
            {
              channel->Commands.push_back(ASCTrack::Command(CONT_ORNAMENT));
            }
          }
          else if (cmd == 0xf4) //tempo
          {
            if (restbytes < 1)
            {
              throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
            }
            Log::Assert(channelWarner, !line.Tempo, Text::WARNING_DUPLICATE_TEMPO);
            line.Tempo = data[cur->Offset++];
          }
          else if (cmd == 0xf5 || cmd == 0xf6) //slide
          {
            if (restbytes < 1)
            {
              throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
            }
            channel->Commands.push_back(ASCTrack::Command(GLISS,
              ((cmd == 0xf5) ? -16 : 16) * static_cast<int8_t>(data[cur->Offset++])));
          }
          else if (cmd == 0xf7 || cmd == 0xf9) //stepped slide
          {
            if (cmd == 0xf7)
            {
              channel->Commands.push_back(ASCTrack::Command(CONT_SAMPLE));
            }
            if (restbytes < 1)
            {
              throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
            }
            channel->Commands.push_back(ASCTrack::Command(SLIDE, static_cast<int8_t>(data[cur->Offset++])));
          }
          else if ((cmd & 0xf9) == 0xf8) //0xf8, 0xfa, 0xfc, 0xfe- envelope
          {
            ASCTrack::CommandsArray::iterator cmdIt = std::find(channel->Commands.begin(), channel->Commands.end(),
              ENVELOPE);
            if (channel->Commands.end() == cmdIt)
            {
              channel->Commands.push_back(ASCTrack::Command(ENVELOPE, cmd & 0xf, -1));
            }
            else
            {
              //strange situation...
              channelWarner.AddMessage(Text::WARNING_DUPLICATE_ENVELOPE);
              cmdIt->Param1 = cmd & 0xf;
            }
          }
          else if (cmd == 0xfb) //amplitude delay
          {
            if (restbytes < 1)
            {
              throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
            }
            const uint_t step = data[cur->Offset++];
            channel->Commands.push_back(ASCTrack::Command(AMPLITUDE_SLIDE, step & 31, step & 32 ? -1 : 1));
          }
        }
        cur->Counter = cur->Period;
      }
    }
  public:
    ASCHolder(Plugin::Ptr plugin, const MetaContainer& container, ModuleRegion& region)
      : SrcPlugin(plugin)
      , Data(ASCTrack::ModuleData::Create())
      , Info(ASCTrack::ModuleInfo::Create(Data))
    {
      //assume all data is correct
      const IO::FastDump& data = IO::FastDump(*container.Data, region.Offset);

      const ASCHeader* const header = safe_ptr_cast<const ASCHeader*>(&data[0]);

      Log::MessagesCollector::Ptr warner(Log::MessagesCollector::Create());

      std::size_t rawSize = 0;
      //parse samples
      {
        const std::size_t samplesOff = fromLE(header->SamplesOffset);
        const ASCSamples* const samples = safe_ptr_cast<const ASCSamples*>(&data[samplesOff]);
        Data->Samples.reserve(samples->Offsets.size());
        uint_t index = 0;
        for (const uint16_t* pSample = samples->Offsets.begin(); pSample != samples->Offsets.end();
          ++pSample, ++index)
        {
          assert(*pSample && fromLE(*pSample) < data.Size());
          const std::size_t sampleOffset = fromLE(*pSample);
          const ASCSample* const sample = safe_ptr_cast<const ASCSample*>(&data[samplesOff + sampleOffset]);
          Data->Samples.push_back(Sample(*sample));
          const Sample& smp = Data->Samples.back();
          if (smp.GetLoop() > smp.GetLoopLimit() || smp.GetLoopLimit() >= smp.GetSize())
          {
            Log::ParamPrefixedCollector lineWarner(*warner, Text::SAMPLE_WARN_PREFIX, index);
            Log::Assert(lineWarner, smp.GetLoop() <= smp.GetLoopLimit(), Text::WARNING_LOOP_OUT_LIMIT);
            Log::Assert(lineWarner, smp.GetLoopLimit() < smp.GetSize(), Text::WARNING_LOOP_OUT_BOUND);
          }
          rawSize = std::max(rawSize, samplesOff + sampleOffset + smp.GetSize() * sizeof(ASCSample::Line));
        }
      }

      //parse ornaments
      {
        const std::size_t ornamentsOff = fromLE(header->OrnamentsOffset);
        const ASCOrnaments* const ornaments = safe_ptr_cast<const ASCOrnaments*>(&data[ornamentsOff]);
        Data->Ornaments.reserve(ornaments->Offsets.size());
        uint_t index = 0;
        for (const uint16_t* pOrnament = ornaments->Offsets.begin(); pOrnament != ornaments->Offsets.end();
          ++pOrnament, ++index)
        {
          assert(*pOrnament && fromLE(*pOrnament) < data.Size());
          const std::size_t ornamentOffset = fromLE(*pOrnament);
          const ASCOrnament* const ornament = safe_ptr_cast<const ASCOrnament*>(&data[ornamentsOff + ornamentOffset]);
          Data->Ornaments.push_back(ASCTrack::Ornament(*ornament));
          const Ornament& orn = Data->Ornaments.back();
          if (orn.GetLoop() > orn.GetLoopLimit() || orn.GetLoopLimit() >= orn.GetSize())
          {
            Log::ParamPrefixedCollector lineWarner(*warner, Text::ORNAMENT_WARN_PREFIX, index);
            Log::Assert(lineWarner, orn.GetLoop() <= orn.GetLoopLimit(), Text::WARNING_LOOP_OUT_LIMIT);
            Log::Assert(lineWarner, orn.GetLoopLimit() < orn.GetSize(), Text::WARNING_LOOP_OUT_BOUND);
          }
          rawSize = std::max(rawSize, ornamentsOff + ornamentOffset + orn.GetSize() * sizeof(ASCOrnament::Line));
        }
      }

      //fill order
      Data->Positions.assign(header->Positions, header->Positions + header->Length);
      //parse patterns
      const std::size_t patternsCount = 1 + *std::max_element(Data->Positions.begin(), Data->Positions.end());
      const uint16_t patternsOff = fromLE(header->PatternsOffset);
      const ASCPattern* pattern = safe_ptr_cast<const ASCPattern*>(&data[patternsOff]);
      assert(patternsCount <= MAX_PATTERNS_COUNT);
      Data->Patterns.resize(patternsCount);
      for (uint_t patNum = 0; patNum < patternsCount; ++patNum, ++pattern)
      {
        Log::ParamPrefixedCollector patternWarner(*warner, Text::PATTERN_WARN_PREFIX, patNum);
        ASCTrack::Pattern& pat(Data->Patterns[patNum]);

        PatternCursors cursors;
        std::transform(pattern->Offsets.begin(), pattern->Offsets.end(), cursors.begin(),
          boost::bind(std::plus<uint_t>(), patternsOff, boost::bind(&fromLE<uint16_t>, _1)));
        uint_t envelopes = 0;
        pat.reserve(MAX_PATTERN_SIZE);
        do
        {
          const uint_t patternSize = pat.size();
          if (patternSize > MAX_PATTERN_SIZE)
          {
            throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
          }
          Log::ParamPrefixedCollector patLineWarner(patternWarner, Text::LINE_WARN_PREFIX, patternSize);
          pat.push_back(ASCTrack::Line());
          ASCTrack::Line& line(pat.back());
          ParsePattern(data, cursors, line, patLineWarner, envelopes);
          //skip lines
          if (const uint_t linesToSkip = std::min_element(cursors.begin(), cursors.end(), PatternCursor::CompareByCounter)->Counter)
          {
            std::for_each(cursors.begin(), cursors.end(), std::bind2nd(std::mem_fun_ref(&PatternCursor::SkipLines), linesToSkip));
            pat.resize(pat.size() + linesToSkip);//add dummies
          }
        }
        while (0xff != data[cursors.front().Offset] || cursors.front().Counter);
        //as warnings
        Log::Assert(patternWarner, 0 == std::max_element(cursors.begin(), cursors.end(), PatternCursor::CompareByCounter)->Counter,
          Text::WARNING_PERIODS);
        Log::Assert(patternWarner, pat.size() <= MAX_PATTERN_SIZE, Text::WARNING_INVALID_PATTERN_SIZE);
        rawSize = std::max<std::size_t>(rawSize, std::max_element(cursors.begin(), cursors.end(), PatternCursor::CompareByOffset)->Offset + 1);
      }

      //fill region
      region.Size = rawSize;
      region.Extract(*container.Data, RawData);

      //meta properties
      const ModuleProperties::Ptr props = ModuleProperties::Create(ASC_PLUGIN_ID);
      {
        const IO::DataContainer::Ptr rawData = region.Extract(*container.Data);
        const ASCID* const id = safe_ptr_cast<const ASCID*>(header->Positions + header->Length);
        const bool validId = id->Check();
        const std::size_t fixedOffset = sizeof(ASCHeader) + validId ? sizeof(*id) : 0;
        const ModuleRegion fixedRegion(fixedOffset, rawSize - fixedOffset);
        props->SetSource(rawData, fixedRegion);
        if (validId)
        {
          props->SetTitle(OptimizeString(FromStdString(id->Title)));
          props->SetAuthor(OptimizeString(FromStdString(id->Author)));
        }
      }
      props->SetPlugins(container.Plugins);
      props->SetPath(container.Path);
      props->SetProgram(Text::ASC_EDITOR);
      props->SetWarnings(warner);

      Info->SetLoopPosition(header->Loop);
      Info->SetTempo(header->Tempo);
      Info->SetModuleProperties(props);
    }

    virtual Plugin::Ptr GetPlugin() const
    {
      return SrcPlugin;
    }

    virtual Information::Ptr GetModuleInformation() const
    {
      return Info;
    }

    virtual Player::Ptr CreatePlayer() const
    {
      return CreateASCPlayer(Info, Data, AYM::CreateChip());
    }

    virtual Error Convert(const Conversion::Parameter& param, Dump& dst) const
    {
      using namespace Conversion;
      Error result;
      if (parameter_cast<RawConvertParam>(&param))
      {
        dst = RawData;
      }
      else if (!ConvertAYMFormat(boost::bind(&CreateASCPlayer, boost::cref(Info), boost::cref(Data), _1),
        param, dst, result))
      {
        return Error(THIS_LINE, ERROR_MODULE_CONVERT, Text::MODULE_ERROR_CONVERSION_UNSUPPORTED);
      }
      return result;
    }
  private:
    const Plugin::Ptr SrcPlugin;
    const ASCTrack::ModuleData::RWPtr Data;
    const ASCTrack::ModuleInfo::Ptr Info;
    Dump RawData;
  };

  struct ASCChannelState
  {
    ASCChannelState()
      : Enabled(false), Envelope(false), EnvelopeTone(0)
      , Volume(15), VolumeAddon(0), VolSlideDelay(0), VolSlideAddon(), VolSlideCounter(0)
      , BaseNoise(0), CurrentNoise(0)
      , Note(0), NoteAddon(0)
      , SampleNum(0), CurrentSampleNum(0), PosInSample(0)
      , OrnamentNum(0), CurrentOrnamentNum(0), PosInOrnament(0)
      , ToneDeviation(0)
      , SlidingSteps(0), Sliding(0), SlidingTargetNote(LIMITER), Glissade(0)
    {
    }
    bool Enabled;
    bool Envelope;
    uint_t EnvelopeTone;
    uint_t Volume;
    int_t VolumeAddon;
    uint_t VolSlideDelay;
    int_t VolSlideAddon;
    uint_t VolSlideCounter;
    int_t BaseNoise;
    int_t CurrentNoise;
    uint_t Note;
    int_t NoteAddon;
    uint_t SampleNum;
    uint_t CurrentSampleNum;
    uint_t PosInSample;
    uint_t OrnamentNum;
    uint_t CurrentOrnamentNum;
    uint_t PosInOrnament;
    int_t ToneDeviation;
    int_t SlidingSteps;//may be infinite (negative)
    int_t Sliding;
    uint_t SlidingTargetNote;
    int_t Glissade;
  };

  typedef AYMPlayer<ASCTrack::ModuleData, boost::array<ASCChannelState, AYM::CHANNELS> > ASCPlayerBase;

  class ASCPlayer : public ASCPlayerBase
  {
  public:
    ASCPlayer(Information::Ptr info, ASCTrack::ModuleData::Ptr data, AYM::Chip::Ptr device)
      : ASCPlayerBase(info, data, device, TABLE_ASM)
    {
#ifdef SELF_TEST
//perform self-test
      AYM::DataChunk chunk;
      do
      {
        assert(Data->Positions.size() > ModState.Track.Position);
        RenderData(chunk);
      }
      while (Data->UpdateState(*Info, Sound::LOOP_NONE, ModState));
      Reset();
#endif
    }

    virtual void RenderData(AYM::DataChunk& chunk)
    {
      const ASCTrack::Line& line = Data->Patterns[ModState.Track.Pattern][ModState.Track.Line];
      //bitmask of channels to sample break
      uint_t breakSample = 0;
      if (0 == ModState.Track.Quirk)//begin note
      {
        for (uint_t chan = 0; chan != line.Channels.size(); ++chan)
        {
          const ASCTrack::Line::Chan& src = line.Channels[chan];
          ASCChannelState& dst = PlayerState[chan];
          if (0 == ModState.Track.Line)
          {
            dst.BaseNoise = 0;
          }
          if (src.Empty())
          {
            continue;
          }
          if (src.Enabled)
          {
            dst.Enabled = *src.Enabled;
          }
          dst.VolSlideCounter = 0;
          dst.SlidingSteps = 0;
          bool contSample = false, contOrnament = false;
          for (ASCTrack::CommandsArray::const_iterator it = src.Commands.begin(), lim = src.Commands.end();
            it != lim; ++it)
          {
            switch (it->Type)
            {
            case ENVELOPE:
              if (-1 != it->Param1)
              {
                chunk.Data[AYM::DataChunk::REG_ENV] = static_cast<uint8_t>(it->Param1);
                chunk.Mask |= 1 << AYM::DataChunk::REG_ENV;
              }
              if (-1 != it->Param2)
              {
                chunk.Data[AYM::DataChunk::REG_TONEE_L] = static_cast<uint8_t>(dst.EnvelopeTone = it->Param2);
                chunk.Mask |= 1 << AYM::DataChunk::REG_TONEE_L;
              }
              break;
            case ENVELOPE_ON:
              dst.Envelope = true;
              break;
            case ENVELOPE_OFF:
              dst.Envelope = false;
              break;
            case NOISE:
              dst.BaseNoise = it->Param1;
              break;
            case CONT_SAMPLE:
              contSample = true;
              break;
            case CONT_ORNAMENT:
              contOrnament = true;
              break;
            case GLISS:
              dst.Glissade = it->Param1;
              dst.SlidingSteps = -1;//infinite sliding
              break;
            case SLIDE:
            {
              dst.SlidingSteps = it->Param1;
              const int_t newSliding = (dst.Sliding | 0xf) ^ 0xf;
              dst.Glissade = -newSliding / dst.SlidingSteps;
              dst.Sliding = dst.Glissade * dst.SlidingSteps;
              break;
            }
            case SLIDE_NOTE:
            {
              dst.SlidingSteps = it->Param1;
              dst.SlidingTargetNote = it->Param2;
              const FrequencyTable& freqTable = AYMHelper->GetFreqTable();
              const int_t newSliding = (contSample ? dst.Sliding / 16 : 0) + freqTable[dst.Note]
                - freqTable[dst.SlidingTargetNote];
              dst.Glissade = -16 * newSliding / dst.SlidingSteps;
              break;
            }
            case AMPLITUDE_SLIDE:
              dst.VolSlideCounter = dst.VolSlideDelay = it->Param1;
              dst.VolSlideAddon = it->Param2;
              break;
            case BREAK_SAMPLE:
              breakSample |= uint_t(1) << chan;
              break;
            default:
              assert(!"Invalid cmd");
              break;
            }
          }
          if (src.OrnamentNum)
          {
            dst.OrnamentNum = *src.OrnamentNum;
          }
          if (src.SampleNum)
          {
            dst.SampleNum = *src.SampleNum;
          }
          if (src.Note)
          {
            dst.Note = *src.Note;
            dst.CurrentNoise = dst.BaseNoise;
            if (dst.SlidingSteps <= 0)
            {
              dst.Sliding = 0;
            }
            if (!contSample)
            {
              dst.CurrentSampleNum = dst.SampleNum;
              dst.PosInSample = 0;
              dst.VolumeAddon = 0;
              dst.ToneDeviation = 0;
            }
            if (!contOrnament)
            {
              dst.CurrentOrnamentNum = dst.OrnamentNum;
              dst.PosInOrnament = 0;
              dst.NoteAddon = 0;
            }
          }
          if (src.Volume)
          {
            dst.Volume = *src.Volume;
          }
        }
      }
      //permanent registers
      chunk.Data[AYM::DataChunk::REG_MIXER] = 0;
      chunk.Mask |= (1 << AYM::DataChunk::REG_MIXER) |
        (1 << AYM::DataChunk::REG_VOLA) | (1 << AYM::DataChunk::REG_VOLB) | (1 << AYM::DataChunk::REG_VOLC);
      for (uint_t chan = 0; chan < AYM::CHANNELS; ++chan, breakSample >>= 1)
      {
        ApplyData(chan, 0 != (breakSample & 1), chunk);
      }
      //count actually enabled channels
      ModState.Track.Channels = static_cast<uint_t>(std::count_if(PlayerState.begin(), PlayerState.end(),
        boost::mem_fn(&ASCChannelState::Enabled)));
    }

    void ApplyData(uint_t chan, bool breakSample, AYM::DataChunk& chunk)
    {
      ASCChannelState& dst = PlayerState[chan];
      const uint_t toneReg = AYM::DataChunk::REG_TONEA_L + 2 * chan;
      const uint_t volReg = AYM::DataChunk::REG_VOLA + chan;
      const uint_t toneMsk = AYM::DataChunk::REG_MASK_TONEA << chan;
      const uint_t noiseMsk = AYM::DataChunk::REG_MASK_NOISEA << chan;

      const FrequencyTable& freqTable = AYMHelper->GetFreqTable();
      if (dst.Enabled)
      {
        const Sample& curSample = Data->Samples[dst.CurrentSampleNum];
        const Sample::Line& curSampleLine = curSample.GetLine(dst.PosInSample);
        const Ornament& curOrnament = Data->Ornaments[dst.CurrentOrnamentNum];
        const Ornament::Line& curOrnamentLine = curOrnament.GetLine(dst.PosInOrnament);

        //calculate volume addon
        if (dst.VolSlideCounter >= 2)
        {
          dst.VolSlideCounter--;
        }
        else if (dst.VolSlideCounter)
        {
          dst.VolumeAddon += dst.VolSlideAddon;
          dst.VolSlideCounter = dst.VolSlideDelay;
        }
        if (ASCSample::INCVOLADD == curSampleLine.Command)
        {
          dst.VolumeAddon++;
        }
        else if (ASCSample::DECVOLADD == curSampleLine.Command)
        {
          dst.VolumeAddon--;
        }
        dst.VolumeAddon = clamp<int_t>(dst.VolumeAddon, -15, 15);
        //calculate tone
        dst.ToneDeviation += curSampleLine.ToneDeviation;
        dst.NoteAddon += curOrnamentLine.NoteAddon;
        const uint_t halfTone = static_cast<uint_t>(clamp<int_t>(int_t(dst.Note) + dst.NoteAddon, 0, 85));
        const uint_t baseFreq = freqTable[halfTone];
        const uint_t tone = (baseFreq + dst.ToneDeviation + dst.Sliding / 16) & 0xfff;
        if (dst.SlidingSteps != 0)
        {
          if (dst.SlidingSteps > 0)
          {
            if (!--dst.SlidingSteps &&
                LIMITER != dst.SlidingTargetNote) //finish slide to note
            {
              dst.Note = dst.SlidingTargetNote;
              dst.SlidingTargetNote = LIMITER;
              dst.Sliding = dst.Glissade = 0;
            }
          }
          dst.Sliding += dst.Glissade;
        }
        chunk.Data[toneReg] = static_cast<uint8_t>(tone & 0xff);
        chunk.Data[toneReg + 1] = static_cast<uint8_t>(tone >> 8);
        chunk.Mask |= 3 << toneReg;

        const bool sampleEnvelope = ASCSample::ENVELOPE == curSampleLine.Command;
        //calculate level
        const uint_t level = static_cast<uint_t>((dst.Volume + 1) * clamp<int_t>(dst.VolumeAddon + curSampleLine.Level, 0, 15) / 16);
        chunk.Data[volReg] = static_cast<uint8_t>(level | (dst.Envelope && sampleEnvelope ?
          AYM::DataChunk::REG_MASK_ENV : 0));

        //calculate noise
        dst.CurrentNoise += curOrnamentLine.NoiseAddon;

        //mixer
        if (curSampleLine.ToneMask)
        {
          chunk.Data[AYM::DataChunk::REG_MIXER] |= toneMsk;
        }
        if (curSampleLine.NoiseMask && sampleEnvelope)
        {
          dst.EnvelopeTone += curSampleLine.Adding;
          chunk.Data[AYM::DataChunk::REG_TONEE_L] = static_cast<uint8_t>(dst.EnvelopeTone & 0xff);
          chunk.Mask |= 1 << AYM::DataChunk::REG_TONEE_L;
        }
        else
        {
          dst.CurrentNoise += curSampleLine.Adding;
        }

        if (curSampleLine.NoiseMask)
        {
          chunk.Data[AYM::DataChunk::REG_MIXER] |= noiseMsk;
        }
        else
        {
          chunk.Data[AYM::DataChunk::REG_TONEN] = static_cast<uint8_t>((dst.CurrentNoise + dst.Sliding / 256) & 0x1f);
          chunk.Mask |= 1 << AYM::DataChunk::REG_TONEN;
        }

        //recalc positions
        if (dst.PosInSample++ >= curSample.GetLoopLimit())
        {
          if (!breakSample)
          {
            dst.PosInSample = curSample.GetLoop();
          }
          else if (dst.PosInSample >= curSample.GetSize())
          {
            dst.Enabled = false;
          }
        }
        if (dst.PosInOrnament++ >= curOrnament.GetLoopLimit())
        {
          dst.PosInOrnament = curOrnament.GetLoop();
        }
      }
      else
      {
        chunk.Data[volReg] = 0;
        //????
        chunk.Data[AYM::DataChunk::REG_MIXER] |= toneMsk | noiseMsk;
      }
    }
  };

  Player::Ptr CreateASCPlayer(Information::Ptr info, ASCTrack::ModuleData::Ptr data, AYM::Chip::Ptr device)
  {
    return Player::Ptr(new ASCPlayer(info, data, device));
  }

  //////////////////////////////////////////////////////////////////////////
  bool CheckASCModule(const uint8_t* data, std::size_t size)
  {
    if (size < sizeof(ASCHeader))
    {
      return false;
    }

    const std::size_t limit = std::min(size, MAX_MODULE_SIZE);
    const ASCHeader* const header = safe_ptr_cast<const ASCHeader*>(data);
    const std::size_t headerBusy = sizeof(*header) + header->Length - 1;

    if (!header->Length || limit < headerBusy)
    {
      return false;
    }

    const std::size_t samplesOffset = fromLE(header->SamplesOffset);
    const std::size_t ornamentsOffset = fromLE(header->OrnamentsOffset);
    const std::size_t patternsOffset = fromLE(header->PatternsOffset);
    const uint_t patternsCount = 1 + *std::max_element(header->Positions, header->Positions + header->Length);

    //check patterns count and length
    if (!patternsCount || patternsCount > MAX_PATTERNS_COUNT ||
        header->Positions + header->Length !=
        std::find_if(header->Positions, header->Positions + header->Length,
          std::bind2nd(std::greater_equal<uint_t>(), patternsCount)))
    {
      return false;
    }

    RangeChecker::Ptr checker = RangeChecker::Create(limit);
    checker->AddRange(0, headerBusy);
    //check common ranges
    if (!checker->AddRange(patternsOffset, sizeof(ASCPattern) * patternsCount) ||
        !checker->AddRange(samplesOffset, sizeof(ASCSamples)) ||
        !checker->AddRange(ornamentsOffset, sizeof(ASCOrnaments))
        )
    {
      return false;
    }
    //check samples
    {
      const ASCSamples* const samples = safe_ptr_cast<const ASCSamples*>(data + samplesOffset);
      if (samples->Offsets.end() !=
        std::find_if(samples->Offsets.begin(), samples->Offsets.end(),
          !boost::bind(&RangeChecker::AddRange, checker.get(),
             boost::bind(std::plus<uint_t>(), samplesOffset, boost::bind(&fromLE<uint16_t>, _1)),
             sizeof(ASCSample::Line))))
      {
        return false;
      }
    }
    //check ornaments
    {
      const ASCOrnaments* const ornaments = safe_ptr_cast<const ASCOrnaments*>(data + ornamentsOffset);
      if (ornaments->Offsets.end() !=
        std::find_if(ornaments->Offsets.begin(), ornaments->Offsets.end(),
          !boost::bind(&RangeChecker::AddRange, checker.get(),
            boost::bind(std::plus<uint_t>(), ornamentsOffset, boost::bind(&fromLE<uint16_t>, _1)),
            sizeof(ASCOrnament::Line))))
      {
        return false;
      }
    }
    //check patterns
    {
      const ASCPattern* pattern = safe_ptr_cast<const ASCPattern*>(data + patternsOffset);
      for (uint_t patternNum = 0 ; patternNum < patternsCount; ++patternNum, ++pattern)
      {
        //at least 1 byte
        if (pattern->Offsets.end() !=
          std::find_if(pattern->Offsets.begin(), pattern->Offsets.end(),
          !boost::bind(&RangeChecker::AddRange, checker.get(),
             boost::bind(std::plus<uint_t>(), patternsOffset, boost::bind(&fromLE<uint16_t>, _1)),
             1)))
        {
          return false;
        }
      }
    }
    return true;
  }

  Holder::Ptr CreateASCModule(Plugin::Ptr plugin, const MetaContainer& container, ModuleRegion& region)
  {
    try
    {
      const Holder::Ptr holder(new ASCHolder(plugin, container, region));
#ifdef SELF_TEST
      holder->CreatePlayer();
#endif
      return holder;
    }
    catch (const Error&/*e*/)
    {
      Log::Debug("Core::ASCSupp", "Failed to create holder");
    }
    return Holder::Ptr();
  }

  //////////////////////////////////////////////////////////////////////////
  class ASCPlugin : public PlayerPlugin
                  , public boost::enable_shared_from_this<ASCPlugin>
  {
  public:
    virtual String Id() const
    {
      return ASC_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::ASC_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return ASC_PLUGIN_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | GetSupportedAYMFormatConvertors();
    }

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      return PerformCheck(&CheckASCModule, 0, 0, inputData);
    }

    virtual Module::Holder::Ptr CreateModule(const Parameters::Map& /*parameters*/,
                                             const MetaContainer& container,
                                             ModuleRegion& region) const
    {
      const Plugin::Ptr plugin = shared_from_this();
      return PerformCreate(&CheckASCModule, &CreateASCModule, 0, 0,
        plugin, container, region);
    }
  };
}

namespace ZXTune
{
  void RegisterASCSupport(PluginsEnumerator& enumerator)
  {
    const PlayerPlugin::Ptr plugin(new ASCPlugin());
    enumerator.RegisterPlugin(plugin);
  }
}
