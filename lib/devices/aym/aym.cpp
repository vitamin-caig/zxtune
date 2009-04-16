#include "aym.h"

#include <tools.h>

#include <sound_attrs.h>

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
    {
      Reset();
    }

    virtual void RenderData(const Sound::Parameters& params,
                            DataSource<DataChunk>* src,
                            Sound::Receiver* dst);

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

      LastData.Tick = ~uint64_t(0);
    }

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
    inline unsigned GetVolume(unsigned regVol, bool ym)
    {
      assert(regVol < 32);
      return (ym ? YMVolumeTab : AYVolumeTab)[regVol];
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
  };

  typedef Sound::Sample SoundData[4];

  void ChipImpl::RenderData(const Sound::Parameters& params,
                            DataSource<DataChunk>* src,
                            Sound::Receiver* dst)
  {
    assert(src);
    assert(dst);

    if (~uint64_t(0) == LastData.Tick &&
        !src->GetData(LastData)
       )
    {
      return;//no data at all
    }

    // tick counter and ranges
    uint64_t& curTick = State.Tick;
    const uint64_t startBuffTick = curTick;
    const uint64_t ticksPerSample(params.ClockFreq / params.SoundFreq);
    uint64_t nextSampleTick = startBuffTick + ticksPerSample;

    // rendering context
    SoundData result = {0};
    unsigned HighLevel = ~0;
    for (;;)
    {
      if (curTick >= LastData.Tick) //need to get data
      {
        //output dump
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

        if (! src->GetData(LastData))
        {
          return;//no more data
        }
      }

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
        curTick += 8;//base freq divisor
        if (curTick >= nextSampleTick) //need to store sample
        {
          const bool isYM(0 != (params.Flags & Sound::PSG_TYPE_YM));
          result[0] = GetVolume(ToneBitA & NoiseBitA & VolumeA, isYM);
          result[1] = GetVolume(ToneBitB & NoiseBitB & VolumeB, isYM);
          result[2] = GetVolume(ToneBitC & NoiseBitC & VolumeC, isYM);
          result[3] = (LastData.Mask & DataChunk::BEEPER_BIT) ? ~Sound::Sample(0) : 0
          dst->ApplySample(result, LastData.Mask & DataChunk::BEEPER_MASK ? 4 : 3);
          nextSampleTick += ticksPerSample;
        }
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
          if (Envelope & ~31)
          {
            unsigned envTypeMask = 1 << GetEnv();
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
    }//loop
  }

  void ChipImpl::GetState(Sound::Analyze::Volume& volState, Sound::Analyze::Spectrum& spectrumState) const
  {
  }
}

namespace ZXTune
{
  Chip::Ptr Chip::Create()
  {
    return Chip::Ptr(new ChipImpl);
  }
}