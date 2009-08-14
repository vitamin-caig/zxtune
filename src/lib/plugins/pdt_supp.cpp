#include "plugin_enumerator.h"
#include "plugin_enumerator.h"
#include "tracking_supp.h"
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
  //all samples has base freq at 8kHz (C-1)
  //const std::size_t BASE_FREQ = 8448;
  const std::size_t BASE_FREQ = 4000;
  const std::size_t NOTES = 63;

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

  struct Sample
  {
    explicit Sample(std::size_t loop = 0) : Loop(loop), Data()
    {
    }

    operator bool () const
    {
      return !Data.empty();
    }

    std::size_t Loop;
    std::vector<uint8_t> Data;
    Sound::Analyze::LevelType Gain;
  };

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

  inline Sound::Sample scale(uint8_t inSample)
  {
    return inSample << (8 * (sizeof(Sound::Sample) - sizeof(inSample)) - 1);
  }

  inline uint32_t GainAdder(uint32_t sum, uint8_t sample)
  {
    return sum + abs(int(sample) - 128);
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

  class PDTPlayer : public Tracking::TrackPlayer<4, Sample, Ornament>
  {
    typedef Tracking::TrackPlayer<CHANNELS_COUNT, Sample, Ornament> Parent;

    struct ChannelState
    {
      ChannelState() : Enabled(false), Note(), SampleNum(), PosInSample(), OrnamentNum(), PosInOrnament()
      {
      }
      bool Enabled;
      std::size_t Note;
      std::size_t SampleNum;
      //fixed-point
      std::size_t PosInSample;
      std::size_t OrnamentNum;
      std::size_t PosInOrnament;
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
      : Parent(filename), TableFreq(), FreqTable()
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
      Data.Samples.reserve(ArraySize(header->Samples));
      const uint8_t* samplesData(safe_ptr_cast<const uint8_t*>(header) + sizeof(*header));
      for (const PDTSample* sample = header->Samples; sample != ArrayEnd(header->Samples); ++sample)
      {
        if (sample->Page < PAGES_COUNT && fromLE(sample->Start) >= 0xc000)
        {
          const uint8_t* sampleData(samplesData + PAGE_SIZE * GetPageOrder(sample->Page) + (fromLE(sample->Start) - 0xc000));
          Data.Samples.push_back(Sample(fromLE(sample->Loop)));
          const std::size_t size(fromLE(sample->Size));
          if (size)
          {
            Sample& result(Data.Samples.back());
            result.Data.assign(sampleData, sampleData + size);
            result.Gain = Sound::Analyze::LevelType(std::accumulate(sampleData, sampleData + size, uint32_t(0),
              GainAdder) / size);
          }
        }
        else
        {
          Data.Samples.push_back(Sample());
          Data.Samples.back().Data.resize(1, 128);
        }
      }
      Information.Statistic.Pattern = Data.Patterns.size();
      Information.Statistic.Channels = CHANNELS_COUNT;

      //fill ornaments
      Data.Ornaments.reserve(ORNAMENTS_COUNT + 1);
      Data.Ornaments.push_back(Ornament());
      std::transform(header->Ornaments, ArrayEnd(header->Ornaments), header->OrnLoops, std::back_inserter(Data.Ornaments),
        MakeOrnament);

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
      state.resize(4);
      for (std::size_t chan = 0; chan != 4; ++chan)
      {
        Sound::Analyze::Channel& channel(state[chan]);
        if ((channel.Enabled = Channels[chan].Enabled))
        {
          channel.Level = Data.Samples[Channels[chan].SampleNum].Gain *
            std::numeric_limits<Sound::Analyze::LevelType>::max() / 128;
          channel.Band = Channels[chan].Note;
        }
      }
      return PlaybackState;
    }

    /// Rendering frame
    virtual State RenderFrame(const Sound::Parameters& params, Sound::Receiver& receiver)
    {
      const Line& line(Data.Patterns[CurrentState.Position.Pattern][CurrentState.Position.Note]);
      if (0 == CurrentState.Position.Frame)//begin note
      {
        for (std::size_t chan = 0; chan != line.Channels.size(); ++chan)
        {
          const Line::Chan& src(line.Channels[chan]);
          ChannelState& dst(Channels[chan]);
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
          if (src.SampleNum && Data.Samples[*src.SampleNum])
          {
            dst.SampleNum = *src.SampleNum;
            dst.PosInSample = 0;
          }
          if (src.OrnamentNum)
          {
            dst.OrnamentNum = *src.OrnamentNum;
            dst.PosInOrnament = 0;
          }
          }
        }
      }

      //render frame
      std::size_t steps[CHANNELS_COUNT] = {
        GetStep(Channels[0], params.SoundFreq),
        GetStep(Channels[1], params.SoundFreq),
        GetStep(Channels[2], params.SoundFreq),
        GetStep(Channels[3], params.SoundFreq)
      };
      Sound::Sample result[4];
      const uint64_t nextTick(CurrentState.Tick + params.ClocksPerFrame());
      const uint64_t ticksPerSample(params.ClockFreq / params.SoundFreq);
      while (CurrentState.Tick < nextTick)
      {
        for (std::size_t chan = 0; chan != CHANNELS_COUNT; ++chan)
        {
          if (Channels[chan].Enabled)
          {
            const Sample& sampl(Data.Samples[Channels[chan].SampleNum]);
            std::size_t pos(Channels[chan].PosInSample / Sound::FIXED_POINT_PRECISION);
            if (pos >= sampl.Data.size() || !sampl.Data[pos])//end
            {
              if (sampl.Loop >= sampl.Data.size())//not looped
              {
                Channels[chan].Enabled = false;
              }
              else
              {
                pos = sampl.Loop;
                Channels[chan].PosInSample = pos * Sound::FIXED_POINT_PRECISION;
              }
            }
            result[chan] = scale(sampl.Data[pos]);
            //recalc using coeff
            Channels[chan].PosInSample += steps[chan];
          }
          else
          {
            result[chan] = scale(128);
          }
        }
        receiver.ApplySample(result, ArraySize(result));
        CurrentState.Tick += ticksPerSample;
      }
      CurrentState.Position.Channels = std::count_if(Channels, ArrayEnd(Channels),
        boost::mem_fn(&ChannelState::Enabled));

      return Parent::RenderFrame(params, receiver);
    }

    virtual State SetPosition(std::size_t /*frame*/)
    {
      //TODO
      return PlaybackState;
    }

  private:
    std::size_t GetStep(ChannelState& state, std::size_t freq)
    {
      //table in Hz
      static const double FREQ_TABLE[NOTES] = {
        //octave1
        32.70,  34.65,  36.71,  38.89,  41.20,  43.65,  46.25,  49.00,  51.91,  55.00,  58.27,  61.73,
        //octave2
        65.41,  69.29,  73.42,  77.78,  82.41,  87.30,  92.50,  98.00, 103.82, 110.00, 116.54, 123.46,
        //octave3
        130.82, 138.58, 146.84, 155.56, 164.82, 174.60, 185.00, 196.00, 207.64, 220.00, 233.08, 246.92,
        //octave4
        261.64, 277.16, 293.68, 311.12, 329.64, 349.20, 370.00, 392.00, 415.28, 440.00, 466.16, 493.84,
        //octave5
        523.28, 554.32, 587.36, 622.24, 659.28, 698.40, 740.00, 784.00, 830.56, 880.00, 932.32, 987.68,
        //octave6
        1046.5, 1108.6, 1174.7
      };
      if (TableFreq != freq)
      {
        TableFreq = freq;
        for (std::size_t note = 0; note != NOTES; ++note)
        {
          FreqTable[note] = static_cast<std::size_t>(FREQ_TABLE[note] * Sound::FIXED_POINT_PRECISION *
            BASE_FREQ / (FREQ_TABLE[0] * freq * 2));
        }
      }
      const Ornament& orn(Data.Ornaments[state.OrnamentNum]);
      const int toneStep = static_cast<int>(FreqTable[clamp<int>(state.Note + orn.Data[state.PosInOrnament] / 2, 0, NOTES - 1)]);
      if (state.PosInOrnament++ == orn.LoopEnd)
      {
        state.PosInOrnament = orn.LoopBegin;
      }
      return toneStep;
    }
  private:
    ChannelState Channels[CHANNELS_COUNT];
    std::size_t TableFreq;
    boost::array<std::size_t, NOTES> FreqTable;
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
