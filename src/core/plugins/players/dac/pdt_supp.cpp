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
#include "dac_base.h"
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
//boost includes
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
//text includes
#include <core/text/core.h>
#include <core/text/plugins.h>
#include <core/text/warnings.h>

#define FILE_TAG 30E3A543

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char PDT_PLUGIN_ID[] = {'P', 'D', 'T', 0};
  const String PDT_PLUGIN_VERSION(FromStdString("$Rev$"));

  //hints
  const uint_t ORNAMENTS_COUNT = 11;
  const uint_t SAMPLES_COUNT = 16;
  const uint_t POSITIONS_COUNT = 240;
  const uint_t PAGES_COUNT = 5;
  const uint_t CHANNELS_COUNT = 4;
  const uint_t PATTERN_SIZE = 64;
  const uint_t PATTERNS_COUNT = 32;
  const std::size_t PAGE_SIZE = 0x4000;

  //all samples has base freq at 4kHz (C-1)
  const uint_t BASE_FREQ = 4000;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  typedef boost::array<int8_t, 16> PDTOrnament;

  PACK_PRE struct PDTOrnamentLoop
  {
    uint8_t Begin;
    uint8_t End;
  } PACK_POST;

  PACK_PRE struct PDTSample
  {
    uint8_t Name[8];
    uint16_t Start;
    uint16_t Size;
    uint16_t Loop;
    uint8_t Page;
    uint8_t Padding;
  } PACK_POST;

  PACK_PRE struct PDTNote
  {
    //ccnnnnnn
    //sssspppp
    //c- command
    //n- note
    //p- parameter
    //s- sample
    uint_t GetNote() const
    {
      return NoteComm & 63;
    }

    uint_t GetCommand() const
    {
      return (NoteComm & 192) >> 6;
    }

    uint_t GetParameter() const
    {
      return ParamSample & 15;
    }

    uint_t GetSample() const
    {
      return (ParamSample & 240) >> 4;
    }
    uint8_t NoteComm;
    uint8_t ParamSample;
  } PACK_POST;

  PACK_PRE struct PDTPattern
  {
    PDTNote Notes[PATTERN_SIZE][CHANNELS_COUNT];
  } PACK_POST;

  const uint_t NOTE_EMPTY = 0;

  enum
  {
    CMD_SPECIAL = 0, //see parameter
    CMD_SPEED = 1,   //parameter- speed
    CMD_1 = 2,       //????
    CMD_2 = 3,       //????

    COMMAND_NOORNAMENT = 15,
    COMMAND_BLOCKCHANNEL = 14,
    COMMAND_ENDPATTERN = 13,
    COMMAND_CONTSAMPLE = 12,
    COMMAND_NONE = 0
    //else ornament + 1
  };

  PACK_PRE struct PDTHeader
  {
    boost::array<PDTOrnament, ORNAMENTS_COUNT> Ornaments;
    boost::array<PDTOrnamentLoop, ORNAMENTS_COUNT> OrnLoops;
    uint8_t Padding1[6];
    char Title[32];
    uint8_t Tempo;
    uint8_t Start;
    uint8_t Loop;
    uint8_t Lenght;
    uint8_t Padding2[16];
    boost::array<PDTSample, SAMPLES_COUNT> Samples;
    boost::array<uint8_t, POSITIONS_COUNT> Positions;
    uint16_t LastDatas[PAGES_COUNT];
    uint8_t FreeRAM;
    uint8_t Padding3[5];
    boost::array<PDTPattern, PATTERNS_COUNT> Patterns;
  } PACK_POST;

#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(PDTOrnament) == 16);
  BOOST_STATIC_ASSERT(sizeof(PDTOrnamentLoop) == 2);
  BOOST_STATIC_ASSERT(sizeof(PDTSample) == 16);
  BOOST_STATIC_ASSERT(sizeof(PDTNote) == 2);
  BOOST_STATIC_ASSERT(sizeof(PDTPattern) == 512);
  BOOST_STATIC_ASSERT(sizeof(PDTHeader) == 0x4300);

  const std::size_t MODULE_SIZE = sizeof(PDTHeader) + PAGES_COUNT * PAGE_SIZE;

  // ornament type, differs from default
  struct Ornament
  {
    Ornament()
      : LoopBegin(0), LoopEnd(0), Data()
    {
    }
    Ornament(const PDTOrnament& orn, const PDTOrnamentLoop& loop)
     : LoopBegin(loop.Begin), LoopEnd(loop.End), Data(orn)
    {
    }

    uint_t LoopBegin;
    uint_t LoopEnd;
    PDTOrnament Data;
  };

  inline Ornament MakeOrnament(const PDTOrnament& orn, const PDTOrnamentLoop& loop)
  {
    return Ornament(orn, loop);
  }

  inline uint_t GetPageOrder(uint_t page)
  {
    //1,3,4,6,7
    switch (page)
    {
    case 1:
      return 0;
    case 3:
      return 1;
    case 4:
      return 2;
    case 6:
      return 3;
    case 7:
      return 4;
    default:
      assert(!"Invalid page");
      return 0;
    }
  }

  //digital sample type
  struct Sample
  {
    Sample() : Loop()
    {
    }
    uint_t Loop;
    Dump Data;
  };

  typedef TrackingSupport<CHANNELS_COUNT, Sample, Ornament> PDTTrack;

  // perform module 'playback' right after creating (debug purposes)
  #ifndef NDEBUG
  #define SELF_TEST
  #endif

  Player::Ptr CreatePDTPlayer(Information::Ptr info, PDTTrack::ModuleData::Ptr data, DAC::Chip::Ptr device);

  class PDTHolder : public Holder
  {
    static void ParsePattern(const PDTPattern& src, Log::MessagesCollector& warner, PDTTrack::Pattern& res)
    {
      PDTTrack::Pattern result;
      result.reserve(PATTERN_SIZE);
      bool end(false);
      for (uint_t lineNum = 0; lineNum != PATTERN_SIZE && !end; ++lineNum)
      {
        result.push_back(PDTTrack::Line());
        PDTTrack::Line& dstLine(result.back());
        for (uint_t chanNum = 0; chanNum != CHANNELS_COUNT && !end; ++chanNum)
        {
          Log::ParamPrefixedCollector channelWarner(warner, Text::LINE_CHANNEL_WARN_PREFIX, lineNum, chanNum);

          PDTTrack::Line::Chan& dstChan(dstLine.Channels[chanNum]);
          const PDTNote& note(src.Notes[lineNum][chanNum]);
          const uint_t halftones = note.GetNote();
          if (halftones != NOTE_EMPTY)
          {
            dstChan.Enabled = true;
            dstChan.Note = halftones - 1;
            dstChan.SampleNum = note.GetSample();
          }

          switch (note.GetCommand())
          {
          case CMD_SPEED:
            Log::Assert(channelWarner, !dstLine.Tempo, Text::WARNING_DUPLICATE_TEMPO);
            dstLine.Tempo = note.GetParameter();
            break;
          case CMD_SPECIAL:
            {
              switch (note.GetParameter())
              {
              case COMMAND_NONE:
                if (dstChan.Note)
                {
                  dstChan.OrnamentNum = 0;
                }
                break;
              case COMMAND_NOORNAMENT:
                dstChan.OrnamentNum = 0;
                break;//just nothing
              case COMMAND_CONTSAMPLE:
                dstChan.SampleNum.reset();
                break;
              case COMMAND_ENDPATTERN:
                end = true;
                result.pop_back();
                break;
              case COMMAND_BLOCKCHANNEL:
                dstChan.Enabled = false;
                break;
              default:
                dstChan.OrnamentNum = note.GetParameter();
                break;
              }
            }
            break;
          default:
            channelWarner.AddMessage(Text::WARNING_UNSUPPORTED_COMMAND);
          }
        }
      }
      Log::Assert(warner, result.size() <= PATTERN_SIZE, Text::WARNING_TOO_LONG);
      result.swap(res);
    }

  public:
    PDTHolder(Plugin::Ptr plugin,
      const MetaContainer& container, ModuleRegion& region)
      : SrcPlugin(plugin)
      , Data(PDTTrack::ModuleData::Create())
      , Info(PDTTrack::ModuleInfo::Create(Data))
    {
      //assume that data is ok
      const IO::FastDump& data(*container.Data);
      const PDTHeader* const header(safe_ptr_cast<const PDTHeader*>(data.Data()));

      Log::MessagesCollector::Ptr warner(Log::MessagesCollector::Create());

      //fill order
      Data->Positions.resize(header->Lenght);
      std::copy(header->Positions.begin(), header->Positions.begin() + header->Lenght, Data->Positions.begin());

      //fill patterns
      Data->Patterns.resize(header->Patterns.size());
      for (uint_t patIdx = 0; patIdx != header->Patterns.size(); ++patIdx)
      {
        Log::ParamPrefixedCollector patternWarner(*warner, Text::PATTERN_WARN_PREFIX, patIdx);
        ParsePattern(header->Patterns[patIdx], patternWarner, Data->Patterns[patIdx]);
      }

      //fill samples
      const uint8_t* samplesData(safe_ptr_cast<const uint8_t*>(header) + sizeof(*header));
      Data->Samples.resize(header->Samples.size());
      for (uint_t samIdx = 0; samIdx != header->Samples.size(); ++samIdx)
      {
        const PDTSample& srcSample(header->Samples[samIdx]);
        Sample& dstSample(Data->Samples[samIdx]);
        const uint_t start(fromLE(srcSample.Start));
        if (srcSample.Page < PAGES_COUNT && start >= 0xc000 && srcSample.Size)
        {
          const uint8_t* const sampleData(samplesData + PAGE_SIZE * GetPageOrder(srcSample.Page) +
            (start - 0xc000));
          uint_t size = fromLE(srcSample.Size) - 1;
          while (size && sampleData[size] == 0)
          {
            --size;
          }
          ++size;
          const uint_t loop(fromLE(srcSample.Loop));
          dstSample.Loop = loop >= start ? loop - start : size;
          dstSample.Data.assign(sampleData, sampleData + size);
        }
      }

      //fill ornaments
      Data->Ornaments.reserve(ORNAMENTS_COUNT + 1);
      Data->Ornaments.push_back(Ornament());//first empty ornament
      std::transform(header->Ornaments.begin(), header->Ornaments.end(), header->OrnLoops.begin(),
        std::back_inserter(Data->Ornaments), MakeOrnament);

      //fill region
      region.Offset = 0;
      region.Size = MODULE_SIZE;
      RawData = region.Extract(*container.Data);

      //meta properties
      const ModuleProperties::Ptr props = ModuleProperties::Create(PDT_PLUGIN_ID);
      {
        const ModuleRegion fixedRegion(sizeof(PDTHeader) - sizeof(header->Patterns), sizeof(header->Patterns));
        props->SetSource(RawData, fixedRegion);
      }
      props->SetPlugins(container.Plugins);
      props->SetPath(container.Path);
      props->SetTitle(OptimizeString(FromStdString(header->Title)));
      props->SetProgram(Text::PDT_EDITOR);
      props->SetWarnings(warner);

      //set tracking
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
      const uint_t totalSamples = Data->Samples.size();
      DAC::Chip::Ptr chip(DAC::CreateChip(CHANNELS_COUNT, totalSamples, BASE_FREQ));
      for (uint_t idx = 0; idx != totalSamples; ++idx)
      {
        const Sample& smp = Data->Samples[idx];
        if (smp.Data.size())
        {
          chip->SetSample(idx, smp.Data, smp.Loop);
        }
      }
      return CreatePDTPlayer(Info, Data, chip);
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
    const PDTTrack::ModuleData::RWPtr Data;
    const PDTTrack::ModuleInfo::Ptr Info;
    IO::DataContainer::Ptr RawData;
  };

  class PDTPlayer : public Player
  {
    struct OrnamentState
    {
      OrnamentState() : Object(), Position()
      {
      }
      const Ornament* Object;
      uint_t Position;

      //offset in bytes- to convert in positions, divide by ornament element size
      int_t GetOffset() const
      {
        return Object ? Object->Data[Position] / 2: 0;
      }

      void Update()
      {
        if (Object && Position++ == Object->LoopEnd)
        {
          Position = Object->LoopBegin;
        }
      }

      void Reset()
      {
        Position = 0;
      }
    };
  public:
    PDTPlayer(Information::Ptr info, PDTTrack::ModuleData::Ptr data, DAC::Chip::Ptr device)
      : Info(info)
      , Data(data)
      , Device(DACDevice::Create(device))
      , CurrentState(MODULE_STOPPED)
      , Interpolation(false)
    {
      Reset();
#ifdef SELF_TEST
//perform self-test
      DAC::DataChunk chunk;
      do
      {
        assert(Data->Positions.size() > ModState.Track.Position);
        RenderData(chunk);
      }
      while (Data->UpdateState(*Info, Sound::LOOP_NONE, ModState));
      Reset();
#endif
    }

    virtual Information::Ptr GetInformation() const
    {
      return Info;
    }

    virtual TrackState::Ptr GetTrackState() const
    {
      return boost::make_shared<StubTrackState>(ModState);
    }

    virtual Analyzer::Ptr GetAnalyzer() const
    {
      return Device;
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
      //count actually enabled channels
      ModState.Track.Channels = Device->ActiveChannels();

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
      std::for_each(Ornaments.begin(), Ornaments.end(), std::mem_fun_ref(&OrnamentState::Reset));
      CurrentState = MODULE_STOPPED;
      return Error();
    }

    virtual Error SetPosition(uint_t frame)
    {
      frame = std::min(frame, ModState.Reference.Frame);
      if (frame < ModState.Track.Frame)
      {
        //reset to beginning in case of moving back
        const uint64_t keepTicks = ModState.Tick;
        Data->InitState(Info->Tempo(), Info->FramesCount(), ModState);
        std::for_each(Ornaments.begin(), Ornaments.end(), std::mem_fun_ref(&OrnamentState::Reset));
        ModState.Tick = keepTicks;
      }
      //fast forward
      DAC::DataChunk chunk;
      while (ModState.Track.Frame < frame)
      {
        //do not update tick for proper rendering
        assert(Data->Positions.size() > ModState.Track.Position);
        RenderData(chunk);
        if (Data->UpdateState(*Info, Sound::LOOP_NONE, ModState))
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
      for (uint_t chan = 0; chan != CHANNELS_COUNT; ++chan)
      {
        DAC::DataChunk::ChannelData dst;
        dst.Channel = chan;
        OrnamentState& ornament = Ornaments[chan];
        const int_t prevOffset = ornament.GetOffset();
        ornament.Update();
        if (0 == ModState.Track.Quirk)//begin note
        {
          const PDTTrack::Line& line = Data->Patterns[ModState.Track.Pattern][ModState.Track.Line];
          const PDTTrack::Line::Chan& src = line.Channels[chan];

          //ChannelState& dst(Channels[chan]);
          if (src.Enabled)
          {
            if ( !(dst.Enabled = *src.Enabled) )
            {
              dst.PosInSample = 0;
              dst.Mask |= DAC::DataChunk::ChannelData::MASK_POSITION;
            }
            dst.Mask |= DAC::DataChunk::ChannelData::MASK_ENABLED;
          }

          if (src.Note)
          {
            if (src.OrnamentNum)
            {
              Ornaments[chan].Object = &Data->Ornaments[*src.OrnamentNum > ORNAMENTS_COUNT ? 0 :
                *src.OrnamentNum];
              Ornaments[chan].Position = 0;
            }
            if (src.SampleNum)
            {
              dst.SampleNum = *src.SampleNum;
              dst.Mask |= DAC::DataChunk::ChannelData::MASK_SAMPLE;
            }
            dst.Note = *src.Note;
            dst.PosInSample = 0;
            dst.Mask |= DAC::DataChunk::ChannelData::MASK_NOTE | DAC::DataChunk::ChannelData::MASK_POSITION;
          }
        }
        const int_t newOffset = ornament.GetOffset();
        if (newOffset != prevOffset)
        {
          dst.NoteSlide = newOffset;
          dst.Mask |= DAC::DataChunk::ChannelData::MASK_NOTESLIDE;
        }

        if (dst.Mask)
        {
          res.push_back(dst);
        }
      }
      chunk.Channels.swap(res);
    }
  private:
    const Information::Ptr Info;
    const PDTTrack::ModuleData::Ptr Data;

    const DACDevice::Ptr Device;
    PlaybackState CurrentState;
    State ModState;
    boost::array<OrnamentState, CHANNELS_COUNT> Ornaments;
    bool Interpolation;
  };

  Player::Ptr CreatePDTPlayer(Information::Ptr info, PDTTrack::ModuleData::Ptr data, DAC::Chip::Ptr device)
  {
    return Player::Ptr(new PDTPlayer(info, data, device));
  }

  //////////////////////////////////////////////////////////////////////////
  inline bool CheckOrnament(const PDTOrnament& orn)
  {
    return orn.end() != std::find_if(orn.begin(), orn.end(), std::bind2nd(std::modulus<uint8_t>(), 2));
  }

  inline bool CheckSample(const PDTSample& samp)
  {
    return fromLE(samp.Size) > 1 &&
      (fromLE(samp.Start) < 0xc000 || fromLE(samp.Start) + fromLE(samp.Size) > 0x10000 ||
      !(samp.Page == 1 || samp.Page == 3 || samp.Page == 4 || samp.Page == 6 || samp.Page == 7));
  }

  bool CheckPDT(const IO::DataContainer& data)
  {
    //check for header
    const std::size_t size = data.Size();
    if (MODULE_SIZE > size)
    {
      return false;
    }
    //check in header
    const PDTHeader* const header = safe_ptr_cast<const PDTHeader*>(data.Data());
    if (!header->Lenght || header->Loop > header->Lenght || !header->Tempo)
    {
      return false;
    }
    if (ArrayEnd(header->LastDatas) != std::find_if(header->LastDatas, ArrayEnd(header->LastDatas),
      boost::bind(&fromLE<uint16_t>, _1) < 0xc000))
    {
      return false;
    }
    if (header->Positions.end() != std::find_if(header->Positions.begin(), header->Positions.end(),
      std::bind2nd(std::greater_equal<uint_t>(), PATTERNS_COUNT)))
    {
      return false;
    }
    //check in ornaments and samples
    if (header->Ornaments.end() != std::find_if(header->Ornaments.begin(), header->Ornaments.end(), CheckOrnament) ||
        header->Samples.end() != std::find_if(header->Samples.begin(), header->Samples.end(), CheckSample)
      )
    {
      return false;
    }
    //TODO
    return true;
  }

  class PDTPlugin : public PlayerPlugin
                  , public boost::enable_shared_from_this<PDTPlugin>
  {
  public:
    virtual String Id() const
    {
      return PDT_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::PDT_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return PDT_PLUGIN_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_STOR_MODULE | CAP_DEV_4DAC | CAP_CONV_RAW;
    }

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      return CheckPDT(inputData);
    }

    virtual Module::Holder::Ptr CreateModule(const Parameters::Accessor& /*parameters*/,
                                             const MetaContainer& container,
                                             ModuleRegion& region) const
    {
      assert(CheckPDT(*container.Data));
      //try to create holder
      try
      {
        const Plugin::Ptr plugin = shared_from_this();
        const Module::Holder::Ptr holder(new PDTHolder(plugin, container, region));
  #ifdef SELF_TEST
        holder->CreatePlayer();
  #endif
        return holder;
      }
      catch (const Error&/*e*/)
      {
        Log::Debug("Core::PDTSupp", "Failed to create holder");
      }
      return Module::Holder::Ptr();
    }
  };
}

namespace ZXTune
{
  void RegisterPDTSupport(PluginsEnumerator& enumerator)
  {
    const PlayerPlugin::Ptr plugin(new PDTPlugin());
    enumerator.RegisterPlugin(plugin);
  }
}
