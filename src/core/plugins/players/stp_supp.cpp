/*
Abstract:
  STP modules playback support

Last changed:
  $Id$

Author:
(C) Vitamin/CAIG/2001
*/

//local includes
#include "convert_helpers.h"
#include "tracking.h"
#include <core/devices/aym_parameters_helper.h>
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
#include <core/error_codes.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <io/container.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
//text includes
#include <core/text/core.h>
#include <core/text/plugins.h>
#include <core/text/warnings.h>

#define FILE_TAG BD5A4CD5

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char STP_PLUGIN_ID[] = {'S', 'T', 'P', 0};
  const String STP_PLUGIN_VERSION(FromStdString("$Rev$"));

  //hints
  const std::size_t MAX_MODULE_SIZE = 16384;
  const uint_t MAX_SAMPLES_COUNT = 15;
  const uint_t MAX_ORNAMENTS_COUNT = 16;
  const uint_t MAX_PATTERN_SIZE = 64;
  const uint_t MAX_ORNAMENT_SIZE = 32;
  const uint_t MAX_SAMPLE_SIZE = 32;

  //detectors
  const DetectFormatChain DETECTORS[] =
  {
    //STP0
    {
      "21??"   // ld hl,xxxx
      "c3??"   // jp xxxx
      "c3??"   // jp xxxx
      "ed4b??" // ld bc,(xxxx)
      "c3??"   // jp xxxx
      "+62+"   // id+name
      "f3"     // di
      "22??"   // ld (xxxx),hl
      "3e?"    // ld a,xx
      "32??"   // ld (xxxx),a
      "32??"   // ld (xxxx),a
      "32??"   // ld (xxxx),a
      "7e"     // ld a,(hl)
      "23"     // inc hl
      "32??"   // ld (xxxx),a
      "cd??"   // call xxxx
      "7e"     // ld a,(hl)
      "32??"   // ld (xxxx),a
      "23"     // inc hl
      ,
      1896
    }
  };
  //////////////////////////////////////////////////////////////////////////
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct STPHeader
  {
    uint8_t Tempo;
    uint16_t PositionsOffset;
    uint16_t PatternsOffset;
    uint16_t OrnamentsOffset;
    uint16_t SamplesOffset;
    uint8_t FixesCount;
  } PACK_POST;

  const uint8_t STP_ID[] =
  {
    'K', 'S', 'A', ' ', 'S', 'O', 'F', 'T', 'W', 'A', 'R', 'E', ' ',
    'C', 'O', 'M', 'P', 'I', 'L', 'A', 'T', 'I', 'O', 'N', ' ', 'O', 'F', ' '
  };

  PACK_PRE struct STPId
  {
    uint8_t Id[sizeof(STP_ID)];
    char Title[25];

    bool Check() const
    {
      return 0 == std::memcmp(Id, STP_ID, sizeof(Id));
    }
  } PACK_POST;

  PACK_PRE struct STPPositions
  {
    uint8_t Lenght;
    uint8_t Loop;
    PACK_PRE struct STPPosEntry
    {
      uint8_t PatternOffset;//*6
      int8_t PatternHeight;
    } PACK_POST;
    STPPosEntry Data[1];
  } PACK_POST;

  PACK_PRE struct STPPattern
  {
    boost::array<uint16_t, AYM::CHANNELS> Offsets;
  } PACK_POST;

  PACK_PRE struct STPOrnament
  {
    uint8_t Loop;
    uint8_t Size;
    int8_t Data[1];
  } PACK_POST;

  PACK_PRE struct STPOrnaments
  {
    boost::array<uint16_t, MAX_ORNAMENTS_COUNT> Offsets;
  } PACK_POST;

  PACK_PRE struct STPSample
  {
    int8_t Loop;
    uint8_t Size;
    PACK_PRE struct Line
    {
      // NxxTaaaa
      // xxnnnnnE
      // vvvvvvvv
      // vvvvvvvv

      // a - level
      // T - tone mask
      // N - noise mask
      // E - env mask
      // n - noise
      // v - signed vibrato
      uint8_t LevelAndFlags;
      uint8_t NoiseAndFlag;
      int16_t Vibrato;

      uint_t GetLevel() const
      {
        return LevelAndFlags & 15;
      }

      bool GetToneMask() const
      {
        return 0 != (LevelAndFlags & 16);
      }

      bool GetNoiseMask() const
      {
        return 0 != (LevelAndFlags & 128);
      }

      bool GetEnvelopeMask() const
      {
        return 0 != (NoiseAndFlag & 1);
      }

      uint_t GetNoise() const
      {
        return (NoiseAndFlag & 62) >> 1;
      }

      int_t GetVibrato() const
      {
        return fromLE(Vibrato);
      }
    } PACK_POST;
    Line Data[1];
  } PACK_POST;

  PACK_PRE struct STPSamples
  {
    boost::array<uint16_t, MAX_SAMPLES_COUNT> Offsets;
  } PACK_POST;

#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(STPHeader) == 10);
  BOOST_STATIC_ASSERT(sizeof(STPPositions) == 4);
  BOOST_STATIC_ASSERT(sizeof(STPPattern) == 6);
  BOOST_STATIC_ASSERT(sizeof(STPOrnament) == 3);
  BOOST_STATIC_ASSERT(sizeof(STPOrnaments) == 32);
  BOOST_STATIC_ASSERT(sizeof(STPSample) == 6);
  BOOST_STATIC_ASSERT(sizeof(STPSamples) == 30);

  struct Sample
  {
    Sample() : Loop(), Data()
    {
    }

    explicit Sample(const STPSample& sample)
      : Loop(sample.Loop), Data(sample.Data, sample.Data + sample.Size)
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
      Line()
        : Level(), Noise(), ToneMask(), NoiseMask(), EnvelopeMask(), Vibrato()
      {
      }

      /*explicit*/Line(const STPSample::Line& line)
        : Level(line.GetLevel()), Noise(line.GetNoise())
        , ToneMask(line.GetToneMask()), NoiseMask(line.GetNoiseMask()), EnvelopeMask(line.GetEnvelopeMask())
        , Vibrato(line.GetVibrato())
      {
      }
      uint_t Level;//0-15
      uint_t Noise;//0-31
      bool ToneMask;
      bool NoiseMask;
      bool EnvelopeMask;
      int_t Vibrato;
    };
    int_t Loop;
    std::vector<Line> Data;
  };

  enum CmdType
  {
    EMPTY,
    ENVELOPE,     //2p
    NOENVELOPE,   //0p
    GLISS,        //1p
  };

  void DescribeSTPPlugin(PluginInformation& info)
  {
    info.Id = STP_PLUGIN_ID;
    info.Description = Text::STP_PLUGIN_INFO;
    info.Version = STP_PLUGIN_VERSION;
    info.Capabilities = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | GetSupportedAYMFormatConvertors();
  }

  typedef TrackingSupport<AYM::CHANNELS, Sample> STPTrack;

  // perform module 'playback' right after creating (debug purposes)
#ifndef NDEBUG
#define SELF_TEST
#endif

  class STPHolder;
  Player::Ptr CreateSTPPlayer(boost::shared_ptr<const STPHolder> mod, AYM::Chip::Ptr device);

  class STPHolder : public Holder, public boost::enable_shared_from_this<STPHolder>
  {
    typedef boost::array<PatternCursor, AYM::CHANNELS> PatternCursors;

    static void ParsePattern(const IO::FastDump& data
      , PatternCursors& cursors
      , STPTrack::Line& line
      , Log::MessagesCollector& warner)
    {
      assert(line.Channels.size() == cursors.size());
      STPTrack::Line::ChannelsArray::iterator channel(line.Channels.begin());
      for (PatternCursors::iterator cur = cursors.begin(); cur != cursors.end(); ++cur, ++channel)
      {
        if (cur->Counter--)
        {
          continue;//has to skip
        }
        Log::ParamPrefixedCollector channelWarner(warner, Text::CHANNEL_WARN_PREFIX, std::distance(line.Channels.begin(), channel));
        for (;;)
        {
          const uint_t cmd = data[cur->Offset++];
          const std::size_t restbytes = data.Size() - cur->Offset;
          if (cmd >= 1 && cmd <= 0x60)//note
          {
            Log::Assert(channelWarner, !channel->Note, Text::WARNING_DUPLICATE_NOTE);
            channel->Note = cmd - 1;
            channel->Enabled = true;
            break;
          }
          else if (cmd >= 0x61 && cmd <= 0x6f)//sample
          {
            Log::Assert(channelWarner, !channel->SampleNum, Text::WARNING_DUPLICATE_SAMPLE);
            channel->SampleNum = cmd - 0x61;
          }
          else if (cmd >= 0x70 && cmd <= 0x7f)//ornament
          {
            Log::Assert(channelWarner, !channel->FindCommand(ENVELOPE), Text::WARNING_DUPLICATE_ENVELOPE);
            Log::Assert(channelWarner, !channel->OrnamentNum, Text::WARNING_DUPLICATE_ORNAMENT);
            channel->OrnamentNum = cmd - 0x70;
            channel->Commands.push_back(STPTrack::Command(NOENVELOPE));
            //TODO
            Log::Assert(channelWarner, !channel->FindCommand(GLISS), "duplicate gliss");
            channel->Commands.push_back(STPTrack::Command(GLISS, 0));
          }
          else if (cmd >= 0x80 && cmd <= 0xbf) //skip
          {
            cur->Period = cmd - 0x80;
          }
          else if (cmd >= 0xc0 && cmd <= 0xcf)
          {
            if (cmd != 0xc0 && restbytes < 1)
            {
              throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
            }
            Log::Assert(channelWarner, !channel->FindCommand(NOENVELOPE), Text::WARNING_DUPLICATE_ENVELOPE);
            channel->Commands.push_back(STPTrack::Command(ENVELOPE, cmd - 0xc0, cmd != 0xc0 ? data[cur->Offset++] : 0));
            Log::Assert(channelWarner, !channel->OrnamentNum, Text::WARNING_DUPLICATE_ORNAMENT);
            channel->OrnamentNum = 0;
            Log::Assert(channelWarner, !channel->FindCommand(GLISS), "duplicate gliss");
            channel->Commands.push_back(STPTrack::Command(GLISS, 0));
          }
          else if (cmd >= 0xd0 && cmd <= 0xdf)//reset
          {
            Log::Assert(channelWarner, !channel->Enabled, Text::WARNING_DUPLICATE_STATE);
            channel->Enabled = false;
            break;
          }
          else if (cmd >= 0xe0 && cmd <= 0xef)//empty
          {
            break;
          }
          else if (cmd == 0xf0) //glissade
          {
            if (restbytes < 1)
            {
              throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
            }
            Log::Assert(channelWarner, !channel->FindCommand(GLISS), "duplicate gliss");
            channel->Commands.push_back(STPTrack::Command(GLISS, static_cast<int8_t>(data[cur->Offset++])));
          }
          else if (cmd >= 0xf1 && cmd <= 0xff) //volume
          {
            Log::Assert(channelWarner, !channel->Volume, Text::WARNING_DUPLICATE_VOLUME);
            channel->Volume = cmd - 0xf1;
          }
        }
        cur->Counter = cur->Period;
      }
    }
  public:
    STPHolder(const MetaContainer& container, ModuleRegion& region)
    {
      //assume that data is ok
      const IO::FastDump& data = IO::FastDump(*container.Data, region.Offset);
      const STPHeader* const header = safe_ptr_cast<const STPHeader*>(&data[0]);

      const STPPositions* const positions = safe_ptr_cast<const STPPositions*>(&data[fromLE(header->PositionsOffset)]);
      const STPPattern* const patterns = safe_ptr_cast<const STPPattern*>(&data[fromLE(header->PatternsOffset)]);
      const STPOrnaments* const ornaments = safe_ptr_cast<const STPOrnaments*>(&data[fromLE(header->OrnamentsOffset)]);
      const STPSamples* const samples = safe_ptr_cast<const STPSamples*>(&data[fromLE(header->SamplesOffset)]);

      Log::MessagesCollector::Ptr warner(Log::MessagesCollector::Create());

      //parse positions
      Data.Positions.resize(positions->Lenght + 1);
      Transpositions.resize(positions->Lenght + 1);
      std::transform(positions->Data, positions->Data + positions->Lenght, Data.Positions.begin(),
        boost::bind(std::divides<uint_t>(),
          boost::bind(&STPPositions::STPPosEntry::PatternOffset, _1),
          6));
      std::transform(positions->Data, positions->Data + positions->Lenght, Transpositions.begin(),
        boost::mem_fn(&STPPositions::STPPosEntry::PatternHeight));

      //parse samples
      Data.Samples.reserve(MAX_SAMPLES_COUNT);
      uint_t index = 0;
      for (const uint16_t* pSample = samples->Offsets.begin(); pSample != samples->Offsets.end();
        ++pSample, ++index)
      {
        assert(*pSample && fromLE(*pSample) < data.Size());
        const STPSample* const sample = safe_ptr_cast<const STPSample*>(&data[fromLE(*pSample)]);
        Data.Samples.push_back(Sample(*sample));
        const Sample& smp = Data.Samples.back();
        if (smp.Loop >= static_cast<signed>(smp.Data.size()))
        {
          Log::ParamPrefixedCollector smpWarner(*warner, Text::SAMPLE_WARN_PREFIX, index);
          smpWarner.AddMessage(Text::WARNING_LOOP_OUT_BOUND);
        }
      }
      std::size_t rawSize = safe_ptr_cast<const uint8_t*>(samples + 1) - data.Data();

      Data.Ornaments.reserve(MAX_ORNAMENTS_COUNT);
      index = 0;
      for (const uint16_t* pOrnament = ornaments->Offsets.begin(); pOrnament != ornaments->Offsets.end();
        ++pOrnament, ++index)
      {
        assert(*pOrnament && fromLE(*pOrnament) < data.Size());
        const STPOrnament* const ornament = safe_ptr_cast<const STPOrnament*>(&data[fromLE(*pOrnament)]);
        Data.Ornaments.push_back(SimpleOrnament(ornament->Data, ornament->Data + ornament->Size, ornament->Loop));
        const SimpleOrnament& orn = Data.Ornaments.back();
        if (orn.Loop > orn.Data.size())
        {
          Log::ParamPrefixedCollector smpWarner(*warner, Text::ORNAMENT_WARN_PREFIX, index);
          smpWarner.AddMessage(Text::WARNING_LOOP_OUT_BOUND);
        }
      }

      //parse patterns
      for (const STPPattern* pattern = patterns;
        static_cast<const void*>(pattern) < static_cast<const void*>(ornaments); ++pattern)
      {
        Data.Patterns.push_back(STPTrack::Pattern());
        Log::ParamPrefixedCollector patternWarner(*warner, Text::PATTERN_WARN_PREFIX, Data.Patterns.size());
        STPTrack::Pattern& pat = Data.Patterns.back();
        PatternCursors cursors;
        std::transform(pattern->Offsets.begin(), pattern->Offsets.end(), cursors.begin(), std::ptr_fun(&fromLE<uint16_t>));
        pat.reserve(MAX_PATTERN_SIZE);
        do
        {
          const uint_t patternSize = pat.size();
          if (patternSize > MAX_PATTERN_SIZE)
          {
            throw Error(THIS_LINE, ERROR_INVALID_FORMAT);//no details
          }
          Log::ParamPrefixedCollector patLineWarner(patternWarner, Text::LINE_WARN_PREFIX, patternSize);
          pat.push_back(STPTrack::Line());
          STPTrack::Line& line = pat.back();
          ParsePattern(data, cursors, line, patLineWarner);
          //skip lines
          if (const uint_t linesToSkip = std::min_element(cursors.begin(), cursors.end(), PatternCursor::CompareByCounter)->Counter)
          {
            std::for_each(cursors.begin(), cursors.end(), std::bind2nd(std::mem_fun_ref(&PatternCursor::SkipLines), linesToSkip));
            pat.resize(pat.size() + linesToSkip);//add dummies
          }
        }
        while (0x00 != data[cursors.front().Offset] || cursors.front().Counter);
        //as warnings
        Log::Assert(patternWarner, 0 == std::max_element(cursors.begin(), cursors.end(), PatternCursor::CompareByCounter)->Counter,
          Text::WARNING_PERIODS);
        Log::Assert(patternWarner, pat.size() <= MAX_PATTERN_SIZE, Text::WARNING_INVALID_PATTERN_SIZE);
        rawSize = std::max<std::size_t>(rawSize, 1 + std::max_element(cursors.begin(), cursors.end(), PatternCursor::CompareByOffset)->Offset);
      }

      //fix samples and ornaments
      std::for_each(Data.Ornaments.begin(), Data.Ornaments.end(), std::mem_fun_ref(&STPTrack::Ornament::Fix));
      std::for_each(Data.Samples.begin(), Data.Samples.end(), std::mem_fun_ref(&STPTrack::Sample::Fix));

      //fill region
      region.Size = rawSize;

      //meta properties
      const std::size_t fixedOffset = fromLE(header->PatternsOffset);
      ExtractMetaProperties(STP_PLUGIN_ID, container, region, ModuleRegion(fixedOffset, rawSize - fixedOffset),
        Data.Info.Properties, RawData);
      const STPId* const id = safe_ptr_cast<const STPId*>(header + 1);
      if (id->Check())
      {
        const String& title(OptimizeString(FromStdString(id->Title)));
        if (!title.empty())
        {
          Data.Info.Properties.insert(Parameters::Map::value_type(Module::ATTR_TITLE, title));
        }
      }
      Data.Info.Properties.insert(Parameters::Map::value_type(Module::ATTR_PROGRAM, String(Text::STP_EDITOR)));

      //tracking properties
      Data.Info.LoopPosition = positions->Loop;//not supported
      Data.Info.PhysicalChannels = AYM::CHANNELS;
      Data.Info.Statistic.Tempo = header->Tempo;
      Data.Info.Statistic.Position = Data.Positions.size();
      Data.Info.Statistic.Pattern = std::count_if(Data.Patterns.begin(), Data.Patterns.end(),
        !boost::bind(&STPTrack::Pattern::empty, _1));
      Data.Info.Statistic.Channels = AYM::CHANNELS;

      STPTrack::CalculateTimings(Data, Data.Info.Statistic.Frame, Data.Info.LoopFrame);
      if (const uint_t msgs = warner->CountMessages())
      {
        Data.Info.Properties.insert(Parameters::Map::value_type(Module::ATTR_WARNINGS_COUNT, msgs));
        Data.Info.Properties.insert(Parameters::Map::value_type(Module::ATTR_WARNINGS, warner->GetMessages('\n')));
      }
    }
    virtual void GetPluginInformation(PluginInformation& info) const
    {
      DescribeSTPPlugin(info);
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
      return CreateSTPPlayer(shared_from_this(), AYM::CreateChip());
    }

    virtual Error Convert(const Conversion::Parameter& param, Dump& dst) const
    {
      using namespace Conversion;
      Error result;
      if (parameter_cast<RawConvertParam>(&param))
      {
        dst = RawData;
      }
      else if (!ConvertAYMFormat(boost::bind(&CreateSTPPlayer, shared_from_this(), _1), param, dst, result))
      {
        return Error(THIS_LINE, ERROR_MODULE_CONVERT, Text::MODULE_ERROR_CONVERSION_UNSUPPORTED);
      }
      return result;
    }
  private:
    friend class STPPlayer;
    Dump RawData;
    STPTrack::ModuleData Data;
    std::vector<int_t> Transpositions;
  };

  class STPPlayer : public Player
  {
    struct ChannelState
    {
      ChannelState()
        : Enabled(false), Envelope(false), Volume(0)
        , Note(0), SampleNum(0), PosInSample(0)
        , OrnamentNum(0), PosInOrnament(0)
        , TonSlide(0), Glissade(0)
      {
      }
      bool Enabled;
      bool Envelope;
      uint_t Volume;
      uint_t Note;
      uint_t SampleNum;
      uint_t PosInSample;
      uint_t OrnamentNum;
      uint_t PosInOrnament;
      int_t TonSlide;
      int_t Glissade;
    };
  public:
    STPPlayer(boost::shared_ptr<const STPHolder> holder, AYM::Chip::Ptr device)
      : Module(holder)
      , Data(Module->Data)
      , Transpositions(Module->Transpositions)
      , AYMHelper(AYM::ParametersHelper::Create(TABLE_SOUNDTRACKER))
      , Device(device)
      , CurrentState(MODULE_STOPPED)
    {
      Reset();
#ifdef SELF_TEST
      //perform self-test
      AYM::DataChunk chunk;
      do
      {
        assert(Data.Positions.size() > ModState.Track.Position);
        RenderData(chunk);
      }
      while (STPTrack::UpdateState(Data, ModState, Sound::LOOP_NONE));
      Reset();
#endif
    }

    virtual const Holder& GetModule() const
    {
      return *Module;
    }

    virtual Error GetPlaybackState(uint_t& timeState,
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
          return Error(THIS_LINE, ERROR_MODULE_END, Text::MODULE_ERROR_MODULE_END);
        }
        receiver.Flush();
        state = CurrentState = MODULE_STOPPED;
        return Error();
      }

      AYM::DataChunk chunk;
      AYMHelper->GetDataChunk(chunk);
      ModState.Tick += params.ClocksPerFrame();
      chunk.Tick = ModState.Tick;
      RenderData(chunk);

      Device->RenderData(params, chunk, receiver);
      if (STPTrack::UpdateState(Data, ModState, params.Looping))
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
      STPTrack::InitState(Data, ModState);
      std::fill(Channels.begin(), Channels.end(), ChannelState());
      CurrentState = MODULE_STOPPED;
      return Error();
    }

    virtual Error SetPosition(uint_t frame)
    {
      if (frame < ModState.Frame)
      {
        //reset to beginning in case of moving back
        const uint64_t keepTicks = ModState.Tick;
        STPTrack::InitState(Data, ModState);
        std::fill(Channels.begin(), Channels.end(), ChannelState());
        ModState.Tick = keepTicks;
      }
      //fast forward
      AYM::DataChunk chunk;
      while (ModState.Frame < frame)
      {
        //do not update tick for proper rendering
        assert(Data.Positions.size() > ModState.Track.Position);
        RenderData(chunk);
        if (!STPTrack::UpdateState(Data, ModState, Sound::LOOP_NONE))
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
        AYMHelper->SetParameters(params);
        return Error();
      }
      catch (const Error& e)
      {
        return Error(THIS_LINE, ERROR_INVALID_PARAMETERS, Text::MODULE_ERROR_SET_PLAYER_PARAMETERS).AddSuberror(e);
      }
    }

  private:
    void RenderData(AYM::DataChunk& chunk)
    {
      const STPTrack::Line& line = Data.Patterns[ModState.Track.Pattern][ModState.Track.Line];
      if (0 == ModState.Track.Frame)//begin note
      {
        for (uint_t chan = 0; chan != line.Channels.size(); ++chan)
        {
          const STPTrack::Line::Chan& src = line.Channels[chan];
          ChannelState& dst = Channels[chan];
          if (src.Enabled)
          {
            dst.Enabled = *src.Enabled;
            dst.PosInSample = 0;
            dst.PosInOrnament = 0;
          }
          if (src.Note)
          {
            dst.Note = *src.Note;
            dst.PosInSample = 0;
            dst.PosInOrnament = 0;
            dst.TonSlide = 0;
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
          for (STPTrack::CommandsArray::const_iterator it = src.Commands.begin(), lim = src.Commands.end(); it != lim; ++it)
          {
            switch (it->Type)
            {
            case ENVELOPE:
              if (it->Param1)
              {
                chunk.Data[AYM::DataChunk::REG_ENV] = static_cast<uint8_t>(it->Param1);
                chunk.Data[AYM::DataChunk::REG_TONEE_L] = static_cast<uint8_t>(it->Param2 & 0xff);
                chunk.Mask |= (1 << AYM::DataChunk::REG_ENV) | (1 << AYM::DataChunk::REG_TONEE_L);
              }
              dst.Envelope = true;
              break;
            case NOENVELOPE:
              dst.Envelope = false;
              break;
            case GLISS:
              dst.Glissade = it->Param1;
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
      ModState.Track.Channels = std::count_if(Channels.begin(), Channels.end(),
        boost::mem_fn(&ChannelState::Enabled));
    }

    void ApplyData(uint_t chan, AYM::DataChunk& chunk)
    {
      ChannelState& dst = Channels[chan];
      const uint_t toneReg = AYM::DataChunk::REG_TONEA_L + 2 * chan;
      const uint_t volReg = AYM::DataChunk::REG_VOLA + chan;
      const uint_t toneMsk = AYM::DataChunk::REG_MASK_TONEA << chan;
      const uint_t noiseMsk = AYM::DataChunk::REG_MASK_NOISEA << chan;

      const FrequencyTable& freqTable(AYMHelper->GetFreqTable());
      if (dst.Enabled)
      {
        const STPTrack::Sample& curSample = Data.Samples[dst.SampleNum];
        const STPTrack::Sample::Line& curSampleLine = curSample.Data[dst.PosInSample];
        const STPTrack::Ornament& curOrnament = Data.Ornaments[dst.OrnamentNum];

        //calculate tone
        dst.TonSlide += dst.Glissade;
        const uint_t halfTone = static_cast<uint_t>(clamp<int_t>(
          int_t(dst.Note) + Transpositions[ModState.Track.Position] +
          (dst.Envelope ? 0 : curOrnament.Data[dst.PosInOrnament]), 0, 95));
        const uint_t tone = static_cast<uint_t>(clamp<int_t>(int_t(freqTable[halfTone]) + dst.TonSlide +
          curSampleLine.Vibrato, 0, 0xffff));

        chunk.Data[toneReg] = static_cast<uint8_t>(tone & 0xff);
        chunk.Data[toneReg + 1] = static_cast<uint8_t>(tone >> 8);
        chunk.Mask |= 3 << toneReg;
        //calculate level
        chunk.Data[volReg] = static_cast<uint8_t>(clamp<int_t>(int_t(curSampleLine.Level) - int_t(dst.Volume),
          0, 15) |
          (curSampleLine.EnvelopeMask && dst.Envelope ? AYM::DataChunk::REG_MASK_ENV : 0));
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
          chunk.Data[AYM::DataChunk::REG_TONEN] = static_cast<uint8_t>(curSampleLine.Noise);
          chunk.Mask |= 1 << AYM::DataChunk::REG_TONEN;
        }

        if (++dst.PosInOrnament >= curOrnament.Data.size())
        {
          dst.PosInOrnament = curOrnament.Loop;
        }

        if (++dst.PosInSample >= curSample.Data.size())
        {
          if (curSample.Loop >= 0)
          {
            dst.PosInSample = curSample.Loop;
          }
          else
          {
            dst.Enabled = false;
          }
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
    const boost::shared_ptr<const STPHolder> Module;
    const STPTrack::ModuleData& Data;
    const std::vector<int_t>& Transpositions;

    AYM::ParametersHelper::Ptr AYMHelper;
    AYM::Chip::Ptr Device;

    PlaybackState CurrentState;
    STPTrack::ModuleState ModState;
    boost::array<ChannelState, AYM::CHANNELS> Channels;
  };

  Player::Ptr CreateSTPPlayer(boost::shared_ptr<const STPHolder> holder, AYM::Chip::Ptr device)
  {
    return Player::Ptr(new STPPlayer(holder, device));
  }

  bool CheckSTPModule(const uint8_t* data, std::size_t limit, const MetaContainer& container,
    Holder::Ptr& holder, ModuleRegion& region)
  {
    limit = std::min(limit, MAX_MODULE_SIZE);
    if (limit < sizeof(STPHeader))
    {
      return false;
    }
    const STPHeader* const header = safe_ptr_cast<const STPHeader*>(data);

    const std::size_t positionsOffset = fromLE(header->PositionsOffset);
    const std::size_t patternsOffset = fromLE(header->PatternsOffset);
    const std::size_t ornamentsOffset = fromLE(header->OrnamentsOffset);
    const std::size_t samplesOffset = fromLE(header->SamplesOffset);

    if (header->Tempo == 0 ||
        positionsOffset > limit ||
        patternsOffset >= ornamentsOffset || (ornamentsOffset - patternsOffset) % sizeof(STPPattern) != 0
        )
    {
      return false;
    }

    RangeChecker::Ptr checker(RangeChecker::CreateShared(limit));
    checker->AddRange(0, sizeof(*header));

    //check for patterns range
    if (!checker->AddRange(patternsOffset, ornamentsOffset - patternsOffset))
    {
      return false;
    }

    const STPId* id = safe_ptr_cast<const STPId*>(header + 1);
    if (id->Check() && !checker->AddRange(sizeof(*header), sizeof(*id)))
    {
      return false;
    }

    // check positions
    const STPPositions* const positions = safe_ptr_cast<const STPPositions*>(data + positionsOffset);
    if (!positions->Lenght ||
        !checker->AddRange(positionsOffset, sizeof(STPPositions::STPPosEntry) * (positions->Lenght - 1)) ||
        positions->Data + positions->Lenght != std::find_if(positions->Data, positions->Data + positions->Lenght,
          boost::bind<uint8_t>(std::modulus<uint8_t>(), boost::bind(&STPPositions::STPPosEntry::PatternOffset, _1), 6))
       )
    {
      return false;
    }
    //check ornaments
    const STPOrnaments* const ornaments = safe_ptr_cast<const STPOrnaments*>(data + ornamentsOffset);
    if (!checker->AddRange(ornamentsOffset, sizeof(*ornaments)))
    {
      return false;
    }
    for (const uint16_t* ornOff = ornaments->Offsets.begin(); ornOff != ornaments->Offsets.end(); ++ornOff)
    {
      const uint_t offset = fromLE(*ornOff);
      if (!offset || offset > limit)
      {
        return false;
      }
      const STPOrnament* const ornament = safe_ptr_cast<const STPOrnament*>(data + offset);
      //may be empty
      if (ornament->Size > MAX_ORNAMENT_SIZE || ornament->Loop > MAX_ORNAMENT_SIZE ||
          (ornament->Size && !checker->AddRange(offset, sizeof(*ornament) + ornament->Size - 1)))
      {
        return false;
      }
    }
    //check samples
    const STPSamples* const samples = safe_ptr_cast<const STPSamples*>(data + samplesOffset);
    if (!checker->AddRange(samplesOffset, sizeof(*samples)))
    {
      return false;
    }
    for (const uint16_t* smpOff = samples->Offsets.begin(); smpOff != samples->Offsets.end(); ++smpOff)
    {
      const uint_t offset = fromLE(*smpOff);
      if (!offset || offset > limit)
      {
        return false;
      }
      const STPSample* const sample = safe_ptr_cast<const STPSample*>(data + offset);
      //may be empty
      if (sample->Size > MAX_SAMPLE_SIZE || sample->Loop > int_t(MAX_SAMPLE_SIZE) ||
          (sample->Size && !checker->AddRange(offset, sizeof(*sample) + (sample->Size - 1) * sizeof(STPSample::Line))))
      {
        return false;
      }
    }
    //try to create holder
    try
    {
      holder.reset(new STPHolder(container, region));
#ifdef SELF_TEST
      holder->CreatePlayer();
#endif
      return true;
    }
    catch (const Error&/*e*/)
    {
      Log::Debug("STPSupp", "Failed to create holder");
    }
    return false;
  }

  //////////////////////////////////////////////////////////////////////////
  bool CreateSTPModule(const Parameters::Map& /*commonParams*/, const MetaContainer& container,
    Holder::Ptr& holder, ModuleRegion& region)
  {
    return PerformDetect(&CheckSTPModule, DETECTORS, ArrayEnd(DETECTORS),
      container, holder, region);
  }
}

namespace ZXTune
{
  void RegisterSTPSupport(PluginsEnumerator& enumerator)
  {
    PluginInformation info;
    DescribeSTPPlugin(info);
    enumerator.RegisterPlayerPlugin(info, CreateSTPModule);
  }
}
