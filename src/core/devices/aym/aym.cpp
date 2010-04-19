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

  class ChipImpl : public Chip
  {
  public:
    ChipImpl()
      : Result(CHANNELS)
      , AccumulatedSamples()
      , State()
      , BitA(), BitB(), BitC(), BitN(), BitE()
      , TimerA(), TimerB(), TimerC(), TimerN(), TimerE()
      , Envelope(), Decay(), Noise()
      , LastData(), LastTicksPerSec(), LastStartTicks(), LastSamplesDone()
    {
      Reset();
    }

    virtual void RenderData(const Sound::RenderParameters& params,
                            const DataChunk& src,
                            Sound::MultichannelReceiver& dst);

    virtual void GetState(Module::Analyze::ChannelsState& state) const;

    virtual void Reset()
    {
      std::fill(Accumulators.begin(), Accumulators.end(), 0);
      AccumulatedSamples = 0;

      State.Tick = 0;
      State.Mask = DataChunk::MASK_ALL_REGISTERS;
      std::fill(State.Data.begin(), State.Data.end(), 0);
      State.Data[DataChunk::REG_MIXER] = 0xff;
      BitA = BitB = BitC = BitN = 0;
      TimerA = TimerB = TimerC = TimerN = TimerE = 0;
      Envelope = Decay = 0;
      Noise = 0;

      LastData.Tick = 0;
      LastTicksPerSec = 0;
    }

  private:
    void ApplyRegistersData();
    void DoRender(const Sound::RenderParameters& params, Sound::MultichannelReceiver& dst);

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
      if (++timer >= (duted ? GetDutedTone(LastData.Data[DataChunk::PARAM_DUTY_CYCLE], bit, tone) : tone))
      {
        timer = 0;
        bit = ~bit;
        return true;
      }
      return false;
    }

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

    uint_t GetVolA() const
    {
      return State.Data[DataChunk::REG_VOLA];
    }

    uint_t GetVolB() const
    {
      return State.Data[DataChunk::REG_VOLB];
    }

    uint_t GetVolC() const
    {
      return State.Data[DataChunk::REG_VOLC];
    }

    uint_t GetMixer() const
    {
      return State.Data[DataChunk::REG_MIXER];
    }

    uint_t GetEnv() const
    {
      return State.Data[DataChunk::REG_ENV];
    }

    Sound::Sample GetVolume(uint_t regVol) const
    {
      assert(regVol < 32);
      return ((LastData.Mask & DataChunk::YM_CHIP) ? YMVolumeTab : AYVolumeTab)[regVol] / 2;
    }

    uint_t GetBandByPeriod(uint_t regVal) const;
  protected:
    //result buffer
    std::vector<Sound::Sample> Result;
    //interpolation-related
    uint_t AccumulatedSamples;
    boost::array<uint_t, CHANNELS> Accumulators;
    //state
    DataChunk State; //time and registers
    uint_t BitA, BitB, BitC, BitN, BitE/*fake*/;
    uint_t TimerA, TimerB, TimerC, TimerN, TimerE;
    uint_t Envelope;
    int_t Decay;
    uint32_t Noise;
    //extrapolation-related
    DataChunk LastData;
    uint64_t LastTicksPerSec;
    uint64_t LastStartTicks;
    uint_t LastSoundFreq;
    uint_t LastSamplesDone;
  };

  void ChipImpl::RenderData(const Sound::RenderParameters& params, const DataChunk& src,
    Sound::MultichannelReceiver& dst)
  {
    if (State.Tick >= LastData.Tick) //need to get data
    {
      LastData = src;
      //output dump
      if (LastData.Mask & DataChunk::MASK_ALL_REGISTERS)
      {
        ApplyRegistersData();
      }
    }
    DoRender(params, dst);
  }

  void ChipImpl::ApplyRegistersData()
  {
    for (uint_t idx = 0, mask = 1; idx != LastData.Data.size(); ++idx, mask <<= 1)
    {
      if (LastData.Mask & mask) //register is in dump
      {
        uint8_t reg = LastData.Data[idx];
        if (mask & REGS_4BIT_SET)
        {
          reg &= 0x0f;
        }
        else if (mask & REGS_5BIT_SET)
        {
          reg &= 0x1f;
        }
        if (DataChunk::REG_ENV == idx)
        {//update r13
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
      } //update reg
    }
  }

  void ChipImpl::DoRender(const Sound::RenderParameters& params, Sound::MultichannelReceiver& dst)
  {
    //for more precise rendering
    if (LastTicksPerSec != params.ClockFreq ||
        LastSoundFreq != params.SoundFreq)
    {
      LastStartTicks = State.Tick;
      LastSoundFreq = params.SoundFreq;
      LastTicksPerSec = params.ClockFreq;
      LastSamplesDone = 1;
    }

    uint64_t& curTick = State.Tick;
    uint64_t nextSampleTick = LastStartTicks + LastTicksPerSec * LastSamplesDone / LastSoundFreq;

    uint_t HighLevel = ~0u;
    //references to mixered bits. updated automatically
    uint_t volA = (((GetVolA() & DataChunk::REG_MASK_VOL) << 1) + 1);
    uint_t volB = (((GetVolB() & DataChunk::REG_MASK_VOL) << 1) + 1);
    uint_t volC = (((GetVolC() & DataChunk::REG_MASK_VOL) << 1) + 1);
    uint_t& ToneBitA = (GetMixer() & DataChunk::REG_MASK_TONEA) ? HighLevel : BitA;
    uint_t& NoiseBitA = (GetMixer() & DataChunk::REG_MASK_NOISEA) ? HighLevel : BitN;
    uint_t& VolumeA = (GetVolA() & DataChunk::REG_MASK_ENV) ? Envelope : volA;
    uint_t& ToneBitB = (GetMixer() & DataChunk::REG_MASK_TONEB) ? HighLevel : BitB;
    uint_t& NoiseBitB = (GetMixer() & DataChunk::REG_MASK_NOISEB) ? HighLevel : BitN;
    uint_t& VolumeB = (GetVolB() & DataChunk::REG_MASK_ENV) ? Envelope : volB;
    uint_t& ToneBitC = (GetMixer() & DataChunk::REG_MASK_TONEC) ? HighLevel : BitC;
    uint_t& NoiseBitC = (GetMixer() & DataChunk::REG_MASK_NOISEC) ? HighLevel : BitN;
    uint_t& VolumeC = (GetVolC() & DataChunk::REG_MASK_ENV) ? Envelope : volC;

    const uint_t duty(LastData.Data[DataChunk::PARAM_DUTY_CYCLE]);
    const uint_t dutyMask(duty > 0 && duty < 100 && duty != 50 ?
      LastData.Data[DataChunk::PARAM_DUTY_CYCLE_MASK] : 0);
    const bool interpolate(0 != (LastData.Mask & DataChunk::INTERPOLATE));

    //render cycle
    while (curTick < LastData.Tick)
    {
      //accumulate samples in interpolation mode
      if (interpolate)
      {
        Accumulators[0] += GetVolume(ToneBitA & NoiseBitA & VolumeA);
        Accumulators[1] += GetVolume(ToneBitB & NoiseBitB & VolumeB);
        Accumulators[2] += GetVolume(ToneBitC & NoiseBitC & VolumeC);
        ++AccumulatedSamples;
      }
      //need to store sample
      if (curTick >= nextSampleTick)
      {
        //calculate average in interpolation mode
        if (interpolate)
        {
          std::transform(Accumulators.begin(), Accumulators.end(), Result.begin(), std::bind2nd(std::divides<uint_t>(), AccumulatedSamples));
          std::fill(Accumulators.begin(), Accumulators.end(), 0);
          AccumulatedSamples = 0;
        }
        else
        {
          Result[0] = GetVolume(ToneBitA & NoiseBitA & VolumeA);
          Result[1] = GetVolume(ToneBitB & NoiseBitB & VolumeB);
          Result[2] = GetVolume(ToneBitC & NoiseBitC & VolumeC);
        }

        dst.ApplySample(Result);
        nextSampleTick = LastStartTicks + LastTicksPerSec * ++LastSamplesDone / LastSoundFreq;
      }
      curTick += 8;//base freq divisor

      DoCycle(0 != (dutyMask & DataChunk::DUTY_CYCLE_MASK_A), GetToneA(), TimerA, BitA);
      DoCycle(0 != (dutyMask & DataChunk::DUTY_CYCLE_MASK_B), GetToneB(), TimerB, BitB);
      DoCycle(0 != (dutyMask & DataChunk::DUTY_CYCLE_MASK_C), GetToneC(), TimerC, BitC);

      if (DoCycle(0 != (dutyMask & DataChunk::DUTY_CYCLE_MASK_N), GetToneN(), TimerN, BitN))
      {
        TimerN = 0;
        Noise = (Noise * 2 + 1) ^ (((Noise >> 16) ^ (Noise >> 13)) & 1);
        BitN = (Noise & 0x10000) ? ~0 : 0;
      }
      if (DoCycle(0 != (dutyMask & DataChunk::DUTY_CYCLE_MASK_E), GetToneE(), TimerE, BitE))
      {
        Envelope += Decay;
        if (Envelope & ~31u)
        {
          const uint_t envTypeMask = 1 << GetEnv();
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
  }

  void ChipImpl::GetState(Module::Analyze::ChannelsState& state) const
  {
    //one channel is envelope
    state.resize(CHANNELS + 1);
    Module::Analyze::Channel& envChan = state[CHANNELS];
    envChan = Module::Analyze::Channel();
    envChan.Band = GetBandByPeriod(256 * State.Data[DataChunk::REG_TONEE_H] | State.Data[DataChunk::REG_TONEE_L]);

    const uint_t noiseBand = GetBandByPeriod(2 * State.Data[DataChunk::REG_TONEN]);
    const uint_t mixer = ~GetMixer();
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
          ? GetBandByPeriod(256 * State.Data[DataChunk::REG_TONEA_H + chan * 2] |
            State.Data[DataChunk::REG_TONEA_L + chan * 2])
          : noiseBand;
      }
    }
  }

  uint_t ChipImpl::GetBandByPeriod(uint_t period) const
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
    const uint_t freq = static_cast<uint_t>(uint64_t(LastTicksPerSec) * FREQ_MULTIPLIER / (16 * (period ? period : 1)));
    const uint_t maxBand = ArraySize(FREQ_TABLE) - 1;
    const uint_t currentBand = std::lower_bound(FREQ_TABLE, ArrayEnd(FREQ_TABLE), freq) - FREQ_TABLE;
    return std::min(currentBand, maxBand);
  }
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
