#include "aym.h"

#include <tools.h>

#include <sound_attrs.h>

#include <limits>
#include <memory>
#include <cassert>

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::AYM;

  enum
  {
    REGS_4BIT_SET = (1 << DataChunk::REG_TONEA_H) | (1 << DataChunk::REG_TONEB_H) |
                    (1 << DataChunk::REG_TONEC_H) | (1 << DataChunk::REG_ENV),
    REGS_5BIT_SET = (1 << DataChunk::REG_TONEN) | (1 << DataChunk::REG_VOLA) |
                    (1 << DataChunk::REG_VOLB) | (1 << DataChunk::REG_VOLC),
  };

  //from US sources
  const Sound::Sample AYVolumeTab[32] = {
    0x0000, 0x0000, 0x0340, 0x0340, 0x04C0, 0x04C0, 0x06F2, 0x06F2,
    0x0A44, 0x0A44, 0x0F13, 0x0F13, 0x1510, 0x1510, 0x227E, 0x227E,
    0x289F, 0x289F, 0x414E, 0x414E, 0x5B21, 0x5B21, 0x7258, 0x7258,
    0x905E, 0x905E, 0xB550, 0xB550, 0xD7A0, 0xD7A0, 0xFFFF, 0xFFFF };
  const Sound::Sample YMVolumeTab[32] = {
    0x0000, 0x0000, 0x00EF, 0x01D0, 0x0290, 0x032A, 0x03EE, 0x04D2,
    0x0611, 0x0782, 0x0912, 0x0A36, 0x0C31, 0x0EB6, 0x1130, 0x13A0,
    0x1751, 0x1BF5, 0x20E2, 0x2594, 0x2CA1, 0x357F, 0x3E45, 0x475E,
    0x5502, 0x6620, 0x7730, 0x8844, 0xA1D2, 0xC102, 0xE0A2, 0xFFFF };

  class ChipImpl : public Chip
  {
  public:
    ChipImpl()
      : State()
      , BitA(), BitB(), BitC(), BitN()
      , TimerA(), TimerB(), TimerC(), TimerN(), TimerE()
      , Envelope(), Decay(), Noise()
      , LastData(), LastTicksPerSec()
    {
      Reset();
    }

    virtual void RenderData(const Sound::Parameters& params,
                            DataSource<DataChunk>& src,
                            Sound::Receiver& dst);
    virtual void RenderData(const Sound::Parameters& params,
                            const DataChunk& src,
                            Sound::Receiver& dst);

    virtual void GetState(Sound::Analyze::Volume& volState, Sound::Analyze::Spectrum& spectrumState) const;

    virtual void Reset()
    {
      State.Tick = 0;
      State.Mask = DataChunk::ALL_REGISTERS;
      std::memset(State.Data, 0, sizeof(State.Data));
      State.Data[DataChunk::REG_MIXER] = 0xff;
      BitA = BitB = BitC = BitN = 0;
      TimerA = TimerB = TimerC = TimerN = TimerE = 0;
      Envelope = Decay = 0;
      Noise = 0;

      LastData.Tick = 0;//~uint64_t(0);
    }

  private:
    void ApplyLastData();
    void DoRender(const Sound::Parameters& params, Sound::Receiver& dst);
  private:
    inline uint16_t GetToneA() const
    {
      return (State.Data[DataChunk::REG_TONEA_H] << 8) + State.Data[DataChunk::REG_TONEA_L];
    }
    inline uint16_t GetToneB() const
    {
      return (State.Data[DataChunk::REG_TONEB_H] << 8) + State.Data[DataChunk::REG_TONEB_L];
    }
    inline uint16_t GetToneC() const
    {
      return (State.Data[DataChunk::REG_TONEC_H] << 8) + State.Data[DataChunk::REG_TONEC_L];
    }
    inline uint8_t GetToneN() const
    {
      return 2 * State.Data[DataChunk::REG_TONEN];//for optimization
    }
    inline uint16_t GetToneE() const
    {
      return (State.Data[DataChunk::REG_TONEE_H] << 8) + State.Data[DataChunk::REG_TONEE_L];
    }
    inline uint8_t GetVolA() const
    {
      return State.Data[DataChunk::REG_VOLA];
    }
    inline uint8_t GetVolB() const
    {
      return State.Data[DataChunk::REG_VOLB];
    }
    inline uint8_t GetVolC() const
    {
      return State.Data[DataChunk::REG_VOLC];
    }
    inline uint8_t GetMixer() const
    {
      return State.Data[DataChunk::REG_MIXER];
    }
    inline uint8_t GetEnv() const
    {
      return State.Data[DataChunk::REG_ENV];
    }
    inline Sound::Sample GetVolume(unsigned regVol, bool ym)
    {
      assert(regVol < 32);
      return (ym ? YMVolumeTab : AYVolumeTab)[regVol] / 2;
    }
  protected:
    //state
    DataChunk State; //time and registers
    unsigned BitA, BitB, BitC, BitN;
    uint16_t TimerA, TimerB, TimerC, TimerN, TimerE;
    unsigned Envelope;
    signed Decay;
    uint32_t Noise;

    DataChunk LastData;
    uint32_t LastTicksPerSec;
  };

  void ChipImpl::RenderData(const Sound::Parameters& params,
                            DataSource<DataChunk>& src,
                            Sound::Receiver& dst)
  {
    for (;;)
    {
      if (State.Tick >= LastData.Tick) //need to get data
      {
        if (! src.GetData(LastData))
        {
          return;//no more data
        }
        //output dump
        if (LastData.Mask)
        {
          ApplyLastData();
        }
      }
      DoRender(params, dst);
    }
  }

  void ChipImpl::RenderData(const Sound::Parameters& params, const DataChunk& src, Sound::Receiver& dst)
  {
    if (State.Tick >= LastData.Tick) //need to get data
    {
      LastData = src;
      //output dump
      if (LastData.Mask)
      {
        ApplyLastData();
      }
    }
    DoRender(params, dst);
  }

  void ChipImpl::ApplyLastData()
  {
    unsigned mask = 1;
    for (unsigned idx = 0; idx != ArraySize(LastData.Data); ++idx, mask <<= 1)
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

  void ChipImpl::DoRender(const Sound::Parameters& params, Sound::Receiver& dst)
  {
    LastTicksPerSec = params.ClockFreq;
    uint64_t& curTick = State.Tick;
    const uint64_t ticksPerSample(params.ClockFreq / params.SoundFreq);
    uint64_t nextSampleTick = curTick + ticksPerSample;

    unsigned HighLevel = ~unsigned(0);
    //references to mixered bits. updated automatically
    unsigned volA = (((GetVolA() & DataChunk::MASK_VOL) << 1) + 1);
    unsigned volB = (((GetVolB() & DataChunk::MASK_VOL) << 1) + 1);
    unsigned volC = (((GetVolC() & DataChunk::MASK_VOL) << 1) + 1);
    unsigned& ToneBitA = (GetMixer() & DataChunk::MASK_TONEA) ? HighLevel : BitA;
    unsigned& NoiseBitA = (GetMixer() & DataChunk::MASK_NOISEA) ? HighLevel : BitN;
    unsigned& VolumeA = (GetVolA() & DataChunk::MASK_ENV) ? Envelope : volA;
    unsigned& ToneBitB = (GetMixer() & DataChunk::MASK_TONEB) ? HighLevel : BitB;
    unsigned& NoiseBitB = (GetMixer() & DataChunk::MASK_NOISEB) ? HighLevel : BitN;
    unsigned& VolumeB = (GetVolB() & DataChunk::MASK_ENV) ? Envelope : volB;
    unsigned& ToneBitC = (GetMixer() & DataChunk::MASK_TONEC) ? HighLevel : BitC;
    unsigned& NoiseBitC = (GetMixer() & DataChunk::MASK_NOISEC) ? HighLevel : BitN;
    unsigned& VolumeC = (GetVolC() & DataChunk::MASK_ENV) ? Envelope : volC;

    while (curTick < LastData.Tick) //render cycle
    {
      if (curTick >= nextSampleTick) //need to store sample
      {
        const bool isYM(0 != (params.Flags & Sound::PSG_TYPE_YM));
        const Sound::Sample result[] = {
          GetVolume(ToneBitA & NoiseBitA & VolumeA, isYM),
          GetVolume(ToneBitB & NoiseBitB & VolumeB, isYM),
          GetVolume(ToneBitC & NoiseBitC & VolumeC, isYM)
        };
        dst.ApplySample(result, ArraySize(result));
        nextSampleTick += ticksPerSample;
      }
      curTick += 8;//base freq divisor
      if (++TimerA >= GetToneA())
      {
        TimerA = 0;
        BitA = ~BitA;
      }
      if (++TimerB >= GetToneB())
      {
        TimerB = 0;
        BitB = ~BitB;
      }
      if (++TimerC >= GetToneC())
      {
        TimerC = 0;
        BitC = ~BitC;
      }
      if (++TimerN >= GetToneN())
      {
        TimerN = 0;
        Noise = (Noise * 2 + 1) ^ (((Noise >> 16) ^ (Noise >> 13)) & 1);
        BitN = (Noise & 0x10000) ? ~0 : 0;
      }
      if (++TimerE >= GetToneE())
      {
        TimerE = 0;
        Envelope += Decay;
        if (Envelope & ~unsigned(31))
        {
          const unsigned envTypeMask = 1 << GetEnv();
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

  void ChipImpl::GetState(Sound::Analyze::Volume& volState, Sound::Analyze::Spectrum& spectrumState) const
  {
    //table in Hz*100
    static const unsigned FREQ_TABLE[Sound::Analyze::TonesCount] = {
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
      418620, 443460, 469890, 497790, 527420, 558720, 592000, 627200, 664450, 704000, 745860, 790140
    }; 
    volState.Array.resize(3);
    volState.ChannelsMask = 0;
    const std::size_t mixer(~GetMixer());
    for (std::size_t chan = 0; chan != 3; ++chan)
    {
      if (mixer & ((DataChunk::MASK_TONEA | DataChunk::MASK_NOISEA) << chan))
      {
        volState.ChannelsMask |= 1 << chan;
        const Sound::Analyze::Level level((State.Data[DataChunk::REG_VOLA + chan] & 0xf) * 
          std::numeric_limits<Sound::Analyze::Level>::max() / 15);
        volState.Array[chan] = level;
        if (mixer & (DataChunk::MASK_TONEA << chan))//tone
        {
          const uint16_t period = (uint16_t(State.Data[DataChunk::REG_TONEA_H + chan * 2]) << 8) |
            State.Data[DataChunk::REG_TONEA_L + chan * 2];
          const std::size_t freq(uint64_t(LastTicksPerSec) * 100 / (16 * (period ? period : 1)));
          spectrumState.Array[std::lower_bound(FREQ_TABLE, ArrayEnd(FREQ_TABLE), freq) - FREQ_TABLE] = level;
        }
      }
    }
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
