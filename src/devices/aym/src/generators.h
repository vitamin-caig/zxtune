/**
 *
 * @file
 *
 * @brief  AY/YM chip generators implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <types.h>

#include <array>
#include <cassert>

namespace Devices::AYM
{
  const uint_t MAX_DUTYCYCLE = 100;
  const uint_t NO_DUTYCYCLE = MAX_DUTYCYCLE / 2;

  const uint_t LOW_LEVEL = 0;
  const uint_t BITS_PER_LEVEL = 5;
  const uint_t HIGH_LEVEL_A = (1 << BITS_PER_LEVEL) - 1;
  const uint_t HIGH_LEVEL_B = HIGH_LEVEL_A << BITS_PER_LEVEL;
  const uint_t HIGH_LEVEL_C = HIGH_LEVEL_B << BITS_PER_LEVEL;
  const uint_t HIGH_LEVEL = HIGH_LEVEL_A | HIGH_LEVEL_B | HIGH_LEVEL_C;

  class NoiseLookup
  {
  public:
    static const uint_t INDEX_MASK = 0x1ffff;

    NoiseLookup()
    {
      uint_t seed = 0xffff;
      for (auto& out : Lookup)
      {
        const bool level = 0 != (seed & 0x10000);
        seed = (seed * 2 + (level != (0 != (seed & 0x2000)))) & INDEX_MASK;
        out = level ? HIGH_LEVEL : LOW_LEVEL;
      }
    }

    uint_t operator[](uint_t idx) const
    {
      return Lookup[idx];
    }

  private:
    std::array<uint_t, INDEX_MASK + 1> Lookup;
  };

  const NoiseLookup NoiseTable;

  // PSG-related functionality
  class ToneGenerator
  {
  public:
    ToneGenerator()
      : DutyCycle(NO_DUTYCYCLE)
    {}

    void Reset()
    {
      Masked = true;
      SetPeriod(0);
      Counter = 0;
    }

    void SetMasked(bool masked)
    {
      Masked = masked;
    }

    void SetPeriod(uint_t period)
    {
      WrapCounter();
      const bool lowPart = Counter < MiddlePeriod;
      const uint_t correction = lowPart ? Counter : Counter - MiddlePeriod;
      DoublePeriod = 2 * std::max<uint_t>(1, period);
      UpdateMiddle();
      Counter = lowPart ? correction : MiddlePeriod + correction;
    }

    void SetDutyCycle(uint_t dutyCycle)
    {
      assert(dutyCycle > 0 && dutyCycle < MAX_DUTYCYCLE);
      DutyCycle = dutyCycle;
      UpdateMiddle();
    }

    void Tick(uint_t ticks)
    {
      Counter += ticks;
    }

    template<uint_t Lo, uint_t Hi>
    uint_t GetLevel() const
    {
      return (Masked || GetFlip()) ? Hi : Lo;
    }

  private:
    void UpdateMiddle()
    {
      if (DutyCycle == NO_DUTYCYCLE)
      {
        MiddlePeriod = DoublePeriod / 2;
      }
      else
      {
        MiddlePeriod = DutyCycle * DoublePeriod / MAX_DUTYCYCLE;
      }
    }

    bool GetFlip() const
    {
      WrapCounter();
      return Counter >= MiddlePeriod;
    }

    void WrapCounter() const
    {
      // Some of the hardware platforms has no native div/mod operations, so it's better to substract.
      // In any case, it's better than sequental incremental/comparing
      const uint_t mask = DoublePeriod - 1;
      // if power of two
      if (0 == (DoublePeriod & mask))
      {
        Counter &= mask;
      }
      else
      {
        while (Counter >= DoublePeriod)
        {
          Counter -= DoublePeriod;
        }
      }
    }

  private:
    bool Masked = true;
    uint_t DoublePeriod = 2;
    uint_t DutyCycle;
    uint_t MiddlePeriod = 1;
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
    uint_t GetLevel() const
    {
      UpdateIndex();
      return NoiseTable[Index & NoiseTable.INDEX_MASK];
    }
  };

  /*
    Type Waveform
    0..3 \_______
    4..7 /|______
    8    \|\|\|\|
    9    \_______
    10   \/\/\/\/
    11   \|
    12   /|/|/|/|
    13   /
    14   /\/\/\/\
    15   /|______
  */
  class EnvelopeGenerator : public CountingGenerator
  {
  public:
    EnvelopeGenerator() = default;

    void Reset()
    {
      Type = 0;
      Level = 0;
      Decay = 0;
      CountingGenerator::Reset();
    }

    void SetType(uint_t type)
    {
      Type = type;
      Counter = Index = 0;
      if (Type & 4)  // down-up envelopes
      {
        Level = 0;
        Decay = 1;
      }
      else  // up-down envelopes
      {
        Level = 31;
        Decay = -1;
      }
    }

    void SetPeriod(uint_t period)
    {
      Update();
      CountingGenerator::SetPeriod(period);
    }

    uint_t GetLevel() const
    {
      Update();
      return Level;
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
      if (0 == (Level & ~31u))
      {
        return;
      }
      const uint_t envTypeMask = 1 << Type;
      if (envTypeMask
          & ((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 9)
             | (1 << 15)))
      {
        Level = 0;
        Decay = 0;
      }
      else if (envTypeMask & ((1 << 8) | (1 << 12)))
      {
        Level &= 31;
      }
      else if (envTypeMask & ((1 << 10) | (1 << 14)))
      {
        Decay = -Decay;
        Level += Decay;
      }
      else
      {
        Level = 31;
        Decay = 0;  // 11, 13
      }
    }

  private:
    uint_t Type = 0;
    mutable uint_t Level = 0;
    mutable int_t Decay = 0;
  };
}  // namespace Devices::AYM
