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

#define FILE_TAG AB8BEC8B

namespace
{
  using namespace ZXTune;

  const String TEXT_CHI_INFO("ChipTracker modules support");
  const String TEXT_CHI_VERSION("0.1");
  const String TEXT_CHI_EDITOR("ChipTracker");

  //TODO
  const String::value_type TEXT_EMPTY[] = {0};
  //////////////////////////////////////////////////////////////////////////

  const uint8_t CHI_SIGNATURE[] = {'C', 'H', 'I', 'P', 'v', '1', '.', '0'};

  const std::size_t MAX_MODULE_SIZE = 65536;
  const std::size_t MAX_PATTERN_SIZE = 64;

  typedef IO::FastDump<uint8_t> FastDump;
  //all samples has base freq at 8kHz (C-1)
  const std::size_t BASE_FREQ = 8448;
  const std::size_t NOTES = 60;

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
    SampleDescr Samples[16];
    uint8_t Reserved[21];
    uint8_t SampleNames[16][8];
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
    CHINote Notes[64][4];
    CHINoteParam Params[64][4];
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

  inline Sound::Sample scale(uint8_t inSample)
  {
    return inSample << (8 * (sizeof(Sound::Sample) - sizeof(inSample)) - 1);
  }

  inline uint32_t GainAdder(uint32_t sum, uint8_t sample)
  {
    return sum + abs(int(sample) - 128);
  }

  void Describing(ModulePlayer::Info& info);

  typedef Log::WarningsCollector::AutoPrefixParam<std::size_t> IndexPrefix;
  typedef Log::WarningsCollector::AutoPrefixParam2<std::size_t, std::size_t> DoublePrefix;

  class PlayerImpl : public Tracking::TrackPlayer<4, Sample>
  {
    typedef Tracking::TrackPlayer<4, Sample> Parent;

    struct ChannelState
    {
      ChannelState() : Enabled(false), Note(), SampleNum(), PosInSample(), Sliding(), Glissade()
      {
      }
      bool Enabled;
      std::size_t Note;
      std::size_t SampleNum;
      //fixed-point
      std::size_t PosInSample;
      signed Sliding;
      signed Glissade;
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
        for (std::size_t chanNum = 0; chanNum != 4; ++chanNum)
        {
          DoublePrefix pfx(warner, "Line %1%: Channel %2%: ", lineNum, chanNum);
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
              warner.Assert(!dstLine.Tempo, "duplicated tempo");
              dstLine.Tempo = param.Parameter;
            }
            else if (3 == chanNum)
            {
              end = true;
            }
            else
            {
              warner.Warning("special command in invalid channel");
            }
          }
        }
      }
      warner.Assert(result.size() <= MAX_PATTERN_SIZE, "too long");
      return result;
    }

  public:
    PlayerImpl(const String& filename, const FastDump& data)
      : Parent(filename), TableFreq(), FreqTable()
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
        IndexPrefix pfx(warner, "Pattern %1%: ", pat - patBegin);
        Data.Patterns.push_back(ParsePattern(*pat, warner));
      }
      //fill samples
      Data.Samples.reserve(ArraySize(header->Samples));
      const uint8_t* sampleData(safe_ptr_cast<const uint8_t*>(patBegin + Information.Statistic.Pattern));
      std::size_t memLeft(data.Size() - (sampleData - &data[0]));
      for (const CHIHeader::SampleDescr* sample = header->Samples; sample != ArrayEnd(header->Samples); ++sample)
      {
        Data.Samples.push_back(Sample(fromLE(sample->Loop)));
        const std::size_t size(std::min<std::size_t>(memLeft, fromLE(sample->Length)));
        if (size)
        {
          Sample& result(Data.Samples.back());
          result.Data.assign(sampleData, sampleData + size);
          result.Gain = Sound::Analyze::LevelType(std::accumulate(sampleData, sampleData + size, uint32_t(0),
            GainAdder) / size);
          sampleData += align(size, 256);
          if (size != fromLE(sample->Length))
          {
            warner.Warning("unexpected end of file");
            break;
          }
          memLeft -= align(size, 256);
        }
      }
      Information.Statistic.Pattern = Data.Patterns.size();
      Information.Statistic.Channels = 4;
      const String& warnings(warner.GetWarnings());
      if (!warnings.empty())
      {
        Information.Properties.insert(StringMap::value_type(Module::ATTR_WARNINGS, warnings));
      }

      FillProperties(TEXT_CHI_EDITOR, TEXT_EMPTY, String(header->Name, ArrayEnd(header->Name)), 
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
          dst.Sliding = dst.Glissade = 0;
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
          if (src.SampleNum && Data.Samples[*src.SampleNum])
          {
            dst.SampleNum = *src.SampleNum;
            dst.PosInSample = 0;
          }
          for (CommandsArray::const_iterator it = src.Commands.begin(), lim = src.Commands.end(); it != lim; ++it)
          {
            switch (it->Type)
            {
            case SAMPLE_OFFSET:
              dst.PosInSample = it->Param1 * Sound::FIXED_POINT_PRECISION;
              break;
            case SLIDE:
              dst.Glissade = it->Param1;
              break;
            default:
              assert(!"Invalid command");
            }
          }
        }
      }

      //render frame
      std::size_t steps[4] = {
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
        for (std::size_t chan = 0; chan != 4; ++chan)
        {
          if (Channels[chan].Enabled)
          {
            const Sample& sampl(Data.Samples[Channels[chan].SampleNum]);
            std::size_t pos(Channels[chan].PosInSample / Sound::FIXED_POINT_PRECISION);
            if (pos >= sampl.Data.size())//end
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
      for (std::size_t chan = 0; chan != 4; ++chan)
      {
        Channels[chan].Sliding += Channels[chan].Glissade;
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
    std::size_t GetStep(const ChannelState& state, std::size_t freq)
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
      //TODO: proper sliding
      const int toneStep = static_cast<int>(FreqTable[state.Note]);
      const int toneSlide(int64_t(state.Sliding) * Sound::FIXED_POINT_PRECISION * BASE_FREQ /
        int64_t(FREQ_TABLE[0] * freq * 2));
      return clamp<int>(toneStep + toneSlide, int(FreqTable.front()), int(FreqTable.back()));
    }
  private:
    ChannelState Channels[4];
    std::size_t TableFreq;
    boost::array<std::size_t, NOTES> FreqTable;
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
    return ModulePlayer::Ptr(new PlayerImpl(filename, FastDump(data)));
  }

  PluginAutoRegistrator registrator(Checking, Creating, Describing);
}
