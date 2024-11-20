/**
 *
 * @file
 *
 * @brief  SAA chip generators implementation
 *
 * @author vitamin.caig@gmail.com
 *
 * @note Based on sources of PerfectZX emulator
 *
 **/

#pragma once

#include "devices/saa.h"

#include "types.h"

#include <array>

namespace Devices::SAA
{
  const uint_t LOW_LEVEL = 0;
  const uint_t HIGH_LEVEL = 15;
  const uint_t MAX_VALUE = HIGH_LEVEL + 1;

  class FastSample
  {
  public:
    FastSample() = default;

    FastSample(uint_t left, uint_t right)
      : Value(left | (right << 16))
    {}

    uint_t Left() const
    {
      return Value & 0xffff;
    }

    uint_t Right() const
    {
      return Value >> 16;
    }

    void Add(const FastSample& rh)
    {
      Value += rh.Value;
    }

    Sound::Sample Convert() const
    {
      return {ToSample(Left()), ToSample(Right())};
    }

  private:
    static Sound::Sample::Type ToSample(uint_t idx)
    {
      return Sound::Sample::MID + (idx << 8) + (idx << 5);
    }

  private:
    uint_t Value = 0;
  };

  // PSG-related functionality
  class ToneGenerator
  {
  public:
    ToneGenerator()
    {
      UpdatePeriod();
    }

    void Reset()
    {
      Masked = true;
      Frequency = Octave = 0;
      UpdatePeriod();
      Counter = 0;
    }

    void SetMasked(bool masked)
    {
      Masked = masked;
    }

    void SetNumber(uint_t freq)
    {
      Frequency = freq;
      UpdatePeriod();
    }

    void SetOctave(uint_t octave)
    {
      Octave = octave & 7;
      UpdatePeriod();
    }

    uint_t GetHalfPeriod() const
    {
      // octave0: 31Hz...61Hz, full period is 258064(0x3f010) .. 141147(0x2004b)
      //...
      // octave7: 3910Hz...7810Hz, full period is 2046(0x7fe) .. 1024 (0x400)
      return (511 - Frequency) << (8 - Octave);
    }

    void Tick(uint_t ticks)
    {
      Counter += ticks;
    }

    template<uint_t Hi>
    uint_t GetLevel() const
    {
      return Masked || GetFlip() ? Hi : LOW_LEVEL;
    }

    bool IsMasked() const
    {
      return Masked;
    }

  private:
    void UpdatePeriod()
    {
      SetHalfPeriod(GetHalfPeriod());
    }

    void SetHalfPeriod(uint_t period)
    {
      WrapCounter();
      const bool lowPart = Counter < HalfPeriod;
      const uint_t correction = lowPart ? Counter : Counter - HalfPeriod;
      HalfPeriod = std::max<uint_t>(1, period);
      FullPeriod = 2 * HalfPeriod;
      Counter = lowPart ? correction : HalfPeriod + correction;
    }

    bool GetFlip() const
    {
      WrapCounter();
      return Counter >= HalfPeriod;
    }

    void WrapCounter() const
    {
      // Some of the hardware platforms has no native div/mod operations, so it's better to substract.
      // In any case, it's better than sequental incremental/comparing
      const uint_t mask = FullPeriod - 1;
      // if power of two
      if (0 == (FullPeriod & mask))
      {
        Counter &= mask;
      }
      else
      {
        while (Counter >= FullPeriod)
        {
          Counter -= FullPeriod;
        }
      }
    }

  private:
    bool Masked = true;
    uint_t Frequency = 0;
    uint_t Octave = 0;
    uint_t HalfPeriod = 1;
    uint_t FullPeriod = 2;
    mutable uint_t Counter = 0;
  };

  class CountingGenerator
  {
  public:
    CountingGenerator() = default;

    void Reset()
    {
      Period = 1;
      Counter = 0;
      Index = 0;
    }

    void SetPeriod(uint_t period)
    {
      UpdateIndex();
      Period = std::max<uint_t>(1, period);
    }

    void Tick(uint_t ticks)
    {
      Counter += ticks;
    }

  protected:
    void UpdateIndex() const
    {
      uint_t mask = Period - 1;
      if (0 == (Period & mask))
      {
        uint_t delta = Counter & ~mask;
        Counter &= mask;
        while (mask != 0)
        {
          delta >>= 1;
          mask >>= 1;
        }
        Index += delta;
      }
      else
      {
        while (Counter >= Period)
        {
          Counter -= Period;
          ++Index;
        }
      }
    }

  protected:
    uint_t Period = 1;
    mutable uint_t Counter = 0;
    mutable uint_t Index = 0;
  };

  class NoiseGenerator : public CountingGenerator
  {
  public:
    NoiseGenerator() = default;

    void Reset()
    {
      ExternalPeriod = 1;
      Mixer = 0;
      CountingGenerator::Reset();
      SetMode(0);
      Seed = 0;
    }

    void SetPeriod(uint_t period)
    {
      ExternalPeriod = period;
      if (Mode == 3)
      {
        CountingGenerator::SetPeriod(period);
      }
    }

    void SetMode(uint_t mode)
    {
      Mode = mode;
      if (Mode == 3)
      {
        CountingGenerator::SetPeriod(ExternalPeriod);
      }
      else
      {
        CountingGenerator::SetPeriod(128 << Mode);
      }
    }

    void SetMixer(uint_t mix)
    {
      Mixer = mix;
    }

    uint_t GetLevel() const
    {
      if (Mixer)
      {
        Update();
        return 0 != (Seed & 0x0001) ? (HIGH_LEVEL ^ Mixer) : HIGH_LEVEL;
      }
      else
      {
        return HIGH_LEVEL;
      }
    }

    uint_t GetMixer() const
    {
      return Mixer;
    }

    uint_t GetPeriod() const
    {
      return Period;
    }

  private:
    void Update() const
    {
      uint_t prevIndex = Index;
      UpdateIndex();
      while (prevIndex++ != Index)
      {
        Seed <<= 1;
        const uint_t bits = Seed & 0x8080;
        Seed |= 0 == bits || 0x8080 == bits;
      }
    }

  private:
    uint_t Mode = 0;
    uint_t ExternalPeriod = 1;
    uint_t Mixer = 0;
    mutable uint_t Seed = 0;
  };

  /*
    Type Waveform
    0    ________
    1    --------
    2    \_______
    3    \|\|\|\|
    4    /\______
    5    /\/\/\/\
    6    /|______
    7    /|/|/|/|
  */
  class EnvelopeGenerator : public CountingGenerator
  {
  public:
    EnvelopeGenerator() = default;

    void Reset()
    {
      Enabled = false;
      Type = 0;
      Level = 0;
      Decay = 0;
      XorValue = 0;
      CountingGenerator::Reset();
    }

    void SetControl(uint_t ctrl)
    {
      // TODO: implement external clocking
      XorValue = 0 != (ctrl & 0x01) ? HIGH_LEVEL : 0;
      Enabled = 0 != (ctrl & 0x80);
      if (Enabled)
      {
        Type = (ctrl & 0x0e) >> 1;
        const int_t decayStep = 1 + ((ctrl & 0x10) >> 4);
        Counter = Index = 0;
        if (Type < 2)
        {
          Level = Type ? HIGH_LEVEL : 0;
          Decay = 0;
        }
        else if (Type < 4)
        {
          Level = HIGH_LEVEL;
          Decay = -decayStep;
        }
        else
        {
          Level = 0;
          Decay = decayStep;
        }
      }
    }

    void SetPeriod(uint_t period)
    {
      Update();
      CountingGenerator::SetPeriod(period);
    }

    FastSample GetLevel(const FastSample& in) const
    {
      if (Enabled)
      {
        Update();
        return {Scale(in.Left(), Level), Scale(in.Right(), Level ^ XorValue)};
      }
      else
      {
        return in;
      }
    }

    uint_t GetRepetitionPeriod() const
    {
      if (Enabled)
      {
        switch (Type)
        {
        case 3:
        case 7:
          return Period * GetSteps();
        case 5:
          return 2 * Period * GetSteps();
        }
      }
      return 0;
    }

  private:
    void Update() const
    {
      if (Decay)
      {
        uint_t prevIndex = Index;
        UpdateIndex();
        while (prevIndex++ != Index)
        {
          UpdateStep();
        }
      }
    }

    void UpdateStep() const
    {
      Level += Decay;
      if (0 == (Level & ~HIGH_LEVEL))
      {
        return;
      }
      switch (Type)
      {
      case 3:
      case 7:
        Level &= HIGH_LEVEL;
        break;
      case 4:
      case 5:
        Decay = -Decay;
        Level += Decay;
        if (Decay < 0 || Type == 5)
        {
          break;
        }
        [[fallthrough]];
      case 2:
      case 6:
        Level = 0;
        Decay = 0;
        break;
      }
    }

    static uint_t Scale(uint_t lh, uint_t rh)
    {
      return lh * rh / MAX_VALUE;
    }

    uint_t GetSteps() const
    {
      return Decay > 0 ? MAX_VALUE / Decay : (Decay < 0 ? MAX_VALUE / -Decay : 1);
    }

  private:
    bool Enabled = false;
    uint_t Type = 0;
    mutable uint_t Level = 0;
    mutable int_t Decay = 0;
    uint_t XorValue = 0;
  };
}  // namespace Devices::SAA
