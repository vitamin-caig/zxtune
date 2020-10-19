/**
* 
* @file
*
* @brief  multidevice-based chiptunes support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/players/multi/multi_base.h"
//common includes
#include <contract.h>
#include <make_ptr.h>
//library includes
#include <parameters/merged_accessor.h>
#include <parameters/visitor.h>
#include <sound/loop.h>
//std includes
#include <algorithm>

namespace Module
{
  typedef std::vector<Analyzer::Ptr> AnalyzersArray;
  typedef std::vector<Renderer::Ptr> RenderersArray;
  
  class MultiAnalyzer : public Module::Analyzer
  {
  public:
    explicit MultiAnalyzer(AnalyzersArray delegates)
      : Delegates(std::move(delegates))
    {
    }

    SpectrumState GetState() const override
    {
      SpectrumState result;
      for (const auto& delegate : Delegates)
      {
        const auto& portion = delegate->GetState();
        std::transform(portion.Data.begin(), portion.Data.end(), result.Data.begin(), result.Data.begin(), &SpectrumState::AccumulateLevel);
      }
      return result;
    }
    
    static Ptr Create(const RenderersArray& renderers)
    {
      const auto count = renderers.size();
      Require(count > 1);
      AnalyzersArray delegates(count);
      std::transform(renderers.begin(), renderers.end(), delegates.begin(),
          [](const Renderer::Ptr& renderer) {return renderer->GetAnalyzer();});
      return MakePtr<MultiAnalyzer>(std::move(delegates));
    }
  private:
    const AnalyzersArray Delegates;
  };
 
  class CompositeReceiver : public Sound::Receiver
  {
  public:
    typedef std::shared_ptr<CompositeReceiver> Ptr;
    
    CompositeReceiver(Sound::Receiver::Ptr delegate, uint_t streams)
      : Delegate(std::move(delegate))
      , Buffer(streams)
    {
    }
    
    void ApplyData(Sound::Chunk data) override
    {
      Buffer.Mix(data);
    }
    
    void Flush() override
    {
    }

    bool NeedStream(uint_t idx) const
    {
      return Buffer.NeedStream(idx);
    }

    void FinishFrame()
    {
      Delegate->ApplyData(Buffer.Convert());
      Delegate->Flush();
    }
  private:
    class WideSample
    {
    public:
      WideSample()
        : Left()
        , Right()
      {
      }
      
      explicit WideSample(const Sound::Sample& rh)
        : Left(rh.Left())
        , Right(rh.Right())
      {
      }
      
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
      Sound::Sample::WideType Left;
      Sound::Sample::WideType Right;
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

      void Mix(const Sound::Chunk& data)
      {
        auto offset = Portions[DoneStreams];
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
        }
        Portions[DoneStreams] = offset;
        ++DoneStreams;
      }
      
      Sound::Chunk Convert()
      {
        const auto avail = *std::min_element(Portions.begin(), Portions.begin());
        Require(avail);
        const auto divisor = DoneStreams;
        Sound::Chunk result(avail);
        std::transform(Buffer.begin(), Buffer.begin() + avail, result.begin(),
           [divisor](WideSample in) {return in.Convert(divisor);});
        Consume(avail);
        DoneStreams = 0;
        return result;
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
      std::vector<uint_t> Portions;
      uint_t DoneStreams = 0;
    };
  private:
    const Sound::Receiver::Ptr Delegate;
    CumulativeChunk Buffer;
  };
  
  class MultiRenderer : public Renderer
  {
  public:
    MultiRenderer(RenderersArray delegates, CompositeReceiver::Ptr target)
      : Delegates(std::move(delegates))
      , Target(std::move(target))
      , Analysis(MultiAnalyzer::Create(Delegates))
    {
    }

    State::Ptr GetState() const override
    {
      return Delegates.front()->GetState();
    }

    Analyzer::Ptr GetAnalyzer() const override
    {
      return Analysis;
    }

    bool RenderFrame(const Sound::LoopParameters& looped) override
    {
      static const Sound::LoopParameters INFINITE_LOOP{true, 0};
      bool result = true;
      for (std::size_t idx = 0, lim = Delegates.size(); idx != lim; ++idx)
      {
        if (Target->NeedStream(idx))
        {
          result &= Delegates[idx]->RenderFrame(idx == 0 ? looped : INFINITE_LOOP);
        }
      }
      if (result)
      {
        Target->FinishFrame();
      }
      return result;
    }

    void Reset() override
    {
      for (const auto& renderer : Delegates)
      {
        renderer->Reset();
      }
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      for (const auto& renderer : Delegates)
      {
        renderer->SetPosition(request);
      }
    }
    
    static Ptr Create(uint_t samplerate, Parameters::Accessor::Ptr params, const Multi::HoldersArray& holders, Sound::Receiver::Ptr target)
    {
      const auto count = holders.size();
      Require(count > 1);
      const CompositeReceiver::Ptr receiver = MakePtr<CompositeReceiver>(target, count);
      RenderersArray delegates(count);
      for (std::size_t idx = 0; idx != count; ++idx)
      {
        const auto& holder = holders[idx];
        delegates[idx] = holder->CreateRenderer(samplerate, params, receiver);
      }
      return MakePtr<MultiRenderer>(std::move(delegates), std::move(receiver));
    }
  private:
    const RenderersArray Delegates;
    const CompositeReceiver::Ptr Target;
    const Analyzer::Ptr Analysis;
  };
  
  class MultiHolder : public Holder
  {
  public:
    MultiHolder(Parameters::Accessor::Ptr props, Multi::HoldersArray delegates)
      : Properties(std::move(props))
      , Delegates(std::move(delegates))
    {
    }

    Information::Ptr GetModuleInformation() const override
    {
      return Delegates.front()->GetModuleInformation();
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const override
    {
      return MultiRenderer::Create(samplerate, std::move(params), Delegates, std::move(target));
    }
  private:
    const Parameters::Accessor::Ptr Properties;
    const Multi::HoldersArray Delegates;
    const Information::Ptr Info;
  };
}

namespace Module
{
  namespace Multi
  { 
    Module::Holder::Ptr CreateHolder(Parameters::Accessor::Ptr params, const HoldersArray& holders)
    {
      Require(!holders.empty());
      return holders.size() == 1
           ? CreateMixedPropertiesHolder(holders.front(), params)
           : MakePtr<MultiHolder>(params, holders);
    }
  }
}
