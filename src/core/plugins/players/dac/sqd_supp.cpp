/*
Abstract:
  SQD modules playback support

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
#include <tools.h>
//library includes
#include <binary/typed_container.h>
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <devices/dac_sample_factories.h>
#include <sound/mixer_factory.h>
//std includes
#include <set>
#include <utility>
//boost includes
#include <boost/bind.hpp>
//text includes
#include <formats/text/chiptune.h>

#define FILE_TAG 44AA4DF8

namespace
{
  const Debug::Stream Dbg("Core::SQDSupp");
}

namespace SQD
{
  const std::size_t MAX_MODULE_SIZE = 0x4400 + 8 * 0x4000;
  const std::size_t MAX_POSITIONS_COUNT = 100;
  const std::size_t MAX_PATTERN_SIZE = 64;
  const std::size_t PATTERNS_COUNT = 32;
  const std::size_t CHANNELS_COUNT = 4;
  const std::size_t SAMPLES_COUNT = 16;

  //all samples has base freq at 4kHz (C-1)
  const uint_t BASE_FREQ = 4000;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct Pattern
  {
    PACK_PRE struct Line
    {
      PACK_PRE struct Channel
      {
        uint8_t NoteCmd;
        uint8_t SampleEffect;

        bool IsEmpty() const
        {
          return 0 == NoteCmd;
        }

        bool IsRest() const
        {
          return 61 == NoteCmd;
        }

        const uint8_t* GetNewTempo() const
        {
          return 62 == NoteCmd
            ? &SampleEffect : 0;
        }

        bool IsEndOfPattern() const
        {
          return 63 == NoteCmd;
        }

        const uint8_t* GetVolumeSlidePeriod() const
        {
          return 64 == NoteCmd
            ? &SampleEffect : 0;
        }

        uint_t GetVolumeSlideDirection() const
        {
          return NoteCmd >> 6;
        }

        uint_t GetNote() const
        {
          return (NoteCmd & 63) - 1;
        }

        uint_t GetSample() const
        {
          return SampleEffect & 15;
        }

        uint_t GetSampleVolume() const
        {
          return SampleEffect >> 4;
        }
      } PACK_POST;

      Channel Channels[CHANNELS_COUNT];
    } PACK_POST;

    Line Lines[MAX_PATTERN_SIZE];
  } PACK_POST;

  PACK_PRE struct SampleInfo
  {
    uint16_t Start;
    uint16_t Loop;
    uint8_t IsLooped;
    uint8_t Bank;
    uint8_t Padding[2];
  } PACK_POST;

  PACK_PRE struct LayoutInfo
  {
    uint16_t Address;
    uint8_t Bank;
    uint8_t Sectors;
  } PACK_POST;

  PACK_PRE struct Header
  {
    //+0
    uint8_t SamplesData[0x80];
    //+0x80
    uint8_t Banks[0x40];
    //+0xc0
    boost::array<LayoutInfo, 8> Layouts;
    //+0xe0
    uint8_t Padding1[0x20];
    //+0x100
    boost::array<char, 0x20> Title;
    //+0x120
    SampleInfo Samples[SAMPLES_COUNT];
    //+0x1a0
    boost::array<uint8_t, MAX_POSITIONS_COUNT> Positions;
    //+0x204
    uint8_t PositionsLimit;
    //+0x205
    uint8_t Padding2[0xb];
    //+0x210
    uint8_t Tempo;
    //+0x211
    uint8_t Loop;
    //+0x212
    uint8_t Length;
    //+0x213
    uint8_t Padding3[0xed];
    //+0x300
    boost::array<uint8_t, 8> SampleNames[SAMPLES_COUNT];
    //+0x380
    uint8_t Padding4[0x80];
    //+0x400
    boost::array<Pattern, PATTERNS_COUNT> Patterns;
    //+0x4400
  } PACK_POST;

#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(Header) == 0x4400);

  //supported tracking commands
  enum CmdType
  {
    //no parameters
    EMPTY,
    //1 param
    VOLUME_SLIDE_PERIOD,
    //1 param
    VOLUME_SLIDE,
  };

  const std::size_t BIG_SAMPLE_ADDR = 0x8000;
  const std::size_t SAMPLES_ADDR = 0xc000;
  const std::size_t SAMPLES_LIMIT = 0x10000;
}

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //stub for ornament
  struct VoidType {};

  typedef TrackingSupport<SQD::CHANNELS_COUNT, Devices::DAC::Sample::Ptr, VoidType> SQDTrack;

  // perform module 'playback' right after creating (debug purposes)
  #ifndef NDEBUG
  #define SELF_TEST
  #endif

  Renderer::Ptr CreateSQDRenderer(Parameters::Accessor::Ptr params, Information::Ptr info, SQDTrack::ModuleData::Ptr data, Devices::DAC::Chip::Ptr device);

  class SQDHolder : public Holder
  {
    static void ParsePattern(const SQD::Pattern& src, SQDTrack::BuildContext& result)
    {
      bool end = false;
      for (uint_t lineNum = 0; !end && lineNum != SQD::MAX_PATTERN_SIZE; ++lineNum)
      {
        const SQD::Pattern::Line& srcLine = src.Lines[lineNum];
        result.SetLine(lineNum);
        for (uint_t chanNum = 0; chanNum != SQD::CHANNELS_COUNT; ++chanNum)
        {
          const SQD::Pattern::Line::Channel& srcChan = srcLine.Channels[chanNum];
          if (srcChan.IsEmpty())
          {
            continue;
          }
          else if (srcChan.IsEndOfPattern())
          {
            end = true;
            break;
          }
          else if (const uint8_t* newTempo = srcChan.GetNewTempo())
          {
            result.CurLine->SetTempo(*newTempo);
            continue;
          }

          result.SetChannel(chanNum);
          CellBuilder& dstChan = *result.CurChannel;
          if (srcChan.IsRest())
          {
            dstChan.SetEnabled(false);
          }
          else
          {
            dstChan.SetEnabled(true);
            dstChan.SetNote(srcChan.GetNote());
            dstChan.SetSample(srcChan.GetSample());
            dstChan.SetVolume(srcChan.GetSampleVolume());
            if (const uint8_t* newPeriod = srcChan.GetVolumeSlidePeriod())
            {
              dstChan.AddCommand(SQD::VOLUME_SLIDE_PERIOD, *newPeriod);
            }
            if (const uint_t slideDirection = srcChan.GetVolumeSlideDirection())
            {
              dstChan.AddCommand(SQD::VOLUME_SLIDE, 1 == slideDirection ? -1 : 1);
            }
          }
        }
      }
    }

  public:
    SQDHolder(ModuleProperties::RWPtr properties, Binary::Container::Ptr rawData, std::size_t& usedSize)
      : Data(SQDTrack::ModuleData::Create())
      , Properties(properties)
      , Info(CreateTrackInfo(Data, SQD::CHANNELS_COUNT))
    {
      //assume data is correct
      const Binary::TypedContainer& data(*rawData);
      const SQD::Header& header = *data.GetField<SQD::Header>(0);

      //fill order
      const uint_t positionsCount = header.Length;
      Data->Positions.resize(positionsCount);
      std::copy(header.Positions.begin(), header.Positions.begin() + positionsCount, Data->Positions.begin());

      //fill patterns
      const std::size_t patternsCount = 1 + *std::max_element(Data->Positions.begin(), Data->Positions.end());
      SQDTrack::BuildContext builder(*Data);
      for (std::size_t patIdx = 0; patIdx < std::min(patternsCount, SQD::PATTERNS_COUNT); ++patIdx)
      {
        builder.SetPattern(patIdx);
        ParsePattern(header.Patterns[patIdx], builder);
      }

      std::size_t lastData = sizeof(header);
      //bank => <offset, size>
      typedef std::map<std::size_t, std::pair<std::size_t, std::size_t> > Bank2OffsetAndSize;
      Bank2OffsetAndSize regions;
      for (std::size_t layIdx = 0; layIdx != header.Layouts.size(); ++layIdx)
      {
        const SQD::LayoutInfo& layout = header.Layouts[layIdx];
        const std::size_t addr = fromLE(layout.Address);
        const std::size_t size = 256 * layout.Sectors;
        if (addr >= SQD::BIG_SAMPLE_ADDR && addr + size <= SQD::SAMPLES_LIMIT)
        {
          regions[layout.Bank] = std::make_pair(lastData, size);
        }
        lastData += size;
      }

      //fill samples
      Data->Samples.resize(SQD::SAMPLES_COUNT);
      for (uint_t samIdx = 0; samIdx != SQD::SAMPLES_COUNT; ++samIdx)
      {
        const SQD::SampleInfo& srcSample = header.Samples[samIdx];
        const std::size_t addr = fromLE(srcSample.Start);
        if (addr < SQD::BIG_SAMPLE_ADDR)
        {
          continue;
        }
        const std::size_t sampleBase = addr < SQD::SAMPLES_ADDR
          ? SQD::BIG_SAMPLE_ADDR
          : SQD::SAMPLES_ADDR;
        const std::size_t rawLoop = fromLE(srcSample.Loop);
        if (rawLoop < addr)
        {
          continue;
        }
        const Bank2OffsetAndSize::const_iterator it = regions.find(srcSample.Bank);
        if (it == regions.end())
        {
          continue;
        }
        const std::size_t size = std::min(SQD::SAMPLES_LIMIT - addr, it->second.second);//TODO: get from samples layout
        const std::size_t sampleOffset = it->second.first + addr - sampleBase;
        const uint8_t* const sampleStart = data.GetField<uint8_t>(sampleOffset);
        const uint8_t* const sampleEnd = std::find(sampleStart, sampleStart + size, 0);
        if (const Binary::Data::Ptr content = rawData->GetSubcontainer(sampleStart - data.GetField<uint8_t>(0), sampleEnd - sampleStart))
        {
          const std::size_t loop = srcSample.IsLooped ? rawLoop - sampleBase : content->Size();
          Data->Samples[samIdx] = Devices::DAC::CreateU8Sample(content, loop);
        }
      }
      Data->LoopPosition = header.Loop;
      Data->InitialTempo = header.Tempo;

      usedSize = lastData;

      //meta properties
      {
        const ModuleRegion fixedRegion(offsetof(SQD::Header, Patterns), sizeof(header.Patterns));
        Properties->SetSource(usedSize, fixedRegion);
      }
      const String title = *header.Title.begin() == '|' && *header.Title.rbegin() == '|'
        ? String(header.Title.begin() + 1, header.Title.end() - 1)
        : String(header.Title.begin(), header.Title.end());
      Properties->SetTitle(OptimizeString(title));
      Properties->SetProgram(Text::SQDIGITALTRACKER_DECODER_DESCRIPTION);
    }

    virtual Information::Ptr GetModuleInformation() const
    {
      return Info;
    }

    virtual Parameters::Accessor::Ptr GetModuleProperties() const
    {
      return Properties;
    }

    virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const
    {
      const Sound::FourChannelsMixer::Ptr mixer = Sound::CreateFourChannelsMixer(params);
      mixer->SetTarget(target);
      const Devices::DAC::Receiver::Ptr receiver = DAC::CreateReceiver(mixer);
      const Devices::DAC::ChipParameters::Ptr chipParams = DAC::CreateChipParameters(params);
      const Devices::DAC::Chip::Ptr chip(Devices::DAC::CreateChip(SQD::CHANNELS_COUNT, SQD::BASE_FREQ, chipParams, receiver));
      for (uint_t idx = 0, lim = Data->Samples.size(); idx != lim; ++idx)
      {
        chip->SetSample(idx, Data->Samples[idx]);
      }
      return CreateSQDRenderer(params, Info, Data, chip);
    }
  private:
    const SQDTrack::ModuleData::RWPtr Data;
    const ModuleProperties::RWPtr Properties;
    const Information::Ptr Info;
  };

  class SQDRenderer : public Renderer
  {
    struct VolumeState
    {
      VolumeState()
        : Value(16)
        , SlideDirection(0)
        , SlideCounter(0)
        , SlidePeriod(0)
      {
      }

      int_t Value;
      int_t SlideDirection;
      uint_t SlideCounter;
      uint_t SlidePeriod;

      bool Update()
      {
        if (SlideDirection && !--SlideCounter)
        {
          Value += SlideDirection;
          SlideCounter = SlidePeriod;
          if (-1 == SlideDirection && -1 == Value)
          {
            Value = 0;
          }
          else if (Value == 17)
          {
            Value = 16;
          }
          return true;
        }
        return false;
      }

      void Reset()
      {
        SlideDirection = 0;
        SlideCounter = 0;
      }
    };
  public:
    SQDRenderer(Parameters::Accessor::Ptr params, Information::Ptr info, SQDTrack::ModuleData::Ptr data, Devices::DAC::Chip::Ptr device)
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
        LastRenderTime += Params->FrameDuration();
        Devices::DAC::DataChunk chunk;
        RenderData(chunk);
        chunk.TimeStamp = LastRenderTime;
        Device->RenderData(chunk);
        Device->Flush();
        Iterator->NextFrame(Params->Looped());
      }
      return Iterator->IsValid();
    }

    virtual void Reset()
    {
      Device->Reset();
      Iterator->Reset();
      LastRenderTime = Time::Microseconds();
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
      const TrackState::Ptr state = Iterator->GetStateObserver();
      DAC::TrackBuilder track;
      SynthesizeData(*state, track);
      track.GetResult(chunk.Channels);
    }

    void SynthesizeData(const TrackState& state, DAC::TrackBuilder& track)
    {
      SynthesizeChannelsData(track);
      if (0 == state.Quirk())
      {
        GetNewLineState(state, track);
      }
    }

    void SynthesizeChannelsData(DAC::TrackBuilder& track)
    {
      for (uint_t chan = 0; chan != SQD::CHANNELS_COUNT; ++chan)
      {
        VolumeState& vol = Volumes[chan];
        if (vol.Update())
        {
          DAC::ChannelDataBuilder builder = track.GetChannel(chan);
          builder.SetLevelInPercents(100 * vol.Value / 16);
        }
      }
    }

    void GetNewLineState(const TrackState& state, DAC::TrackBuilder& track)
    {
      std::for_each(Volumes.begin(), Volumes.end(), std::mem_fun_ref(&VolumeState::Reset));
      if (const Line::Ptr line = Data->Patterns->Get(state.Pattern())->GetLine(state.Line()))
      {
        for (uint_t chan = 0; chan != SQDTrack::CHANNELS; ++chan)
        {
          if (const Cell::Ptr src = line->GetChannel(chan))
          {
            DAC::ChannelDataBuilder builder = track.GetChannel(chan);
            GetNewChannelState(*src, Volumes[chan], builder);
          }
        }
      }
    }

    void GetNewChannelState(const Cell& src, VolumeState& vol, DAC::ChannelDataBuilder& builder)
    {
      if (const bool* enabled = src.GetEnabled())
      {
        builder.SetEnabled(*enabled);
        if (!*enabled)
        {
          builder.SetPosInSample(0);
        }
      }
      if (const uint_t* note = src.GetNote())
      {
        builder.SetNote(*note);
        builder.SetPosInSample(0);
      }
      if (const uint_t* sample = src.GetSample())
      {
        builder.SetSampleNum(*sample);
        builder.SetPosInSample(0);
      }
      if (const uint_t* volume = src.GetVolume())
      {
        vol.Value = *volume;
        builder.SetLevelInPercents(100 * vol.Value / 16);
      }
      for (CommandsIterator it = src.GetCommands(); it; ++it)
      {
        switch (it->Type)
        {
        case SQD::VOLUME_SLIDE_PERIOD:
          vol.SlideCounter = vol.SlidePeriod = it->Param1;
          break;
        case SQD::VOLUME_SLIDE:
          vol.SlideDirection = it->Param1;
          break;
        default:
          assert(!"Invalid command");
        }
      }
    }
  private:
    const SQDTrack::ModuleData::Ptr Data;
    const DAC::TrackParameters::Ptr Params;
    const Devices::DAC::Chip::Ptr Device;
    const StateIterator::Ptr Iterator;
    boost::array<VolumeState, SQD::CHANNELS_COUNT> Volumes;
    Time::Microseconds LastRenderTime;
  };

  Renderer::Ptr CreateSQDRenderer(Parameters::Accessor::Ptr params, Information::Ptr info, SQDTrack::ModuleData::Ptr data, Devices::DAC::Chip::Ptr device)
  {
    return Renderer::Ptr(new SQDRenderer(params, info, data, device));
  }

  bool CheckSQD(const Binary::Container& data)
  {
    //check for header
    const std::size_t size(data.Size());
    if (sizeof(SQD::Header) > size)
    {
      return false;
    }
    const SQD::Header* const header(safe_ptr_cast<const SQD::Header*>(data.Start()));
    //check layout
    std::size_t lastData = sizeof(*header);
    std::set<uint_t> banks, bigBanks;
    for (std::size_t layIdx = 0; layIdx != header->Layouts.size(); ++layIdx)
    {
      const SQD::LayoutInfo& layout = header->Layouts[layIdx];
      const std::size_t addr = fromLE(layout.Address);
      const std::size_t size = 256 * layout.Sectors;
      if (addr + size > SQD::SAMPLES_LIMIT)
      {
        continue;
      }
      if (addr >= SQD::SAMPLES_ADDR)
      {
        banks.insert(layout.Bank);
      }
      else if (addr >= SQD::BIG_SAMPLE_ADDR)
      {
        bigBanks.insert(layout.Bank);
      }
      lastData += size;
    }
    if (lastData > size)
    {
      return false;
    }
    if (bigBanks.size() > 1)
    {
      return false;
    }

    //check samples
    for (uint_t samIdx = 0; samIdx != SQD::SAMPLES_COUNT; ++samIdx)
    {
      const SQD::SampleInfo& srcSample = header->Samples[samIdx];
      const std::size_t addr = fromLE(srcSample.Start);
      if (addr < SQD::BIG_SAMPLE_ADDR)
      {
        continue;
      }
      const std::size_t loop = fromLE(srcSample.Loop);
      if (loop < addr)
      {
        return false;
      }
      const bool bigSample = addr < SQD::SAMPLES_ADDR;
      if (!(bigSample ? bigBanks : banks).count(srcSample.Bank))
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
  const Char ID[] = {'S', 'Q', 'D', 0};
  const Char* const INFO = Text::SQDIGITALTRACKER_DECODER_DESCRIPTION;
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_4DAC | CAP_CONV_RAW;


  const std::string SQD_FORMAT(
    "?{192}"
    //layouts
    "(0080-c0 58-5f 01-80){8}"
    "?{32}"
    //title
    "20-7f{32}"
    "?{128}"
    //positions
    "00-1f{100}"
    "ff"
    "?{11}"
    //tempo???
    "02-10"
    //loop
    "00-63"
    //length
    "01-64"
  );

  class SQDModulesFactory : public ModulesFactory
  {
  public:
    SQDModulesFactory()
      : Format(Binary::Format::Create(SQD_FORMAT, sizeof(SQD::Header)))
    {
    }

    virtual bool Check(const Binary::Container& inputData) const
    {
      return Format->Match(inputData) && CheckSQD(inputData);
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
        const Holder::Ptr holder(new SQDHolder(properties, data, usedSize));
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
  void RegisterSQDSupport(PlayerPluginsRegistrator& registrator)
  {
    const ModulesFactory::Ptr factory = boost::make_shared<SQDModulesFactory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, INFO, CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
