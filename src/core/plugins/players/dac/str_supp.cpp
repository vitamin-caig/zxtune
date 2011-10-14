/*
Abstract:
  STR modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "dac_base.h"
#include "core/plugins/registrator.h"
#include "core/plugins/utils.h"
#include "core/plugins/players/creation_result.h"
#include "core/plugins/players/module_properties.h"
#include "core/plugins/players/tracking.h"
//common includes
#include <byteorder.h>
#include <error_tools.h>
#include <logging.h>
#include <tools.h>
//library includes
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <core/error_codes.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
//std includes
#include <utility>
//boost includes
#include <boost/bind.hpp>
//text includes
#include <core/text/core.h>
#include <core/text/plugins.h>
#include <core/text/warnings.h>

#define FILE_TAG ADBE77A4

namespace STR
{
  const std::size_t MAX_MODULE_SIZE = 0x87a0;
  const std::size_t MAX_POSITIONS_COUNT = 0x40;
  const std::size_t MAX_PATTERN_SIZE = 64;
  const std::size_t PATTERNS_COUNT = 16;
  const std::size_t CHANNELS_COUNT = 3;
  const std::size_t SAMPLES_COUNT = 16;

  //all samples has base freq at 2kHz (C-1)
  const uint_t BASE_FREQ = 2000;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct Pattern
  {
    static const uint8_t LIMIT = 0xff;

    PACK_PRE struct Line
    {
      uint8_t Note[CHANNELS_COUNT];
      uint8_t Sample[CHANNELS_COUNT];
    } PACK_POST;

    Line Lines[MAX_PATTERN_SIZE];
    uint8_t Limit;
  } PACK_POST;

  PACK_PRE struct SampleInfo
  {
    uint8_t AddrHi;
    uint8_t SizeHiDoubled;
  } PACK_POST;

  PACK_PRE struct Header
  {
    //+0
    uint8_t Tempo;
    //+1
    boost::array<uint8_t, MAX_POSITIONS_COUNT> Positions;
    //+0x41
    boost::array<uint16_t, MAX_POSITIONS_COUNT> PositionsPtrs;
    //+0xc1
    char Title[10];
    //+0xcb
    uint8_t LastPositionDoubled;
    //+0xcc
    boost::array<Pattern, PATTERNS_COUNT> Patterns;
    //+0x18dc
    SampleInfo SampleDescriptions[SAMPLES_COUNT];
    //+0x18fc
    uint8_t Padding[4];
    //+0x1900
    boost::array<uint8_t, 10> SampleNames[SAMPLES_COUNT];
    //+0x19a0
    uint8_t Samples[1];
  } PACK_POST;

#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(Header) == 0x19a1);

  const uint_t NOTE_EMPTY = 0;
  const uint_t NOTE_BASE = 1;

  const uint_t SAMPLE_EMPTY = 0;
  const uint_t PAUSE = 0x10;
  const uint_t SAMPLE_BASE = 0x25;

  const uint_t MODULE_BASE = 0x7260;
  const uint_t SAMPLES_ADDR = MODULE_BASE + offsetof(Header, Samples);
  const uint_t SAMPLES_LIMIT_ADDR = 0xfa00;
}

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //stub for ornament
  struct VoidType {};

  typedef TrackingSupport<STR::CHANNELS_COUNT, uint_t, Dump, VoidType> STRTrack;

  // perform module 'playback' right after creating (debug purposes)
  #ifndef NDEBUG
  #define SELF_TEST
  #endif

  Renderer::Ptr CreateSTRRenderer(Parameters::Accessor::Ptr params, Information::Ptr info, STRTrack::ModuleData::Ptr data, Devices::DAC::Chip::Ptr device);

  class STRHolder : public Holder
  {
    static void ParsePattern(const STR::Pattern& src, STRTrack::Pattern& res)
    {
      STRTrack::Pattern result;
      for (uint_t lineNum = 0; lineNum != STR::MAX_PATTERN_SIZE; ++lineNum)
      {
        const STR::Pattern::Line& srcLine = src.Lines[lineNum];
        if (srcLine.Note[0] == STR::Pattern::LIMIT)
        {
          break;
        }
        STRTrack::Line& dstLine = result.AddLine();
        for (uint_t chanNum = 0; chanNum != STR::CHANNELS_COUNT; ++chanNum)
        {
          STRTrack::Line::Chan& dstChan = dstLine.Channels[chanNum];
          const uint_t note = srcLine.Note[chanNum];
          if (STR::NOTE_EMPTY != note)
          {
            dstChan.Enabled = true;
            dstChan.Note = note - STR::NOTE_BASE;
          }
          switch (const uint_t sample = srcLine.Sample[chanNum])
          {
          case STR::PAUSE:
            dstChan.Enabled = false;
          case STR::SAMPLE_EMPTY:
            break;
          default:
            {
              const uint_t sampleNum = sample - STR::SAMPLE_BASE;
              if (sampleNum < STR::SAMPLES_COUNT)
              {
                dstChan.SampleNum = sampleNum;
              }
            }
          }
        }
      }
      result.Swap(res);
    }

  public:
    STRHolder(ModuleProperties::RWPtr properties, Binary::Container::Ptr rawData, std::size_t& usedSize)
      : Data(STRTrack::ModuleData::Create())
      , Properties(properties)
      , Info(CreateTrackInfo(Data, STR::CHANNELS_COUNT))
    {
      //assume data is correct
      const IO::FastDump& data(*rawData);
      const STR::Header* const header(safe_ptr_cast<const STR::Header*>(data.Data()));

      //fill order
      const uint_t positionsCount = header->LastPositionDoubled / 2;
      Data->Positions.resize(positionsCount);
      std::transform(header->Positions.begin(), header->Positions.begin() + positionsCount, Data->Positions.begin(),
        std::bind2nd(std::minus<uint8_t>(), uint8_t(1)));

      //fill patterns
      const std::size_t patternsCount = 1 + *std::max_element(Data->Positions.begin(), Data->Positions.end());
      Data->Patterns.resize(patternsCount);
      for (std::size_t patIdx = 0; patIdx < std::min(patternsCount, STR::PATTERNS_COUNT); ++patIdx)
      {
        ParsePattern(header->Patterns[patIdx], Data->Patterns[patIdx]);
      }
      //fill samples
      std::size_t lastData = 0;
      Data->Samples.resize(STR::SAMPLES_COUNT);
      for (uint_t samIdx = 0; samIdx != STR::SAMPLES_COUNT; ++samIdx)
      {
        const uint_t absAddr = 256 * header->SampleDescriptions[samIdx].AddrHi;
        if (!absAddr)
        {
          continue;
        }
        const std::size_t maxSize = 128 * header->SampleDescriptions[samIdx].SizeHiDoubled;
        if (absAddr + maxSize > STR::SAMPLES_LIMIT_ADDR)
        {
          continue;
        }
        const uint8_t* const sampleStart = header->Samples + (absAddr - STR::SAMPLES_ADDR);
        const uint8_t* const sampleEnd = sampleStart + maxSize;
        Data->Samples[samIdx].assign(sampleStart, std::find(sampleStart, sampleEnd, 0));
        lastData = std::max(lastData, std::size_t(absAddr - STR::MODULE_BASE + maxSize));
      }
      Data->LoopPosition = 0;
      Data->InitialTempo = header->Tempo;

      usedSize = lastData;

      //meta properties
      {
        const ModuleRegion fixedRegion(offsetof(STR::Header, Patterns), sizeof(header->Patterns));
        Properties->SetSource(usedSize, fixedRegion);
      }
      Properties->SetTitle(OptimizeString(FromCharArray(header->Title)));
      Properties->SetProgram(Text::STR_EDITOR);
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

      const Devices::DAC::Receiver::Ptr receiver = DAC::CreateReceiver(target);
      const Devices::DAC::ChipParameters::Ptr chipParams = DAC::CreateChipParameters(params);
      const Devices::DAC::Chip::Ptr chip(Devices::DAC::CreateChip(STR::CHANNELS_COUNT, totalSamples, STR::BASE_FREQ, chipParams, receiver));
      for (uint_t idx = 0; idx != totalSamples; ++idx)
      {
        const Dump& smp(Data->Samples[idx]);
        if (const std::size_t size = smp.size())
        {
          chip->SetSample(idx, smp, size);
        }
      }
      return CreateSTRRenderer(params, Info, Data, chip);
    }

    virtual Error Convert(const Conversion::Parameter& spec, Parameters::Accessor::Ptr /*params*/, Dump& dst) const
    {
      using namespace Conversion;
      if (parameter_cast<RawConvertParam>(&spec))
      {
        Properties->GetData(dst);
      }
      else
      {
        return Error(THIS_LINE, ERROR_MODULE_CONVERT, Text::MODULE_ERROR_CONVERSION_UNSUPPORTED);
      }
      return Error();
    }
  private:
    const STRTrack::ModuleData::RWPtr Data;
    const ModuleProperties::RWPtr Properties;
    const Information::Ptr Info;
  };

  class STRRenderer : public Renderer
  {
  public:
    STRRenderer(Parameters::Accessor::Ptr params, Information::Ptr info, STRTrack::ModuleData::Ptr data, Devices::DAC::Chip::Ptr device)
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
      const STRTrack::Line* const line = Data->Patterns[state->Pattern()].GetLine(state->Line());
      for (uint_t chan = 0; chan != STR::CHANNELS_COUNT; ++chan)
      {
        //begin note
        if (line && 0 == state->Quirk())
        {
          const STRTrack::Line::Chan& src = line->Channels[chan];
          Devices::DAC::DataChunk::ChannelData dst;
          dst.Channel = chan;
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
          }
          if (src.SampleNum)
          {
            dst.SampleNum = *src.SampleNum;
            dst.PosInSample = 0;
          }
          //store if smth new
          if (dst.Enabled || dst.Note || dst.SampleNum || dst.PosInSample)
          {
            res.push_back(dst);
          }
        }
      }
      chunk.Channels.swap(res);
    }
  private:
    const STRTrack::ModuleData::Ptr Data;
    const DAC::TrackParameters::Ptr Params;
    const Devices::DAC::Chip::Ptr Device;
    const StateIterator::Ptr Iterator;
    uint64_t LastRenderTime;
  };

  Renderer::Ptr CreateSTRRenderer(Parameters::Accessor::Ptr params, Information::Ptr info, STRTrack::ModuleData::Ptr data, Devices::DAC::Chip::Ptr device)
  {
    return Renderer::Ptr(new STRRenderer(params, info, data, device));
  }

  bool CheckSTR(const Binary::Container& data)
  {
    //check for header
    const std::size_t size(data.Size());
    if (sizeof(STR::Header) > size)
    {
      return false;
    }
    const STR::Header* const header(safe_ptr_cast<const STR::Header*>(data.Data()));
    if (header->LastPositionDoubled < 2 || header->LastPositionDoubled > 128 || 0 != (header->LastPositionDoubled & 1))
    {
      return false;
    }
    //check samples
    for (uint_t samIdx = 0; samIdx != STR::SAMPLES_COUNT; ++samIdx)
    {
      const uint_t absAddr = 256 * header->SampleDescriptions[samIdx].AddrHi;
      if (!absAddr)
      {
        continue;
      }
      if (absAddr < STR::SAMPLES_ADDR)
      {
        return false;
      }
      const std::size_t maxSize = 128 * header->SampleDescriptions[samIdx].SizeHiDoubled;
      if (absAddr + maxSize > STR::SAMPLES_LIMIT_ADDR)
      {
        return false;
      }
      if (absAddr + maxSize - STR::MODULE_BASE > size)
      {
        return false;
      }
    }
    return true;
  }
}

namespace
{
  using namespace ZXTune;

  //plugin attributes
  const Char ID[] = {'S', 'T', 'R', 0};
  const Char* const INFO = Text::STR_PLUGIN_INFO;
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_3DAC | CAP_CONV_RAW;


  const std::string SAT_FORMAT(
    "01-10" //tempo
    "01-10{64}" //positions
    "?73-8b"  //first position ptr
    "+126+"  //other ptrs
    "20-7f{10}" //title
    "%xxxxxxx0" //doubled last position
  );

  class STRModulesFactory : public ModulesFactory
  {
  public:
    STRModulesFactory()
      : Format(Binary::Format::Create(SAT_FORMAT))
    {
    }

    virtual bool Check(const Binary::Container& inputData) const
    {
      return Format->Match(inputData.Data(), inputData.Size()) && CheckSTR(inputData);
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
        const Holder::Ptr holder(new STRHolder(properties, data, usedSize));
        return holder;
      }
      catch (const Error&/*e*/)
      {
        Log::Debug("Core::STRSupp", "Failed to create holder");
      }
      return Holder::Ptr();
    }
  private:
    const Binary::Format::Ptr Format;
  };
}

namespace ZXTune
{
  void RegisterSTRSupport(PluginsRegistrator& registrator)
  {
    const ModulesFactory::Ptr factory = boost::make_shared<STRModulesFactory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, INFO, CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
