/*
Abstract:
  DST modules playback support

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
#include <set>
#include <utility>
//boost includes
#include <boost/bind.hpp>
//text includes
#include <core/text/core.h>
#include <core/text/plugins.h>
#include <core/text/warnings.h>

#define FILE_TAG 3226C730

namespace
{
  const std::string THIS_MODULE("Core::DSTSupp");
}

namespace DST
{
  const std::size_t COMPILED_MODULE_SIZE = 0x1c200;
  const std::size_t MODULE_SIZE = 0x1b200;
  const std::size_t MAX_POSITIONS_COUNT = 99;
  const std::size_t MAX_PATTERN_SIZE = 64;
  const std::size_t PATTERNS_COUNT = 32;
  const std::size_t CHANNELS_COUNT = 3;
  const std::size_t SAMPLES_COUNT = 16;

  //all samples has base freq at 8kHz (C-1)
  const uint_t BASE_FREQ = 8000;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct Pattern
  {
    PACK_PRE struct Line
    {
      PACK_PRE struct Channel
      {
        uint8_t Note;
        uint8_t Sample;
      } PACK_POST;

      Channel Channels[CHANNELS_COUNT];
    } PACK_POST;

    Line Lines[MAX_PATTERN_SIZE];
  } PACK_POST;

  PACK_PRE struct SampleInfo
  {
    uint16_t Start;
    uint16_t Loop;
    uint8_t Page;
    uint8_t NumberInBank;
    uint16_t Size;
    char Name[8];
  } PACK_POST;

  PACK_PRE struct Header
  {
    //+0
    uint8_t Loop;
    //+1
    boost::array<uint8_t, MAX_POSITIONS_COUNT> Positions;
    //+0x64
    uint8_t Tempo;
    //+0x65
    uint8_t Length;
    //+0x66
    char Title[28];
    //+0x82
    uint8_t Unknown[0x46];
    //+0xc8
    uint8_t Zeroes[0x38];
    //+0x100
    SampleInfo Samples[SAMPLES_COUNT];
    //+0x200
    uint8_t FirstPage[0x4000];
    //+0x4200
    Pattern Patterns[PATTERNS_COUNT];
    //0x7200
  } PACK_POST;

#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(Header) == 0x7200);

  const uint_t NOTE_EMPTY = 0;
  const uint_t NOTE_BASE = 1;
  const uint_t NOTE_PAUSE = 0x80;
  const uint_t NOTE_SPEED = 0x81;
  const uint_t NOTE_END = 0x82;

  struct Sample
  {
    Dump Data;
    std::size_t Loop;

    Sample()
      : Loop()
    {
    }

    bool Is4Bit() const
    {
      if (Data.empty())
      {
        return false;
      }
      for (Dump::const_iterator it = Data.begin(), lim = Data.end(); it != lim; ++it)
      {
        const uint_t hiVal = *it >> 4;
        if (hiVal != 0xa && hiVal != 0xf)
        {
          return false;
        }
      }
      return true;
    }

    void Truncate()
    {
      Dump::iterator const endOf = std::find(Data.begin(), Data.end(), 0xff);
      Data.erase(endOf, Data.end());
    }

    void Convert4bitTo8Bit()
    {
      assert(Is4Bit());
      std::transform(Data.begin(), Data.end(), Data.begin(), std::bind2nd(std::multiplies<uint8_t>(), 16));
    }
  };

  static const std::size_t SAMPLES_OFFSETS[8] = {0x200, 0x7200, 0x0, 0xb200, 0xf200, 0x0, 0x13200, 0x17200};
  static const std::size_t SAMPLES_OFFSETS_COMPILED[8] = {0x200, 0x8200, 0x0, 0xc200, 0x10200, 0x0, 0x14200, 0x18200};
}

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //stub for ornament
  struct VoidType {};

  typedef TrackingSupport<DST::CHANNELS_COUNT, uint_t, DST::Sample, VoidType> DSTTrack;

  // perform module 'playback' right after creating (debug purposes)
  #ifndef NDEBUG
  #define SELF_TEST
  #endif

  Renderer::Ptr CreateDSTRenderer(Parameters::Accessor::Ptr params, Information::Ptr info, DSTTrack::ModuleData::Ptr data, Devices::DAC::Chip::Ptr device);

  class DSTHolder : public Holder
  {
    static void ParsePattern(const DST::Pattern& src, DSTTrack::Pattern& res)
    {
      DSTTrack::Pattern result;
      bool end = false;
      for (uint_t lineNum = 0; !end && lineNum != DST::MAX_PATTERN_SIZE; ++lineNum)
      {
        const DST::Pattern::Line& srcLine = src.Lines[lineNum];
        DSTTrack::Line& dstLine = result.AddLine();
        for (uint_t chanNum = 0; chanNum != DST::CHANNELS_COUNT; ++chanNum)
        {
          const DST::Pattern::Line::Channel& srcChan = srcLine.Channels[chanNum];
          DSTTrack::Line::Chan& dstChan = dstLine.Channels[chanNum];
          const uint_t note = srcChan.Note;
          if (note >= DST::NOTE_PAUSE)
          {
            if (DST::NOTE_PAUSE == note)
            {
              dstChan.Enabled = false;
            }
            else if (DST::NOTE_SPEED == note)
            {
              dstLine.Tempo = srcChan.Sample;
            }
            else if (DST::NOTE_END == note)
            {
              end = true;
              break;
            }
          }
          else if (DST::NOTE_EMPTY != note)
          {
            if (srcChan.Sample >= DST::SAMPLES_COUNT)
            {
              dstChan.Enabled = false;
            }
            else
            {
              dstChan.Enabled = true;
              dstChan.Note = note - DST::NOTE_BASE;
              dstChan.SampleNum = srcChan.Sample;
            }
          }
        }
      }
      result.Swap(res);
    }

  public:
    DSTHolder(ModuleProperties::RWPtr properties, Binary::Container::Ptr rawData, std::size_t& usedSize)
      : Data(DSTTrack::ModuleData::Create())
      , Properties(properties)
      , Info(CreateTrackInfo(Data, DST::CHANNELS_COUNT))
    {
      //assume data is correct
      const IO::FastDump& data(*rawData);
      const DST::Header* const header(safe_ptr_cast<const DST::Header*>(data.Data()));

      //fill order
      const uint_t positionsCount = header->Length;
      Data->Positions.resize(positionsCount);
      std::copy(header->Positions.begin(), header->Positions.begin() + positionsCount, Data->Positions.begin());

      //fill patterns
      const std::size_t patternsCount = 1 + *std::max_element(Data->Positions.begin(), Data->Positions.end());
      Data->Patterns.resize(patternsCount);
      for (std::size_t patIdx = 0; patIdx < std::min(patternsCount, DST::PATTERNS_COUNT); ++patIdx)
      {
        ParsePattern(header->Patterns[patIdx], Data->Patterns[patIdx]);
      }
      //fill samples
      Data->Samples.resize(DST::SAMPLES_COUNT);

      const bool isCompiled = ArrayEnd(header->Zeroes) != std::find_if(header->Zeroes, ArrayEnd(header->Zeroes), std::bind1st(std::not_equal_to<uint8_t>(), 0));
      const std::size_t* const offsets = isCompiled ? DST::SAMPLES_OFFSETS_COMPILED : DST::SAMPLES_OFFSETS;

      std::size_t fourBitSamples = 0;
      std::size_t eightBitSamples = 0;
      for (uint_t samIdx = 0; samIdx != DST::SAMPLES_COUNT; ++samIdx)
      {
        const DST::SampleInfo& srcSample = header->Samples[samIdx];
        if (!srcSample.Size)
        {
          Log::Debug(THIS_MODULE, "Empty sample %1%", samIdx);
          continue;
        }

        if (srcSample.Page < 0x51 || srcSample.Page > 0x57 || !offsets[srcSample.Page & 0xf])
        {
          Log::Debug(THIS_MODULE, "Skipped sample %1%", samIdx);
          continue;
        }

        const std::size_t PAGE_SIZE = 0x4000;

        const bool loMemPage = 0x57 == srcSample.Page;
        const std::size_t BASE_ADDR = loMemPage ? 0x8000 : 0xc000;
        const std::size_t MAX_SIZE = loMemPage ? 0x8000 : PAGE_SIZE;

        const std::size_t sampleSize = fromLE(srcSample.Size);
        if (fromLE(srcSample.Start) < BASE_ADDR ||
            fromLE(srcSample.Loop) < BASE_ADDR ||
            sampleSize > MAX_SIZE)
        {
          Log::Debug(THIS_MODULE, "Skipped invalid sample %1%", samIdx);
          continue;
        }
        const std::size_t sampleOffsetInPage = fromLE(srcSample.Start) - BASE_ADDR;

        DST::Sample& dstSample = Data->Samples[samIdx];
        if (sampleOffsetInPage < PAGE_SIZE && 
            sampleOffsetInPage + sampleSize >= PAGE_SIZE)
        {
          if (!loMemPage)
          {
            continue;//invalid sample
          }
          const std::size_t firstOffset = offsets[0] + sampleOffsetInPage;
          const uint8_t* const firstBegin = &data[firstOffset];
          const std::size_t firstCopy = PAGE_SIZE - sampleOffsetInPage;
          const std::size_t secondOffset = offsets[7];
          const uint8_t* const secondBegin = &data[secondOffset];
          const std::size_t secondCopy = sampleOffsetInPage + sampleSize - PAGE_SIZE;
          dstSample.Data.resize(sampleSize);
          std::copy(secondBegin, secondBegin + secondCopy,
            std::copy(firstBegin, firstBegin + firstCopy, dstSample.Data.begin()));
          dstSample.Loop = fromLE(srcSample.Loop) - BASE_ADDR;
          Log::Debug(THIS_MODULE, "Sample %1% splitted: #%2$05x..#%3$05x + #%4$05x..#%5$05x", samIdx, 
            firstOffset, firstOffset + firstCopy, secondOffset, secondOffset + secondCopy);
        }
        else
        {
          const std::size_t dataOffset = (loMemPage
            ? (sampleOffsetInPage < PAGE_SIZE
               ? offsets[0]
               : offsets[7] - PAGE_SIZE
              )
            : offsets[srcSample.Page & 0x0f]) + sampleOffsetInPage;
          const uint8_t* const begin = &data[dataOffset];
          dstSample.Data.assign(begin, begin + sampleSize);
          dstSample.Loop = fromLE(srcSample.Loop) - BASE_ADDR;
          Log::Debug(THIS_MODULE, "Sample %1%: #%2$05x..#%3$05x", samIdx, dataOffset, dataOffset + sampleSize);
        }
        dstSample.Truncate();
        if (dstSample.Is4Bit())
        {
          Log::Debug(THIS_MODULE, "Sample %1% is 4 bit", samIdx);
          ++fourBitSamples;
        }
        else
        {
          Log::Debug(THIS_MODULE, "Sample %1% is 8 bit", samIdx);
          ++eightBitSamples;
        }
      }
      const bool oldVersion = fourBitSamples != 0;
      if (oldVersion)
      {
        assert(0 == eightBitSamples);
        std::for_each(Data->Samples.begin(), Data->Samples.end(), std::mem_fun_ref(&DST::Sample::Convert4bitTo8Bit));
      }
      else
      {
        assert(0 == fourBitSamples);
      }

      Data->LoopPosition = header->Loop;
      Data->InitialTempo = header->Tempo;

      usedSize = isCompiled ? DST::COMPILED_MODULE_SIZE : DST::MODULE_SIZE;

      //meta properties
      {
        const ModuleRegion fixedRegion(offsetof(DST::Header, Patterns), sizeof(header->Patterns));
        Properties->SetSource(usedSize, fixedRegion);
      }
      Properties->SetTitle(OptimizeString(FromCharArray(header->Title)));
      Properties->SetProgram(oldVersion ? Text::DST_EDITOR_AY : Text::DST_EDITOR_DAC);
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
      const Devices::DAC::Chip::Ptr chip(Devices::DAC::CreateChip(DST::CHANNELS_COUNT, totalSamples, DST::BASE_FREQ, chipParams, receiver));
      for (uint_t idx = 0; idx != totalSamples; ++idx)
      {
        const DST::Sample& smp(Data->Samples[idx]);
        if (const std::size_t size = smp.Data.size())
        {
          chip->SetSample(idx, smp.Data, smp.Loop);
        }
      }
      return CreateDSTRenderer(params, Info, Data, chip);
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
    const DSTTrack::ModuleData::RWPtr Data;
    const ModuleProperties::RWPtr Properties;
    const Information::Ptr Info;
  };

  class DSTRenderer : public Renderer
  {
  public:
    DSTRenderer(Parameters::Accessor::Ptr params, Information::Ptr info, DSTTrack::ModuleData::Ptr data, Devices::DAC::Chip::Ptr device)
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
      const DSTTrack::Line* const line = Data->Patterns[state->Pattern()].GetLine(state->Line());
      for (uint_t chan = 0; chan != DST::CHANNELS_COUNT; ++chan)
      {
        //begin note
        if (line && 0 == state->Quirk())
        {
          const DSTTrack::Line::Chan& src = line->Channels[chan];
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
    const DSTTrack::ModuleData::Ptr Data;
    const DAC::TrackParameters::Ptr Params;
    const Devices::DAC::Chip::Ptr Device;
    const StateIterator::Ptr Iterator;
    uint64_t LastRenderTime;
  };

  Renderer::Ptr CreateDSTRenderer(Parameters::Accessor::Ptr params, Information::Ptr info, DSTTrack::ModuleData::Ptr data, Devices::DAC::Chip::Ptr device)
  {
    return Renderer::Ptr(new DSTRenderer(params, info, data, device));
  }

  bool IsValidPattern(const DST::Pattern& src)
  {
    bool end = false;
    for (uint_t lineNum = 0; !end && lineNum != DST::MAX_PATTERN_SIZE; ++lineNum)
    {
      const DST::Pattern::Line& srcLine = src.Lines[lineNum];
      for (uint_t chanNum = 0; chanNum != DST::CHANNELS_COUNT; ++chanNum)
      {
        const DST::Pattern::Line::Channel& srcChan = srcLine.Channels[chanNum];
        const uint_t note = srcChan.Note;
        if (note >= DST::NOTE_PAUSE)
        {
          if (note > DST::NOTE_END)
          {
            return false;
          }
          else if (DST::NOTE_END == note)
          {
            end = true;
            break;
          }
        }
        else if (DST::NOTE_EMPTY != note)
        {
          if (srcChan.Sample >= DST::SAMPLES_COUNT || srcChan.Note > DST::NOTE_BASE + 48)
          {
            return false;
          }
        }
      }
    }
    return true;
  }

  bool CheckDST(const Binary::Container& data)
  {
    //check for header
    const std::size_t size(data.Size());
    if (size < sizeof(DST::Header))
    {
      return false;
    }
    const DST::Header* const header(safe_ptr_cast<const DST::Header*>(data.Data()));
    const bool isCompiled = ArrayEnd(header->Zeroes) != std::find_if(header->Zeroes, ArrayEnd(header->Zeroes), std::bind1st(std::not_equal_to<uint8_t>(), 0));
    const std::size_t moduleSize = isCompiled ? DST::COMPILED_MODULE_SIZE : DST::MODULE_SIZE;
    if (size < moduleSize)
    {
      return false;
    }
    //check patterns
    const std::set<std::size_t> patNums = std::set<std::size_t>(header->Positions.begin(), header->Positions.begin() + header->Length);
    for (std::set<std::size_t>::const_iterator it = patNums.begin(), lim = patNums.end(); it != lim; ++it)
    {
      const DST::Pattern& pat = header->Patterns[*it];
      if (!IsValidPattern(pat))
      {
        return false;
      }
    }
    //check samples
    const std::size_t* const offsets = isCompiled ? DST::SAMPLES_OFFSETS_COMPILED : DST::SAMPLES_OFFSETS;
    for (uint_t samIdx = 0; samIdx != DST::SAMPLES_COUNT; ++samIdx)
    {
      const DST::SampleInfo& srcSample = header->Samples[samIdx];
      if (!srcSample.Size)
      {
        continue;
      }

      if (srcSample.Page < 0x51 || srcSample.Page > 0x57 || !offsets[srcSample.Page & 0xf])
      {
        return false;
      }

      const std::size_t PAGE_SIZE = 0x4000;

      const bool loMemPage = 0x57 == srcSample.Page;
      const std::size_t BASE_ADDR = loMemPage ? 0x8000 : 0xc000;
      const std::size_t MAX_SIZE = loMemPage ? 0x8000 : PAGE_SIZE;

      const std::size_t sampleSize = fromLE(srcSample.Size);
      if (fromLE(srcSample.Start) < BASE_ADDR ||
          fromLE(srcSample.Loop) < BASE_ADDR ||
          sampleSize > MAX_SIZE)
      {
        return false;
      }
      const std::size_t sampleOffsetInPage = fromLE(srcSample.Start) - BASE_ADDR;

      if (sampleOffsetInPage < PAGE_SIZE && 
          sampleOffsetInPage + sampleSize >= PAGE_SIZE &&
          !loMemPage)
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
  const Char ID[] = {'D', 'S', 'T', 0};
  const Char* const INFO = Text::DST_PLUGIN_INFO;
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_3DAC | CAP_CONV_RAW;


  const std::string DST_FORMAT(
    "00-63"     //loop
    "00-1f{99}" //positions
    "02-0f"     //tempo
    "01-64"     //length
    "20-7f{28}" //title
    //skip compiled
    "+44+"
    "ff{10}"
    "+8+"//"ae7eae7e51000000"
    "20{8}"
    //"00{56}"
  );

  class DSTModulesFactory : public ModulesFactory
  {
  public:
    DSTModulesFactory()
      : Format(Binary::Format::Create(DST_FORMAT))
    {
    }

    virtual bool Check(const Binary::Container& inputData) const
    {
      return Format->Match(inputData.Data(), inputData.Size()) && CheckDST(inputData);
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
        const Holder::Ptr holder(new DSTHolder(properties, data, usedSize));
        return holder;
      }
      catch (const Error&/*e*/)
      {
        Log::Debug("Core::DSTSupp", "Failed to create holder");
      }
      return Holder::Ptr();
    }
  private:
    const Binary::Format::Ptr Format;
  };
}

namespace ZXTune
{
  void RegisterDSTSupport(PluginsRegistrator& registrator)
  {
    const ModulesFactory::Ptr factory = boost::make_shared<DSTModulesFactory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, INFO, CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
