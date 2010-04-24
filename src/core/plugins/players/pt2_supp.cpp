/*
Abstract:
  PT2 modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "aym_tracking.h"
#include "convert_helpers.h"
#include "tracking.h"
#include <core/plugins/detect_helper.h>
#include <core/plugins/utils.h>
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

#define FILE_TAG 077C8579

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  const Char PT2_PLUGIN_ID[] = {'P', 'T', '2', 0};
  const String PT2_PLUGIN_VERSION(FromStdString("$Rev$"));

  const uint_t LIMITER = ~uint_t(0);

  //hints
  const std::size_t MAX_MODULE_SIZE = 16384;
  const uint_t MAX_PATTERN_SIZE = 64;
  const uint_t MAX_PATTERN_COUNT = 64;//TODO

  //checkers
  const DetectFormatChain DETECTORS[] =
  {
    //PT20
    {
      "21??"    //ld hl,xxxx
      "c3??"    //jp xxxx
      "c3+563+" //jp xxxx:ds 561
      "f3"      //di
      "e5"      //push hl
      "22??"    //ld (xxxx),hl
      "e5"      //push hl
      "7e"      //ld a,(hl)
      "32??"    //ld (xxxx),a
      "32??"    //ld (xxxx),a
      "23"      //inc hl
      "23"      //inc hl
      "7e"      //ld a,(hl)
      "23"      //inc hl
      "22??"    //ld (xxxx),hl
      "11??"    //ld de,xxxx(32)
      "19"      //add hl,de
      "19"      //add hl,de
      "22??"    //ld (xxxx),hl
      "19"      //add hl,de
      "5e"      //ld e,(hl)
      "23"      //inc hl
      "56"      //ld d,(hl)
      "23"      //inc hl
      "01??"    //ld bc,xxxx(30)
      "09"      //add hl,bc
      ,
      2591
    },
    //PT21
    {
      "21??"    //ld hl,xxxx
      "c3??"    //jp xxxx
      "c3+14+"  //jp xxxx
                //ds 12
      "322e31"
      ,
      0xa2f
    },
    //PT24
    {
      "21??"    //ld hl,xxxx
      "1803"    //jr $+3
      "c3??"    //jp xxxx
      "f3"      //di
      "e5"      //push hl
      "7e"      //ld a,(hl)
      "32??"    //ld (xxxx),a
      "32??"    //ld (xxxx),a
      "23"      //inc hl
      "23"      //inc hl
      "7e"      //ld a,(hl)
      "23"      //inc hl
      ,
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
    boost::array<uint16_t, 32> SamplesOffsets;
    boost::array<uint16_t, 16> OrnamentsOffsets;
    uint16_t PatternsOffset;
    char Name[30];
    uint8_t Positions[1];
  } PACK_POST;

  const uint8_t POS_END_MARKER = 0xff;

  PACK_PRE struct PT2Sample
  {
    uint8_t Size;
    uint8_t Loop;

    uint_t GetSize() const
    {
      return sizeof(PT2Sample) + (Size - 1) * sizeof(Line);
    }

    PACK_PRE struct Line
    {
      //nnnnnsTN
      //aaaaHHHH
      //LLLLLLLL
      
      //HHHHLLLLLLLL - vibrato
      //s - vibrato sign
      //a - level
      //N - noise off
      //T - tone off
      //n - noise value
      uint8_t NoiseAndFlags;
      uint8_t LevelHiVibrato;
      uint8_t LoVibrato;
      
      bool GetNoiseMask() const
      {
        return 0 != (NoiseAndFlags & 1);
      }
      
      bool GetToneMask() const
      {
        return 0 != (NoiseAndFlags & 2);
      }
      
      uint_t GetNoise() const
      {
        return NoiseAndFlags >> 3;
      }
      
      uint_t GetLevel() const
      {
        return LevelHiVibrato >> 4;
      }
      
      int_t GetVibrato() const
      {
        const int_t val(((LevelHiVibrato & 0x0f) << 8) | LoVibrato);
        return (NoiseAndFlags & 4) ? val : -val;
      }
    } PACK_POST;
    Line Data[1];
  } PACK_POST;

  PACK_PRE struct PT2Ornament
  {
    uint8_t Size;
    uint8_t Loop;
    int8_t Data[1];

    uint_t GetSize() const
    {
      return sizeof(PT2Ornament) + (Size - 1);
    }
  } PACK_POST;

  PACK_PRE struct PT2Pattern
  {
    boost::array<uint16_t, 3> Offsets;

    bool Check() const
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

  //sample type
  struct Sample
  {
    Sample() : Loop(), Data()
    {
    }

    Sample(uint_t size, uint_t loop) : Loop(loop), Data(size)
    {
    }

    //make safe sample
    void Fix()
    {
      if (Data.empty())
      {
        Data.resize(1);
      }
    }

    struct Line
    {
      Line() : Level(), Noise(), ToneOff(), NoiseOff(), Vibrato()
      {
      }
      /*explicit*/Line(const PT2Sample::Line& src)
        : Level(src.GetLevel()), Noise(src.GetNoise())
        , ToneOff(src.GetToneMask()), NoiseOff(src.GetNoiseMask())
        , Vibrato(src.GetVibrato())
      {
      }
      uint_t Level;//0-15
      uint_t Noise;//0-31
      bool ToneOff;
      bool NoiseOff;
      int_t Vibrato;
    };
    
    uint_t Loop;
    std::vector<Line> Data;
  };

  inline Sample ParseSample(const IO::FastDump& data, uint16_t offset, std::size_t& rawSize)
  {
    const uint_t off = fromLE(offset);
    const PT2Sample* const sample = safe_ptr_cast<const PT2Sample*>(&data[off]);
    if (0 == offset || !sample->Size)
    {
      return Sample(1, 0);//safe
    }
    Sample tmp(sample->Size, sample->Loop);
    std::copy(sample->Data, sample->Data + sample->Size, tmp.Data.begin());
    rawSize = std::max<std::size_t>(rawSize, off + sample->GetSize());
    return tmp;
  }
  
  inline SimpleOrnament ParseOrnament(const IO::FastDump& data, uint16_t offset, std::size_t& rawSize)
  {
    const uint_t off = fromLE(offset);
    const PT2Ornament* const ornament = safe_ptr_cast<const PT2Ornament*>(&data[off]);
    if (0 == offset || !ornament->Size)
    {
      return SimpleOrnament(1, 0);//safe version
    }
    rawSize = std::max<std::size_t>(rawSize, off + ornament->GetSize());
    return SimpleOrnament(ornament->Data, ornament->Data + ornament->Size, ornament->Loop);
  }

  //supported commands
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

  void DescribePT2Plugin(PluginInformation& info)
  {
    info.Id = PT2_PLUGIN_ID;
    info.Description = Text::PT2_PLUGIN_INFO;
    info.Version = PT2_PLUGIN_VERSION;
    info.Capabilities = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | GetSupportedAYMFormatConvertors();
  }

  typedef TrackingSupport<AYM::CHANNELS, Sample> PT2Track;
  
  Player::Ptr CreatePT2Player(Holder::ConstPtr mod, const PT2Track::ModuleData& data, AYM::Chip::Ptr device);

  class PT2Holder : public Holder, public boost::enable_shared_from_this<PT2Holder>
  {
    typedef boost::array<PatternCursor, AYM::CHANNELS> PatternCursors;
    
    void ParsePattern(const IO::FastDump& data
      , PatternCursors& cursors
      , PT2Track::Line& line
      , Log::MessagesCollector& warner
      )
    {
      assert(line.Channels.size() == cursors.size());
      PT2Track::Line::ChannelsArray::iterator channel(line.Channels.begin());
      for (PatternCursors::iterator cur = cursors.begin(); cur != cursors.end(); ++cur, ++channel)
      {
        if (cur->Counter--)
        {
          continue;//has to skip
        }

        Log::ParamPrefixedCollector channelWarner(warner, Text::CHANNEL_WARN_PREFIX, std::distance(line.Channels.begin(), channel));
        for (;;)
        {
          const uint_t cmd(data[cur->Offset++]);
          const std::size_t restbytes = data.Size() - cur->Offset;
          if (cmd >= 0xe1) //sample
          {
            Log::Assert(channelWarner, !channel->SampleNum, Text::WARNING_DUPLICATE_SAMPLE);
            const uint_t num = cmd - 0xe0;
            channel->SampleNum = num;
            Log::Assert(channelWarner, !Data.Samples[num].Data.empty(), Text::WARNING_INVALID_SAMPLE);
          }
          else if (cmd == 0xe0) //sample 0 - shut up
          {
            Log::Assert(channelWarner, !channel->Enabled, Text::WARNING_DUPLICATE_STATE);
            channel->Enabled = false;
            break;
          }
          else if (cmd >= 0x80 && cmd <= 0xdf)//note
          {
            Log::Assert(channelWarner, !channel->Enabled, Text::WARNING_DUPLICATE_STATE);
            channel->Enabled = true;
            const uint_t note(cmd - 0x80);
            //for note gliss calculate limit manually
            const PT2Track::CommandsArray::iterator noteGlissCmd(
              std::find(channel->Commands.begin(), channel->Commands.end(), GLISS_NOTE));
            if (channel->Commands.end() != noteGlissCmd)
            {
              noteGlissCmd->Param2 = int_t(note);
            }
            else
            {
              Log::Assert(channelWarner, !channel->Note, Text::WARNING_DUPLICATE_NOTE);
              channel->Note = note;
            }
            break;
          }
          else if (cmd == 0x7f) //env off
          {
            Log::Assert(channelWarner, !channel->FindCommand(ENVELOPE) && !channel->FindCommand(NOENVELOPE),
              Text::WARNING_DUPLICATE_ENVELOPE);
            channel->Commands.push_back(NOENVELOPE);
          }
          else if (cmd >= 0x71 && cmd <= 0x7e) //envelope
          {
            //required 2 bytes
            if (restbytes < 2)
            {
              throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
            }
            Log::Assert(channelWarner, !channel->FindCommand(ENVELOPE) && !channel->FindCommand(NOENVELOPE),
              Text::WARNING_DUPLICATE_ENVELOPE);
            channel->Commands.push_back(
              PT2Track::Command(ENVELOPE, cmd - 0x70, data[cur->Offset] | (uint_t(data[cur->Offset + 1]) << 8)));
            cur->Offset += 2;
          }
          else if (cmd == 0x70)//quit
          {
            break;
          }
          else if (cmd >= 0x60 && cmd <= 0x6f)//ornament
          {
            Log::Assert(channelWarner, !channel->OrnamentNum, Text::WARNING_DUPLICATE_ORNAMENT);
            const uint_t num = cmd - 0x60;
            channel->OrnamentNum = num;
            Log::Assert(channelWarner, !(num && Data.Ornaments[num].Data.empty()), Text::WARNING_INVALID_ORNAMENT);
          }
          else if (cmd >= 0x20 && cmd <= 0x5f)//skip
          {
            cur->Period = cmd - 0x20;
          }
          else if (cmd >= 0x10 && cmd <= 0x1f)//volume
          {
            Log::Assert(channelWarner, !channel->Volume, Text::WARNING_DUPLICATE_VOLUME);
            channel->Volume = cmd - 0x10;
          }
          else if (cmd == 0x0f)//new delay
          {
            //required 1 byte
            if (restbytes < 1)
            {
              throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
            }
            Log::Assert(channelWarner, !line.Tempo, Text::WARNING_DUPLICATE_TEMPO);
            line.Tempo = data[cur->Offset++];
          }
          else if (cmd == 0x0e)//gliss
          {
            //TODO: warn
            //required 1 byte
            if (restbytes < 1)
            {
              throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
            }
            channel->Commands.push_back(PT2Track::Command(GLISS, static_cast<int8_t>(data[cur->Offset++])));
          }
          else if (cmd == 0x0d)//note gliss
          {
            //required 3 bytes
            if (restbytes < 3)
            {
              throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
            }
            //too late when note is filled
            Log::Assert(channelWarner, !channel->Note, Text::WARNING_INVALID_NOTE_GLISS);
            channel->Commands.push_back(PT2Track::Command(GLISS_NOTE, static_cast<int8_t>(data[cur->Offset])));
            //ignore delta due to error
            cur->Offset += 3;
          }
          else if (cmd == 0x0c) //gliss off
          {
            //TODO: warn
            channel->Commands.push_back(NOGLISS);
          }
          else //noise add
          {
            //required 1 byte
            if (restbytes < 1)
            {
              throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
            }
            channel->Commands.push_back(PT2Track::Command(NOISE_ADD, static_cast<int8_t>(data[cur->Offset++])));
          }
        }
        cur->Counter = cur->Period;
      }
    }

  public:
    PT2Holder(const MetaContainer& container, ModuleRegion& region)
    {
      //assume all data is correct
      const IO::FastDump& data = IO::FastDump(*container.Data, region.Offset);
      const PT2Header* const header = safe_ptr_cast<const PT2Header*>(&data[0]);
      const PT2Pattern* patterns = safe_ptr_cast<const PT2Pattern*>(&data[fromLE(header->PatternsOffset)]);

      Log::MessagesCollector::Ptr warner(Log::MessagesCollector::Create());

      std::size_t rawSize = 0;
      //fill samples
      Data.Samples.reserve(header->SamplesOffsets.size());
      std::transform(header->SamplesOffsets.begin(), header->SamplesOffsets.end(),
        std::back_inserter(Data.Samples), boost::bind(&ParseSample, boost::cref(data), _1, boost::ref(rawSize)));
      //fill ornaments
      Data.Ornaments.reserve(header->OrnamentsOffsets.size());
      std::transform(header->OrnamentsOffsets.begin(), header->OrnamentsOffsets.end(),
        std::back_inserter(Data.Ornaments), boost::bind(&ParseOrnament, boost::cref(data), _1, boost::ref(rawSize)));

      //fill patterns
      Data.Patterns.resize(MAX_PATTERN_COUNT);
      uint_t index(0);
      for (const PT2Pattern* pattern = patterns; pattern->Check(); ++pattern, ++index)
      {
        Log::ParamPrefixedCollector patternWarner(*warner, Text::PATTERN_WARN_PREFIX, index);
        PT2Track::Pattern& pat = Data.Patterns[index];
        
        PatternCursors cursors;
        std::transform(pattern->Offsets.begin(), pattern->Offsets.end(), cursors.begin(), &fromLE<uint16_t>);
        pat.reserve(MAX_PATTERN_SIZE);
        do
        {
          const uint_t patternSize = pat.size();
          if (patternSize > MAX_PATTERN_SIZE)
          {
            throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
          }
          Log::ParamPrefixedCollector patLineWarner(patternWarner, Text::LINE_WARN_PREFIX, patternSize);
          pat.push_back(PT2Track::Line());
          PT2Track::Line& line = pat.back();
          ParsePattern(data, cursors, line, patLineWarner);
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
          Text::WARNING_PERIODS);
        Log::Assert(patternWarner, pat.size() <= MAX_PATTERN_SIZE, Text::WARNING_INVALID_PATTERN_SIZE);
        rawSize = std::max<std::size_t>(rawSize, 1 + std::max_element(cursors.begin(), cursors.end(), PatternCursor::CompareByOffset)->Offset);
      }

      //fill order
      for (const uint8_t* curPos = header->Positions; POS_END_MARKER != *curPos; ++curPos)
      {
        if (!Data.Patterns[*curPos].empty())
        {
          Data.Positions.push_back(*curPos);
        }
      }
      Log::Assert(*warner, header->Length == Data.Positions.size(), Text::WARNING_INVALID_LENGTH);

      //fix samples and ornaments
      std::for_each(Data.Ornaments.begin(), Data.Ornaments.end(), std::mem_fun_ref(&PT2Track::Ornament::Fix));
      std::for_each(Data.Samples.begin(), Data.Samples.end(), std::mem_fun_ref(&PT2Track::Sample::Fix));
      
      //fill region
      region.Size = rawSize;
      
      //meta properties
      const std::size_t fixedOffset(sizeof(PT2Header) + header->Length - 1);
      ExtractMetaProperties(PT2_PLUGIN_ID, container, region, ModuleRegion(fixedOffset, rawSize - fixedOffset),
        Data.Info.Properties, RawData);
      const String& title(OptimizeString(FromStdString(header->Name)));
      if (!title.empty())
      {
        Data.Info.Properties.insert(Parameters::Map::value_type(Module::ATTR_TITLE, title));
      }
      Data.Info.Properties.insert(Parameters::Map::value_type(Module::ATTR_PROGRAM, String(Text::PT2_EDITOR)));
      
      //tracking properties
      Data.Info.LoopPosition = header->Loop;
      Data.Info.PhysicalChannels = AYM::CHANNELS;
      Data.Info.Statistic.Tempo = header->Tempo;
      Data.Info.Statistic.Position = Data.Positions.size();
      Data.Info.Statistic.Pattern = std::count_if(Data.Patterns.begin(), Data.Patterns.end(),
        !boost::bind(&PT2Track::Pattern::empty, _1));
      Data.Info.Statistic.Channels = AYM::CHANNELS;
      PT2Track::CalculateTimings(Data, Data.Info.Statistic.Frame, Data.Info.LoopFrame);
      if (const uint_t msgs = warner->CountMessages())
      {
        Data.Info.Properties.insert(Parameters::Map::value_type(Module::ATTR_WARNINGS_COUNT, msgs));
        Data.Info.Properties.insert(Parameters::Map::value_type(Module::ATTR_WARNINGS, warner->GetMessages('\n')));
      }
    }
    virtual void GetPluginInformation(PluginInformation& info) const
    {
      DescribePT2Plugin(info);
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
      return CreatePT2Player(shared_from_this(), Data, AYM::CreateChip());
    }
    
    virtual Error Convert(const Conversion::Parameter& param, Dump& dst) const
    {
      using namespace Conversion;
      Error result;
      if (parameter_cast<RawConvertParam>(&param))
      {
        dst = RawData;
      }
      else if (!ConvertAYMFormat(boost::bind(&CreatePT2Player, shared_from_this(), boost::cref(Data), _1), param, dst, result))
      {
        return Error(THIS_LINE, ERROR_MODULE_CONVERT, Text::MODULE_ERROR_CONVERSION_UNSUPPORTED);
      }
      return result;
    }
  private:
    friend class PT2Player;
    Dump RawData;
    PT2Track::ModuleData Data;
  };

  inline uint_t GetVolume(uint_t volume, uint_t level)
  {
    return (volume * 17 + (volume > 7 ? 1 : 0)) * level / 256;
  }

  struct PT2ChannelState
  {
    PT2ChannelState()
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

  typedef AYMPlayerBase<PT2Track, boost::array<PT2ChannelState, AYM::CHANNELS> > PT2PlayerBase;

  class PT2Player : public PT2PlayerBase
  {
  public:
    PT2Player(Holder::ConstPtr holder, const PT2Track::ModuleData& data, AYM::Chip::Ptr device)
       : PT2PlayerBase(holder, data, device, TABLE_PROTRACKER2)
    {
    }

    virtual void RenderData(AYM::DataChunk& chunk)
    {
      const PT2Track::Line& line(Data.Patterns[ModState.Track.Pattern][ModState.Track.Line]);
      if (0 == ModState.Track.Frame)//begin note
      {
        for (uint_t chan = 0; chan != line.Channels.size(); ++chan)
        {
          const PT2Track::Line::Chan& src(line.Channels[chan]);
          PT2ChannelState& dst(PlayerState[chan]);
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
          for (PT2Track::CommandsArray::const_iterator it = src.Commands.begin(), lim = src.Commands.end(); it != lim; ++it)
          {
            switch (it->Type)
            {
            case ENVELOPE:
              chunk.Data[AYM::DataChunk::REG_ENV] = static_cast<uint8_t>(it->Param1);
              chunk.Data[AYM::DataChunk::REG_TONEE_L] = static_cast<uint8_t>(it->Param2 & 0xff);
              chunk.Data[AYM::DataChunk::REG_TONEE_H] = static_cast<uint8_t>(it->Param2 >> 8);
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
      for (uint_t chan = 0; chan < AYM::CHANNELS; ++chan)
      {
        ApplyData(chan, chunk);
      }
      //count actually enabled channels
      ModState.Track.Channels = static_cast<uint_t>(std::count_if(PlayerState.begin(), PlayerState.end(),
        boost::mem_fn(&PT2ChannelState::Enabled)));
    }
    
    void ApplyData(uint_t chan, AYM::DataChunk& chunk)
    {
      PT2ChannelState& dst(PlayerState[chan]);
      const uint_t toneReg(AYM::DataChunk::REG_TONEA_L + 2 * chan);
      const uint_t volReg = AYM::DataChunk::REG_VOLA + chan;
      const uint_t toneMsk = AYM::DataChunk::REG_MASK_TONEA << chan;
      const uint_t noiseMsk = AYM::DataChunk::REG_MASK_NOISEA << chan;

      const FrequencyTable& freqTable(AYMHelper->GetFreqTable());
      if (dst.Enabled && dst.SampleNum)
      {
        const Sample& curSample(Data.Samples[dst.SampleNum]);
        const Sample::Line& curSampleLine(curSample.Data[dst.PosInSample]);
        const SimpleOrnament& curOrnament(Data.Ornaments[dst.OrnamentNum]);

        //calculate tone
        const uint_t halfTone = static_cast<uint_t>(clamp<int_t>(int_t(dst.Note) + curOrnament.Data[dst.PosInOrnament], 0, freqTable.size() - 1));
        const uint_t tone = static_cast<uint_t>(clamp<int_t>(int_t(freqTable[halfTone]) + dst.Sliding + curSampleLine.Vibrato, 0, 0xfff));
        if (dst.SlidingTargetNote != LIMITER)
        {
          const uint_t nextTone(freqTable[dst.Note] + dst.Sliding + dst.Glissade);
          const uint_t slidingTarget(freqTable[dst.SlidingTargetNote]);
          if ((dst.Glissade > 0 && nextTone >= slidingTarget) ||
              (dst.Glissade < 0 && nextTone <= slidingTarget))
          {
            dst.Note = dst.SlidingTargetNote;
            dst.SlidingTargetNote = LIMITER;
            dst.Sliding = dst.Glissade = 0;
          }
        }
        dst.Sliding += dst.Glissade;
        chunk.Data[toneReg] = static_cast<uint8_t>(tone & 0xff);
        chunk.Data[toneReg + 1] = static_cast<uint8_t>(tone >> 8);
        chunk.Mask |= 3 << toneReg;
        //calculate level
        chunk.Data[volReg] = static_cast<uint8_t>(GetVolume(dst.Volume, curSampleLine.Level)
          | uint8_t(dst.Envelope ? AYM::DataChunk::REG_MASK_ENV : 0));
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
          chunk.Data[AYM::DataChunk::REG_TONEN] = static_cast<uint8_t>(clamp<int_t>(curSampleLine.Noise + dst.NoiseAdd, 0, 31));
          chunk.Mask |= 1 << AYM::DataChunk::REG_TONEN;
        }

        if (++dst.PosInSample >= curSample.Data.size())
        {
          dst.PosInSample = curSample.Loop;
        }
        if (++dst.PosInOrnament >= curOrnament.Data.size())
        {
          dst.PosInOrnament = curOrnament.Loop;
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

  Player::Ptr CreatePT2Player(Holder::ConstPtr holder, const PT2Track::ModuleData& data, AYM::Chip::Ptr device)
  {
    return Player::Ptr(new PT2Player(holder, data, device));
  }

  //////////////////////////////////////////////////
  bool CheckPT2Module(const uint8_t* data, std::size_t size, const MetaContainer& container,
    Holder::Ptr& holder, ModuleRegion& region)
  {
    if (sizeof(PT2Header) > size)
    {
      return false;
    }

    const PT2Header* const header(safe_ptr_cast<const PT2Header*>(data));
    if (header->Tempo < 2 || header->Loop >= header->Length ||
        header->Length < 1 || header->Length + 1 + sizeof(*header) > size)
    {
      return false;
    }
    const uint_t lowlimit(1 +
      std::find(header->Positions, data + header->Length + sizeof(*header) + 1, POS_END_MARKER) - data);
    if (lowlimit - sizeof(*header) != header->Length)//too big positions list
    {
      return false;
    }

    //check patterns offset
    const uint_t patOff(fromLE(header->PatternsOffset));
    if (!in_range<std::size_t>(patOff, lowlimit, size))
    {
      return false;
    }

    RangeChecker::Ptr checker(RangeChecker::CreateShared(size));
    checker->AddRange(0, lowlimit);
    //check patterns
    {
      uint_t patternsCount(0);
      for (const PT2Pattern* patPos(safe_ptr_cast<const PT2Pattern*>(data + patOff));
        patPos->Check();
        ++patPos, ++patternsCount)
      {
        if (patternsCount > MAX_PATTERN_COUNT ||
            !checker->AddRange(patOff + sizeof(PT2Pattern) * patternsCount, sizeof(PT2Pattern)) ||
            //simple check if in range- performance issue
            patPos->Offsets.end() != std::find_if(patPos->Offsets.begin(), patPos->Offsets.end(),
              !boost::bind(&in_range<uint_t>, boost::bind(&fromLE<uint16_t>, _1), lowlimit, size)))
        {
          return false;
        }
      }
      if (!patternsCount)
      {
        return false;
      }
      //find invalid patterns in position
      if (header->Positions + header->Length !=
          std::find_if(header->Positions, header->Positions + header->Length, std::bind2nd(std::greater_equal<uint_t>(),
          patternsCount)))
      {
        return false;
      }
    }

    //check samples
    for (const uint16_t* sampOff = header->SamplesOffsets.begin(); sampOff != header->SamplesOffsets.end(); ++sampOff)
    {
      if (const uint_t offset = fromLE(*sampOff))
      {
        if (!checker->AddRange(offset, sizeof(PT2Sample)))
        {
          return false;
        }
        const PT2Sample* const sample(safe_ptr_cast<const PT2Sample*>(data + offset));
        if (!checker->AddRange(offset + sizeof(PT2Sample), sample->GetSize() - sizeof(PT2Sample)))
        {
          return false;
        }
      }
    }
    //check ornaments
    for (const uint16_t* ornOff = header->OrnamentsOffsets.begin(); ornOff != header->OrnamentsOffsets.end(); ++ornOff)
    {
      if (const uint_t offset = fromLE(*ornOff))
      {
        if (!checker->AddRange(offset, sizeof(PT2Ornament)))
        {
          return false;
        }
        const PT2Ornament* const ornament(safe_ptr_cast<const PT2Ornament*>(data + offset));
        if (!checker->AddRange(offset + sizeof(PT2Ornament), ornament->GetSize() - sizeof(PT2Ornament)))
        {
          return false;
        }
      }
    }
    //try to create holder
    try
    {
      holder.reset(new PT2Holder(container, region));
#ifdef SELF_TEST
      holder->CreatePlayer();
#endif
      return true;
    }
    catch (const Error&/*e*/)
    {
      Log::Debug("Core::PT2Supp", "Failed to create holder");
    }
    return false;
  }

  //////////////////////////////////////////////////////////////////////////
  bool CreatePT2Module(const Parameters::Map& /*commonParams*/, const MetaContainer& container,
    Holder::Ptr& holder, ModuleRegion& region)
  {
    return PerformDetect(&CheckPT2Module, DETECTORS, ArrayEnd(DETECTORS),
      container, holder, region);
  }
}

namespace ZXTune
{
  void RegisterPT2Support(PluginsEnumerator& enumerator)
  {
    PluginInformation info;
    DescribePT2Plugin(info);
    enumerator.RegisterPlayerPlugin(info, CreatePT2Module);
  }
}
