/**
 *
 * @file
 *
 * @brief  AY/YM dump builder implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "devices/aym/dumper/dump_builder.h"
// common includes
#include <make_ptr.h>
// std includes
#include <utility>

namespace Devices::AYM
{
  class RenderState
  {
  public:
    using Ptr = std::unique_ptr<RenderState>;
    virtual ~RenderState() = default;

    virtual void Reset() = 0;

    virtual void Add(const Registers& delta) = 0;
    virtual Registers GetBase() const = 0;
    virtual Registers GetDelta() const = 0;
    virtual void CommitDelta() = 0;
  };

  uint8_t GetValueMask(Registers::Index idx)
  {
    const uint_t REGS_4BIT_SET = (1 << Registers::TONEA_H) | (1 << Registers::TONEB_H) | (1 << Registers::TONEC_H)
                                 | (1 << Registers::ENV);
    const uint_t REGS_5BIT_SET = (1 << Registers::TONEN) | (1 << Registers::VOLA) | (1 << Registers::VOLB)
                                 | (1 << Registers::VOLC);

    const uint_t mask = 1 << idx;
    if (mask & REGS_4BIT_SET)
    {
      return 0x0f;
    }
    else if (mask & REGS_5BIT_SET)
    {
      return 0x1f;
    }
    else
    {
      return 0xff;
    }
  }

  void ApplyMerge(Registers& dst, const Registers& src)
  {
    for (Registers::IndicesIterator it(src); it; ++it)
    {
      const Registers::Index idx = *it;
      dst[idx] = src[idx] & GetValueMask(idx);
    }
  }

  class NotOptimizedRenderState : public RenderState
  {
  public:
    void Reset() override
    {
      Base = Registers();
      Delta = Registers();
    }

    void Add(const Registers& delta) override
    {
      ApplyMerge(Delta, delta);
    }

    Registers GetBase() const override
    {
      return Base;
    }

    Registers GetDelta() const override
    {
      return Delta;
    }

    void CommitDelta() override
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
    void Add(const Registers& delta) override
    {
      for (Registers::IndicesIterator it(delta); it; ++it)
      {
        const Registers::Index reg = *it;
        const uint8_t newVal = delta[reg] & GetValueMask(reg);
        if (Registers::ENV != reg && Base.Has(reg))
        {
          if (newVal == Base[reg])
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
    FrameDumper(Time::Microseconds frameDuration, FramedDumpBuilder::Ptr builder, RenderState::Ptr state)
      : FrameDuration(frameDuration)
      , Builder(std::move(builder))
      , State(std::move(state))
    {
      Reset();
    }

    void RenderData(const DataChunk& src) override
    {
      if (!(src.TimeStamp < NextFrame))
      {
        FinishFrame();
      }
      State->Add(src.Data);
    }

    void RenderData(const std::vector<DataChunk>& src) override
    {
      for (const auto& chunk : src)
      {
        RenderData(chunk);
      }
    }

    void Reset() override
    {
      Builder->Initialize();
      State->Reset();
      FramesToSkip = 0;
      NextFrame = {};
      NextFrame += FrameDuration;
    }

    Binary::Data::Ptr GetDump() override
    {
      if (FramesToSkip)
      {
        const Registers delta = State->GetDelta();
        State->CommitDelta();
        Builder->WriteFrame(FramesToSkip, State->GetBase(), delta);
        FramesToSkip = 0;
      }
      return Builder->GetResult();
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
    const Time::Duration<TimeUnit> FrameDuration;
    const FramedDumpBuilder::Ptr Builder;
    const RenderState::Ptr State;
    uint_t FramesToSkip = 0;
    Stamp NextFrame;
  };

  Dumper::Ptr CreateDumper(const DumperParameters& params, FramedDumpBuilder::Ptr builder)
  {
    RenderState::Ptr state;
    switch (params.OptimizationLevel())
    {
    case DumperParameters::NONE:
      state = MakePtr<NotOptimizedRenderState>();
      break;
    default:
      state = MakePtr<OptimizedRenderState>();
    }
    return MakePtr<FrameDumper>(params.FrameDuration(), std::move(builder), std::move(state));
  }
}  // namespace Devices::AYM
