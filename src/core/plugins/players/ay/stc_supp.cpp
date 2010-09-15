/*
Abstract:
  STC modules playback support

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
#include <boost/make_shared.hpp>
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
    Sample() : Loop(), LoopLimit(), Lines()
    {
    }

    explicit Sample(const STCSample& sample)
      : Loop(std::min<uint_t>(sample.Loop, ArraySize(sample.Data)))
      , LoopLimit(std::min<uint_t>(sample.Loop + sample.LoopSize + 1, ArraySize(sample.Data)))
      , Lines(sample.Data, ArrayEnd(sample.Data))
    {
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

  enum CmdType
  {
    EMPTY,
    ENVELOPE,     //2p
    NOENVELOPE,   //0p
  };

  typedef TrackingSupport<AYM::CHANNELS, Sample> STCTrack;
  typedef std::vector<int_t> STCTransposition;

  struct STCModuleData : public STCTrack::ModuleData
  {
    typedef boost::shared_ptr<STCModuleData> RWPtr;
    typedef boost::shared_ptr<const STCModuleData> Ptr;

    STCModuleData()
      : STCTrack::ModuleData()
    {
    }

    STCTransposition Transpositions;
  };

  // forward declaration
  Player::Ptr CreateSTCPlayer(Information::Ptr info, STCModuleData::Ptr data, AYM::Chip::Ptr device);

  class STCHolder : public Holder
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
            Log::Assert(channelWarner, !num || Data->Samples[num].GetSize(), Text::WARNING_INVALID_SAMPLE);
          }
          else if (cmd >= 0x70 && cmd <= 0x7f)//ornament
          {
            Log::Assert(channelWarner, !channel->FindCommand(ENVELOPE), Text::WARNING_DUPLICATE_ENVELOPE);
            channel->Commands.push_back(STCTrack::Command(NOENVELOPE));
            Log::Assert(channelWarner, !channel->OrnamentNum, Text::WARNING_DUPLICATE_ORNAMENT);
            const uint_t num = cmd - 0x70;
            channel->OrnamentNum = num;
            Log::Assert(channelWarner, !num || Data->Ornaments[num].GetSize(), Text::WARNING_INVALID_ORNAMENT);
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
    STCHolder(Plugin::Ptr plugin, const MetaContainer& container, ModuleRegion& region)
      : SrcPlugin(plugin)
      , Data(boost::make_shared<STCModuleData>())
      , Info(STCTrack::ModuleInfo::Create(Data))
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
      Data->Samples.resize(MAX_SAMPLES_COUNT);
      for (const STCSample* sample = samples; static_cast<const void*>(sample) < static_cast<const void*>(positions); ++sample)
      {
        if (sample->Number >= Data->Samples.size())
        {
          warner->AddMessage(Text::WARNING_INVALID_SAMPLE);
          continue;
        }
        Data->Samples[sample->Number] = Sample(*sample);
      }

      //parse ornaments
      Data->Ornaments.resize(MAX_ORNAMENTS_COUNT);
      for (const STCOrnament* ornament = ornaments; static_cast<const void*>(ornament) < static_cast<const void*>(patterns); ++ornament)
      {
        if (ornament->Number >= Data->Ornaments.size())
        {
          warner->AddMessage(Text::WARNING_INVALID_ORNAMENT);
          continue;
        }
        Data->Ornaments[ornament->Number] =
          STCTrack::Ornament(ArraySize(ornament->Data), ornament->Data, ArrayEnd(ornament->Data));
      }

      //parse patterns
      std::size_t rawSize(0);
      Data->Patterns.resize(MAX_PATTERN_COUNT);
      for (const STCPattern* pattern = patterns; *pattern; ++pattern)
      {
        if (!pattern->Number || pattern->Number >= MAX_PATTERN_COUNT)
        {
          warner->AddMessage(Text::WARNING_INVALID_PATTERN);
          continue;
        }
        Log::ParamPrefixedCollector patternWarner(*warner, Text::PATTERN_WARN_PREFIX, pattern->Number - 1);
        STCTrack::Pattern& pat(Data->Patterns[pattern->Number - 1]);
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
      Data->Positions.reserve(positions->Lenght + 1);
      Data->Transpositions.reserve(positions->Lenght + 1);
      for (const STCPositions::STCPosEntry* posEntry(positions->Data);
        static_cast<const void*>(posEntry) < static_cast<const void*>(ornaments); ++posEntry)
      {
        const uint_t pattern = posEntry->PatternNum - 1;
        if (pattern < MAX_PATTERN_COUNT && !Data->Patterns[pattern].empty())
        {
          Data->Positions.push_back(pattern);
          Data->Transpositions.push_back(posEntry->PatternHeight);
        }
      }
      Log::Assert(*warner, Data->Positions.size() == uint_t(positions->Lenght + 1), Text::WARNING_INVALID_POSITIONS);

      //fill region
      region.Size = rawSize;
      RawData = region.Extract(*container.Data);
      
      //meta properties
      const ModuleProperties::Ptr props = ModuleProperties::Create(STC_PLUGIN_ID);
      {
        const ModuleRegion fixedRegion(sizeof(STCHeader), rawSize - sizeof(STCHeader));
        props->SetSource(RawData, fixedRegion);
      }
      props->SetPlugins(container.Plugins);
      props->SetPath(container.Path);
      props->SetProgram(OptimizeString(FromStdString(header->Identifier)));
      props->SetWarnings(warner);

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
      return CreateSTCPlayer(Info, Data, AYM::CreateChip());
    }
    
    virtual Error Convert(const Conversion::Parameter& param, Dump& dst) const
    {
      using namespace Conversion;
      Error result;
      if (parameter_cast<RawConvertParam>(&param))
      {
        const uint8_t* const data = static_cast<const uint8_t*>(RawData->Data());
        dst.assign(data, data + RawData->Size());
      }
      else if (!ConvertAYMFormat(boost::bind(&CreateSTCPlayer, boost::cref(Info), boost::cref(Data), _1),
        param, dst, result))
      {
        return Error(THIS_LINE, ERROR_MODULE_CONVERT, Text::MODULE_ERROR_CONVERSION_UNSUPPORTED);
      }
      return result;
    }
  private:
    const Plugin::Ptr SrcPlugin;
    const STCModuleData::RWPtr Data;
    const STCTrack::ModuleInfo::Ptr Info;
    IO::DataContainer::Ptr RawData;
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

  typedef AYMPlayer<STCModuleData, boost::array<STCChannelState, AYM::CHANNELS> > STCPlayerBase;

  class STCPlayer : public STCPlayerBase
  {
  public:
    STCPlayer(Information::Ptr info, STCModuleData::Ptr data, AYM::Chip::Ptr device)
      : STCPlayerBase(info, data, device, TABLE_SOUNDTRACKER)
    {
#ifdef SELF_TEST
//perform self-test
      AYMTrackSynthesizer synthesizer(*AYMHelper);
      do
      {
        assert(Data->Positions.size() > ModState.Track.Position);
        SynthesizeData(synthesizer);
      }
      while (Data->UpdateState(*Info, Sound::LOOP_NONE, ModState));
      Reset();
#endif
    }

    virtual void SynthesizeData(AYMTrackSynthesizer& synthesizer)
    {
      const STCTrack::Line& line(Data->Patterns[ModState.Track.Pattern][ModState.Track.Line]);
      if (0 == ModState.Track.Quirk)//begin note
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
              synthesizer.SetEnvelope(it->Param1, it->Param2);
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
      for (uint_t chan = 0; chan < AYM::CHANNELS; ++chan)
      {
        SynthesizeChannel(chan, synthesizer);
      }
      //count actually enabled channels
      ModState.Track.Channels = std::count_if(PlayerState.begin(), PlayerState.end(), boost::mem_fn(&STCChannelState::Enabled));
    }
     
    void SynthesizeChannel(uint_t chan, AYMTrackSynthesizer& synthesizer)
    {
      STCChannelState& dst(PlayerState[chan]);

      if (!dst.Enabled)
      {
        synthesizer.SetLevel(chan, 0);
        return;
      }

      const STCTrack::Sample& curSample = Data->Samples[dst.SampleNum];
      const STCTrack::Sample::Line& curSampleLine = curSample.GetLine(dst.PosInSample);
      const STCTrack::Ornament& curOrnament = Data->Ornaments[dst.OrnamentNum];

      //apply level
      synthesizer.SetLevel(chan, curSampleLine.Level);
      //apply envelope
      if (dst.Envelope)
      {
        synthesizer.EnableEnvelope(chan);
      }
      //apply tone
      if (!curSampleLine.EnvelopeMask)
      {
        //apply tone
        const int_t halftones = int_t(dst.Note) + curOrnament.GetLine(dst.PosInSample) + Data->Transpositions[ModState.Track.Position];
        synthesizer.SetTone(chan, halftones, curSampleLine.Effect);
      }
      //apply noise
      if (!curSampleLine.NoiseMask)
      {
        synthesizer.SetNoise(chan, curSampleLine.Noise);
      }

      if (++dst.PosInSample >= (dst.LoopedInSample ? curSample.GetLoopLimit() : curSample.GetSize()))
      {
        if (curSample.GetLoop() && curSample.GetLoop() < curSample.GetSize())
        {
          dst.PosInSample = curSample.GetLoop();
          dst.LoopedInSample = true;
        }
        else
        {
          dst.Enabled = false;
        }
      }
    }
  };
  
  Player::Ptr CreateSTCPlayer(Information::Ptr info, STCModuleData::Ptr data, AYM::Chip::Ptr device)
  {
    return Player::Ptr(new STCPlayer(info, data, device));
  }

  bool CheckSTCModule(const uint8_t* data, std::size_t limit) 
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
    return true;
  }
  
  Holder::Ptr CreateSTCModule(Plugin::Ptr plugin, const MetaContainer& container, ModuleRegion& region)
  {
    try
    {
      const Holder::Ptr holder(new STCHolder(plugin, container, region));
#ifdef SELF_TEST
      holder->CreatePlayer();
#endif
      return holder;
    }
    catch (const Error&/*e*/)
    {
      Log::Debug("Core::STCSupp", "Failed to create holder");
    }
    return Holder::Ptr();
  }

  //////////////////////////////////////////////////////////////////////////
  class STCPlugin : public PlayerPlugin
                  , public boost::enable_shared_from_this<STCPlugin>
  {
  public:
    virtual String Id() const
    {
      return STC_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::STC_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return STC_PLUGIN_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | GetSupportedAYMFormatConvertors();
    }

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      return PerformCheck(&CheckSTCModule, DETECTORS, ArrayEnd(DETECTORS), inputData);
    }

    virtual Module::Holder::Ptr CreateModule(const Parameters::Accessor& /*parameters*/,
                                             const MetaContainer& container,
                                             ModuleRegion& region) const
    {
      const Plugin::Ptr plugin = shared_from_this();
      return PerformCreate(&CheckSTCModule, &CreateSTCModule, DETECTORS, ArrayEnd(DETECTORS),
        plugin, container, region);
    }
  };
}

namespace ZXTune
{
  void RegisterSTCSupport(PluginsEnumerator& enumerator)
  {
    const PlayerPlugin::Ptr plugin(new STCPlugin());
    enumerator.RegisterPlugin(plugin);
  }
}
