/*
Abstract:
  STC modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "aym_base.h"
#include "convert_helpers.h"
#include "tracking.h"
#include <core/plugins/detect_helper.h>
#include <core/plugins/utils.h>
//common includes
#include <byteorder.h>
#include <error_tools.h>
#include <logging.h>
#include <messages_collector.h>
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

#define FILE_TAG D9281D1D

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char STC_PLUGIN_ID[] = {'S', 'T', 'C', 0};
  const String STC_PLUGIN_VERSION(FromStdString("$Rev$"));

  //hints
  const std::size_t MAX_STC_MODULE_SIZE = 16384;
  const uint_t MAX_SAMPLES_COUNT = 16;
  const uint_t MAX_ORNAMENTS_COUNT = 16;
  const uint_t MAX_PATTERN_SIZE = 64;
  const uint_t MAX_PATTERN_COUNT = 32;
  const uint_t SAMPLE_ORNAMENT_SIZE = 32;

  //detectors
  static const DetectFormatChain DETECTORS[] =
  {
    {
      "21??"   //ld hl,xxxx
      "c3??"   //jp xxxx
      "c3??"   //jp xxxx
      "f3"     //di
      "7e"     //ld a,(hl)
      "32??"   //ld (xxxx),a
      "22??"   //ld (xxxx),hl
      "23"     //inc hl
      "cd??"   //call xxxx
      "1a"     //ld a,(de)
      "13"     //inc de
      "3c"     //inc a
      "32??"   //ld (xxxx),a
      "ed53??" //ld (xxxx),de
      "cd??"   //call xxxx
      "ed53??" //ld (xxxx),de
      "d5"     //push de
      "cd??"   //call xxxx
      "ed53??" //ld (xxxx),de
      "21??"   //ld hl,xxxx
      "cd??"   //call xxxx
      "eb"     //ex de,hl
      "22??"   //ld (xxxx),hl
      "21??"   //ld hl,xxxx
      "22??"   //ld(xxxx),hl
      "21??"   //ld hl,xxxx
      "11??"   //ld de,xxxx
      "01??"   //ld bc,xxxx
      "70"     //ld (hl),b
      "edb0"   //ldir
      "e1"     //pop hl
      "01??"   //ld bc,xxxx
      "af"     //xor a
      "cd??"   //call xxxx
      "3d"     //dec a
      "32??"   //ld (xxxx),a
      "32??"   //ld (xxxx),a
      "32??"   //ld (xxxx),a
      "3e?"    //ld a,x
      "32??"   //ld (xxxx),a
      "23"     //inc hl
      "22??"   //ld (xxxx),hl
      "22??"   //ld (xxxx),hl
      "22??"   //ld (xxxx),hl
      "cd??"   //call xxxx
      "fb"     //ei
      "c9"     //ret
      ,
      0x43c
    }
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
    char Identifier[18];
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
    boost::array<uint16_t, AYM::CHANNELS> Offsets;

    operator bool () const
    {
      return 0xff != Number;
    }
  } PACK_POST;

  PACK_PRE struct STCOrnament
  {
    uint8_t Number;
    int8_t Data[SAMPLE_ORNAMENT_SIZE];
  } PACK_POST;

  PACK_PRE struct STCSample
  {
    uint8_t Number;
    // EEEEaaaa
    // NESnnnnn
    // eeeeeeee
    // Ee - effect
    // a - level
    // N - noise mask, n- noise value
    // E - envelope mask
    // S - effect sign
    PACK_PRE struct Line
    {
      uint8_t EffHiAndLevel;
      uint8_t NoiseAndMasks;
      uint8_t EffLo;
      
      uint_t GetLevel() const
      {
        return EffHiAndLevel & 15;
      }
      
      int_t GetEffect() const
      {
        const int_t val = int_t(EffHiAndLevel & 240) * 16 + EffLo;
        return (NoiseAndMasks & 32) ? val : -val;
      }
      
      uint_t GetNoise() const
      {
        return NoiseAndMasks & 31;
      }
      
      bool GetNoiseMask() const
      {
        return 0 != (NoiseAndMasks & 128);
      }
      
      bool GetEnvelopeMask() const
      {
        return 0 != (NoiseAndMasks & 64);
      }
    } PACK_POST;
    Line Data[SAMPLE_ORNAMENT_SIZE];
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

  // currently used sample
  struct Sample
  {
    Sample() : Data(), Loop(), LoopLimit()
    {
    }

    explicit Sample(const STCSample& sample)
      : Data(sample.Data, ArrayEnd(sample.Data))
      , Loop(std::min<uint_t>(sample.Loop, Data.size()))
      , LoopLimit(std::min<uint_t>(sample.Loop + sample.LoopSize + 1, Data.size()))
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
      Line() : Level(), Noise(), NoiseMask(), EnvelopeMask(), Effect()
      {
      }
      
      /*explicit*/Line(const STCSample::Line& line)
        : Level(line.GetLevel()), Noise(line.GetNoise()), NoiseMask(line.GetNoiseMask())
        , EnvelopeMask(line.GetEnvelopeMask()), Effect(line.GetEffect())
      {
      }
      uint_t Level;//0-15
      uint_t Noise;//0-31
      bool NoiseMask;
      bool EnvelopeMask;
      int_t Effect;
    };
    
    std::vector<Line> Data;
    uint_t Loop;
    uint_t LoopLimit;
  };

  enum CmdType
  {
    EMPTY,
    ENVELOPE,     //2p
    NOENVELOPE,   //0p
  };

  void DescribeSTCPlugin(PluginInformation& info)
  {
    info.Id = STC_PLUGIN_ID;
    info.Description = Text::STC_PLUGIN_INFO;
    info.Version = STC_PLUGIN_VERSION;
    info.Capabilities = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | GetSupportedAYMFormatConvertors();
  }

  typedef TrackingSupport<AYM::CHANNELS, Sample> STCTrack;
  typedef std::vector<int_t> STCTransposition;

  // forward declaration
  Player::Ptr CreateSTCPlayer(Holder::ConstPtr mod, const STCTrack::ModuleData& data,
    const STCTransposition& transpositions, AYM::Chip::Ptr device);

  class STCHolder : public Holder, public boost::enable_shared_from_this<STCHolder>
  {
    typedef boost::array<PatternCursor, AYM::CHANNELS> PatternCursors;

    void ParsePattern(const IO::FastDump& data
      , PatternCursors& cursors
      , STCTrack::Line& line
      , Log::MessagesCollector& warner
      )
    {
      assert(line.Channels.size() == cursors.size());
      STCTrack::Line::ChannelsArray::iterator channel(line.Channels.begin());
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
          //ornament==0 and sample==0 are valid - no ornament and no sample respectively
          //ornament==0 and sample==0 are valid - no ornament and no sample respectively
          if (cmd <= 0x5f)//note
          {
            Log::Assert(channelWarner, !channel->Note, Text::WARNING_DUPLICATE_NOTE);
            channel->Note = cmd;
            Log::Assert(channelWarner, !channel->Enabled, Text::WARNING_DUPLICATE_STATE);
            channel->Enabled = true;
            break;
          }
          else if (cmd >= 0x60 && cmd <= 0x6f)//sample
          {
            Log::Assert(channelWarner, !channel->SampleNum, Text::WARNING_DUPLICATE_SAMPLE);
            const uint_t num = cmd - 0x60;
            channel->SampleNum = num;
            Log::Assert(channelWarner, !(num && Data.Samples[num].Data.empty()), Text::WARNING_INVALID_SAMPLE);
          }
          else if (cmd >= 0x70 && cmd <= 0x7f)//ornament
          {
            Log::Assert(channelWarner, !channel->FindCommand(ENVELOPE), Text::WARNING_DUPLICATE_ENVELOPE);
            channel->Commands.push_back(STCTrack::Command(NOENVELOPE));
            Log::Assert(channelWarner, !channel->OrnamentNum, Text::WARNING_DUPLICATE_ORNAMENT);
            const uint_t num = cmd - 0x70;
            channel->OrnamentNum = num;
            Log::Assert(channelWarner, !(num && Data.Ornaments[num].Data.empty()), Text::WARNING_INVALID_ORNAMENT);
          }
          else if (cmd == 0x80)//reset
          {
            Log::Assert(channelWarner, !channel->Enabled, Text::WARNING_DUPLICATE_STATE);
            channel->Enabled = false;
            break;
          }
          else if (cmd == 0x81)//empty
          {
            break;
          }
          else if (cmd >= 0x82 && cmd <= 0x8e)//orn 0, without/with envelope
          {
            Log::Assert(channelWarner, !channel->OrnamentNum, Text::WARNING_DUPLICATE_ORNAMENT);
            channel->OrnamentNum = 0;
            Log::Assert(channelWarner, !channel->FindCommand(ENVELOPE) && !channel->FindCommand(NOENVELOPE),
              Text::WARNING_DUPLICATE_ENVELOPE);
            if (cmd == 0x82)
            {
              channel->Commands.push_back(STCTrack::Command(NOENVELOPE));
            }
            else if (!restbytes)
            {
              throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
            }
            else
            {
              channel->Commands.push_back(STCTrack::Command(ENVELOPE, cmd - 0x80, data[cur->Offset++]));
            }
          }
          else if (cmd < 0xa1 || !restbytes)
          {
            throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
          }
          else //skip
          {
            cur->Period = cmd - 0xa1;
          }
        }
        cur->Counter = cur->Period;
      }
    }
  public:
    STCHolder(const MetaContainer& container, ModuleRegion& region)
    {
      //assume that data is ok
      const IO::FastDump& data = IO::FastDump(*container.Data, region.Offset);
      const STCHeader* const header(safe_ptr_cast<const STCHeader*>(data.Data()));
      const STCSample* const samples(safe_ptr_cast<const STCSample*>(header + 1));
      const STCPositions* const positions(safe_ptr_cast<const STCPositions*>(&data[fromLE(header->PositionsOffset)]));
      const STCOrnament* const ornaments(safe_ptr_cast<const STCOrnament*>(&data[fromLE(header->OrnamentsOffset)]));
      const STCPattern* const patterns(safe_ptr_cast<const STCPattern*>(&data[fromLE(header->PatternsOffset)]));

      Log::MessagesCollector::Ptr warner(Log::MessagesCollector::Create());

      //parse samples
      Data.Samples.resize(MAX_SAMPLES_COUNT);
      for (const STCSample* sample = samples; static_cast<const void*>(sample) < static_cast<const void*>(positions); ++sample)
      {
        if (sample->Number >= Data.Samples.size())
        {
          warner->AddMessage(Text::WARNING_INVALID_SAMPLE);
          continue;
        }
        Data.Samples[sample->Number] = Sample(*sample);
      }

      //parse ornaments
      Data.Ornaments.resize(MAX_ORNAMENTS_COUNT);
      for (const STCOrnament* ornament = ornaments; static_cast<const void*>(ornament) < static_cast<const void*>(patterns); ++ornament)
      {
        if (ornament->Number >= Data.Ornaments.size())
        {
          warner->AddMessage(Text::WARNING_INVALID_ORNAMENT);
          continue;
        }
        Data.Ornaments[ornament->Number] =
          STCTrack::Ornament(ornament->Data, ArrayEnd(ornament->Data), ArraySize(ornament->Data));
      }

      //parse patterns
      std::size_t rawSize(0);
      Data.Patterns.resize(MAX_PATTERN_COUNT);
      for (const STCPattern* pattern = patterns; *pattern; ++pattern)
      {
        if (!pattern->Number || pattern->Number >= MAX_PATTERN_COUNT)
        {
          warner->AddMessage(Text::WARNING_INVALID_PATTERN);
          continue;
        }
        Log::ParamPrefixedCollector patternWarner(*warner, Text::PATTERN_WARN_PREFIX, pattern->Number - 1);
        STCTrack::Pattern& pat(Data.Patterns[pattern->Number - 1]);
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
          pat.push_back(STCTrack::Line());
          STCTrack::Line& line(pat.back());
          ParsePattern(data, cursors, line, patLineWarner);
          //skip lines
          if (const uint_t linesToSkip = std::min_element(cursors.begin(), cursors.end(), PatternCursor::CompareByCounter)->Counter)
          {
            std::for_each(cursors.begin(), cursors.end(), std::bind2nd(std::mem_fun_ref(&PatternCursor::SkipLines), linesToSkip));
            pat.resize(pat.size() + linesToSkip);//add dummies
          }
        }
        while (0xff != data[cursors.front().Offset] || cursors.front().Counter);
        Log::Assert(patternWarner, 0 == std::max_element(cursors.begin(), cursors.end(), PatternCursor::CompareByCounter)->Counter,
          Text::WARNING_PERIODS);
        Log::Assert(patternWarner, pat.size() <= MAX_PATTERN_SIZE, Text::WARNING_INVALID_PATTERN_SIZE);
        rawSize = std::max<std::size_t>(rawSize, 1 + std::max_element(cursors.begin(), cursors.end(), PatternCursor::CompareByOffset)->Offset);
      }
      //parse positions
      Data.Positions.reserve(positions->Lenght + 1);
      Transpositions.reserve(positions->Lenght + 1);
      for (const STCPositions::STCPosEntry* posEntry(positions->Data);
        static_cast<const void*>(posEntry) < static_cast<const void*>(ornaments); ++posEntry)
      {
        const uint_t pattern = posEntry->PatternNum - 1;
        if (pattern < MAX_PATTERN_COUNT && !Data.Patterns[pattern].empty())
        {
          Data.Positions.push_back(pattern);
          Transpositions.push_back(posEntry->PatternHeight);
        }
      }
      Log::Assert(*warner, Data.Positions.size() == uint_t(positions->Lenght + 1), Text::WARNING_INVALID_POSITIONS);

      //fix samples and ornaments
      std::for_each(Data.Ornaments.begin(), Data.Ornaments.end(), std::mem_fun_ref(&STCTrack::Ornament::Fix));
      std::for_each(Data.Samples.begin(), Data.Samples.end(), std::mem_fun_ref(&STCTrack::Sample::Fix));
      
      //fill region
      region.Size = rawSize;
      
      //meta properties
      ExtractMetaProperties(STC_PLUGIN_ID, container, region, ModuleRegion(sizeof(STCHeader), rawSize - sizeof(STCHeader)),
        Data.Info.Properties, RawData);
      const String& prog(OptimizeString(FromStdString(header->Identifier)));
      if (!prog.empty())
      {
        Data.Info.Properties.insert(Parameters::Map::value_type(Module::ATTR_PROGRAM, prog));
      }
      
      //tracking properties
      Data.Info.LoopPosition = 0;//not supported here
      Data.Info.PhysicalChannels = AYM::CHANNELS;
      Data.Info.Statistic.Tempo = header->Tempo;
      Data.Info.Statistic.Position = Data.Positions.size();
      Data.Info.Statistic.Pattern = std::count_if(Data.Patterns.begin(), Data.Patterns.end(),
        !boost::bind(&STCTrack::Pattern::empty, _1));
      Data.Info.Statistic.Channels = AYM::CHANNELS;
      STCTrack::CalculateTimings(Data, Data.Info.Statistic.Frame, Data.Info.LoopFrame);
      if (const uint_t msgs = warner->CountMessages())
      {
        Data.Info.Properties.insert(Parameters::Map::value_type(Module::ATTR_WARNINGS_COUNT, msgs));
        Data.Info.Properties.insert(Parameters::Map::value_type(Module::ATTR_WARNINGS, warner->GetMessages('\n')));
      }
    }

    virtual void GetPluginInformation(PluginInformation& info) const
    {
      DescribeSTCPlugin(info);
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
      return CreateSTCPlayer(shared_from_this(), Data, Transpositions, AYM::CreateChip());
    }
    
    virtual Error Convert(const Conversion::Parameter& param, Dump& dst) const
    {
      using namespace Conversion;
      Error result;
      if (parameter_cast<RawConvertParam>(&param))
      {
        dst = RawData;
      }
      else if (!ConvertAYMFormat(boost::bind(&CreateSTCPlayer, shared_from_this(), boost::cref(Data), boost::cref(Transpositions), _1),
        param, dst, result))
      {
        return Error(THIS_LINE, ERROR_MODULE_CONVERT, Text::MODULE_ERROR_CONVERSION_UNSUPPORTED);
      }
      return result;
    }
  private:
    Dump RawData;
    STCTrack::ModuleData Data;
    STCTransposition Transpositions;
  };

  struct STCChannelState
  {
    STCChannelState()
      : Enabled(false), Envelope(false)
      , Note(), SampleNum(0), PosInSample(0)
      , OrnamentNum(0), LoopedInSample(false)
    {
    }
    bool Enabled;
    bool Envelope;
    uint_t Note;
    uint_t SampleNum;
    uint_t PosInSample;
    uint_t OrnamentNum;
    bool LoopedInSample;
  };

  typedef AYMPlayer<STCTrack, boost::array<STCChannelState, AYM::CHANNELS> > STCPlayerBase;

  class STCPlayer : public STCPlayerBase
  {
  public:
    STCPlayer(Holder::ConstPtr holder, const STCTrack::ModuleData& data,
      const STCTransposition& transpositions, AYM::Chip::Ptr device)
      : STCPlayerBase(holder, data, device, TABLE_SOUNDTRACKER)
      , Transpositions(transpositions)
    {
#ifdef SELF_TEST
//perform self-test
      AYM::DataChunk chunk;
      do
      {
        assert(Data.Positions.size() > ModState.Track.Position);
        RenderData(chunk);
      }
      while (STCTrack::UpdateState(Data, ModState, Sound::LOOP_NONE));
      Reset();
#endif
    }

    virtual void RenderData(AYM::DataChunk& chunk)
    {
      const STCTrack::Line& line(Data.Patterns[ModState.Track.Pattern][ModState.Track.Line]);
      if (0 == ModState.Track.Frame)//begin note
      {
        for (uint_t chan = 0; chan != line.Channels.size(); ++chan)
        {
          const STCTrack::Line::Chan& src(line.Channels[chan]);
          STCChannelState& dst(PlayerState[chan]);
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
          for (STCTrack::CommandsArray::const_iterator it = src.Commands.begin(), lim = src.Commands.end(); it != lim; ++it)
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
      for (uint_t chan = 0; chan < AYM::CHANNELS; ++chan)
      {
        ApplyData(chan, chunk);
      }
      //count actually enabled channels
      ModState.Track.Channels = std::count_if(PlayerState.begin(), PlayerState.end(), boost::mem_fn(&STCChannelState::Enabled));
    }
     
    void ApplyData(uint_t chan, AYM::DataChunk& chunk)
    {
      STCChannelState& dst(PlayerState[chan]);
      const uint_t toneReg(AYM::DataChunk::REG_TONEA_L + 2 * chan);
      const uint_t volReg = AYM::DataChunk::REG_VOLA + chan;
      const uint_t toneMsk = AYM::DataChunk::REG_MASK_TONEA << chan;
      const uint_t noiseMsk = AYM::DataChunk::REG_MASK_NOISEA << chan;

      const FrequencyTable& freqTable(AYMHelper->GetFreqTable());
      if (dst.Enabled)
      {
        const STCTrack::Sample& curSample(Data.Samples[dst.SampleNum]);
        const STCTrack::Sample::Line& curSampleLine(curSample.Data[dst.PosInSample]);
        const STCTrack::Ornament& curOrnament(Data.Ornaments[dst.OrnamentNum]);

        //calculate tone
        const uint_t halfTone = static_cast<uint_t>(clamp<int_t>(int_t(dst.Note) + curOrnament.Data[dst.PosInSample] + Transpositions[ModState.Track.Position],
          0, freqTable.size() - 1));
        const uint_t tone = static_cast<uint_t>(clamp<int_t>(int_t(freqTable[halfTone]) + curSampleLine.Effect, 0, 0xfff));

        chunk.Data[toneReg] = static_cast<uint8_t>(tone & 0xff);
        chunk.Data[toneReg + 1] = static_cast<uint8_t>(tone >> 8);
        chunk.Mask |= 3 << toneReg;
        //calculate level
        chunk.Data[volReg] = static_cast<uint8_t>(curSampleLine.Level | (dst.Envelope ? AYM::DataChunk::REG_MASK_ENV : 0));
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
          chunk.Data[AYM::DataChunk::REG_TONEN] = static_cast<uint8_t>(curSampleLine.Noise);
          chunk.Mask |= 1 << AYM::DataChunk::REG_TONEN;
        }

        if (++dst.PosInSample >= (dst.LoopedInSample ? curSample.LoopLimit : curSample.Data.size()))
        {
          if (curSample.Loop && curSample.Loop < curSample.Data.size())
          {
            dst.PosInSample = curSample.Loop;
            dst.LoopedInSample = true;
          }
          else
          {
            dst.Enabled = false;
          }
        }
      }
      else//not enabled
      {
        chunk.Data[volReg] = 0;
        //????
        chunk.Data[AYM::DataChunk::REG_MIXER] |= toneMsk | noiseMsk;
      }
    }
  private:
    const std::vector<int_t>& Transpositions;
  };
  
  Player::Ptr CreateSTCPlayer(Holder::ConstPtr mod, const STCTrack::ModuleData& data,
    const STCTransposition& transpositions, AYM::Chip::Ptr device)
  {
    return Player::Ptr(new STCPlayer(mod, data, transpositions, device));
  }

  bool CheckSTCModule(const uint8_t* data, std::size_t limit, const MetaContainer& container,
    Holder::Ptr& holder, ModuleRegion& region)
  {
    if (limit < sizeof(STCHeader))
    {
      return false;
    }
    const STCHeader* const header(safe_ptr_cast<const STCHeader*>(data));
    const std::size_t samOff(sizeof(STCHeader));
    const std::size_t posOff(fromLE(header->PositionsOffset));
    const std::size_t ornOff(fromLE(header->OrnamentsOffset));
    const std::size_t patOff(fromLE(header->PatternsOffset));
    //check offsets
    if (posOff > limit || ornOff > limit || patOff > limit)
    {
      return false;
    }
    const STCPositions* const positions(safe_ptr_cast<const STCPositions*>(data + posOff));
    const STCOrnament* const ornaments(safe_ptr_cast<const STCOrnament*>(data + ornOff));
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
    //check patterns
    for (const STCPattern* pattern = safe_ptr_cast<const STCPattern*>(data + patOff); *pattern; ++pattern)
    {
      if (!pattern->Number || pattern->Number >= MAX_PATTERN_COUNT)
      {
        continue;
      }
      if (pattern->Offsets.end() != std::find_if(pattern->Offsets.begin(), pattern->Offsets.end(),
        boost::bind(&fromLE<uint16_t>, _1) > limit - 1))
      {
        return false;
      }
    }
    //try to create holder
    try
    {
      holder.reset(new STCHolder(container, region));
#ifdef SELF_TEST
      holder->CreatePlayer();
#endif
      return true;
    }
    catch (const Error&/*e*/)
    {
      Log::Debug("STCSupp", "Failed to create holder");
    }
    return false;
  }

  //////////////////////////////////////////////////////////////////////////
  bool CreateSTCModule(const Parameters::Map& /*commonParams*/, const MetaContainer& container,
    Holder::Ptr& holder, ModuleRegion& region)
  {
    return PerformDetect(&CheckSTCModule, DETECTORS, ArrayEnd(DETECTORS),
      container, holder, region);
  }
}

namespace ZXTune
{
  void RegisterSTCSupport(PluginsEnumerator& enumerator)
  {
    PluginInformation info;
    DescribeSTCPlugin(info);
    enumerator.RegisterPlayerPlugin(info, CreateSTCModule);
  }
}
