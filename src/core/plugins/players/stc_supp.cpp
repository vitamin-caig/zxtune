/*
Abstract:
  STC modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "freq_tables.h"
#include "tracking.h"
#include "../enumerator.h"

#include <byteorder.h>
#include <error_tools.h>
#include <messages_collector.h>
#include <tools.h>
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <core/devices/aym/aym.h>
#include <core/error_codes.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <io/container.h>
#include <sound/render_params.h>

#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <valarray>

#include <text/core.h>
#include <text/plugins.h>
#include <text/warnings.h>

#define FILE_TAG D9281D1D

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  const Char STC_PLUGIN_ID[] = {'S', 'T', 'C', 0};
  const String TEXT_STC_VERSION(FromChar("$Rev$"));

  //hints
  const std::size_t MAX_MODULE_SIZE = 16384;
  const std::size_t MAX_SAMPLES_COUNT = 16;
  const std::size_t MAX_ORNAMENTS_COUNT = 16;
  const std::size_t MAX_PATTERN_SIZE = 64;
  const std::size_t MAX_PATTERN_COUNT = 32;//TODO

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
    uint8_t Identifier[18];
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
    uint16_t Offsets[3];

    operator bool () const
    {
      return 0xff != Number;
    }
  } PACK_POST;

  PACK_PRE struct STCOrnament
  {
    uint8_t Number;
    int8_t Data[32];
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
      
      unsigned GetLevel() const
      {
        return EffHiAndLevel & 15;
      }
      
      signed GetEffect() const
      {
        const signed val = EffLo + (signed(EffHiAndLevel & 240) << 4);
        return (NoiseAndMasks & 32) ? val : -val;
      }
      
      unsigned GetNoise() const
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
    Line Data[32];
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

  struct Sample
  {
    Sample() : Loop(), LoopLimit(), Data()
    {
    }

    explicit Sample(const STCSample& sample)
    : Loop(sample.Loop), LoopLimit(sample.Loop + sample.LoopSize + 1), Data(sample.Data, ArrayEnd(sample.Data))
    {
      assert(Loop <= LoopLimit);
      assert(LoopLimit <= Data.size());
    }

    unsigned Loop;
    unsigned LoopLimit;

    struct Line
    {
      /*explicit*/Line(const STCSample::Line& line)
        : Level(line.GetLevel()), Noise(line.GetNoise()), NoiseMask(line.GetNoiseMask())
        , EnvelopeMask(line.GetEnvelopeMask()), Effect(line.GetEffect())
      {
      }
      unsigned Level;//0-15
      unsigned Noise;//0-31
      bool NoiseMask;
      bool EnvelopeMask;
      signed Effect;
    };
    std::vector<Line> Data;
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
    info.Description = TEXT_STC_INFO;
    info.Version = TEXT_STC_VERSION;
    info.Capabilities = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW;
  }

  typedef TrackingSupport<AYM::CHANNELS, Sample> STCTrack;
  
  class STCHolder;
  Player::Ptr CreateSTCPlayer(boost::shared_ptr<const STCHolder> mod);

  class STCHolder : public Holder, public boost::enable_shared_from_this<STCHolder>
  {
    static void ParsePattern(const IO::FastDump& data
      , std::vector<std::size_t>& offsets
      , STCTrack::Line& line
      , std::valarray<std::size_t>& periods
      , std::valarray<std::size_t>& counters
      , Log::MessagesCollector& warner
      )
    {
      for (unsigned chan = 0; chan != line.Channels.size(); ++chan)
      {
        if (counters[chan]--)
        {
          continue;//has to skip
        }

        Log::ParamPrefixedCollector channelWarner(warner, TEXT_CHANNEL_WARN_PREFIX, chan);
        for (;;)
        {
          const uint8_t cmd(data[offsets[chan]++]);
          STCTrack::Line::Chan& channel(line.Channels[chan]);
          if (cmd <= 0x5f)//note
          {
            Log::Assert(channelWarner, !channel.Note, TEXT_WARNING_DUPLICATE_NOTE);
            channel.Note = cmd;
            channel.Enabled = true;
            break;
          }
          else if (cmd >= 0x60 && cmd <= 0x6f)//sample
          {
            Log::Assert(channelWarner, !channel.SampleNum, TEXT_WARNING_DUPLICATE_SAMPLE);
            channel.SampleNum = cmd - 0x60;
          }
          else if (cmd >= 0x70 && cmd <= 0x7f)//ornament
          {
            Log::Assert(channelWarner, channel.Commands.end() == std::find(channel.Commands.begin(), channel.Commands.end(), ENVELOPE),
              TEXT_WARNING_DUPLICATE_ENVELOPE);
            Log::Assert(channelWarner, !channel.OrnamentNum, TEXT_WARNING_DUPLICATE_ORNAMENT);
            channel.OrnamentNum = cmd - 0x70;
            channel.Commands.push_back(STCTrack::Command(NOENVELOPE));
          }
          else if (cmd == 0x80)//reset
          {
            Log::Assert(channelWarner, !channel.Enabled, TEXT_WARNING_DUPLICATE_STATE);
            channel.Enabled = false;
            break;
          }
          else if (cmd == 0x81)//empty
          {
            break;
          }
          else if (cmd == 0x82)//orn 0
          {
            Log::Assert(channelWarner, channel.Commands.end() == std::find(channel.Commands.begin(), channel.Commands.end(), ENVELOPE),
              TEXT_WARNING_DUPLICATE_ENVELOPE);
            Log::Assert(channelWarner, !channel.OrnamentNum, TEXT_WARNING_DUPLICATE_ORNAMENT);
            channel.OrnamentNum = 0;
            channel.Commands.push_back(STCTrack::Command(NOENVELOPE));
          }
          else if (cmd >= 0x83 && cmd <= 0x8e)//orn 0 with envelope
          {
            Log::Assert(channelWarner, channel.Commands.end() == std::find(channel.Commands.begin(), channel.Commands.end(), NOENVELOPE),
              TEXT_WARNING_DUPLICATE_ENVELOPE);
            Log::Assert(channelWarner, !channel.OrnamentNum, TEXT_WARNING_DUPLICATE_ORNAMENT);
            channel.Commands.push_back(STCTrack::Command(ENVELOPE, cmd - 0x80, data[offsets[chan]++]));
            channel.OrnamentNum = 0;
          }
          else //skip
          {
            periods[chan] = cmd - 0xa1;
          }
        }
        counters[chan] = periods[chan];
      }
    }
  public:
    STCHolder(const MetaContainer& container, ModuleRegion& region)
    {
      //assume that data is ok
      const IO::FastDump& data(*container.Data);
      const STCHeader* const header(safe_ptr_cast<const STCHeader*>(data.Data()));
      const STCSample* const samples(safe_ptr_cast<const STCSample*>(header + 1));
      const STCPositions* const positions(safe_ptr_cast<const STCPositions*>(&data[fromLE(header->PositionsOffset)]));
      const STCOrnament* const ornaments(safe_ptr_cast<const STCOrnament*>(&data[fromLE(header->OrnamentsOffset)]));
      const STCPattern* const patterns(safe_ptr_cast<const STCPattern*>(&data[fromLE(header->PatternsOffset)]));

      Log::MessagesCollector::Ptr warner(Log::MessagesCollector::Create());
      //parse positions
      Data.Positions.reserve(positions->Lenght + 1);
      Transpositions.reserve(positions->Lenght + 1);
      for (const STCPositions::STCPosEntry* posEntry(positions->Data);
        static_cast<const void*>(posEntry) < static_cast<const void*>(ornaments); ++posEntry)
      {
        Data.Positions.push_back(posEntry->PatternNum - 1);
        Transpositions.push_back(posEntry->PatternHeight);
      }
      Log::Assert(*warner, Data.Positions.size() == std::size_t(positions->Lenght) + 1, TEXT_WARNING_INVALID_LENGTH);

      //parse samples
      Data.Samples.resize(MAX_SAMPLES_COUNT);
      for (const STCSample* sample = samples; static_cast<const void*>(sample) < static_cast<const void*>(positions); ++sample)
      {
        if (sample->Number >= Data.Samples.size())
        {
          warner->AddMessage(TEXT_WARNING_INVALID_SAMPLE);
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
          warner->AddMessage(TEXT_WARNING_INVALID_ORNAMENT);
          continue;
        }
        Data.Ornaments[ornament->Number] = STCTrack::Ornament(static_cast<unsigned>(ArraySize(ornament->Data)), 0);
        Data.Ornaments[ornament->Number].Data.assign(ornament->Data, ArrayEnd(ornament->Data));
      }

      //parse patterns
      std::size_t rawSize(0);
      Data.Patterns.resize(MAX_PATTERN_COUNT);
      for (const STCPattern* pattern = patterns; *pattern; ++pattern)
      {
        if (!pattern->Number || pattern->Number >= Data.Patterns.size())
        {
          warner->AddMessage(TEXT_WARNING_INVALID_PATTERN);
          continue;
        }
        Log::ParamPrefixedCollector patternWarner(*warner, TEXT_PATTERN_WARN_PREFIX, static_cast<unsigned>(pattern->Number - 1));
        STCTrack::Pattern& pat(Data.Patterns[pattern->Number - 1]);
        std::vector<std::size_t> offsets(ArraySize(pattern->Offsets));
        std::valarray<std::size_t> periods(std::size_t(0), ArraySize(pattern->Offsets));
        std::valarray<std::size_t> counters(std::size_t(0), ArraySize(pattern->Offsets));
        std::transform(pattern->Offsets, ArrayEnd(pattern->Offsets), offsets.begin(), &fromLE<uint16_t>);
        pat.reserve(MAX_PATTERN_SIZE);
        do
        {
          Log::ParamPrefixedCollector patLineWarner(*warner, TEXT_LINE_WARN_PREFIX, static_cast<unsigned>(pat.size()));
          pat.push_back(STCTrack::Line());
          STCTrack::Line& line(pat.back());
          ParsePattern(data, offsets, line, periods, counters, patLineWarner);
          //skip lines
          if (const std::size_t linesToSkip = counters.min())
          {
            counters -= linesToSkip;
            pat.resize(pat.size() + linesToSkip);//add dummies
          }
        }
        while (0xff != data[offsets[0]] || counters[0]);
        Log::Assert(patternWarner, 0 == counters.max(), TEXT_WARNING_PERIODS);
        Log::Assert(patternWarner, pat.size() <= MAX_PATTERN_SIZE, TEXT_WARNING_INVALID_PATTERN_SIZE);
        rawSize = std::max<std::size_t>(rawSize, 1 + *std::max_element(offsets.begin(), offsets.end()));
      }
      //fill region
      region.Offset = 0;
      region.Size = rawSize;
      
      //meta properties
      ExtractMetaProperties(STC_PLUGIN_ID, container, region, Data.Info.Properties, RawData);
      Data.Info.Properties.insert(StringMap::value_type(Module::ATTR_PROGRAM,
        String(header->Identifier, ArrayEnd(header->Identifier))));
      
      //tracking properties
      Data.Info.LoopPosition = 0;//not supported here
      Data.Info.PhysicalChannels = AYM::CHANNELS;
      Data.Info.Statistic.Tempo = header->Tempo;
      Data.Info.Statistic.Position = static_cast<unsigned>(Data.Positions.size());
      Data.Info.Statistic.Pattern = static_cast<unsigned>(std::count_if(Data.Patterns.begin(), Data.Patterns.end(),
        !boost::bind(&STCTrack::Pattern::empty, _1)));
      Data.Info.Statistic.Channels = AYM::CHANNELS;
      STCTrack::CalculateTimings(Data, Data.Info.Statistic.Frame, Data.Info.LoopFrame);
      if (warner->HasMessages())
      {
        Data.Info.Properties.insert(StringMap::value_type(Module::ATTR_WARNINGS, warner->GetMessages('\n')));
      }
    }

    virtual void GetPlayerInfo(PluginInformation& info) const
    {
      DescribeSTCPlugin(info);
    }

    virtual void GetModuleInformation(Information& info) const
    {
      info = Data.Info;
    }
    
    virtual Player::Ptr CreatePlayer() const
    {
      return CreateSTCPlayer(shared_from_this());
    }
    
    virtual Error Convert(const Conversion::Parameter& param, Dump& dst) const
    {
      using namespace Conversion;
      if (parameter_cast<RawConvertParam>(&param))
      {
        dst = RawData;
        return Error();
      }
      else
      {
        return Error(THIS_LINE, ERROR_MODULE_CONVERT, TEXT_MODULE_ERROR_CONVERSION_UNSUPPORTED);
      }
    }
  private:
    friend class STCPlayer;
    Dump RawData;
    STCTrack::ModuleData Data;
    std::vector<signed> Transpositions;
  };

  
  class STCPlayer : public Player
  {
    struct ChannelState
    {
      ChannelState()
        : Enabled(false), Envelope(false)
        , Note(), SampleNum(0), PosInSample(0)
        , OrnamentNum(0), LoopedInSample(false)
      {
      }
      bool Enabled;
      bool Envelope;
      unsigned Note;
      unsigned SampleNum;
      unsigned PosInSample;
      unsigned OrnamentNum;
      bool LoopedInSample;
    };
  public:
    explicit STCPlayer(boost::shared_ptr<const STCHolder> holder)
       : Module(holder)
       , Data(Module->Data)
       , Transpositions(Module->Transpositions)
       , YMChip(false)
       , Device(AYM::CreateChip())
       , Looped()
       , CurrentState(MODULE_STOPPED)
    {
      ThrowIfError(GetFreqTable(TABLE_SOUNDTRACKER, FreqTable));
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
      if (Data.Positions.size() <= ModState.Track.Position + 1 &&
          MODULE_STOPPED == CurrentState)
      {
        return Error(THIS_LINE, ERROR_MODULE_END, TEXT_MODULE_ERROR_MODULE_END);
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
      if (STCTrack::UpdateState(Data, ModState, Looped))
      {
        state = CurrentState = MODULE_PLAYING;
      }
      else
      {
        receiver.Flush();
        state = CurrentState = MODULE_STOPPED;
      }
      return Error();
    }

    virtual Error Reset()
    {
      Device->Reset();
      STCTrack::InitState(Data, ModState);
      std::fill(ChanState.begin(), ChanState.end(), ChannelState());
      CurrentState = MODULE_STOPPED;
      return Error();
    }

    virtual Error SetPosition(unsigned /*frame*/)
    {
      //TODO
      return Error(THIS_LINE, 1, "Not implemented");
    }

    virtual Error SetParameters(const Parameters::Map& params)
    {
      try
      {
        if (const Parameters::IntType* const type = Parameters::FindByName<Parameters::IntType>(params,
          Parameters::ZXTune::Core::AYM::TYPE))
        {
          YMChip = 0 != (*type & 1);//only one chip
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
      const STCTrack::Line& line(Data.Patterns[ModState.Track.Pattern][ModState.Track.Line]);
      if (0 == ModState.Track.Frame)//begin note
      {
        for (unsigned chan = 0; chan != line.Channels.size(); ++chan)
        {
          const STCTrack::Line::Chan& src(line.Channels[chan]);
          ChannelState& dst(ChanState[chan]);
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

      if (dst.Enabled)
      {
        const STCTrack::Sample& curSample(Data.Samples[dst.SampleNum]);
        const STCTrack::Sample::Line& curSampleLine(curSample.Data[dst.PosInSample]);
        const STCTrack::Ornament& curOrnament(Data.Ornaments[dst.OrnamentNum]);

        //calculate tone
        const unsigned halfTone = static_cast<unsigned>(clamp<int>(
          signed(dst.Note) + curOrnament.Data[dst.PosInSample] + Transpositions[ModState.Track.Position],
          0, static_cast<int>(FreqTable.size() - 1)));
        const uint16_t tone = static_cast<uint16_t>(clamp(FreqTable[halfTone] + curSampleLine.Effect, 0, 0xffff));

        chunk.Data[toneReg] = static_cast<uint8_t>(tone & 0xff);
        chunk.Data[toneReg + 1] = static_cast<uint8_t>(tone >> 8);
        chunk.Mask |= 3 << toneReg;
        //calculate level
        chunk.Data[volReg] = curSampleLine.Level | static_cast<uint8_t>(dst.Envelope ? AYM::DataChunk::MASK_ENV : 0);
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
          chunk.Data[AYM::DataChunk::REG_TONEN] = curSampleLine.Noise;
          chunk.Mask |= 1 << AYM::DataChunk::REG_TONEN;
        }

        if (++dst.PosInSample >= (dst.LoopedInSample ? curSample.LoopLimit : curSample.Data.size()))
        {
          if (curSample.Loop)
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
    const boost::shared_ptr<const STCHolder> Module;
    const STCTrack::ModuleData& Data;
    const std::vector<signed>& Transpositions;

    FrequencyTable FreqTable;
    bool YMChip;
    AYM::Chip::Ptr Device;
    bool Looped;
    PlaybackState CurrentState;
    STCTrack::ModuleState ModState;
    boost::array<ChannelState, AYM::CHANNELS> ChanState;
  };
  
  Player::Ptr CreateSTCPlayer(boost::shared_ptr<const STCHolder> holder)
  {
    return Player::Ptr(new STCPlayer(holder));
  }

  //////////////////////////////////////////////////////////////////////////
  bool CreateSTCModule(const Parameters::Map& /*commonParams*/, const MetaContainer& container, 
    Holder::Ptr& holder, ModuleRegion& region)
  {
    //perform fast check
    const std::size_t limit(std::min(container.Data->Size(), MAX_MODULE_SIZE));
    if (limit < sizeof(STCHeader))
    {
      return false;
    }
    const uint8_t* const data(static_cast<const uint8_t*>(container.Data->Data()));
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
    //try to create holder
    try
    {
      holder.reset(new STCHolder(container, region));
      return true;
    }
    catch (const Error&/*e*/)
    {
      //TODO: log error
    }
    return false;
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
