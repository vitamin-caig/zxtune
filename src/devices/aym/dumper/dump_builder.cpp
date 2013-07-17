/*
Abstract:
  AY dump builder implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "dump_builder.h"
//boost includes
#include <boost/make_shared.hpp>
#include <boost/ref.hpp>

namespace
{
  using namespace Devices::AYM;

  class RenderState
  {
  public:
    virtual ~RenderState() {}

    virtual void Reset() = 0;

    virtual void Update(const DataChunk& delta) = 0;
    virtual void Flush(const Stamp& stamp) = 0;

    virtual DataChunk GetCurrent() const = 0;
    virtual DataChunk GetDeltaFromPrevious() const = 0;
  };

  void ApplyMerge(DataChunk& dst, const DataChunk& src)
  {
    if (0 == src.Mask)
    {
      return;
    }
    for (uint_t reg = 0, mask = 1; reg < DataChunk::REG_LAST; ++reg, mask <<= 1)
    {
      if (src.Mask & mask)
      {
        dst.Mask |= mask;
        dst.Data[reg] = src.Data[reg];
      }
    }
  }

  class NotOptimizedRenderState : public RenderState
  {
  public:
    virtual void Reset()
    {
      Current = DataChunk();
      Delta = DataChunk();
    }

    virtual void Update(const DataChunk& delta)
    {
      ApplyMerge(Current, delta);
      ApplyMerge(Delta, delta);
    }

    void Flush(const Stamp& stamp)
    {
      Delta.Mask = 0;
      Current.TimeStamp = stamp;
    }

    virtual DataChunk GetCurrent() const
    {
      return Current;
    }

    virtual DataChunk GetDeltaFromPrevious() const
    {
      return Delta;
    }
  private:
    DataChunk Current;
    DataChunk Delta;
  };

  void ApplyOptimizedMerge(DataChunk& dst, const DataChunk& src)
  {
    if (0 == src.Mask)
    {
      return;
    }
    for (uint_t reg = 0, mask = 1; reg < DataChunk::REG_LAST; ++reg, mask <<= 1)
    {
      if (src.Mask & mask)
      {
        const uint8_t oldReg = dst.Data[reg];
        const uint8_t newReg = src.Data[reg];
        if (oldReg != newReg || reg == DataChunk::REG_ENV)
        {
          dst.Mask |= mask;
          dst.Data[reg] = src.Data[reg];
        }
      }
    }
  }

  class OptimizedRenderState : public RenderState
  {
  public:
    virtual void Reset()
    {
      Current = DataChunk();
      Delta = DataChunk();
    }

    virtual void Update(const DataChunk& delta)
    {
      ApplyOptimizedMerge(Current, delta);
      ApplyOptimizedMerge(Delta, delta);
    }

    void Flush(const Stamp& stamp)
    {
      Delta.Mask = 0;
      Current.TimeStamp = stamp;
    }

    virtual DataChunk GetCurrent() const
    {
      return Current;
    }

    virtual DataChunk GetDeltaFromPrevious() const
    {
      return Delta;
    }
  private:
    DataChunk Current;
    DataChunk Delta;
  };

  class ExtraOptimizedRenderState : public RenderState
  {
  public:
    virtual void Reset()
    {
      Previous = DataChunk();
      Current = DataChunk();
    }

    virtual void Update(const DataChunk& delta)
    {
      static const uint8_t MASKS[] =
      {
        0xff, 0x0f,//tonea
        0xff, 0x0f,//toneb
        0xff, 0x0f,//tonec
        0x1f,//tonen
        0x3f,//mixer
        0x1f, 0x1f, 0x1f,//volumes
        0xff, 0xff,//envelope tone
        0x1f,//envelope type
        0xff//beeper?
      };
      if (0 == delta.Mask)
      {
        return;
      }

      //update only different and r13
      for (uint_t reg = 0, mask = 1; reg < DataChunk::REG_LAST; ++reg, mask <<= 1)
      {
        if (delta.Mask & mask)
        {
          const uint8_t valBits = MASKS[reg];
          const uint8_t oldReg = Current.Data[reg];
          const uint8_t newReg = delta.Data[reg] & valBits;
          if (oldReg != newReg || reg == DataChunk::REG_ENV)
          {
            Current.Mask |= mask;
            Current.Data[reg] = newReg;
          }
        }
      }
    }

    void Flush(const Stamp& stamp)
    {
      const DataChunk& delta = GetDeltaFromPrevious();
      ApplyOptimizedMerge(Previous, delta);
      Current.TimeStamp = stamp;
      Current.Mask &= ~delta.Mask;
    }

    virtual DataChunk GetCurrent() const
    {
      return Current;
    }

    virtual DataChunk GetDeltaFromPrevious() const
    {
      uint_t uselessRegisters = 0;

      //Current.Mask contain info about registers changed since last Flush call relatively to Previous
      const uint8_t mixer = ~GetRegister(DataChunk::REG_MIXER);
      for (uint_t chan = 0; chan != 3; ++chan)
      {
        if (const bool zeroVolume = 0 == GetRegister(DataChunk::REG_VOLA + chan))
        {
          uselessRegisters |= ((1 << DataChunk::REG_TONEA_L) | (1 << DataChunk::REG_TONEA_H)) << chan * 2;
        }
      }
      if (const bool noiseDisabled = 0 == (mixer & (DataChunk::REG_MASK_NOISEA | DataChunk::REG_MASK_NOISEB | DataChunk::REG_MASK_NOISEC)))
      {
        uselessRegisters |= 1 << DataChunk::REG_TONEN;
      }
      if (const bool envelopeDisabled = 0 == ((GetRegister(DataChunk::REG_VOLA) | GetRegister(DataChunk::REG_VOLB) | GetRegister(DataChunk::REG_VOLC)) & DataChunk::REG_MASK_ENV))
      {
        uselessRegisters |= (1 << DataChunk::REG_TONEE_L) | (1 << DataChunk::REG_TONEE_H);
      }

      DataChunk result;
      const uint_t changedRegisters = Current.Mask & ~uselessRegisters;
      for (uint_t reg = 0, mask = 1; reg < DataChunk::REG_LAST; ++reg, mask <<= 1)
      {
        if (changedRegisters & mask)
        {
          const uint8_t oldReg = Previous.Data[reg];
          const uint8_t newReg = Current.Data[reg];
          if (oldReg != newReg || reg == DataChunk::REG_ENV)
          {
            result.Mask |= mask;
            result.Data[reg] = newReg;
          }
        }
      }
      return result;
    }
  private:
    uint8_t GetRegister(uint_t idx) const
    {
      const uint_t regMask = 1 << idx;
      return 0 != (Current.Mask & regMask) ? Current.Data[idx] : Previous.Data[idx];
    }
  private:
    DataChunk Previous;
    DataChunk Current;
  };

  class FrameDumper : public Dumper
  {
  public:
    FrameDumper(const Time::Microseconds& frameDuration, FramedDumpBuilder::Ptr builder, std::auto_ptr<RenderState> state)
      : FrameDuration(frameDuration)
      , Builder(builder)
      , State(state)
      , FramesToSkip(0)
    {
      Reset();
    }

    virtual void RenderData(const DataChunk& src)
    {
      Buffer.push_back(src);
    }

    virtual void Flush()
    {
      const DataChunk& current = State->GetCurrent();
      Stamp nextFrame = current.TimeStamp;
      nextFrame += FrameDuration;
      for (std::vector<DataChunk>::iterator it = Buffer.begin(), lim = Buffer.end(); it != lim; ++it)
      {
        if (it->TimeStamp < nextFrame)
        {
          State->Update(*it);
        }
        else 
        {
          ++FramesToSkip;
          const DataChunk& delta = State->GetDeltaFromPrevious();
          if (delta.Mask)
          {
            Builder->WriteFrame(FramesToSkip, current, delta);
            FramesToSkip = 0;
          }
          State->Flush(nextFrame);
          nextFrame += FrameDuration;
        }
      }
      Buffer.clear();
    }

    virtual void Reset()
    {
      Builder->Initialize();
      Buffer.clear();
      State->Reset();
      FramesToSkip = 0;
    }

    virtual void GetDump(Dump& result) const
    {
      if (FramesToSkip)
      {
        Builder->WriteFrame(FramesToSkip, State->GetCurrent(), State->GetDeltaFromPrevious());
        FramesToSkip = 0;
      }
      Builder->GetResult(result);
    }
  private:
    const Stamp FrameDuration;
    const FramedDumpBuilder::Ptr Builder;
    const std::auto_ptr<RenderState> State;
    std::vector<DataChunk> Buffer;
    mutable uint_t FramesToSkip;
  };
}

namespace Devices
{
  namespace AYM
  {
    Dumper::Ptr CreateDumper(DumperParameters::Ptr params, FramedDumpBuilder::Ptr builder)
    {
      std::auto_ptr<RenderState> state;
      switch (params->OptimizationLevel())
      {
      case DumperParameters::NONE:
        state.reset(new NotOptimizedRenderState());
        break;
      case DumperParameters::MAXIMUM:
        state.reset(new ExtraOptimizedRenderState());
        break;
      default:
        state.reset(new OptimizedRenderState());
      }
      return boost::make_shared<FrameDumper>(params->FrameDuration(), builder, boost::ref(state));
    }
  }
}
