/*
Abstract:
  PDT modules playback support

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

  typedef TrackingSupport<CHANNELS_COUNT, CmdType, Sample, VoidType> CHITrack;

  // perform module 'playback' right after creating (debug purposes)
  #ifndef NDEBUG
  #define SELF_TEST
  #endif

  Renderer::Ptr CreateCHIRenderer(Parameters::Accessor::Ptr params, Information::Ptr info, CHITrack::ModuleData::Ptr data, Devices::DAC::Chip::Ptr device);

  class CHIHolder : public Holder
  {
    static void ParsePattern(const CHIPattern& src, CHITrack::Pattern& res)
    {
      CHITrack::Pattern result;
      bool end = false;
      for (uint_t lineNum = 0; lineNum != MAX_PATTERN_SIZE && !end; ++lineNum)
      {
        CHITrack::Line& dstLine = result.AddLine();
        for (uint_t chanNum = 0; chanNum != CHANNELS_COUNT; ++chanNum)
        {
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
              dstLine.Tempo = param.GetParameter();
            }
            //last channel - stop
            else if (3 == chanNum)
            {
              end = true;
            }
          }
        }
      }
      result.Swap(res);
    }

  public:
    CHIHolder(ModuleProperties::RWPtr properties, Parameters::Accessor::Ptr parameters, IO::DataContainer::Ptr rawData, std::size_t& usedSize)
      : Data(CHITrack::ModuleData::Create())
      , Properties(properties)
      , Info(CreateTrackInfo(Data, CHANNELS_COUNT))
      , Params(parameters)
    {
      //assume data is correct
      const IO::FastDump& data(*rawData);
      const CHIHeader* const header(safe_ptr_cast<const CHIHeader*>(data.Data()));

      //fill order
      Data->Positions.resize(header->Length + 1);
      std::copy(header->Positions, header->Positions + header->Length + 1, Data->Positions.begin());

      //fill patterns
      const uint_t patternsCount = 1 + *std::max_element(Data->Positions.begin(), Data->Positions.end());
      Data->Patterns.resize(patternsCount);
      const CHIPattern* const patBegin(safe_ptr_cast<const CHIPattern*>(&data[sizeof(CHIHeader)]));
      for (const CHIPattern* pat = patBegin; pat != patBegin + patternsCount; ++pat)
      {
        ParsePattern(*pat, Data->Patterns[pat - patBegin]);
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
            break;
          }
          memLeft -= alignedSize;
        }
      }
      Data->LoopPosition = header->Loop;
      Data->InitialTempo = header->Tempo;

      usedSize = data.Size() - memLeft;

      //meta properties
      {
        const ModuleRegion fixedRegion(sizeof(CHIHeader), sizeof(CHIPattern) * patternsCount);
        Properties->SetSource(usedSize, fixedRegion);
      }
      Properties->SetTitle(OptimizeString(FromCharArray(header->Name)));
      Properties->SetProgram(Strings::Format(Text::CHI_EDITOR, FromCharArray(header->Version)));
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
      return Parameters::CreateMergedAccessor(Params, Properties);
    }

    virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::MultichannelReceiver::Ptr target) const
    {
      const uint_t totalSamples = static_cast<uint_t>(Data->Samples.size());

      const Devices::DAC::Receiver::Ptr receiver = DAC::CreateReceiver(target);
      const Devices::DAC::ChipParameters::Ptr chipParams = DAC::CreateChipParameters(params);
      const Devices::DAC::Chip::Ptr chip(Devices::DAC::CreateChip(CHANNELS_COUNT, totalSamples, BASE_FREQ, chipParams, receiver));
      for (uint_t idx = 0; idx != totalSamples; ++idx)
      {
        const Sample& smp(Data->Samples[idx]);
        if (smp.Data.size())
        {
          chip->SetSample(idx, smp.Data, smp.Loop);
        }
      }
      return CreateCHIRenderer(params, Info, Data, chip);
    }

    virtual Error Convert(const Conversion::Parameter& param, Dump& dst) const
    {
      using namespace Conversion;
      if (parameter_cast<RawConvertParam>(&param))
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
    const CHITrack::ModuleData::RWPtr Data;
    const ModuleProperties::RWPtr Properties;
    const Information::Ptr Info;
    const Parameters::Accessor::Ptr Params;
  };

  class CHIRenderer : public Renderer
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
    CHIRenderer(Parameters::Accessor::Ptr params, Information::Ptr info, CHITrack::ModuleData::Ptr data, Devices::DAC::Chip::Ptr device)
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
      std::fill(Gliss.begin(), Gliss.end(), GlissData());
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
      const CHITrack::Line* const line = Data->Patterns[state->Pattern()].GetLine(state->Line());
      for (uint_t chan = 0; chan != CHANNELS_COUNT; ++chan)
      {
        GlissData& gliss(Gliss[chan]);
        Devices::DAC::DataChunk::ChannelData dst;
        dst.Channel = chan;
        if (gliss.Sliding)
        {
          dst.FreqSlideHz = 0;
          gliss.Sliding = gliss.Glissade = 0;
        }
        //begin note
        if (line && 0 == state->Quirk())
        {
          const CHITrack::Line::Chan& src = line->Channels[chan];
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
          for (CHITrack::CommandsArray::const_iterator it = src.Commands.begin(), lim = src.Commands.end(); it != lim; ++it)
          {
            switch (it->Type)
            {
            case SAMPLE_OFFSET:
              dst.PosInSample = it->Param1;
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
        if (dst.Enabled || dst.Note || dst.NoteSlide || dst.FreqSlideHz || dst.SampleNum || dst.PosInSample)
        {
          res.push_back(dst);
        }
      }
      chunk.Channels.swap(res);
    }
  private:
    const CHITrack::ModuleData::Ptr Data;
    const DAC::TrackParameters::Ptr Params;
    const Devices::DAC::Chip::Ptr Device;
    const StateIterator::Ptr Iterator;
    boost::array<GlissData, CHANNELS_COUNT> Gliss;
    uint64_t LastRenderTime;
  };

  Renderer::Ptr CreateCHIRenderer(Parameters::Accessor::Ptr params, Information::Ptr info, CHITrack::ModuleData::Ptr data, Devices::DAC::Chip::Ptr device)
  {
    return Renderer::Ptr(new CHIRenderer(params, info, data, device));
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

  const std::string CHI_FORMAT(
    "4348495076"    // uint8_t Signature[5];
    "3x2e3x"        // char Version[3];
  );

  class CHIPlugin : public PlayerPlugin
                  , public ModulesFactory
                  , public boost::enable_shared_from_this<CHIPlugin>
  {
  public:
    typedef boost::shared_ptr<const CHIPlugin> Ptr;
    
    CHIPlugin()
      : Format(DataFormat::Create(CHI_FORMAT))
    {
    }

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

    virtual DetectionResult::Ptr Detect(DataLocation::Ptr inputData, const Module::DetectCallback& callback) const
    {
      const CHIPlugin::Ptr self = shared_from_this();
      return DetectModuleInLocation(self, self, inputData, callback);
    }
  private:
    virtual DataFormat::Ptr GetFormat() const
    {
      return Format;
    }

    virtual Holder::Ptr CreateModule(ModuleProperties::RWPtr properties, Parameters::Accessor::Ptr parameters, IO::DataContainer::Ptr data, std::size_t& usedSize) const
    {
      try
      {
        assert(Check(*data));
        const Holder::Ptr holder(new CHIHolder(properties, parameters, data, usedSize));
        return holder;
      }
      catch (const Error&/*e*/)
      {
        Log::Debug("Core::CHISupp", "Failed to create holder");
      }
      return Holder::Ptr();
    }
  private:
    const DataFormat::Ptr Format;
  };
}

namespace ZXTune
{
  void RegisterCHISupport(PluginsRegistrator& registrator)
  {
    const PlayerPlugin::Ptr plugin(new CHIPlugin());
    registrator.RegisterPlugin(plugin);
  }
}
