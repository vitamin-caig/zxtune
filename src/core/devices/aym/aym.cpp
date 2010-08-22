/*
Abstract:
  AY/YM chips interface implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  Based on sources of UnrealSpeccy by SMT
*/

//local includes
#include <core/devices/aym.h>
//common includes
#include <tools.h>
//library includes
#include <sound/sound_types.h>
#include <sound/render_params.h>
//std includes
#include <cassert>
#include <functional>
#include <limits>
#include <memory>

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::AYM;

  enum
  {
    // set of registers which required input data masking (4 or 5 lsb)
    REGS_4BIT_SET = (1 << DataChunk::REG_TONEA_H) | (1 << DataChunk::REG_TONEB_H) |
                    (1 << DataChunk::REG_TONEC_H) | (1 << DataChunk::REG_ENV),
    REGS_5BIT_SET = (1 << DataChunk::REG_TONEN) | (1 << DataChunk::REG_VOLA) |
                    (1 << DataChunk::REG_VOLB) | (1 << DataChunk::REG_VOLC),
  };

  BOOST_STATIC_ASSERT(DataChunk::PARAM_LAST < 8 * sizeof(uint_t));

  // chip-specific volume tables- ym supports 32 volume steps, ay - only 16
  const Sound::Sample AYVolumeTab[32] =
  {
    0x0000, 0x0000, 0x0340, 0x0340, 0x04C0, 0x04C0, 0x06F2, 0x06F2,
    0x0A44, 0x0A44, 0x0F13, 0x0F13, 0x1510, 0x1510, 0x227E, 0x227E,
    0x289F, 0x289F, 0x414E, 0x414E, 0x5B21, 0x5B21, 0x7258, 0x7258,
    0x905E, 0x905E, 0xB550, 0xB550, 0xD7A0, 0xD7A0, 0xFFFF, 0xFFFF
  };
  const Sound::Sample YMVolumeTab[32] =
  {
    0x0000, 0x0000, 0x00EF, 0x01D0, 0x0290, 0x032A, 0x03EE, 0x04D2,
    0x0611, 0x0782, 0x0912, 0x0A36, 0x0C31, 0x0EB6, 0x1130, 0x13A0,
    0x1751, 0x1BF5, 0x20E2, 0x2594, 0x2CA1, 0x357F, 0x3E45, 0x475E,
    0x5502, 0x6620, 0x7730, 0x8844, 0xA1D2, 0xC102, 0xE0A2, 0xFFFF
  };

  typedef boost::array<uint_t, CHANNELS> LayoutData;
  
  const LayoutData LAYOUTS[] =
  {
    { {0, 1, 2} }, //ABC
    { {0, 2, 1} }, //ACB
    { {1, 0, 2} }, //BAC
    { {2, 0, 1} }, //BCA
    { {2, 1, 0} }, //CBA
    { {1, 2, 0} }, //CAB
  };


  const uint_t AYM_CLOCK_DIVISOR = 8;

  typedef boost::array<Sound::Sample, CHANNELS> ChannelsData;

  //PSG-related functionality
  class PSG
  {
  public:
    PSG()
      : VolA(State.Data[DataChunk::REG_VOLA]), VolB(State.Data[DataChunk::REG_VOLB]), VolC(State.Data[DataChunk::REG_VOLC])
      , Mixer(State.Data[DataChunk::REG_MIXER])
      , EnvType(State.Data[DataChunk::REG_ENV])
      , BitA(), BitB(), BitC(), BitN(), BitE()
      , TimerA(), TimerB(), TimerC(), TimerN(), TimerE()
      , Envelope(), Decay(), Noise()
      , DutyCycle(State.Data[DataChunk::PARAM_DUTY_CYCLE])
      , DutyCycleMask(State.Data[DataChunk::PARAM_DUTY_CYCLE_MASK])
      , Layout(State.Data[DataChunk::PARAM_LAYOUT])
    {
      State.Data[DataChunk::REG_MIXER] = 0xff;
    }

    void Reset()
    {
      State = DataChunk();
      State.Data[DataChunk::REG_MIXER] = 0xff;
      BitA = BitB = BitC = BitN = 0;
      TimerA = TimerB = TimerC = TimerN = TimerE = 0;
      Envelope = Decay = 0;
      Noise = 0;
    }

    void ApplyData(const DataChunk& data)
    {
      assert(data.Tick > State.Tick);
      for (uint_t idx = 0, mask = 1; idx != data.Data.size(); ++idx, mask <<= 1)
      {
        if (idx > DataChunk::REG_ENV)
        {
          //copy parameters
          State.Data[idx] = data.Data[idx];
        }
        else if (0 != (data.Mask & mask))
        {
          //copy registers
          uint8_t reg = data.Data[idx];
          //limit values
          if (mask & REGS_4BIT_SET)
          {
            reg &= 0x0f;
          }
          else if (mask & REGS_5BIT_SET)
          {
            reg &= 0x1f;
          }
          //update r13 
          if (DataChunk::REG_ENV == idx)
          {
            TimerE = 0;
            if (reg & 4) //down-up envelopes
            {
              Envelope = 0;
              Decay = 1;
            }
            else //up-down envelopes
            {
              Envelope = 31;
              Decay = -1;
            }
          }
          State.Data[idx] = reg;
        }
      }
      State.Mask = data.Mask;
    }

    void Tick()
    {
      DoCycle(0 != (DutyCycleMask & DataChunk::DUTY_CYCLE_MASK_A), GetToneA(), TimerA, BitA);
      DoCycle(0 != (DutyCycleMask & DataChunk::DUTY_CYCLE_MASK_B), GetToneB(), TimerB, BitB);
      DoCycle(0 != (DutyCycleMask & DataChunk::DUTY_CYCLE_MASK_C), GetToneC(), TimerC, BitC);

      if (DoCycle(0 != (DutyCycleMask & DataChunk::DUTY_CYCLE_MASK_N), GetToneN(), TimerN, BitN))
      {
        TimerN = 0;
        Noise = (Noise * 2 + 1) ^ (((Noise >> 16) ^ (Noise >> 13)) & 1);
        BitN = (Noise & 0x10000) ? ~0 : 0;
      }
      if (DoCycle(0 != (DutyCycleMask & DataChunk::DUTY_CYCLE_MASK_E), GetToneE(), TimerE, BitE))
      {
        Envelope += Decay;
        if (Envelope & ~31u)
        {
          const uint_t envTypeMask = 1 << EnvType;
          if (envTypeMask & ((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) |
                             (1 << 5) | (1 << 6) | (1 << 7) | (1 << 9) | (1 << 15)))
          {
            Envelope = Decay = 0;
          }
          else
          {
            if (envTypeMask & ((1 << 8) | (1 << 12)))
            {
              Envelope &= 31;
            }
            else
            {
              if (envTypeMask & ((1 << 10) | (1 << 14)))
              {
                Decay = -Decay;
                Envelope += Decay;
              }
              else
              {
                Envelope = 31;
                Decay = 0; //11, 13
              }
            }
          }
        }
      }//envelope
    }

    void GetLevels(ChannelsData& result) const
    {
      const uint_t HighLevel = ~0u;
      //references to mixered bits. updated automatically
      const uint_t levelA = (((VolA & DataChunk::REG_MASK_VOL) << 1) + 1);
      const uint_t levelB = (((VolB & DataChunk::REG_MASK_VOL) << 1) + 1);
      const uint_t levelC = (((VolC & DataChunk::REG_MASK_VOL) << 1) + 1);
      const uint_t toneBitA = (Mixer & DataChunk::REG_MASK_TONEA) ? HighLevel : BitA;
      const uint_t noiseBitA = (Mixer & DataChunk::REG_MASK_NOISEA) ? HighLevel : BitN;
      const uint_t outA = (VolA & DataChunk::REG_MASK_ENV) ? Envelope : levelA;
      const uint_t toneBitB = (Mixer & DataChunk::REG_MASK_TONEB) ? HighLevel : BitB;
      const uint_t noiseBitB = (Mixer & DataChunk::REG_MASK_NOISEB) ? HighLevel : BitN;
      const uint_t outB = (VolB & DataChunk::REG_MASK_ENV) ? Envelope : levelB;
      const uint_t toneBitC = (Mixer & DataChunk::REG_MASK_TONEC) ? HighLevel : BitC;
      const uint_t noiseBitC = (Mixer & DataChunk::REG_MASK_NOISEC) ? HighLevel : BitN;
      const uint_t outC = (VolC & DataChunk::REG_MASK_ENV) ? Envelope : levelC;

      assert(Layout < ArraySize(LAYOUTS));
      const LayoutData& layout = LAYOUTS[Layout];
      result[layout[0]] = GetVolume(toneBitA & noiseBitA & outA);
      result[layout[1]] = GetVolume(toneBitB & noiseBitB & outB);
      result[layout[2]] = GetVolume(toneBitC & noiseBitC & outC);
    }


    virtual void GetState(uint64_t ticksPerSec, Module::Analyze::ChannelsState& state) const
    {
      //one channel is envelope    
      state.resize(CHANNELS + 1);    
      Module::Analyze::Channel& envChan = state[CHANNELS];
      envChan = Module::Analyze::Channel();
      envChan.Band = GetBandByPeriod(ticksPerSec, 16 * GetToneE());
      const uint_t noiseBand = GetBandByPeriod(ticksPerSec, GetToneN());
      const uint_t mixer = ~Mixer;
      for (uint_t chan = 0; chan != CHANNELS; ++chan) 
      {
        const uint_t volReg = State.Data[DataChunk::REG_VOLA + chan];
        //accumulate level in envelope channel      
        if (volReg & DataChunk::REG_MASK_ENV)
        {        
          envChan.Enabled = true;
          envChan.Level += std::numeric_limits<Module::Analyze::LevelType>::max() / CHANNELS;
        }
        //calculate tone channel
        Module::Analyze::Channel& channel = state[chan];
        const bool hasNoise = 0 != (mixer & (uint_t(DataChunk::REG_MASK_NOISEA) << chan));
        const bool hasTone = 0 != (mixer & (uint_t(DataChunk::REG_MASK_TONEA) << chan));
        if ( (channel.Enabled = hasNoise || hasTone) )
        {
          channel.Level = static_cast<Module::Analyze::LevelType>(
            (volReg & DataChunk::REG_MASK_VOL) * std::numeric_limits<Module::Analyze::LevelType>::max() / 15);
          channel.Band = hasTone
            ? GetBandByPeriod(ticksPerSec, 256 * State.Data[DataChunk::REG_TONEA_H + chan * 2] |
              State.Data[DataChunk::REG_TONEA_L + chan * 2])
            : noiseBand;
        }
      } 
    }

  private:
    uint_t GetBandByPeriod(uint64_t ticksPerSec, uint_t period) const
    {
      const uint_t FREQ_MULTIPLIER = 100;
      //table in Hz * FREQ_MULTIPLIER
      static const uint_t FREQ_TABLE[] =
      {
        //octave1
        3270,   3465,   3671,   3889,   4120,   4365,   4625,   4900,   5191,   5500,   5827,   6173,
        //octave2
        6541,   6929,   7342,   7778,   8241,   8730,   9250,   9800,  10382,  11000,  11654,  12346,
        //octave3
        13082,  13858,  14684,  15556,  16482,  17460,  18500,  19600,  20764,  22000,  23308,  24692,
        //octave4
        26164,  27716,  29368,  31112,  32964,  34920,  37000,  39200,  41528,  44000,  46616,  49384,
        //octave5
        52328,  55432,  58736,  62224,  65928,  69840,  74000,  78400,  83056,  88000,  93232,  98768,
        //octave6
        104650, 110860, 117470, 124450, 131860, 139680, 148000, 156800, 166110, 176000, 186460, 197540,
        //octave7
        209310, 221720, 234940, 248890, 263710, 279360, 296000, 313600, 332220, 352000, 372930, 395070,
        //octave8
        418620, 443460, 469890, 497790, 527420, 558720, 592000, 627200, 664450, 704000, 745860, 790140,
        //octave9
        837200, 886980, 939730, 995610,1054800,1117500,1184000,1254400,1329000,1408000,1491700,1580400
      };
      const uint_t freq = static_cast<uint_t>(ticksPerSec * FREQ_MULTIPLIER / (2 * AYM_CLOCK_DIVISOR * (period ? period : 1)));
      const uint_t maxBand = ArraySize(FREQ_TABLE) - 1;
      const uint_t currentBand = std::lower_bound(FREQ_TABLE, ArrayEnd(FREQ_TABLE), freq) - FREQ_TABLE;
      return std::min(currentBand, maxBand);
    }
  private:
    uint_t GetToneA() const
    {
      return 256 * State.Data[DataChunk::REG_TONEA_H] + State.Data[DataChunk::REG_TONEA_L];
    }

    uint_t GetToneB() const
    {
      return 256 * State.Data[DataChunk::REG_TONEB_H] + State.Data[DataChunk::REG_TONEB_L];
    }

    uint_t GetToneC() const
    {
      return 256 * State.Data[DataChunk::REG_TONEC_H] + State.Data[DataChunk::REG_TONEC_L];
    }

    uint_t GetToneN() const
    {
      return 2 * State.Data[DataChunk::REG_TONEN];//for optimization
    }

    uint_t GetToneE() const
    {
      return 256 * State.Data[DataChunk::REG_TONEE_H] + State.Data[DataChunk::REG_TONEE_L];
    }
    /*
    calculate square pulse width according to current state and duty parameter
    e.g. if duty == 25
      _     _
     | |   | |
    _| |___| |_

     |1| 3 |1| ...

    positive pulse width is 1/4 of period value
    */
    static inline uint_t GetDutedTone(uint_t duty, uint_t state, uint_t tone)
    {
      assert(duty > 0 && duty < 100);
      return (state ? 100 - duty : duty) * tone * 2 / 100;
    }

    //perform one cycle
    //return true if bit is updated (flipped)
    bool DoCycle(bool duted, uint_t tone, uint_t& timer, uint_t& bit) const
    {
      if (++timer >= (duted ? GetDutedTone(DutyCycle, bit, tone) : tone))
      {
        timer = 0;
        bit = ~bit;
        return true;
      }
      return false;
    }

    Sound::Sample GetVolume(uint_t regVol) const
    {
      assert(regVol < 32);
      return ((State.Mask & DataChunk::YM_CHIP) ? YMVolumeTab : AYVolumeTab)[regVol] / 2;
    }
  private:
    //registers state
    DataChunk State;
    //aliases for registers
    uint8_t& VolA;
    uint8_t& VolB;
    uint8_t& VolC;
    uint8_t& Mixer;
    uint8_t& EnvType;
    //counters state
    uint_t BitA, BitB, BitC, BitN, BitE/*fake*/;
    uint_t TimerA, TimerB, TimerC, TimerN, TimerE;
    uint_t Envelope;
    int_t Decay;
    uint32_t Noise;
    //related parameters
    uint8_t& DutyCycle;
    uint8_t& DutyCycleMask;
    uint8_t& Layout;
  };

  class Renderer
  {
  public:
    explicit Renderer(PSG& generator)
      : Generator(generator)
      , Interpolate(false)
      , Levels()
      , AccumulatedSamples()
    {
    }

    void Reset()
    {
      std::fill(Levels.begin(), Levels.end(), 0);
      AccumulatedSamples = 0;
      return Generator.Reset();
    }

    void ApplyData(const DataChunk& data)
    {
      Interpolate = 0 != (data.Mask & DataChunk::INTERPOLATE);
      return Generator.ApplyData(data);
    }

    void Tick()
    {
      if (Interpolate)
      {
        Generator.GetLevels(Levels);
        std::transform(Levels.begin(), Levels.end(), Accumulators.begin(), Accumulators.begin(), std::plus<uint_t>());
        ++AccumulatedSamples;
      }
      Generator.Tick();
    }

    void GetLevels(std::vector<Sound::Sample>& result)
    {
      assert(result.size() == CHANNELS);//for optimization
      if (Interpolate)
      {
        std::transform(Accumulators.begin(), Accumulators.end(), result.begin(), std::bind2nd(std::divides<uint_t>(), AccumulatedSamples));
        std::fill(Accumulators.begin(), Accumulators.end(), 0);
        AccumulatedSamples = 0;
      }
      else
      {
        Generator.GetLevels(Levels);
        std::copy(Levels.begin(), Levels.end(), result.begin());
      }
    }
  private:
    PSG& Generator;
    bool Interpolate;
    ChannelsData Levels;
    boost::array<uint_t, CHANNELS> Accumulators;
    uint_t AccumulatedSamples;
  };

  class ClockSource
  {
  public:
    ClockSource()
      : CurrentTick()
      , NextSoundTick()
      , LastTick()
      , TicksPerSec()
      , RenderStartTick()
      , SoundFreq()
      , SamplesDone()
    {
    }

    void Reset()
    {
      CurrentTick = 0;
      NextSoundTick = 0;
      LastTick = 0;
      TicksPerSec = 0;
      RenderStartTick = 0;
      SoundFreq = 0;
      SamplesDone = 0;
    }

    void ApplyData(const Sound::RenderParameters& params, const DataChunk& data)
    {
      //for more precise rendering
      if (TicksPerSec != params.ClockFreq ||
          SoundFreq != params.SoundFreq)
      {
        RenderStartTick = CurrentTick;
        SoundFreq = params.SoundFreq;
        TicksPerSec = params.ClockFreq;
        SamplesDone = 1;
        CalcNextSoundTick();
      }
      LastTick = data.Tick;
    }

    bool Tick(bool& outSound)
    {
      assert(CurrentTick < LastTick);
      CurrentTick += AYM_CLOCK_DIVISOR;
      if ( (outSound = CurrentTick >= NextSoundTick) )
      {
        ++SamplesDone;
        CalcNextSoundTick();
      }
      return CurrentTick < LastTick;
    }
  private:
    void CalcNextSoundTick()
    {
      NextSoundTick = RenderStartTick + TicksPerSec * SamplesDone / SoundFreq;
      assert(NextSoundTick > CurrentTick);
    }
  private:
    //context
    uint64_t CurrentTick;
    uint64_t NextSoundTick;
    uint64_t LastTick;
    //parameters
    uint64_t TicksPerSec;
    uint64_t RenderStartTick;
    uint_t SoundFreq;
    uint_t SamplesDone;
  };

  class ChipImpl : public Chip
  {
  public:
    ChipImpl()
      : Render(Generator)
      , Result(CHANNELS)
    {
      Reset();
    }

    virtual void RenderData(const Sound::RenderParameters& params,
                            const DataChunk& src,
                            Sound::MultichannelReceiver& dst)
    {
      TicksPerSecond = params.ClockFreq;
      Render.ApplyData(src);
      Clock.ApplyData(params, src);
      for (;;)
      {
        bool renderSound = false;
        const bool inFrame = Clock.Tick(renderSound);
        Render.Tick();
        if (renderSound)
        {
          Render.GetLevels(Result);
          dst.ApplyData(Result);
        }
        if (!inFrame)
        {
          break;
        }
      }
    }

    virtual void GetState(Module::Analyze::ChannelsState& state) const
    {
      return Generator.GetState(TicksPerSecond, state);
    }

    virtual void Reset()
    {
      Render.Reset();
      Clock.Reset();
    }

  protected:
    PSG Generator;
    Renderer Render;
    ClockSource Clock;
    //context
    uint64_t TicksPerSecond;
    std::vector<Sound::Sample> Result;
  };
}

namespace ZXTune
{
  namespace AYM
  {
    Chip::Ptr CreateChip()
    {
      return Chip::Ptr(new ChipImpl);
    }
  }
}
