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
#include "core/plugins/registrator.h"
#include "core/plugins/utils.h"
#include "core/plugins/players/creation_result.h"
#include "core/plugins/players/module_properties.h"
#include "core/plugins/players/ay/ay_conversion.h"
//common includes
#include <byteorder.h>
#include <debug_log.h>
#include <error_tools.h>
#include <tools.h>
//library includes
#include <binary/typed_container.h>
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
//boost includes
#include <boost/bind.hpp>
//text includes
#include <formats/text/chiptune.h>

#define FILE_TAG 30E3A543

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  const Debug::Stream Dbg("Core::PDTSupp");

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
    uint8_t Length;
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

  typedef TrackingSupport<CHANNELS_COUNT, uint_t, Sample, Ornament> PDTTrack;

  // perform module 'playback' right after creating (debug purposes)
  #ifndef NDEBUG
  #define SELF_TEST
  #endif

  Renderer::Ptr CreatePDTRenderer(Parameters::Accessor::Ptr params, Information::Ptr info, PDTTrack::ModuleData::Ptr data, Devices::DAC::Chip::Ptr device);

  class PDTHolder : public Holder
  {
    static void ParsePattern(const PDTPattern& src, PDTTrack::Pattern& res)
    {
      PDTTrack::Pattern result;
      bool end(false);
      for (uint_t lineNum = 0; lineNum != PATTERN_SIZE && !end; ++lineNum)
      {
        PDTTrack::Line dstLine = PDTTrack::Line();
        for (uint_t chanNum = 0; chanNum != CHANNELS_COUNT && !end; ++chanNum)
        {
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
            break;
          }
        }
        if (!end)
        {
          result.AddLine() = dstLine;
        }
      }
      result.Swap(res);
    }

  public:
    PDTHolder(ModuleProperties::RWPtr properties, Binary::Container::Ptr rawData, std::size_t& usedSize)
      : Data(PDTTrack::ModuleData::Create())
      , Properties(properties)
      , Info(CreateTrackInfo(Data, CHANNELS_COUNT))
    {
      //assume that data is ok
      const Binary::TypedContainer& data(*rawData);
      const PDTHeader& header = *data.GetField<PDTHeader>(0);

      //fill order
      Data->Positions.resize(header.Length);
      std::copy(header.Positions.begin(), header.Positions.begin() + header.Length, Data->Positions.begin());

      //fill patterns
      Data->Patterns.resize(header.Patterns.size());
      for (uint_t patIdx = 0; patIdx != header.Patterns.size(); ++patIdx)
      {
        ParsePattern(header.Patterns[patIdx], Data->Patterns[patIdx]);
      }

      //fill samples
      const uint8_t* const samplesData = data.GetField<uint8_t>(sizeof(header));
      Data->Samples.resize(header.Samples.size());
      for (uint_t samIdx = 0; samIdx != header.Samples.size(); ++samIdx)
      {
        const PDTSample& srcSample(header.Samples[samIdx]);
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
      std::transform(header.Ornaments.begin(), header.Ornaments.end(), header.OrnLoops.begin(),
        std::back_inserter(Data->Ornaments), MakeOrnament);

      Data->LoopPosition = header.Loop;
      Data->InitialTempo = header.Tempo;

      usedSize = MODULE_SIZE;
      {
        const ModuleRegion fixedRegion(sizeof(PDTHeader) - sizeof(header.Patterns), sizeof(header.Patterns));
        Properties->SetSource(usedSize, fixedRegion);
      }
      Properties->SetTitle(OptimizeString(FromCharArray(header.Title)));
      Properties->SetProgram(Text::PRODIGITRACKER_DECODER_DESCRIPTION);
    }

    virtual Plugin::Ptr GetPlugin() const
    {
      return Properties->GetPlugin();
    }

    virtual Information::Ptr GetModuleInformation() const
    {
      return Info;
    }

    virtual Parameters::Accessor::Ptr GetModuleProperties() const
    {
      return Properties;
    }

    virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::MultichannelReceiver::Ptr target) const
    {
      const uint_t totalSamples = static_cast<uint_t>(Data->Samples.size());

      const Devices::DAC::Receiver::Ptr receiver = DAC::CreateReceiver(target, CHANNELS_COUNT);
      const Devices::DAC::ChipParameters::Ptr chipParams = DAC::CreateChipParameters(params);
      const Devices::DAC::Chip::Ptr chip(Devices::DAC::CreateChip(CHANNELS_COUNT, totalSamples, BASE_FREQ, chipParams, receiver));
      for (uint_t idx = 0; idx != totalSamples; ++idx)
      {
        const Sample& smp = Data->Samples[idx];
        if (smp.Data.size())
        {
          chip->SetSample(idx, smp.Data, smp.Loop);
        }
      }
      return CreatePDTRenderer(params, Info, Data, chip);
    }

    virtual Binary::Data::Ptr Convert(const Conversion::Parameter& spec, Parameters::Accessor::Ptr /*params*/) const
    {
      using namespace Conversion;
      if (parameter_cast<RawConvertParam>(&spec))
      {
        return Properties->GetData();
      }
      throw CreateUnsupportedConversionError(THIS_LINE, spec);
    }
  private:
    const PDTTrack::ModuleData::RWPtr Data;
    const ModuleProperties::RWPtr Properties;
    const Information::Ptr Info;
  };

  class PDTRenderer : public Renderer
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
    PDTRenderer(Parameters::Accessor::Ptr params, Information::Ptr info, PDTTrack::ModuleData::Ptr data, Devices::DAC::Chip::Ptr device)
      : Data(data)
      , Params(DAC::TrackParameters::Create(params))
      , Device(device)
      , Iterator(CreateTrackStateIterator(info, Data))
      , LastRenderTime(0)
    {
#ifdef SELF_TEST
//perform self-test
      Devices::DAC::DataChunk chunk;
      while (Iterator->IsValid())
      {
        RenderData(chunk);
        Iterator->NextFrame(false);
      }
      Reset();
#endif
    }

    virtual TrackState::Ptr GetTrackState() const
    {
      return Iterator->GetStateObserver();
    }

    virtual Analyzer::Ptr GetAnalyzer() const
    {
      return DAC::CreateAnalyzer(Device);
    }

    virtual bool RenderFrame()
    {
      if (Iterator->IsValid())
      {
        LastRenderTime += Params->FrameDurationMicrosec();
        Devices::DAC::DataChunk chunk;
        RenderData(chunk);
        chunk.TimeInUs = LastRenderTime;
        Device->RenderData(chunk);
        Iterator->NextFrame(Params->Looped());
      }
      return Iterator->IsValid();
    }

    virtual void Reset()
    {
      Device->Reset();
      Iterator->Reset();
      std::for_each(Ornaments.begin(), Ornaments.end(), std::mem_fun_ref(&OrnamentState::Reset));
      LastRenderTime = 0;
    }

    virtual void SetPosition(uint_t frame)
    {
      const TrackState::Ptr state = Iterator->GetStateObserver();
      if (frame < state->Frame())
      {
        //reset to beginning in case of moving back
        Iterator->Reset();
      }
      //fast forward
      Devices::DAC::DataChunk chunk;
      while (state->Frame() < frame && Iterator->IsValid())
      {
        //do not update tick for proper rendering
        RenderData(chunk);
        Iterator->NextFrame(false);
      }
    }

  private:
    void RenderData(Devices::DAC::DataChunk& chunk)
    {
      std::vector<Devices::DAC::DataChunk::ChannelData> res;
      const TrackState::Ptr state = Iterator->GetStateObserver();
      const PDTTrack::Line* const line = Data->Patterns[state->Pattern()].GetLine(state->Line());
      for (uint_t chan = 0; chan != CHANNELS_COUNT; ++chan)
      {
        Devices::DAC::DataChunk::ChannelData dst;
        dst.Channel = chan;
        OrnamentState& ornament = Ornaments[chan];
        const int_t prevOffset = ornament.GetOffset();
        ornament.Update();
        if (line && 0 == state->Quirk())//begin note
        {
          const PDTTrack::Line::Chan& src = line->Channels[chan];

          //ChannelState& dst(Channels[chan]);
          if (src.Enabled)
          {
            if ( !(dst.Enabled = *src.Enabled) )
            {
              dst.PosInSample = 0;
            }
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
            }
            dst.Note = *src.Note;
            dst.PosInSample = 0;
          }
        }
        const int_t newOffset = ornament.GetOffset();
        if (newOffset != prevOffset)
        {
          dst.NoteSlide = newOffset;
        }

        if (dst.Enabled || dst.Note || dst.NoteSlide || dst.SampleNum || dst.PosInSample)
        {
          res.push_back(dst);
        }
      }
      chunk.Channels.swap(res);
    }
  private:
    const Information::Ptr Info;
    const PDTTrack::ModuleData::Ptr Data;
    const DAC::TrackParameters::Ptr Params;
    const Devices::DAC::Chip::Ptr Device;
    const StateIterator::Ptr Iterator;
    boost::array<OrnamentState, CHANNELS_COUNT> Ornaments;
    uint64_t LastRenderTime;
  };

  Renderer::Ptr CreatePDTRenderer(Parameters::Accessor::Ptr params, Information::Ptr info, PDTTrack::ModuleData::Ptr data, Devices::DAC::Chip::Ptr device)
  {
    return Renderer::Ptr(new PDTRenderer(params, info, data, device));
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

  bool CheckPDT(const Binary::Container& data)
  {
    //check for header
    const std::size_t size = data.Size();
    if (MODULE_SIZE > size)
    {
      return false;
    }
    //check in header
    const PDTHeader* const header = safe_ptr_cast<const PDTHeader*>(data.Start());
    if (!header->Length || header->Loop > header->Length || !header->Tempo)
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
}

namespace
{
  using namespace ZXTune;

  //plugin attributes
  const Char ID[] = {'P', 'D', 'T', 0};
  const Char* const INFO = Text::PRODIGITRACKER_DECODER_DESCRIPTION;
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_4DAC | CAP_CONV_RAW;

  const std::string PDT_FORMAT(
    //boost::array<PDTOrnament, ORNAMENTS_COUNT> Ornaments;
    "?{176}"
    //boost::array<PDTOrnamentLoop, ORNAMENTS_COUNT> OrnLoops;
    "?{22}"
    //uint8_t Padding1[6];
    "?{6}"
    //char Title[32];
    "?{32}"
    //uint8_t Tempo;
    "03-63"
    //uint8_t Start;
    "00-ef"
    //uint8_t Loop;
    "00-ef"
    //uint8_t Length;
    "01-f0"
    //uint8_t Padding2[16];
    "00{16}"
    //boost::array<PDTSample, SAMPLES_COUNT> Samples;
    /*
    uint8_t Name[8];
    uint16_t Start;
    uint16_t Size;
    uint16_t Loop;
    uint8_t Page;
    uint8_t Padding;
    */
    "(???????? ?5x|3x|c0-ff ?00-40 ?5x|3x|c0-ff 00|01|03|04|06|07 00){16}"
    //boost::array<uint8_t, POSITIONS_COUNT> Positions;
    "(00-1f){240}"
    //uint16_t LastDatas[PAGES_COUNT];
    "(?c0-ff){5}"
    /*
    uint8_t FreeRAM;
    uint8_t Padding3[5];
    */
  );

  class PDTModulesFactory : public ModulesFactory
  {
  public:
    PDTModulesFactory()
      : Format(Binary::Format::Create(PDT_FORMAT, MODULE_SIZE))
    {
    }
    
    virtual bool Check(const Binary::Container& inputData) const
    {
      return Format->Match(inputData) && CheckPDT(inputData);
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Format;
    }

    virtual Holder::Ptr CreateModule(ModuleProperties::RWPtr properties, Binary::Container::Ptr data, std::size_t& usedSize) const
    {
      try
      {
        assert(Check(*data));
        const Holder::Ptr holder(new PDTHolder(properties, data, usedSize));
        return holder;
      }
      catch (const Error&/*e*/)
      {
        Dbg("Failed to create holder");
      }
      return Holder::Ptr();
    }
  private:
    const Binary::Format::Ptr Format;
  };
}

namespace ZXTune
{
  void RegisterPDTSupport(PluginsRegistrator& registrator)
  {
    const ModulesFactory::Ptr factory = boost::make_shared<PDTModulesFactory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, INFO, CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
