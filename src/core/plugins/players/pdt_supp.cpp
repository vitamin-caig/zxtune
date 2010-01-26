/*
Abstract:
  STC modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/
#include "tracking.h"
#include "utils.h"
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
#include <core/devices/dac.h>

#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <text/core.h>
#include <text/plugins.h>
#include <text/warnings.h>

#define FILE_TAG 30E3A543

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char PDT_PLUGIN_ID[] = {'P', 'D', 'T', 0};
  const String TEXT_PDT_VERSION(FromChar("Revision: $Rev$"));

  //////////////////////////////////////////////////////////////////////////
  const unsigned ORNAMENTS_COUNT = 11;
  const unsigned SAMPLES_COUNT = 16;
  const unsigned POSITIONS_COUNT = 240;
  const unsigned PAGES_COUNT = 5;
  const unsigned PAGE_SIZE = 0x4000;
  const unsigned CHANNELS_COUNT = 4;
  const unsigned PATTERN_SIZE = 64;
  const unsigned PATTERNS_COUNT = 32;

  //all samples has base freq at 4kHz (C-1)
  const unsigned BASE_FREQ = 4000;

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
    unsigned GetNote() const
    {
      return NoteComm & 63;
    }
    
    unsigned GetCommand() const
    {
      return (NoteComm & 192) >> 6;
    }
    
    unsigned GetParameter() const
    {
      return ParamSample & 15;
    }
    
    unsigned GetSample() const
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

  const unsigned NOTE_EMPTY = 0;
  const unsigned CMD_SPECIAL = 0; //see parameter
  const unsigned CMD_SPEED = 1;//parameter- speed
  const unsigned CMD_1 = 2;//????
  const unsigned CMD_2 = 3;//????

  const unsigned COMMAND_NOORNAMENT = 15;
  const unsigned COMMAND_BLOCKCHANNEL = 14;
  const unsigned COMMAND_ENDPATTERN = 13;
  const unsigned COMMAND_CONTSAMPLE = 12;
  const unsigned COMMAND_NONE = 0;
  //else ornament + 1

  PACK_PRE struct PDTHeader
  {
    boost::array<PDTOrnament, ORNAMENTS_COUNT> Ornaments;
    boost::array<PDTOrnamentLoop, ORNAMENTS_COUNT> OrnLoops;
    uint8_t Padding1[6];
    boost::array<uint8_t, 32> Title;
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

    unsigned LoopBegin;
    unsigned LoopEnd;
    PDTOrnament Data;
  };

  inline Ornament MakeOrnament(const PDTOrnament& orn, const PDTOrnamentLoop& loop)
  {
    return Ornament(orn, loop);
  }

  inline unsigned GetPageOrder(unsigned page)
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

  void DescribePDTPlugin(PluginInformation& info)
  {
    info.Id = PDT_PLUGIN_ID;
    info.Description = TEXT_PDT_INFO;
    info.Version = TEXT_PDT_VERSION;
    info.Capabilities = CAP_STOR_MODULE | CAP_DEV_4DAC | CAP_CONV_RAW;
  }

  struct Sample
  {
    Sample() : Loop()
    {
    }
    unsigned Loop;
    Dump Data;
  };

  typedef TrackingSupport<CHANNELS_COUNT, Sample, Ornament> PDTTrack;
  
  // perform module 'playback' right after creating (debug purposes)
  #ifndef NDEBUG
  #define SELF_TEST
  #endif
  
  // forward declaration
  class PDTHolder;
  Player::Ptr CreatePDTPlayer(boost::shared_ptr<const PDTHolder> mod, DAC::Chip::Ptr device);

  class PDTHolder : public Holder, public boost::enable_shared_from_this<PDTHolder>
  {
    void ParsePattern(const PDTPattern& src, Log::MessagesCollector& warner, PDTTrack::Pattern& res)
    {
      PDTTrack::Pattern result;
      result.reserve(PATTERN_SIZE);
      bool end(false);
      for (unsigned lineNum = 0; lineNum != PATTERN_SIZE && !end; ++lineNum)
      {
        result.push_back(PDTTrack::Line());
        PDTTrack::Line& dstLine(result.back());
        for (unsigned chanNum = 0; chanNum != CHANNELS_COUNT && !end; ++chanNum)
        {
          Log::ParamPrefixedCollector channelWarner(warner, TEXT_LINE_CHANNEL_WARN_PREFIX, lineNum, chanNum);

          PDTTrack::Line::Chan& dstChan(dstLine.Channels[chanNum]);
          const PDTNote& note(src.Notes[lineNum][chanNum]);
          const unsigned halftones = note.GetNote();
          if (halftones != NOTE_EMPTY)
          {
            dstChan.Enabled = true;
            dstChan.Note = halftones - 1;
            dstChan.SampleNum = note.GetSample();
          }

          switch (note.GetCommand())
          {
          case CMD_SPEED:
            Log::Assert(channelWarner, !dstLine.Tempo, TEXT_WARNING_DUPLICATE_TEMPO);
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
            channelWarner.AddMessage("Unsupported command");
          }
        }
      }
      Log::Assert(warner, result.size() <= PATTERN_SIZE, TEXT_WARNING_TOO_LONG);
      result.swap(res);
    }

  public:
    PDTHolder(const MetaContainer& container, ModuleRegion& region)
    {
      //assume that data is ok
      const IO::FastDump& data(*container.Data);
      const PDTHeader* const header(safe_ptr_cast<const PDTHeader*>(data.Data()));

      Log::MessagesCollector::Ptr warner(Log::MessagesCollector::Create());
      
      //fill order
      Data.Positions.resize(header->Lenght);
      std::copy(header->Positions.begin(), header->Positions.begin() + header->Lenght, Data.Positions.begin());
      
      //fill patterns
      Data.Patterns.resize(header->Patterns.size());
      for (unsigned patIdx = 0; patIdx != header->Patterns.size(); ++patIdx)
      {
        Log::ParamPrefixedCollector patternWarner(*warner, TEXT_PATTERN_WARN_PREFIX, patIdx);
        ParsePattern(header->Patterns[patIdx], patternWarner, Data.Patterns[patIdx]);
      }
            
      //fill samples
      const uint8_t* samplesData(safe_ptr_cast<const uint8_t*>(header) + sizeof(*header));
      Data.Samples.resize(header->Samples.size());
      for (unsigned samIdx = 0; samIdx != header->Samples.size(); ++samIdx)
      {
        const PDTSample& srcSample(header->Samples[samIdx]);
        Sample& dstSample(Data.Samples[samIdx]);
        const unsigned start(fromLE(srcSample.Start));
        if (srcSample.Page < PAGES_COUNT && start >= 0xc000 && srcSample.Size)
        {
          const uint8_t* const sampleData(samplesData + PAGE_SIZE * GetPageOrder(srcSample.Page) + 
            (start - 0xc000));
          unsigned size = fromLE(srcSample.Size) - 1;
          while (size && sampleData[size] == 0)
          {
            --size;
          }
          ++size;
          const unsigned loop(fromLE(srcSample.Loop));
          dstSample.Loop = loop >= start ? loop - start : size;
          dstSample.Data.assign(sampleData, sampleData + size);
        }
      }
      
      //fill ornaments
      Data.Ornaments.reserve(ORNAMENTS_COUNT + 1);
      Data.Ornaments.push_back(Ornament());//first empty ornament
      std::transform(header->Ornaments.begin(), header->Ornaments.end(), header->OrnLoops.begin(), 
        std::back_inserter(Data.Ornaments), MakeOrnament);

      //fill region
      region.Offset = 0;
      region.Size = MODULE_SIZE;
      
      //meta properties
      ExtractMetaProperties(PDT_PLUGIN_ID, container, region, Data.Info.Properties, RawData);
      Data.Info.Properties.insert(Parameters::Map::value_type(Module::ATTR_PROGRAM, String(TEXT_PDT_EDITOR)));
      const String& title(OptimizeString(String(header->Title.begin(), header->Title.end())));
      if (!title.empty())
      {
        Data.Info.Properties.insert(Parameters::Map::value_type(Module::ATTR_TITLE, title));
      }
      
      //tracking properties
      Data.Info.LoopPosition = header->Loop;
      Data.Info.PhysicalChannels = CHANNELS_COUNT;
      Data.Info.Statistic.Tempo = header->Tempo;
      Data.Info.Statistic.Position = static_cast<unsigned>(Data.Positions.size());
      Data.Info.Statistic.Pattern = static_cast<unsigned>(std::count_if(Data.Patterns.begin(), Data.Patterns.end(),
        !boost::bind(&PDTTrack::Pattern::empty, _1)));
      Data.Info.Statistic.Channels = CHANNELS_COUNT;
      PDTTrack::CalculateTimings(Data, Data.Info.Statistic.Frame, Data.Info.LoopFrame);
      if (const unsigned msgs = warner->CountMessages())
      {
        Data.Info.Properties.insert(Parameters::Map::value_type(Module::ATTR_WARNINGS_COUNT, msgs));
        Data.Info.Properties.insert(Parameters::Map::value_type(Module::ATTR_WARNINGS, warner->GetMessages('\n')));
      }
    }

    virtual void GetPluginInformation(PluginInformation& info) const
    {
      DescribePDTPlugin(info);
    }

    virtual void GetModuleInformation(Information& info) const
    {
      info = Data.Info;
    }
    
    virtual Player::Ptr CreatePlayer() const
    {
      const unsigned totalSamples(static_cast<unsigned>(Data.Samples.size()));
      DAC::Chip::Ptr chip(DAC::CreateChip(CHANNELS_COUNT, totalSamples, BASE_FREQ));
      for (unsigned idx = 0; idx != totalSamples; ++idx)
      {
        const Sample& smp(Data.Samples[idx]);
        if (smp.Data.size())
        {
          chip->SetSample(idx, smp.Data, smp.Loop);
        }
      }
      return CreatePDTPlayer(shared_from_this(), chip);
    }
    
    virtual Error Convert(const Conversion::Parameter& param, Dump& dst) const
    {
      using namespace Conversion;
      if (parameter_cast<RawConvertParam>(&param))
      {
        dst = RawData;
      }
      else
      {
        return Error(THIS_LINE, ERROR_MODULE_CONVERT, TEXT_MODULE_ERROR_CONVERSION_UNSUPPORTED);
      }
      return Error();
    }
  private:
    friend class PDTPlayer;
    Dump RawData;
    PDTTrack::ModuleData Data;
  };
    
  class PDTPlayer : public Player
  {
    struct OrnamentState
    {
      OrnamentState() : Object(), Position()
      {
      }
      const Ornament* Object;
      unsigned Position;

      signed GetOffset() const
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
    PDTPlayer(boost::shared_ptr<const PDTHolder> holder, DAC::Chip::Ptr device)
      : Module(holder)
      , Data(Module->Data)
      , Device(device)
      , CurrentState(MODULE_STOPPED)
      , Interpolation(false)
    {
      Reset();
#ifdef SELF_TEST
//perform self-test
      DAC::DataChunk chunk;
      do
      {
        assert(Data.Positions.size() > ModState.Track.Position);
        RenderData(chunk);
      }
      while (PDTTrack::UpdateState(Data, ModState, Sound::LOOP_NONE));
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
      analyzeState = ChanState;
      return Error();
    }

    virtual Error RenderFrame(const Sound::RenderParameters& params,
                              PlaybackState& state,
                              Sound::MultichannelReceiver& receiver)
    {
      // check if finished
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
      
      DAC::DataChunk chunk;
      ModState.Tick += params.ClocksPerFrame();
      chunk.Tick = ModState.Tick;
      chunk.Interpolate = Interpolation;
      RenderData(chunk);
      Device->RenderData(params, chunk, receiver);
      Device->GetState(ChanState);
      //count actually enabled channels
      ModState.Track.Channels = static_cast<unsigned>(std::count_if(ChanState.begin(), ChanState.end(),
        boost::mem_fn(&Analyze::Channel::Enabled)));

      if (PDTTrack::UpdateState(Data, ModState, params.Looping))
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
      PDTTrack::InitState(Data, ModState);
      std::for_each(Ornaments.begin(), Ornaments.end(), std::mem_fun_ref(&OrnamentState::Reset));
      CurrentState = MODULE_STOPPED;
      return Error();
    }

    virtual Error SetPosition(unsigned frame)
    {
      if (frame < ModState.Frame)
      {
        //reset to beginning in case of moving back
        const uint64_t keepTicks = ModState.Tick;
        PDTTrack::InitState(Data, ModState);
        std::for_each(Ornaments.begin(), Ornaments.end(), std::mem_fun_ref(&OrnamentState::Reset));
        ModState.Tick = keepTicks;
      }
      //fast forward
      DAC::DataChunk chunk;
      while (ModState.Frame < frame)
      {
        //do not update tick for proper rendering
        assert(Data.Positions.size() > ModState.Track.Position);
        RenderData(chunk);
        if (!PDTTrack::UpdateState(Data, ModState, Sound::LOOP_NONE))
        {
          break;
        }
      }
      return Error();
    }

    virtual Error SetParameters(const Parameters::Map& params)
    {
      if (const Parameters::IntType* interpolation = Parameters::FindByName<Parameters::IntType>(params,
        Parameters::ZXTune::Core::DAC::INTERPOLATION))
      {
        Interpolation = (0 != *interpolation);
      }
      return Error();
    }
    
  private:
    void RenderData(DAC::DataChunk& chunk)
    {
      std::vector<DAC::DataChunk::ChannelData> res;
      for (unsigned chan = 0; chan != CHANNELS_COUNT; ++chan)
      {
        DAC::DataChunk::ChannelData dst;
        dst.Channel = chan;
        OrnamentState& ornament(Ornaments[chan]);
        const signed prevOffset(ornament.GetOffset());
        ornament.Update();
        if (0 == ModState.Track.Frame)//begin note
        {
          const PDTTrack::Line& line(Data.Patterns[ModState.Track.Pattern][ModState.Track.Line]);
          const PDTTrack::Line::Chan& src(line.Channels[chan]);

          //ChannelState& dst(Channels[chan]);
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
            if (src.OrnamentNum)
            {
              Ornaments[chan].Object = &Data.Ornaments[*src.OrnamentNum > ORNAMENTS_COUNT ? 0 :
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
        const signed newOffset(ornament.GetOffset());
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
    const boost::shared_ptr<const PDTHolder> Module;
    const PDTTrack::ModuleData& Data;

    DAC::Chip::Ptr Device;
    PlaybackState CurrentState;
    PDTTrack::ModuleState ModState;
    boost::array<OrnamentState, CHANNELS_COUNT> Ornaments;
    Analyze::ChannelsState ChanState;
    bool Interpolation;
  };
  
  Player::Ptr CreatePDTPlayer(boost::shared_ptr<const PDTHolder> holder, DAC::Chip::Ptr device)
  {
    return Player::Ptr(new PDTPlayer(holder, device));
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

  bool Checking(const IO::DataContainer& data)
  {
    //check for header
    const std::size_t size(data.Size());
    if (MODULE_SIZE > size)
    {
      return false;
    }
    const PDTHeader* const header(safe_ptr_cast<const PDTHeader*>(data.Data()));
    if (!header->Lenght || header->Loop > header->Lenght || !header->Tempo)
    {
      return false;
    }
    if (header->Ornaments.end() != std::find_if(header->Ornaments.begin(), header->Ornaments.end(), CheckOrnament) ||
        header->Samples.end() != std::find_if(header->Samples.begin(), header->Samples.end(), CheckSample)
      )
    {
      return false;
    }
    if (ArrayEnd(header->LastDatas) != std::find_if(header->LastDatas, ArrayEnd(header->LastDatas),
      boost::bind(&fromLE<uint16_t>, _1) < 0xc000))
    {
      return false;
    }
    if (header->Positions.end() != std::find_if(header->Positions.begin(), header->Positions.end(),
      std::bind2nd(std::greater_equal<uint8_t>(), static_cast<uint8_t>(PATTERNS_COUNT))))
    {
      return false;
    }
    //TODO
    return true;
  }

  bool CreatePDTModule(const Parameters::Map& /*commonParams*/, const MetaContainer& container,
    Holder::Ptr& holder, ModuleRegion& region)
  {
    if (Checking(*container.Data))
    {
      //try to create holder
      try
      {
        holder.reset(new PDTHolder(container, region));
  #ifdef SELF_TEST
        holder->CreatePlayer();
  #endif
        return true;
      }
      catch (const Error&/*e*/)
      {
        //TODO: log error
      }
    }
    return false;
  }
}

namespace ZXTune
{
  void RegisterPDTSupport(PluginsEnumerator& enumerator)
  {
    PluginInformation info;
    DescribePDTPlugin(info);
    enumerator.RegisterPlayerPlugin(info, CreatePDTModule);
  }
}
