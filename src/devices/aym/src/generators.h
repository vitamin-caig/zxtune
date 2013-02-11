/*
Abstract:
  AY/YM chips generators helpers

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  Based on sources of UnrealSpeccy by SMT and Xpeccy sources by SamStyle
*/

#pragma once
#ifndef DEVICES_AYM_GENERATORS_H_DEFINED
#define DEVICES_AYM_GENERATORS_H_DEFINED

//common includes
#include <types.h>
//boost includes
#include <boost/array.hpp>

namespace Devices
{
  namespace AYM
  {
    const uint_t MAX_DUTYCYCLE = 100;
    const uint_t NO_DUTYCYCLE = MAX_DUTYCYCLE / 2;

    const uint_t LOW_LEVEL = 0;
    const uint_t HIGH_LEVEL = ~LOW_LEVEL;

    class NoiseLookup
    {
    public:
      static const uint_t INDEX_MASK = 0x1ffff;

      NoiseLookup()
      {
        uint_t seed = 0xffff;
        for (std::size_t idx = 0; idx != Lookup.size(); ++idx)
        {
          const bool level = 0 != (seed & 0x10000);
          seed = (seed * 2 + (level != (0 != (seed & 0x2000)))) & INDEX_MASK;
          Lookup[idx] = level ? HIGH_LEVEL : LOW_LEVEL;
        }
      }

      uint_t operator[] (uint_t idx) const
      {
        return Lookup[idx];
      }
    private:
      boost::array<uint_t, INDEX_MASK + 1> Lookup;
    };

    const NoiseLookup NoiseTable;

    //PSG-related functionality
    class Generator
    {
    public:
      Generator()
        : DoublePeriod(2)
        , DutyCycle(NO_DUTYCYCLE)
        , MiddlePeriod(1)
        , Counter()
      {
      }

      void Reset()
      {
        SetPeriod(0);
        ResetCounter();
      }

      void SetPeriod(uint_t period)
      {
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

      void ResetCounter()
      {
        Counter = 0;
      }
    private:
      void UpdateMiddle()
      {
        MiddlePeriod = DutyCycle * DoublePeriod / MAX_DUTYCYCLE;
      }
    protected:
      uint_t DoublePeriod;
      uint_t DutyCycle;
      uint_t MiddlePeriod;
      mutable uint_t Counter;
    };

    //Some of the hardware platforms has no native div/mod operations, so it's better to substract.
    //In any case, it's better than sequental incremental/comparing
    class FlipFlopGenerator : public Generator
    {
    protected:
      bool GetFlip() const
      {
        const uint_t mask = DoublePeriod - 1;
        //if power of two
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
        return Counter >= MiddlePeriod;
      }
    };

    class CountingGenerator : public Generator
    {
    public:
      //for counting generators use duty cycle >= 50% for valid periods calculating
      void SetDutyCycle(uint_t dutyCycle)
      {
        Generator::SetDutyCycle(std::max(dutyCycle, MAX_DUTYCYCLE - dutyCycle));
      }
    protected:
      uint_t GetPeriodsPassed() const
      {
        uint_t res = 0;
        uint_t mask = DoublePeriod - 1;
        //if power of two
        if (0 == (DoublePeriod & mask))
        {
          res = Counter & ~mask;
          Counter &= mask;
          while (mask != 1)
          {
            res >>= 1;
            mask >>= 1;
          }
        }
        else
        {
          while (Counter >= DoublePeriod)
          {
            Counter -= DoublePeriod;
            res += 2;
          }
        }
        if (Counter >= MiddlePeriod)
        {
          //additional 'modulo' to take into account half of period
          ++res;
          Counter = MiddlePeriod - (DoublePeriod - Counter);
        }
        return res;
      }
    };

    class ToneGenerator : public FlipFlopGenerator
    {
    public:
      void SetPeriod(uint_t period)
      {
        GetFlip();
        FlipFlopGenerator::SetPeriod(period);
      }

      uint_t GetLevel() const
      {
        return GetFlip() ? HIGH_LEVEL : LOW_LEVEL;
      }
    };

    class NoiseGenerator : public CountingGenerator
    {
    public:
      NoiseGenerator()
        : Index()
      {
      }

      void Reset()
      {
        Index = 0;
        CountingGenerator::Reset();
      }

      void SetPeriod(uint_t period)
      {
        UpdateIndex();
        CountingGenerator::SetPeriod(period);
      }

      uint_t GetLevel() const
      {
        UpdateIndex();
        return NoiseTable[Index];
      }
    private:
      void UpdateIndex() const
      {
        Index += GetPeriodsPassed();
        Index &= NoiseTable.INDEX_MASK;
      }
    private:
      mutable uint_t Index;
      bool Masked;
    };

    class EnvelopeGenerator : public CountingGenerator
    {
    public:
      EnvelopeGenerator()
        : Type()
        , Level()
        , Decay()
      {
      }

      void Reset()
      {
        Type = 0;
        Level = 0;
        Decay = 0;
        CountingGenerator::Reset();
      }

      void SetType(uint_t type)
      {
        CountingGenerator::ResetCounter();
        Type = type;
        if (Type & 4) //down-up envelopes
        {
          Level = 0;
          Decay = 1;
        }
        else //up-down envelopes
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
          for (uint_t rounds = GetPeriodsPassed(); rounds && Decay; --rounds)
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
        if (envTypeMask & ((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) |
                           (1 << 5) | (1 << 6) | (1 << 7) | (1 << 9) | (1 << 15)))
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
          Decay = 0; //11, 13
        }
      }
    private:
      uint_t Type;
      mutable uint_t Level;
      mutable int_t Decay;
    };
  }
}

#endif //DEVICES_AYM_GENERATORS_H_DEFINED
