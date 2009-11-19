#include "plugin_enumerator.h"
#include "tracking_supp.h"

#include "../devices/dac/dac.h"
#include "../io/container.h"
#include "../io/warnings_collector.h"

#include <error.h>

#include <player_attrs.h>
#include <convert_parameters.h>

#include <boost/array.hpp>
#include <boost/static_assert.hpp>

#include <limits>
#include <numeric>

#include <text/common.h>
#include <text/plugins.h>
#include <text/warnings.h>

#define FILE_TAG AB8BEC8B

namespace
{
  using namespace ZXTune;

  const String TEXT_CHI_VERSION(FromChar("Revision: $Rev$"));

  //////////////////////////////////////////////////////////////////////////

  const uint8_t CHI_SIGNATURE[] = {'C', 'H', 'I', 'P', 'v', '1', '.', '0'};

  const std::size_t MAX_MODULE_SIZE = 65536;
  const std::size_t MAX_PATTERN_SIZE = 64;

  typedef IO::FastDump<uint8_t> FastDump;
  //all samples has base freq at 8kHz (C-1)
  const std::size_t BASE_FREQ = 8448;

  const std::size_t CHANNELS_COUNT = 4;
  const std::size_t SAMPLES_COUNT = 16;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct CHIHeader
  {
    uint8_t Signature[8];
    uint8_t Name[32];
    uint8_t Tempo;
    uint8_t Length;
    uint8_t Loop;
    PACK_PRE struct SampleDescr
    {
      uint16_t Loop;
      uint16_t Length;
    } PACK_POST;
    SampleDescr Samples[SAMPLES_COUNT];
    uint8_t Reserved[21];
    uint8_t SampleNames[SAMPLES_COUNT][8];
    uint8_t Positions[256];
  } PACK_POST;

  const unsigned NOTE_EMPTY = 0;
  const unsigned NOTE_BASE = 1;
  const unsigned PAUSE = 63;
  PACK_PRE struct CHINote
  {
    uint8_t Command : 2;
    uint8_t Note : 6;
  } PACK_POST;

  const unsigned SOFFSET = 0;
  const unsigned SLIDEDN = 1;
  const unsigned SLIDEUP = 2;
  const unsigned SPECIAL = 3;
  PACK_PRE struct CHINoteParam
  {
    uint8_t Parameter : 4;
    uint8_t Sample : 4;
  } PACK_POST;

  PACK_PRE struct CHIPattern
  {
    CHINote Notes[MAX_PATTERN_SIZE][CHANNELS_COUNT];
    CHINoteParam Params[MAX_PATTERN_SIZE][CHANNELS_COUNT];
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(CHIHeader) == 512);
  BOOST_STATIC_ASSERT(sizeof(CHIPattern) == 512);

  enum CmdType
  {
    EMPTY,
    SAMPLE_OFFSET,  //1p
    SLIDE,          //1p
  };

  void Describing(ModulePlayer::Info& info);

  typedef Log::WarningsCollector::AutoPrefixParam<std::size_t> IndexPrefix;
  typedef Log::WarningsCollector::AutoPrefixParam2<std::size_t, std::size_t> DoublePrefix;

  struct VoidType {};

  class CHIPlayer : public Tracking::TrackPlayer<CHANNELS_COUNT, VoidType>
  {
    typedef Tracking::TrackPlayer<CHANNELS_COUNT, VoidType> Parent;

    struct GlissData
    {
      GlissData() : Sliding(), Glissade()
      {
      }
      int Sliding;
      int Glissade;

      void Update()
      {
        Sliding += Glissade;
      }
    };

    static Parent::Pattern ParsePattern(const CHIPattern& src, Log::WarningsCollector& warner)
    {
      Parent::Pattern result;
      result.reserve(MAX_PATTERN_SIZE);
      bool end(false);
      for (std::size_t lineNum = 0; lineNum != MAX_PATTERN_SIZE && !end; ++lineNum)
      {
        result.push_back(Parent::Line());
        Parent::Line& dstLine(result.back());
        for (std::size_t chanNum = 0; chanNum != CHANNELS_COUNT; ++chanNum)
        {
          DoublePrefix pfx(warner, TEXT_LINE_CHANNEL_WARN_PREFIX, lineNum, chanNum);
          Parent::Line::Chan& dstChan(dstLine.Channels[chanNum]);
          const CHINote& note(src.Notes[lineNum][chanNum]);
          const CHINoteParam& param(src.Params[lineNum][chanNum]);
          if (NOTE_EMPTY != note.Note)
          {
            if (PAUSE == note.Note)
            {
              dstChan.Enabled = false;
            }
            else
            {
              dstChan.Enabled = true;
              dstChan.Note = note.Note - NOTE_BASE;
              dstChan.SampleNum = param.Sample;
            }
          }
          switch (note.Command)
          {
          case SOFFSET:
            if (param.Parameter)
            {
              dstChan.Commands.push_back(Parent::Command(SAMPLE_OFFSET, 512 * param.Parameter));
            }
            break;
          case SLIDEDN:
            dstChan.Commands.push_back(Parent::Command(SLIDE, -static_cast<int8_t>(param.Parameter)));
            break;
          case SLIDEUP:
            dstChan.Commands.push_back(Parent::Command(SLIDE, static_cast<int8_t>(param.Parameter)));
            break;
          case SPECIAL:
            if (0 == chanNum)
            {
              warner.Assert(!dstLine.Tempo, TEXT_WARNING_DUPLICATE_TEMPO);
              dstLine.Tempo = param.Parameter;
            }
            else if (3 == chanNum)
            {
              end = true;
            }
            else
            {
              warner.Warning(TEXT_WARNING_INVALID_CHANNEL);
            }
          }
        }
      }
      warner.Assert(result.size() <= MAX_PATTERN_SIZE, TEXT_WARNING_TOO_LONG);
      return result;
    }

  public:
    CHIPlayer(const String& filename, const FastDump& data)
      : Parent(filename), Chip(DAC::CreateChip(CHANNELS_COUNT, SAMPLES_COUNT, BASE_FREQ))
    {
      //assume data is correct
      const CHIHeader* const header(safe_ptr_cast<const CHIHeader*>(&data[0]));
      Information.Statistic.Tempo = header->Tempo;
      Information.Statistic.Position = header->Length + 1;
      Information.Loop = header->Loop;

      Log::WarningsCollector warner;
      //fill order
      Data.Positions.resize(header->Length + 1);
      std::copy(header->Positions, header->Positions + header->Length + 1, Data.Positions.begin());
      //fill patterns
      Information.Statistic.Pattern = 1 + *std::max_element(Data.Positions.begin(), Data.Positions.end());
      Data.Patterns.reserve(Information.Statistic.Pattern);
      const CHIPattern* const patBegin(safe_ptr_cast<const CHIPattern*>(&data[sizeof(CHIHeader)]));
      for (const CHIPattern* pat = patBegin; pat != patBegin + Information.Statistic.Pattern; ++pat)
      {
        IndexPrefix pfx(warner, TEXT_PATTERN_WARN_PREFIX, pat - patBegin);
        Data.Patterns.push_back(ParsePattern(*pat, warner));
      }
      //fill samples
      const uint8_t* sampleData(safe_ptr_cast<const uint8_t*>(patBegin + Information.Statistic.Pattern));
      std::size_t memLeft(data.Size() - (sampleData - &data[0]));
      for (const CHIHeader::SampleDescr* sample = header->Samples; sample != ArrayEnd(header->Samples); ++sample)
      {
        const std::size_t size(std::min<std::size_t>(memLeft, fromLE(sample->Length)));
        if (size)
        {
          Chip->SetSample(sample - header->Samples, Dump(sampleData, sampleData + size), fromLE(sample->Loop));
          sampleData += align(size, 256);
          if (size != fromLE(sample->Length))
          {
            warner.Warning(TEXT_WARNING_UNEXPECTED_END);
            break;
          }
          memLeft -= align(size, 256);
        }
      }
      Information.Statistic.Pattern = Data.Patterns.size();
      Information.Statistic.Channels = CHANNELS_COUNT;
      const String& warnings(warner.GetWarnings());
      if (!warnings.empty())
      {
        Information.Properties.insert(StringMap::value_type(Module::ATTR_WARNINGS, warnings));
      }

      FillProperties(TEXT_CHI_EDITOR, String(), String(header->Name, ArrayEnd(header->Name)),
        &data[0], sampleData - &data[0]);

      InitTime();
    }

    virtual void GetInfo(Info& info) const
    {
      Describing(info);
    }

    /// Retrieving current state of sound
    virtual State GetSoundState(Sound::Analyze::ChannelsState& state) const
    {
      Chip->GetState(state);
      return PlaybackState;
    }

    /// Rendering frame
    virtual State RenderFrame(const Sound::Parameters& params, Sound::Receiver& receiver)
    {
      DAC::DataChunk data;
      data.Tick = CurrentState.Tick += params.ClocksPerFrame();

      const Line& line(Data.Patterns[CurrentState.Position.Pattern][CurrentState.Position.Note]);
      if (0 == CurrentState.Position.Frame)//begin note
      {
        for (std::size_t chan = 0; chan != line.Channels.size(); ++chan)
        {
          const Line::Chan& src(line.Channels[chan]);

          GlissData& gliss(Gliss[chan]);
          DAC::DataChunk::ChannelData dst;
          dst.Channel = chan;
          if (gliss.Sliding)
          {
            dst.FreqSlideHz = gliss.Sliding = gliss.Glissade = 0;
            dst.Mask |= DAC::DataChunk::ChannelData::MASK_FREQSLIDE;
          }

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
          for (CommandsArray::const_iterator it = src.Commands.begin(), lim = src.Commands.end(); it != lim; ++it)
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

          if (dst.Mask)
          {
            data.Channels.push_back(dst);
          }
        }
      }

      Chip->RenderData(params, data, receiver);

      std::for_each(Gliss.begin(), Gliss.end(), std::mem_fun_ref(&GlissData::Update));
      Sound::Analyze::ChannelsState state;
      Chip->GetState(state);
      
      CurrentState.Position.Channels = std::count_if(state.begin(), state.end(),
        boost::mem_fn(&Sound::Analyze::Channel::Enabled));
      
      return Parent::RenderFrame(params, receiver);
    }

    virtual State Reset()
    {
      Chip->Reset();
      return Parent::Reset();
    }
  private:
    DAC::Chip::Ptr Chip;
    boost::array<GlissData, CHANNELS_COUNT> Gliss;
  };
  //////////////////////////////////////////////////////////////////////////
  void Describing(ModulePlayer::Info& info)
  {
    info.Capabilities = CAP_DEV_SOUNDRIVE | CAP_CONV_RAW;
    info.Properties.clear();
    info.Properties.insert(StringMap::value_type(ATTR_DESCRIPTION, TEXT_CHI_INFO));
    info.Properties.insert(StringMap::value_type(ATTR_VERSION, TEXT_CHI_VERSION));
  }

  bool Checking(const String& /*filename*/, const IO::DataContainer& data, uint32_t /*capFilter*/)
  {
    //check for header
    const std::size_t size(data.Size());
    if (sizeof(CHIHeader) > size/* || size > MAX_MODULE_SIZE*/)
    {
      return false;
    }
    const CHIHeader* const header(safe_ptr_cast<const CHIHeader*>(data.Data()));
    return 0 == std::memcmp(header->Signature, CHI_SIGNATURE, sizeof(CHI_SIGNATURE));
    //TODO: additional checks
  }

  ModulePlayer::Ptr Creating(const String& filename, const IO::DataContainer& data, uint32_t /*capFilter*/)
  {
    assert(Checking(filename, data, 0) || !"Attempt to create chi player on invalid data");
    return ModulePlayer::Ptr(new CHIPlayer(filename, FastDump(data)));
  }

  PluginAutoRegistrator registrator(Checking, Creating, Describing);
}
