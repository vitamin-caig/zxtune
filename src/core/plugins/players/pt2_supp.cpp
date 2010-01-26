/*
Abstract:
  PT2 modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "freq_tables_internal.h"
#include "tracking.h"
#include "utils.h"
#include "../detector.h"
#include "../enumerator.h"

#include <byteorder.h>
#include <error_tools.h>
#include <messages_collector.h>
#include <tools.h>
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <core/error_codes.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <io/container.h>
#include <sound/dummy_receiver.h>
#include <sound/render_params.h>
#include <core/devices/aym.h>

#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <text/core.h>
#include <text/plugins.h>
#include <text/warnings.h>

#define FILE_TAG 077C8579

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  const Char PT2_PLUGIN_ID[] = {'P', 'T', '2', 0};
  const String TEXT_PT2_VERSION(FromChar("Revision: $Rev$"));

  const std::size_t LIMITER(~0u);

  //hints
  const std::size_t MAX_MODULE_SIZE = 16384;
  const std::size_t MAX_PATTERN_SIZE = 64;
  const std::size_t MAX_PATTERN_COUNT = 64;//TODO


  //checkers
  static const DetectFormatChain DETECTORS[] = {
    /*PT20
    {
      "21??c3??c3+563+f3e522??e57e32",

    },
    */
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
    boost::array<char, 30> Name;
    uint8_t Positions[1];
  } PACK_POST;

  const uint8_t POS_END_MARKER = 0xff;

  PACK_PRE struct PT2Sample
  {
    uint8_t Size;
    uint8_t Loop;
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
      
      unsigned GetNoise() const
      {
        return NoiseAndFlags >> 3;
      }
      
      unsigned GetLevel() const
      {
        return LevelHiVibrato >> 4;
      }
      
      signed GetVibrato() const
      {
        const signed val(((LevelHiVibrato & 0x0f) << 8) | LoVibrato);
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

  struct Sample
  {
    Sample() : Loop(), Data()
    {
    }

    Sample(unsigned size, unsigned loop) : Loop(loop), Data(size)
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
      unsigned Level;//0-15
      unsigned Noise;//0-31
      bool ToneOff;
      bool NoiseOff;
      signed Vibrato;
    };
    
    unsigned Loop;
    std::vector<Line> Data;
  };

  inline Sample CreateSample(const IO::FastDump& data, uint16_t offset)
  {
    const PT2Sample* const sample(safe_ptr_cast<const PT2Sample*>(&data[fromLE(offset)]));
    if (0 == offset || !sample->Size)
    {
      return Sample(1, 0);//safe
    }
    Sample tmp(sample->Size, sample->Loop);
    std::copy(sample->Data, sample->Data + sample->Size, tmp.Data.begin());
    return tmp;
  }
  
  inline SimpleOrnament CreateOrnament(const IO::FastDump& data, uint16_t offset)
  {
    const PT2Ornament* const ornament(safe_ptr_cast<const PT2Ornament*>(&data[fromLE(offset)]));
    if (0 == offset || !ornament->Size)
    {
      return SimpleOrnament(1, 0);//safe version
    }
    return SimpleOrnament(ornament->Data, ornament->Data + ornament->Size, ornament->Loop);
  }

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

  void DescribePT2Plugin(PluginInformation& info)
  {
    info.Id = PT2_PLUGIN_ID;
    info.Description = TEXT_PT2_INFO;
    info.Version = TEXT_PT2_VERSION;
    info.Capabilities = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | CAP_CONV_PSG;
  }

  typedef TrackingSupport<AYM::CHANNELS, Sample> PT2Track;
  
  // perform module 'playback' right after creating (debug purposes)
  #ifndef NDEBUG
  #define SELF_TEST
  #endif
  
  // forward declaration
  class PT2Holder;
  Player::Ptr CreatePT2Player(boost::shared_ptr<const PT2Holder> mod, AYM::Chip::Ptr device);

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

        Log::ParamPrefixedCollector channelWarner(warner, TEXT_CHANNEL_WARN_PREFIX,
          static_cast<unsigned>(std::distance(line.Channels.begin(), channel)));
        for (;;)
        {
          const uint8_t cmd(data[cur->Offset++]);
          const std::size_t restbytes = data.Size() - cur->Offset;
          if (cmd >= 0xe1) //sample
          {
            Log::Assert(channelWarner, !channel->SampleNum, TEXT_WARNING_DUPLICATE_SAMPLE);
            const unsigned num = cmd - 0xe0;
            channel->SampleNum = num;
            Log::Assert(channelWarner, !Data.Samples[num].Data.empty(), TEXT_WARNING_INVALID_SAMPLE);
          }
          else if (cmd == 0xe0) //sample 0 - shut up
          {
            Log::Assert(channelWarner, !channel->Enabled, TEXT_WARNING_DUPLICATE_STATE);
            channel->Enabled = false;
            break;
          }
          else if (cmd >= 0x80 && cmd <= 0xdf)//note
          {
            Log::Assert(channelWarner, !channel->Enabled, TEXT_WARNING_DUPLICATE_STATE);
            channel->Enabled = true;
            const unsigned note(cmd - 0x80);
            //for note gliss calculate limit manually
            const PT2Track::CommandsArray::iterator noteGlissCmd(
              std::find(channel->Commands.begin(), channel->Commands.end(), GLISS_NOTE));
            if (channel->Commands.end() != noteGlissCmd)
            {
              noteGlissCmd->Param2 = int(note);
            }
            else
            {
              Log::Assert(channelWarner, !channel->Note, TEXT_WARNING_DUPLICATE_NOTE);
              channel->Note = note;
            }
            break;
          }
          else if (cmd == 0x7f) //env off
          {
            Log::Assert(channelWarner, !channel->FindCommand(ENVELOPE) && !channel->FindCommand(NOENVELOPE),
              TEXT_WARNING_DUPLICATE_ENVELOPE);
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
              TEXT_WARNING_DUPLICATE_ENVELOPE);
            channel->Commands.push_back(
              PT2Track::Command(ENVELOPE, cmd - 0x70, data[cur->Offset] | (unsigned(data[cur->Offset + 1]) << 8)));
            cur->Offset += 2;
          }
          else if (cmd == 0x70)//quit
          {
            break;
          }
          else if (cmd >= 0x60 && cmd <= 0x6f)//ornament
          {
            Log::Assert(channelWarner, !channel->OrnamentNum, TEXT_WARNING_DUPLICATE_ORNAMENT);
            const unsigned num = cmd - 0x60;
            channel->OrnamentNum = num;
            Log::Assert(channelWarner, !(num && Data.Ornaments[num].Data.empty()), TEXT_WARNING_INVALID_ORNAMENT);
          }
          else if (cmd >= 0x20 && cmd <= 0x5f)//skip
          {
            cur->Period = cmd - 0x20;
          }
          else if (cmd >= 0x10 && cmd <= 0x1f)//volume
          {
            Log::Assert(channelWarner, !channel->Volume, TEXT_WARNING_DUPLICATE_VOLUME);
            channel->Volume = cmd - 0x10;
          }
          else if (cmd == 0x0f)//new delay
          {
            //required 1 byte
            if (restbytes < 1)
            {
              throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
            }
            Log::Assert(channelWarner, !line.Tempo, TEXT_WARNING_DUPLICATE_TEMPO);
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
            Log::Assert(channelWarner, !channel->Note, TEXT_WARNING_INVALID_NOTE_GLISS);
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
            channel->Commands.push_back(PT2Track::Command(NOISE_ADD, static_cast<int8_t>(data[cur->Offset])));
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
      const PT2Header* const header(safe_ptr_cast<const PT2Header*>(&data[0]));
      const PT2Pattern* patterns(safe_ptr_cast<const PT2Pattern*>(&data[fromLE(header->PatternsOffset)]));

      Log::MessagesCollector::Ptr warner(Log::MessagesCollector::Create());

      //fill samples
      Data.Samples.reserve(header->SamplesOffsets.size());
      std::transform(header->SamplesOffsets.begin(), header->SamplesOffsets.end(),
        std::back_inserter(Data.Samples), boost::bind(&CreateSample, boost::cref(data), _1));
      //fill ornaments
      Data.Ornaments.reserve(header->OrnamentsOffsets.size());
      std::transform(header->OrnamentsOffsets.begin(), header->OrnamentsOffsets.end(),
        std::back_inserter(Data.Ornaments), boost::bind(&CreateOrnament, boost::cref(data), _1));
        
      //fill patterns
      std::size_t rawSize(0);
      Data.Patterns.resize(MAX_PATTERN_COUNT);
      unsigned index(0);
      for (const PT2Pattern* pattern = patterns; pattern->Check(); ++pattern, ++index)
      {
        Log::ParamPrefixedCollector patternWarner(*warner, TEXT_PATTERN_WARN_PREFIX, index);
        PT2Track::Pattern& pat(Data.Patterns[index]);
        
        PatternCursors cursors;
        std::transform(pattern->Offsets.begin(), pattern->Offsets.end(), cursors.begin(), &fromLE<uint16_t>);
        pat.reserve(MAX_PATTERN_SIZE);
        do
        {
          Log::ParamPrefixedCollector patLineWarner(patternWarner, TEXT_LINE_WARN_PREFIX, static_cast<unsigned>(pat.size()));
          pat.push_back(PT2Track::Line());
          PT2Track::Line& line(pat.back());
          ParsePattern(data, cursors, line, patLineWarner);
          //skip lines
          if (const unsigned linesToSkip = std::min_element(cursors.begin(), cursors.end(), PatternCursor::CompareByCounter)->Counter)
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
        rawSize = std::max<std::size_t>(rawSize, 1 + std::max_element(cursors.begin(), cursors.end(), PatternCursor::CompareByOffset)->Offset);
      }

      //fill order
      Data.Positions.assign(header->Positions,
        std::find(header->Positions, header->Positions + header->Length, POS_END_MARKER));
      Log::Assert(*warner, header->Length == Data.Positions.size(), TEXT_WARNING_INVALID_LENGTH);

      //fix samples and ornaments
      std::for_each(Data.Ornaments.begin(), Data.Ornaments.end(), std::mem_fun_ref(&PT2Track::Ornament::Fix));
      std::for_each(Data.Samples.begin(), Data.Samples.end(), std::mem_fun_ref(&PT2Track::Sample::Fix));
      
      //fill region
      region.Size = rawSize;
      
      //meta properties
      ExtractMetaProperties(PT2_PLUGIN_ID, container, region, Data.Info.Properties, RawData);
      const String& title(OptimizeString(String(header->Name.begin(), header->Name.end())));
      if (!title.empty())
      {
        Data.Info.Properties.insert(StringMap::value_type(Module::ATTR_TITLE, title));
      }
      
      //tracking properties
      Data.Info.LoopPosition = header->Loop;
      Data.Info.PhysicalChannels = AYM::CHANNELS;
      Data.Info.Statistic.Tempo = header->Tempo;
      Data.Info.Statistic.Position = static_cast<unsigned>(Data.Positions.size());
      Data.Info.Statistic.Pattern = static_cast<unsigned>(std::count_if(Data.Patterns.begin(), Data.Patterns.end(),
        !boost::bind(&PT2Track::Pattern::empty, _1)));
      Data.Info.Statistic.Channels = AYM::CHANNELS;
      PT2Track::CalculateTimings(Data, Data.Info.Statistic.Frame, Data.Info.LoopFrame);
      if (const unsigned msgs = warner->CountMessages())
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
    
    virtual Player::Ptr CreatePlayer() const
    {
      return CreatePT2Player(shared_from_this(), AYM::CreateChip());
    }
    
    virtual Error Convert(const Conversion::Parameter& param, Dump& dst) const
    {
      using namespace Conversion;
      if (parameter_cast<RawConvertParam>(&param))
      {
        dst = RawData;
      }
      else if (parameter_cast<PSGConvertParam>(&param))
      {
        Dump tmp;
        Player::Ptr player(CreatePT2Player(shared_from_this(), AYM::CreatePSGDumper(tmp)));
        Sound::DummyReceiverObject<Sound::MultichannelReceiver> receiver;
        Sound::RenderParameters params;
        for (Player::PlaybackState state = Player::MODULE_PLAYING; Player::MODULE_PLAYING == state;)
        {
          if (const Error& err = player->RenderFrame(params, state, receiver))
          {
            return Error(THIS_LINE, ERROR_MODULE_CONVERT, TEXT_MODULE_ERROR_CONVERT_PSG).AddSuberror(err);
          }
        }
        dst.swap(tmp);
      }
      else
      {
        return Error(THIS_LINE, ERROR_MODULE_CONVERT, TEXT_MODULE_ERROR_CONVERSION_UNSUPPORTED);
      }
      return Error();
    }
  private:
    friend class PT2Player;
    Dump RawData;
    PT2Track::ModuleData Data;
  };

  inline uint8_t GetVolume(unsigned volume, unsigned level)
  {
    return static_cast<uint8_t>((volume * 17 + (volume > 7 ? 1 : 0)) * level >> 8);
  }

  class PT2Player : public Player
  {
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
      unsigned Note;
      unsigned SampleNum;
      unsigned PosInSample;
      unsigned OrnamentNum;
      unsigned PosInOrnament;
      unsigned Volume;
      signed NoiseAdd;
      signed Sliding;
      unsigned SlidingTargetNote;
      signed Glissade;
    };
  public:
    PT2Player(boost::shared_ptr<const PT2Holder> holder, AYM::Chip::Ptr device)
       : Module(holder)
       , Data(Module->Data)
       , YMChip(false)
       , Device(device)
       , CurrentState(MODULE_STOPPED)
    {
      ThrowIfError(GetFreqTable(TABLE_PROTRACKER2, FreqTable));
      Reset();
#ifdef SELF_TEST
//perform self-test
      AYM::DataChunk chunk;
      do
      {
        assert(Data.Positions.size() > ModState.Track.Position);
        RenderData(chunk);
      }
      while (PT2Track::UpdateState(Data, ModState, Sound::LOOP_NONE));
      Reset();
#endif
    }

    virtual const Holder& GetModule() const
    {
      return *Module;
    }

    virtual Error GetPlaybackState(unsigned& timeState,
                                   Tracking& trackState,
                                   Analyze::ChannelsState& analyzeState) const
    {
      timeState = ModState.Frame;
      trackState = ModState.Track;
      Device->GetState(analyzeState);
      return Error();
    }

    virtual Error RenderFrame(const Sound::RenderParameters& params,
                              PlaybackState& state,
                              Sound::MultichannelReceiver& receiver)
    {
      if (ModState.Frame >= Data.Info.Statistic.Frame)
      {
        if (MODULE_STOPPED == CurrentState)
        {
          return Error(THIS_LINE, ERROR_MODULE_END, TEXT_MODULE_ERROR_MODULE_END);
        }
        receiver.Flush();
        state = CurrentState = MODULE_STOPPED;
        return Error();
      }

      AYM::DataChunk chunk;
      ModState.Tick += params.ClocksPerFrame();
      chunk.Tick = ModState.Tick;
      RenderData(chunk);
      if (YMChip)
      {
        chunk.Mask |= AYM::DataChunk::YM_CHIP;
      }
      Device->RenderData(params, chunk, receiver);
      if (PT2Track::UpdateState(Data, ModState, params.Looping))
      {
        CurrentState = MODULE_PLAYING;
      }
      else
      {
        receiver.Flush();
        CurrentState = MODULE_STOPPED;
      }
      state = CurrentState;
      return Error();
    }

    virtual Error Reset()
    {
      Device->Reset();
      PT2Track::InitState(Data, ModState);
      std::fill(ChanState.begin(), ChanState.end(), ChannelState());
      CurrentState = MODULE_STOPPED;
      return Error();
    }

    virtual Error SetPosition(unsigned frame)
    {
      if (frame < ModState.Frame)
      {
        //reset to beginning in case of moving back
        const uint64_t keepTicks = ModState.Tick;
        PT2Track::InitState(Data, ModState);
        std::fill(ChanState.begin(), ChanState.end(), ChannelState());
        ModState.Tick = keepTicks;
      }
      //fast forward
      AYM::DataChunk chunk;
      while (ModState.Frame < frame)
      {
        //do not update tick for proper rendering
        assert(Data.Positions.size() > ModState.Track.Position);
        RenderData(chunk);
        if (!PT2Track::UpdateState(Data, ModState, Sound::LOOP_NONE))
        {
          break;
        }
      }
      return Error();
    }

    virtual Error SetParameters(const Parameters::Map& params)
    {
      try
      {
        Parameters::IntType intParam = 0;
        if (Parameters::FindByName(params, Parameters::ZXTune::Core::AYM::TYPE, intParam))
        {
          YMChip = 0 != (intParam & 1);//only one chip
        }
        if (const Parameters::StringType* const table = Parameters::FindByName<Parameters::StringType>(params,
          Parameters::ZXTune::Core::AYM::TABLE))
        {
          ThrowIfError(GetFreqTable(*table, FreqTable));
        }
        else if (const Parameters::DataType* const table = Parameters::FindByName<Parameters::DataType>(params,
          Parameters::ZXTune::Core::AYM::TABLE))
        {
          if (table->size() != FreqTable.size() * sizeof(FreqTable.front()))
          {
            throw MakeFormattedError(THIS_LINE, ERROR_INVALID_PARAMETERS,
              TEXT_MODULE_ERROR_INVALID_FREQ_TABLE_SIZE, table->size());
          }
          std::memcpy(&FreqTable.front(), &table->front(), table->size());
        }
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, ERROR_INVALID_PARAMETERS, TEXT_MODULE_ERROR_SET_PLAYER_PARAMETERS).AddSuberror(e);
      }
    }
  private:
    void RenderData(AYM::DataChunk& chunk)
    {
      const PT2Track::Line& line(Data.Patterns[ModState.Track.Pattern][ModState.Track.Line]);
      if (0 == ModState.Track.Frame)//begin note
      {
        for (unsigned chan = 0; chan != line.Channels.size(); ++chan)
        {
          const PT2Track::Line::Chan& src(line.Channels[chan]);
          ChannelState& dst(ChanState[chan]);
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
      for (unsigned chan = 0; chan < AYM::CHANNELS; ++chan)
      {
        ApplyData(chan, chunk);
      }
      //count actually enabled channels
      ModState.Track.Channels = static_cast<unsigned>(std::count_if(ChanState.begin(), ChanState.end(),
        boost::mem_fn(&ChannelState::Enabled)));
    }
    
    void ApplyData(unsigned chan, AYM::DataChunk& chunk)
    {
      ChannelState& dst(ChanState[chan]);
      const unsigned toneReg(AYM::DataChunk::REG_TONEA_L + 2 * chan);
      const unsigned volReg = AYM::DataChunk::REG_VOLA + chan;
      const unsigned toneMsk = AYM::DataChunk::MASK_TONEA << chan;
      const unsigned noiseMsk = AYM::DataChunk::MASK_NOISEA << chan;

      if (dst.Enabled && dst.SampleNum)
      {
        const Sample& curSample(Data.Samples[dst.SampleNum]);
        const Sample::Line& curSampleLine(curSample.Data[dst.PosInSample]);
        const SimpleOrnament& curOrnament(Data.Ornaments[dst.OrnamentNum]);

        //calculate tone
        const unsigned halfTone(clamp<unsigned>(dst.Note + curOrnament.Data[dst.PosInOrnament], 0, 95));
        const uint16_t tone = static_cast<uint16_t>(clamp(FreqTable[halfTone] + dst.Sliding + curSampleLine.Vibrato, 0, 0xffff));
        if (dst.SlidingTargetNote != LIMITER)
        {
          const unsigned nextTone(FreqTable[dst.Note] + dst.Sliding + dst.Glissade);
          const unsigned slidingTarget(FreqTable[dst.SlidingTargetNote]);
          if ((dst.Glissade > 0 && nextTone >= slidingTarget) ||
              (dst.Glissade < 0 && nextTone <= slidingTarget))
          {
            dst.Note = dst.SlidingTargetNote;
            dst.SlidingTargetNote = LIMITER;
            dst.Sliding = dst.Glissade = 0;
          }
        }
        dst.Sliding += dst.Glissade;
        chunk.Data[toneReg] = uint8_t(tone & 0xff);
        chunk.Data[toneReg + 1] = uint8_t(tone >> 8);
        chunk.Mask |= 3 << toneReg;
        //calculate level
        chunk.Data[volReg] = GetVolume(dst.Volume, curSampleLine.Level)
          | uint8_t(dst.Envelope ? AYM::DataChunk::MASK_ENV : 0);
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
          chunk.Data[AYM::DataChunk::REG_TONEN] = uint8_t(clamp<signed>(curSampleLine.Noise + dst.NoiseAdd, 0, 31));
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
  private:
    const boost::shared_ptr<const PT2Holder> Module;
    const PT2Track::ModuleData& Data;

    FrequencyTable FreqTable;
    bool YMChip;
    AYM::Chip::Ptr Device;
    PlaybackState CurrentState;
    PT2Track::ModuleState ModState;
    boost::array<ChannelState, AYM::CHANNELS> ChanState;
  };

  Player::Ptr CreatePT2Player(boost::shared_ptr<const PT2Holder> holder, AYM::Chip::Ptr device)
  {
    return Player::Ptr(new PT2Player(holder, device));
  }

  //////////////////////////////////////////////////
  bool Check(const uint8_t* data, std::size_t size, const MetaContainer& container,
    Holder::Ptr& holder, ModuleRegion& region)
  {
    if (sizeof(PT2Header) > size)
    {
      return false;
    }

    const PT2Header* const header(safe_ptr_cast<const PT2Header*>(data));
    if (header->Length < 1 || header->Tempo < 2 || header->Loop >= header->Length)
    {
      return false;
    }
    const unsigned lowlimit(1 + std::find(header->Positions, data + size, POS_END_MARKER) - data);
    if (lowlimit - sizeof(*header) != header->Length)//too big positions list
    {
      return false;
    }

    const boost::function<bool(unsigned)> checker = !boost::bind(&in_range<unsigned>, _1, lowlimit, size - 1);

    //check offsets
    for (const uint16_t* sampOff = header->SamplesOffsets.begin(); sampOff != header->SamplesOffsets.end(); ++sampOff)
    {
      const unsigned offset(fromLE(*sampOff));
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
    for (const uint16_t* ornOff = header->OrnamentsOffsets.begin(); ornOff != header->OrnamentsOffsets.end(); ++ornOff)
    {
      const unsigned offset(fromLE(*ornOff));
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
    const unsigned patOff(fromLE(header->PatternsOffset));
    if (checker(patOff))
    {
      return false;
    }
    //check patterns
    unsigned patternsCount(0);
    for (const PT2Pattern* patPos(safe_ptr_cast<const PT2Pattern*>(data + patOff));
      patPos->Check();
      ++patPos, ++patternsCount)
    {
      if (patPos->Offsets.end() != std::find_if(patPos->Offsets.begin(), patPos->Offsets.end(), checker))
      {
        return false;
      }
    }
    if (!patternsCount)
    {
      return false;
    }
    if (header->Positions + header->Length != 
      std::find_if(header->Positions, header->Positions + header->Length, std::bind2nd(std::greater_equal<uint8_t>(),
        patternsCount)))
    {
      return false;
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
      //TODO: log error
    }
    return false;
  }

  //////////////////////////////////////////////////////////////////////////
  bool CreatePT2Module(const Parameters::Map& /*commonParams*/, const MetaContainer& container,
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
  void RegisterPT2Support(PluginsEnumerator& enumerator)
  {
    PluginInformation info;
    DescribePT2Plugin(info);
    enumerator.RegisterPlayerPlugin(info, CreatePT2Module);
  }
}
