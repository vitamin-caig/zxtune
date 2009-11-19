#include "plugin_enumerator.h"
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
#include <algorithm>
#include <functional>

#include <text/common.h>
#include <text/plugins.h>
#include <text/warnings.h>

#define FILE_TAG 30E3A543

namespace
{
  using namespace ZXTune;

  const String TEXT_PDT_VERSION(FromChar("Revision: $Rev$"));

  //////////////////////////////////////////////////////////////////////////
  const std::size_t ORNAMENTS_COUNT = 11;
  const std::size_t SAMPLES_COUNT = 16;
  const std::size_t POSITIONS_COUNT = 240;
  const std::size_t PAGES_COUNT = 5;
  const std::size_t PAGE_SIZE = 0x4000;
  const std::size_t CHANNELS_COUNT = 4;
  const std::size_t PATTERN_SIZE = 64;
  const std::size_t PATTERNS_COUNT = 32;

  typedef IO::FastDump<uint8_t> FastDump;
  //all samples has base freq at 4kHz (C-1)
  const std::size_t BASE_FREQ = 4000;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  typedef int8_t PDTOrnament[16];

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
    uint8_t Note : 6;
    uint8_t Command : 2;
    uint8_t Parameter : 4;
    uint8_t Sample : 4;
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
    PDTOrnament Ornaments[ORNAMENTS_COUNT];
    PDTOrnamentLoop OrnLoops[ORNAMENTS_COUNT];
    uint8_t Padding1[6];
    uint8_t Title[32];
    uint8_t Tempo;
    uint8_t Start;
    uint8_t Loop;
    uint8_t Lenght;
    uint8_t Padding2[16];
    PDTSample Samples[SAMPLES_COUNT];
    uint8_t Positions[POSITIONS_COUNT];
    uint16_t LastDatas[PAGES_COUNT];
    uint8_t FreeRAM;
    uint8_t Padding3[5];
    PDTPattern Patterns[PATTERNS_COUNT];
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
      : LoopBegin(0), LoopEnd(0), Data(1)
    {
    }
    Ornament(const PDTOrnament& orn, const PDTOrnamentLoop& loop)
     : LoopBegin(loop.Begin), LoopEnd(loop.End), Data(orn, ArrayEnd(orn))
    {
    }

    std::size_t LoopBegin;
    std::size_t LoopEnd;
    std::vector<int8_t> Data;
  };

  inline Ornament MakeOrnament(const PDTOrnament& orn, const PDTOrnamentLoop& loop)
  {
    return Ornament(orn, loop);
  }

  inline std::size_t GetPageOrder(std::size_t page)
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

  void Describing(ModulePlayer::Info& info);

  typedef Log::WarningsCollector::AutoPrefixParam<std::size_t> IndexPrefix;
  typedef Log::WarningsCollector::AutoPrefixParam2<std::size_t, std::size_t> DoublePrefix;

  struct VoidType {};

  class PDTPlayer : public Tracking::TrackPlayer<CHANNELS_COUNT, VoidType, Ornament>
  {
    typedef Tracking::TrackPlayer<CHANNELS_COUNT, VoidType, Ornament> Parent;

    struct OrnamentState
    {
      OrnamentState() : Object(), Position()
      {
      }
      const Ornament* Object;
      std::size_t Position;

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
    };

    static Parent::Pattern ParsePattern(const PDTPattern& src, Log::WarningsCollector& warner)
    {
      Parent::Pattern result;
      result.reserve(PATTERN_SIZE);
      bool end(false);
      for (std::size_t lineNum = 0; lineNum != PATTERN_SIZE && !end; ++lineNum)
      {
        result.push_back(Parent::Line());
        Parent::Line& dstLine(result.back());
        for (std::size_t chanNum = 0; chanNum != CHANNELS_COUNT && !end; ++chanNum)
        {
          DoublePrefix pfx(warner, TEXT_LINE_CHANNEL_WARN_PREFIX, lineNum, chanNum);
          Parent::Line::Chan& dstChan(dstLine.Channels[chanNum]);
          const PDTNote& note(src.Notes[lineNum][chanNum]);
          if (note.Note != NOTE_EMPTY)
          {
            dstChan.Enabled = true;
            dstChan.Note = note.Note - 1;
            dstChan.SampleNum = note.Sample;
          }

          switch (note.Command)
          {
          case CMD_SPEED:
            warner.Assert(!dstLine.Tempo, TEXT_WARNING_DUPLICATE_TEMPO);
            dstLine.Tempo = note.Parameter;
            break;
          case CMD_SPECIAL:
            {
              switch (note.Parameter)
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
                dstChan.OrnamentNum = note.Parameter;
                break;
              }
            }
            break;
          default:
            warner.Warning("Unsupported command");
          }
        }
      }
      warner.Assert(result.size() <= PATTERN_SIZE, TEXT_WARNING_TOO_LONG);
      return result;
    }

  public:
    PDTPlayer(const String& filename, const FastDump& data)
      : Parent(filename), Chip(DAC::CreateChip(CHANNELS_COUNT, SAMPLES_COUNT, BASE_FREQ))
      //, TableFreq(), FreqTable()
    {
      //assume data is correct
      const PDTHeader* const header(safe_ptr_cast<const PDTHeader*>(&data[0]));
      Information.Statistic.Tempo = header->Tempo;
      Information.Statistic.Position = header->Lenght + 1;
      Information.Loop = header->Loop;

      Log::WarningsCollector warner;
      //fill order
      Data.Positions.resize(header->Lenght);
      std::copy(header->Positions, header->Positions + header->Lenght, Data.Positions.begin());
      //fill patterns
      Information.Statistic.Pattern = 1 + *std::max_element(Data.Positions.begin(), Data.Positions.end());
      Data.Patterns.reserve(Information.Statistic.Pattern);
      for (const PDTPattern* pat = header->Patterns; pat != ArrayEnd(header->Patterns); ++pat)
      {
        IndexPrefix pfx(warner, TEXT_PATTERN_WARN_PREFIX, pat - header->Patterns);
        Data.Patterns.push_back(ParsePattern(*pat, warner));
      }
      //fill samples
      const uint8_t* samplesData(safe_ptr_cast<const uint8_t*>(header) + sizeof(*header));
      for (const PDTSample* sample = header->Samples; sample != ArrayEnd(header->Samples); ++sample)
      {
        if (sample->Page < PAGES_COUNT && fromLE(sample->Start) >= 0xc000)
        {
          const uint8_t* sampleData(samplesData + PAGE_SIZE * GetPageOrder(sample->Page) + 
            (fromLE(sample->Start) - 0xc000));
          std::size_t size(fromLE(sample->Size));
          if (size)
          {
	    while (size && !sampleData[size])
	    {
	      --size;
	    }
            Chip->SetSample(sample - header->Samples, Dump(sampleData, sampleData + size + 1), sample->Loop);
          }
        }
      }
      Information.Statistic.Pattern = Data.Patterns.size();
      Information.Statistic.Channels = CHANNELS_COUNT;

      //fill ornaments
      Data.Ornaments.reserve(ORNAMENTS_COUNT + 1);
      Data.Ornaments.push_back(Ornament());
      std::transform(header->Ornaments, ArrayEnd(header->Ornaments), header->OrnLoops, 
        std::back_inserter(Data.Ornaments), MakeOrnament);

      const String& warnings(warner.GetWarnings());
      if (!warnings.empty())
      {
        Information.Properties.insert(StringMap::value_type(Module::ATTR_WARNINGS, warnings));
      }

      FillProperties(TEXT_PDT_EDITOR, String(), String(header->Title, ArrayEnd(header->Title)),
        &data[0], MODULE_SIZE);

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

      for (std::size_t chan = 0; chan != CHANNELS_COUNT; ++chan)
      {
        DAC::DataChunk::ChannelData dst;
        dst.Channel = chan;
        OrnamentState& ornament(Ornaments[chan]);
        const signed prevOffset(ornament.GetOffset());
        ornament.Update();
        if (0 == CurrentState.Position.Frame)//begin note
        {
          const Line& line(Data.Patterns[CurrentState.Position.Pattern][CurrentState.Position.Note]);
          const Line::Chan& src(line.Channels[chan]);

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
          data.Channels.push_back(dst);
        }
      }

      Chip->RenderData(params, data, receiver);
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
    boost::array<OrnamentState, CHANNELS_COUNT> Ornaments;
  };
  //////////////////////////////////////////////////////////////////////////
  void Describing(ModulePlayer::Info& info)
  {
    info.Capabilities = CAP_DEV_SOUNDRIVE | CAP_CONV_RAW;
    info.Properties.clear();
    info.Properties.insert(StringMap::value_type(ATTR_DESCRIPTION, TEXT_PDT_INFO));
    info.Properties.insert(StringMap::value_type(ATTR_VERSION, TEXT_PDT_VERSION));
  }

  inline bool CheckOrnament(const PDTOrnament& orn)
  {
    return ArrayEnd(orn) != std::find_if(orn, ArrayEnd(orn), std::bind2nd(std::modulus<uint8_t>(), 2));
  }

  inline bool CheckSample(const PDTSample& samp)
  {
    return fromLE(samp.Size) > 1 &&
      (fromLE(samp.Start) < 0xc000 || fromLE(samp.Start) + fromLE(samp.Size) > 0x10000 ||
      !(samp.Page == 1 || samp.Page == 3 || samp.Page == 4 || samp.Page == 6 || samp.Page == 7));
  }

  bool Checking(const String& /*filename*/, const IO::DataContainer& data, uint32_t /*capFilter*/)
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
    if (ArrayEnd(header->Ornaments) != std::find_if(header->Ornaments, ArrayEnd(header->Ornaments), CheckOrnament) ||
        ArrayEnd(header->Samples) != std::find_if(header->Samples, ArrayEnd(header->Samples), CheckSample)
      )
    {
      return false;
    }
    if (ArrayEnd(header->LastDatas) != std::find_if(header->LastDatas, ArrayEnd(header->LastDatas),
      boost::bind(&fromLE<uint16_t>, _1) < 0xc000))
    {
      return false;
    }
    //TODO
    return true;
  }

  ModulePlayer::Ptr Creating(const String& filename, const IO::DataContainer& data, uint32_t /*capFilter*/)
  {
    assert(Checking(filename, data, 0) || !"Attempt to create pdt player on invalid data");
    return ModulePlayer::Ptr(new PDTPlayer(filename, FastDump(data)));
  }

  PluginAutoRegistrator registrator(Checking, Creating, Describing);
}
