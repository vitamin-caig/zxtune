/*
Abstract:
  PDT modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "../tracking.h"
#include <core/plugins/enumerator.h>
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
#include <core/error_codes.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <devices/dac.h>
//std includes
#include <utility>
//boost includes
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
//text includes
#include <core/text/core.h>
#include <core/text/plugins.h>
#include <core/text/warnings.h>

#define FILE_TAG AB8BEC8B

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char CHI_PLUGIN_ID[] = {'C', 'H', 'I', 0};
  const String CHI_PLUGIN_VERSION(FromStdString("$Rev$"));

  //////////////////////////////////////////////////////////////////////////
  const uint8_t CHI_SIGNATURE[] = {'C', 'H', 'I', 'P', 'v'};

  const std::size_t MAX_MODULE_SIZE = 65536;
  const std::size_t MAX_PATTERN_SIZE = 64;
  const uint_t CHANNELS_COUNT = 4;
  const uint_t SAMPLES_COUNT = 16;

  //all samples has base freq at 8kHz (C-1)
  const uint_t BASE_FREQ = 8448;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct CHIHeader
  {
    uint8_t Signature[5];
    char Version[3];
    char Name[32];
    uint8_t Tempo;
    uint8_t Length;
    uint8_t Loop;
    PACK_PRE struct SampleDescr
    {
      uint16_t Loop;
      uint16_t Length;
    } PACK_POST;
    boost::array<SampleDescr, SAMPLES_COUNT> Samples;
    uint8_t Reserved[21];
    uint8_t SampleNames[SAMPLES_COUNT][8];//unused
    uint8_t Positions[256];
  } PACK_POST;

  const uint_t NOTE_EMPTY = 0;
  const uint_t NOTE_BASE = 1;
  const uint_t PAUSE = 63;
  PACK_PRE struct CHINote
  {
    //NNNNNNCC
    //N - note
    //C - cmd
    uint_t GetNote() const
    {
      return (NoteCmd & 252) >> 2;
    }

    uint_t GetCommand() const
    {
      return NoteCmd & 3;
    }

    uint8_t NoteCmd;
  } PACK_POST;

  typedef boost::array<CHINote, CHANNELS_COUNT> CHINoteRow;

  //format commands
  enum
  {
    SOFFSET = 0,
    SLIDEDN = 1,
    SLIDEUP = 2,
    SPECIAL = 3
  };

  PACK_PRE struct CHINoteParam
  {
    // SSSSPPPP
    //S - sample
    //P - param
    uint_t GetParameter() const
    {
      return SampParam & 15;
    }

    uint_t GetSample() const
    {
      return (SampParam & 240) >> 4;
    }

    uint8_t SampParam;
  } PACK_POST;

  typedef boost::array<CHINoteParam, CHANNELS_COUNT> CHINoteParamRow;

  PACK_PRE struct CHIPattern
  {
    boost::array<CHINoteRow, MAX_PATTERN_SIZE> Notes;
    boost::array<CHINoteParamRow, MAX_PATTERN_SIZE> Params;
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(CHIHeader) == 512);
  BOOST_STATIC_ASSERT(sizeof(CHIPattern) == 512);

  //supported tracking commands
  enum CmdType
  {
    //no parameters
    EMPTY,
    //sample offset in samples
    SAMPLE_OFFSET,
    //slide in Hz
    SLIDE
  };

  //digital sample type
  struct Sample
  {
    Sample() : Loop()
    {
    }

    uint_t Loop;
    Dump Data;
  };

  //stub for ornament
  struct VoidType {};

  typedef TrackingSupport<CHANNELS_COUNT, Sample, VoidType> CHITrack;

  // perform module 'playback' right after creating (debug purposes)
  #ifndef NDEBUG
  #define SELF_TEST
  #endif

  Player::Ptr CreateCHIPlayer(Information::Ptr info, CHITrack::ModuleData::Ptr data, DAC::Chip::Ptr device);

  class CHIHolder : public Holder
  {
    static void ParsePattern(const CHIPattern& src, Log::MessagesCollector& warner, CHITrack::Pattern& res)
    {
      CHITrack::Pattern result;
      result.reserve(MAX_PATTERN_SIZE);
      bool end = false;
      for (uint_t lineNum = 0; lineNum != MAX_PATTERN_SIZE && !end; ++lineNum)
      {
        result.push_back(CHITrack::Line());
        CHITrack::Line& dstLine = result.back();
        for (uint_t chanNum = 0; chanNum != CHANNELS_COUNT; ++chanNum)
        {
          Log::ParamPrefixedCollector channelWarner(warner, Text::LINE_CHANNEL_WARN_PREFIX, lineNum, chanNum);

          CHITrack::Line::Chan& dstChan = dstLine.Channels[chanNum];
          const CHINote& metaNote = src.Notes[lineNum][chanNum];
          const uint_t note = metaNote.GetNote();
          const CHINoteParam& param = src.Params[lineNum][chanNum];
          if (NOTE_EMPTY != note)
          {
            if (PAUSE == note)
            {
              dstChan.Enabled = false;
            }
            else
            {
              dstChan.Enabled = true;
              dstChan.Note = note - NOTE_BASE;
              dstChan.SampleNum = param.GetSample();
            }
          }
          switch (metaNote.GetCommand())
          {
          case SOFFSET:
            if (const uint_t off = param.GetParameter())
            {
              dstChan.Commands.push_back(CHITrack::Command(SAMPLE_OFFSET, 512 * off));
            }
            break;
          case SLIDEDN:
            if (const uint_t sld = param.GetParameter())
            {
              dstChan.Commands.push_back(CHITrack::Command(SLIDE, -static_cast<int8_t>(sld)));
            }
            break;
          case SLIDEUP:
            if (const uint_t sld = param.GetParameter())
            {
              dstChan.Commands.push_back(CHITrack::Command(SLIDE, static_cast<int8_t>(sld)));
            }
            break;
          case SPECIAL:
            //first channel - tempo
            if (0 == chanNum)
            {
              Log::Assert(channelWarner, !dstLine.Tempo, Text::WARNING_DUPLICATE_TEMPO);
              dstLine.Tempo = param.GetParameter();
            }
            //last channel - stop
            else if (3 == chanNum)
            {
              end = true;
            }
            else
            {
              channelWarner.AddMessage(Text::WARNING_INVALID_CHANNEL);
            }
          }
        }
      }
      Log::Assert(warner, result.size() <= MAX_PATTERN_SIZE, Text::WARNING_TOO_LONG);
      result.swap(res);
    }

  public:
    CHIHolder(Plugin::Ptr plugin, 
      const MetaContainer& container, ModuleRegion& region)
      : SrcPlugin(plugin)
      , Data(CHITrack::ModuleData::Create())
      , Info(CHITrack::ModuleInfo::Create(Data))
    {
      //assume data is correct
      const IO::FastDump& data(*container.Data);
      const CHIHeader* const header(safe_ptr_cast<const CHIHeader*>(data.Data()));

      Log::MessagesCollector::Ptr warner(Log::MessagesCollector::Create());

      //fill order
      Data->Positions.resize(header->Length + 1);
      std::copy(header->Positions, header->Positions + header->Length + 1, Data->Positions.begin());

      //fill patterns
      const uint_t patternsCount = 1 + *std::max_element(Data->Positions.begin(), Data->Positions.end());
      Data->Patterns.resize(patternsCount);
      const CHIPattern* const patBegin(safe_ptr_cast<const CHIPattern*>(&data[sizeof(CHIHeader)]));
      for (const CHIPattern* pat = patBegin; pat != patBegin + patternsCount; ++pat)
      {
        Log::ParamPrefixedCollector patternWarner(*warner, Text::PATTERN_WARN_PREFIX, pat - patBegin);
        ParsePattern(*pat, patternWarner, Data->Patterns[pat - patBegin]);
      }
      //fill samples
      const uint8_t* sampleData(safe_ptr_cast<const uint8_t*>(patBegin + patternsCount));
      std::size_t memLeft(data.Size() - (sampleData - &data[0]));
      Data->Samples.resize(header->Samples.size());
      for (uint_t samIdx = 0; samIdx != header->Samples.size(); ++samIdx)
      {
        const CHIHeader::SampleDescr& srcSample(header->Samples[samIdx]);
        if (const std::size_t size = std::min<std::size_t>(memLeft, fromLE(srcSample.Length)))
        {
          Sample& dstSample(Data->Samples[samIdx]);
          dstSample.Loop = fromLE(srcSample.Loop);
          dstSample.Data.assign(sampleData, sampleData + size);
          const std::size_t alignedSize(align<std::size_t>(size, 256));
          sampleData += alignedSize;
          if (size != fromLE(srcSample.Length))
          {
            warner->AddMessage(Text::WARNING_UNEXPECTED_END);
            break;
          }
          memLeft -= alignedSize;
        }
      }
      //fill region
      region.Offset = 0;
      region.Size = data.Size() - memLeft;
      RawData = region.Extract(*container.Data);

      //meta properties
      const ModuleProperties::Ptr props = ModuleProperties::Create(CHI_PLUGIN_ID);
      {
        const ModuleRegion fixedRegion(sizeof(CHIHeader), sizeof(CHIPattern) * patternsCount);
        props->SetSource(RawData, fixedRegion);
      }
      props->SetPlugins(container.Plugins);
      props->SetPath(container.Path);
      props->SetTitle(OptimizeString(FromStdString(header->Name)));
      props->SetProgram((Formatter(Text::CHI_EDITOR) % FromStdString(header->Version)).str());
      props->SetWarnings(warner);

      //fill tracking properties
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
      const uint_t totalSamples(Data->Samples.size());
      DAC::Chip::Ptr chip(DAC::CreateChip(CHANNELS_COUNT, totalSamples, BASE_FREQ));
      for (uint_t idx = 0; idx != totalSamples; ++idx)
      {
        const Sample& smp(Data->Samples[idx]);
        if (smp.Data.size())
        {
          chip->SetSample(idx, smp.Data, smp.Loop);
        }
      }
      return CreateCHIPlayer(Info, Data, chip);
    }

    virtual Error Convert(const Conversion::Parameter& param, Dump& dst) const
    {
      using namespace Conversion;
      if (parameter_cast<RawConvertParam>(&param))
      {
        const uint8_t* const data = static_cast<const uint8_t*>(RawData->Data());
        dst.assign(data, data + RawData->Size());
      }
      else
      {
        return Error(THIS_LINE, ERROR_MODULE_CONVERT, Text::MODULE_ERROR_CONVERSION_UNSUPPORTED);
      }
      return Error();
    }
  private:
    const Plugin::Ptr SrcPlugin;
    const CHITrack::ModuleData::RWPtr Data;
    const CHITrack::ModuleInfo::Ptr Info;
    IO::DataContainer::Ptr RawData;
  };

  inline static Analyze::Channel AnalyzeDACState(const DAC::ChanState& dacState)
  {
    Analyze::Channel res;
    res.Enabled = dacState.Enabled;
    res.Band = dacState.Band;
    res.Level = dacState.LevelInPercents * std::numeric_limits<Analyze::LevelType>::max() / 100;
    return res;
  }

  class CHIPlayer : public Player
  {
    struct GlissData
    {
      GlissData() : Sliding(), Glissade()
      {
      }
      int_t Sliding;
      int_t Glissade;

      void Update()
      {
        Sliding += Glissade;
      }
    };
  public:
    CHIPlayer(Information::Ptr info, CHITrack::ModuleData::Ptr data, DAC::Chip::Ptr device)
      : Info(info)
      , Data(data)
      , Device(device)
      , CurrentState(MODULE_STOPPED)
      , Interpolation(false)
    {
    }

    virtual Information::Ptr GetInformation() const
    {
      return Info;
    }

    virtual Error GetPlaybackState(State& state,
                                   Analyze::ChannelsState& analyzeState) const
    {
      state = ModState;
      analyzeState.resize(ChanState.size());
      std::transform(ChanState.begin(), ChanState.end(), analyzeState.begin(), &AnalyzeDACState);
      return Error();
    }

    virtual Error RenderFrame(const Sound::RenderParameters& params,
                              PlaybackState& state,
                              Sound::MultichannelReceiver& receiver)
    {
      DAC::DataChunk chunk;
      ModState.Tick += params.ClocksPerFrame();
      chunk.Tick = ModState.Tick;
      chunk.Interpolate = Interpolation;
      RenderData(chunk);
      Device->RenderData(params, chunk, receiver);
      Device->GetState(ChanState);
      //count actually enabled channels
      ModState.Track.Channels = std::count_if(ChanState.begin(), ChanState.end(),
        boost::mem_fn(&DAC::ChanState::Enabled));

      if (Data->UpdateState(*Info, params.Looping, ModState))
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
      Data->InitState(Info->Tempo(), Info->FramesCount(), ModState);
      CurrentState = MODULE_STOPPED;
      return Error();
    }

    virtual Error SetPosition(uint_t frame)
    {
      frame = std::min(frame, ModState.Reference.Frame - 1);
      if (frame < ModState.Track.Frame)
      {
        //reset to beginning in case of moving back
        const uint64_t keepTicks = ModState.Tick;
        Data->InitState(Info->Tempo(), Info->FramesCount(), ModState);
        ModState.Tick = keepTicks;
      }
      //fast forward
      DAC::DataChunk chunk;
      while (ModState.Track.Frame < frame)
      {
        //do not update tick for proper rendering
        assert(Data->Positions.size() > ModState.Track.Position);
        RenderData(chunk);
        if (!Data->UpdateState(*Info, Sound::LOOP_NONE, ModState))
        {
          break;
        }
      }
      ModState.Frame = frame;
      return Error();
    }

    virtual Error SetParameters(const Parameters::Accessor& params)
    {
      Parameters::IntType intVal = 0;
      Interpolation = params.FindIntValue(Parameters::ZXTune::Core::DAC::INTERPOLATION, intVal) &&
        intVal != 0;
      return Error();
    }

  private:
    void RenderData(DAC::DataChunk& chunk)
    {
      std::vector<DAC::DataChunk::ChannelData> res;
      const CHITrack::Line& line(Data->Patterns[ModState.Track.Pattern][ModState.Track.Line]);
      for (uint_t chan = 0; chan != CHANNELS_COUNT; ++chan)
      {
        GlissData& gliss(Gliss[chan]);
        DAC::DataChunk::ChannelData dst;
        dst.Channel = chan;
        if (gliss.Sliding)
        {
          dst.FreqSlideHz = gliss.Sliding = gliss.Glissade = 0;
          dst.Mask |= DAC::DataChunk::ChannelData::MASK_FREQSLIDE;
        }
        //begin note
        if (0 == ModState.Track.Quirk)
        {
          const CHITrack::Line::Chan& src(line.Channels[chan]);
          if (src.Enabled)
          {
            if (!(dst.Enabled = *src.Enabled))
            {
              dst.PosInSample = 0;
              dst.Mask |= DAC::DataChunk::ChannelData::MASK_POSITION;
            }
            dst.Mask |= DAC::DataChunk::ChannelData::MASK_ENABLED;
          }
          if (src.Note)
          {
            dst.Note = *src.Note;
            dst.PosInSample = 0;
            dst.Mask |= DAC::DataChunk::ChannelData::MASK_NOTE | DAC::DataChunk::ChannelData::MASK_POSITION;
          }
          if (src.SampleNum)
          {
            dst.SampleNum = *src.SampleNum;
            dst.PosInSample = 0;
            dst.Mask |= DAC::DataChunk::ChannelData::MASK_SAMPLE | DAC::DataChunk::ChannelData::MASK_POSITION;
          }
          for (CHITrack::CommandsArray::const_iterator it = src.Commands.begin(), lim = src.Commands.end(); it != lim; ++it)
          {
            switch (it->Type)
            {
            case SAMPLE_OFFSET:
              dst.PosInSample = it->Param1;
              dst.Mask |= DAC::DataChunk::ChannelData::MASK_POSITION;
              break;
            case SLIDE:
              gliss.Glissade = it->Param1;
              break;
            default:
              assert(!"Invalid command");
            }
          }
        }
        //store if smth new
        if (dst.Mask)
        {
          res.push_back(dst);
        }
      }
      chunk.Channels.swap(res);
    }
  private:
    const Information::Ptr Info;
    const CHITrack::ModuleData::Ptr Data;
    DAC::Chip::Ptr Device;
    PlaybackState CurrentState;
    State ModState;
    boost::array<GlissData, CHANNELS_COUNT> Gliss;
    DAC::ChannelsState ChanState;
    bool Interpolation;
  };

  Player::Ptr CreateCHIPlayer(Information::Ptr info, CHITrack::ModuleData::Ptr data, DAC::Chip::Ptr device)
  {
    return Player::Ptr(new CHIPlayer(info, data, device));
  }

  bool CheckCHI(const IO::DataContainer& data)
  {
    //check for header
    const std::size_t size(data.Size());
    if (sizeof(CHIHeader) > size)
    {
      return false;
    }
    const CHIHeader* const header(safe_ptr_cast<const CHIHeader*>(data.Data()));
    if (0 != std::memcmp(header->Signature, CHI_SIGNATURE, sizeof(CHI_SIGNATURE)))
    {
      return false;
    }
    const uint_t patternsCount = 1 + *std::max_element(header->Positions, header->Positions + header->Length + 1);
    if (sizeof(*header) + patternsCount * sizeof(CHIPattern) > size)
    {
      return false;
    }
    //TODO: additional checks
    return true;
  }

  class CHIPlugin : public PlayerPlugin
                  , public boost::enable_shared_from_this<CHIPlugin>
  {
  public:
    virtual String Id() const
    {
      return CHI_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::CHI_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return CHI_PLUGIN_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_STOR_MODULE | CAP_DEV_4DAC | CAP_CONV_RAW;
    }

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      return CheckCHI(inputData);
    }
    
    virtual Module::Holder::Ptr CreateModule(const Parameters::Accessor& /*parameters*/,
                                             const MetaContainer& container,
                                             ModuleRegion& region) const
    {
      assert(CheckCHI(*container.Data));
      //try to create holder
      try
      {
        const Plugin::Ptr plugin = shared_from_this();
        const Module::Holder::Ptr holder(new CHIHolder(plugin, container, region));
#ifdef SELF_TEST
        holder->CreatePlayer();
#endif
        return holder;
      }
      catch (const Error&/*e*/)
      {
        Log::Debug("Core::CHISupp", "Failed to create holder");
      }
      return Module::Holder::Ptr();
    }
  };
}

namespace ZXTune
{
  void RegisterCHISupport(PluginsEnumerator& enumerator)
  {
    const PlayerPlugin::Ptr plugin(new CHIPlugin());
    enumerator.RegisterPlugin(plugin);
  }
}
