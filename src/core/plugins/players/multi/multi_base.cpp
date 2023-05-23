/**
 *
 * @file
 *
 * @brief  multidevice-based chiptunes support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/players/multi/multi_base.h"
// common includes
#include <contract.h>
#include <make_ptr.h>
// library includes
#include <parameters/merged_accessor.h>
#include <parameters/visitor.h>
// std includes
#include <algorithm>

namespace Module
{
  using RenderersArray = std::vector<Renderer::Ptr>;

  class WideSample
  {
  public:
    WideSample() = default;

    explicit WideSample(const Sound::Sample& rh)
      : Left(rh.Left())
      , Right(rh.Right())
    {}

    void Add(const Sound::Sample& rh)
    {
      Left += rh.Left();
      Right += rh.Right();
    }

    Sound::Sample Convert(int_t divisor) const
    {
      static_assert(Sound::Sample::MID == 0, "Sound samples should be signed");
      return Sound::Sample(Left / divisor, Right / divisor);
    }

  private:
    Sound::Sample::WideType Left = 0;
    Sound::Sample::WideType Right = 0;
  };

  class CumulativeChunk
  {
  public:
    explicit CumulativeChunk(uint_t streams)
    {
      Buffer.reserve(65536);
      Portions.resize(streams);
    }

    bool NeedStream(uint_t idx) const
    {
      return !Portions[idx];
    }

    void Reset()
    {
      std::fill(Portions.begin(), Portions.end(), 0);
      DoneStreams = 0;
    }

    void MixStream(uint_t idx, const Sound::Chunk& data)
    {
      auto offset = Portions[idx];
      if (data.empty())
      {
        // Empty data means preliminary stream end
        offset = Buffer.size();
      }
      else
      {
        Buffer.resize(std::max(Buffer.size(), offset + data.size()));
        for (const auto& smp : data)
        {
          Buffer[offset++].Add(smp);
        }
        ++DoneStreams;
      }
      Portions[idx] = offset;
    }

    Sound::Chunk Convert()
    {
      if (const auto avail = *std::min_element(Portions.begin(), Portions.end()))
      {
        Sound::Chunk result(avail);
        if (const uint_t divisor = DoneStreams)
        {
          std::transform(Buffer.begin(), Buffer.begin() + avail, result.begin(),
                         [divisor](WideSample in) { return in.Convert(divisor); });
        }
        Consume(avail);
        DoneStreams = 0;
        return result;
      }
      return {};
    }

  private:
    void Consume(std::size_t size)
    {
      for (auto& done : Portions)
      {
        done -= size;
      }
      if (size == Buffer.size())
      {
        Buffer.clear();
      }
      else
      {
        std::copy(Buffer.begin() + size, Buffer.end(), Buffer.begin());
        Buffer.resize(Buffer.size() - size);
      }
    }

  private:
    std::vector<WideSample> Buffer;
    std::vector<std::size_t> Portions;
    uint_t DoneStreams = 0;
  };

  class MultiRenderer : public Renderer
  {
  public:
    explicit MultiRenderer(RenderersArray delegates)
      : Delegates(std::move(delegates))
      , Target(Delegates.size())
    {}

    State::Ptr GetState() const override
    {
      return Delegates.front()->GetState();
    }

    Sound::Chunk Render() override
    {
      for (std::size_t idx = 0, lim = Delegates.size(); idx != lim; ++idx)
      {
        if (Target.NeedStream(idx))
        {
          auto data = Delegates[idx]->Render();
          Target.MixStream(idx, data);
        }
      }
      return Target.Convert();
    }

    void Reset() override
    {
      for (const auto& renderer : Delegates)
      {
        renderer->Reset();
      }
      Target.Reset();
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      for (const auto& renderer : Delegates)
      {
        renderer->SetPosition(request);
      }
      Target.Reset();
    }

    static Ptr Create(uint_t samplerate, const Parameters::Accessor::Ptr& params, const Multi::HoldersArray& holders)
    {
      const auto count = holders.size();
      Require(count > 1);
      RenderersArray delegates(count);
      for (std::size_t idx = 0; idx != count; ++idx)
      {
        const auto& holder = holders[idx];
        delegates[idx] =
            holder->CreateRenderer(samplerate, Parameters::CreateMergedAccessor(holder->GetModuleProperties(), params));
      }
      return MakePtr<MultiRenderer>(std::move(delegates));
    }

  private:
    const RenderersArray Delegates;
    CumulativeChunk Target;
  };

  class MultiHolder : public Holder
  {
  public:
    MultiHolder(Parameters::Accessor::Ptr props, Multi::HoldersArray delegates)
      : Properties(std::move(props))
      , Delegates(std::move(delegates))
    {}

    Information::Ptr GetModuleInformation() const override
    {
      return Delegates.front()->GetModuleInformation();
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr params) const override
    {
      return MultiRenderer::Create(samplerate, params, Delegates);
    }

  private:
    const Parameters::Accessor::Ptr Properties;
    const Multi::HoldersArray Delegates;
    const Information::Ptr Info;
  };
}  // namespace Module

namespace Module::Multi
{
  Module::Holder::Ptr CreateHolder(Parameters::Accessor::Ptr tuneProperties, HoldersArray holders)
  {
    Require(!holders.empty());
    return holders.size() == 1 ? std::move(holders.front())
                               : MakePtr<MultiHolder>(std::move(tuneProperties), std::move(holders));
  }
}  // namespace Module::Multi
