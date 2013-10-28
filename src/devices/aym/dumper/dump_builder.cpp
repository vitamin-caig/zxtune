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

    virtual void Add(const Registers& delta) = 0;
    virtual Registers GetBase() const = 0;
    virtual Registers GetDelta() const = 0;
    virtual void CommitDelta() = 0;
  };

  void ApplyMerge(Registers& dst, const Registers& src)
  {
    for (Registers::IndicesIterator it(src); it; ++it)
    {
      dst[*it] = src[*it];
    }
  }

  class NotOptimizedRenderState : public RenderState
  {
  public:
    virtual void Reset()
    {
      Base = Registers();
      Delta = Registers();
    }

    virtual void Add(const Registers& delta)
    {
      ApplyMerge(Delta, delta);
    }

    virtual Registers GetBase() const
    {
      return Base;
    }

    virtual Registers GetDelta() const
    {
      return Delta;
    }

    void CommitDelta()
    {
      ApplyMerge(Base, Delta);
      Delta = Registers();
    }
  protected:
    Registers Base;
    Registers Delta;
  };

  class OptimizedRenderState : public NotOptimizedRenderState
  {
  public:
    virtual void Add(const Registers& delta)
    {
      for (Registers::IndicesIterator it(delta); it; ++it)
      {
        const Registers::Index reg = *it;
        const uint8_t newVal = delta[reg];
        if (Registers::ENV != reg && Base.Has(reg))
        {
          uint8_t& base = Base[reg];
          if (newVal == base)
          {
            Delta.Reset(reg);
            continue;
          }
        }
        Delta[reg] = newVal;
      }
    }
  };

  class FrameDumper : public Dumper
  {
  public:
    FrameDumper(const Time::Microseconds& frameDuration, FramedDumpBuilder::Ptr builder, std::auto_ptr<RenderState> state)
      : FrameDuration(frameDuration)
      , Builder(builder)
      , State(state)
      , FramesToSkip(0)
      , NextFrame()
    {
      Reset();
    }

    virtual void RenderData(const DataChunk& src)
    {
      if (NextFrame < src.TimeStamp)
      {
        FinishFrame();
      }
      State->Add(src.Data);
    }

    virtual void RenderData(const std::vector<DataChunk>& src)
    {
      for (std::vector<DataChunk>::const_iterator it = src.begin(), lim = src.end(); it != lim; ++it)
      {
        if (NextFrame < it->TimeStamp)
        {
          FinishFrame();
        }
        State->Add(it->Data);
      }
    }

    virtual void Reset()
    {
      Builder->Initialize();
      State->Reset();
      FramesToSkip = 0;
      NextFrame = FrameDuration;
    }

    virtual void GetDump(Dump& result) const
    {
      if (FramesToSkip)
      {
        const Registers delta = State->GetDelta();
        State->CommitDelta();
        Builder->WriteFrame(FramesToSkip, State->GetBase(), delta);
        FramesToSkip = 0;
      }
      Builder->GetResult(result);
    }
  private:
    void FinishFrame()
    {
      ++FramesToSkip;
      const Registers delta = State->GetDelta();
      if (!delta.Empty())
      {
        State->CommitDelta();
        const Registers& current = State->GetBase();
        Builder->WriteFrame(FramesToSkip, current, delta);
        FramesToSkip = 0;
      }
      NextFrame += FrameDuration;
    }
  private:
    const Stamp FrameDuration;
    const FramedDumpBuilder::Ptr Builder;
    const std::auto_ptr<RenderState> State;
    mutable uint_t FramesToSkip;
    Stamp NextFrame;
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
      default:
        state.reset(new OptimizedRenderState());
      }
      return boost::make_shared<FrameDumper>(params->FrameDuration(), builder, boost::ref(state));
    }
  }
}
