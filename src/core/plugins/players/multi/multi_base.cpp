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
#include <parameters/tracking_helper.h>
#include <parameters/visitor.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
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
    
    explicit CompositeReceiver(Sound::Receiver::Ptr delegate)
      : Delegate(std::move(delegate))
      , DoneStreams()
    {
    }
    
    void ApplyData(Sound::Chunk data) override
    {
      if (DoneStreams++)
      {
        Buffer.Mix(data);
      }
      else
      {
        Buffer.Fill(data);
      }
    }
    
    void Flush() override
    {
    }

    void SetBufferSize(uint_t size)
    {
      Buffer.Resize(size);
    }
    
    void FinishFrame()
    {
      Delegate->ApplyData(Buffer.Convert(DoneStreams));
      Delegate->Flush();
      DoneStreams = 0;
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
      void Resize(uint_t size)
      {
        Buffer.resize(size);
      }
      
      void Fill(const Sound::Chunk& data)
      {
        std::size_t idx = 0;
        for (const std::size_t size = GetDataSize(data); idx != size; ++idx)
        {
          Buffer[idx] = WideSample(data[idx]);
        }
        for (const std::size_t limit = Buffer.size(); idx != limit; ++idx)
        {
          Buffer[idx] = WideSample();
        }
      }
      
      void Mix(const Sound::Chunk& data)
      {
        for (std::size_t idx = 0, size = GetDataSize(data); idx != size; ++idx)
        {
          Buffer[idx].Add(data[idx]);
        }
      }
      
      Sound::Chunk Convert(uint_t sources) const
      {
        Sound::Chunk result(Buffer.size());
        std::transform(Buffer.begin(), Buffer.end(), result.begin(),
           [sources](WideSample in) {return in.Convert(sources);});
        return result;
      }
    private:
      std::size_t GetDataSize(const Sound::Chunk& data) const
      {
        const std::size_t bufSize = Buffer.size();
        const std::size_t dataSize = data.size();
        /*
        if (bufSize != dataSize)
        {
          Dbg("Sound data size mismatch buffer=%u data=%u", bufSize, dataSize);
        }
        */
        return std::min(bufSize, dataSize);
      }
    private:
      std::vector<WideSample> Buffer;
    };
  private:
    const Sound::Receiver::Ptr Delegate;
    CumulativeChunk Buffer;
    uint_t DoneStreams;
  };
  
  class MultiRenderer : public Renderer
  {
  public:
    MultiRenderer(RenderersArray delegates, Sound::RenderParameters::Ptr renderParams, CompositeReceiver::Ptr target)
      : Delegates(std::move(delegates))
      , SoundParams(std::move(renderParams))
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
      ApplyParameters();
      bool result = true;
      for (std::size_t idx = 0, lim = Delegates.size(); idx != lim; ++idx)
      {
        result &= Delegates[idx]->RenderFrame(idx == 0 ? looped : INFINITE_LOOP);
      }
      if (result)
      {
        Target->FinishFrame();
      }
      return result;
    }

    void Reset() override
    {
      SoundParams.Reset();
      for (const auto& renderer : Delegates)
      {
        renderer->Reset();
      }
    }

    void SetPosition(uint_t frame) override
    {
      for (const auto& renderer : Delegates)
      {
        renderer->SetPosition(frame);
      }
    }
    
    static Ptr Create(Parameters::Accessor::Ptr params, const Multi::HoldersArray& holders, Sound::Receiver::Ptr target)
    {
      const auto count = holders.size();
      Require(count > 1);
      const CompositeReceiver::Ptr receiver = MakePtr<CompositeReceiver>(target);
      RenderersArray delegates(count);
      for (std::size_t idx = 0; idx != count; ++idx)
      {
        const auto& holder = holders[idx];
        delegates[idx] = holder->CreateRenderer(params, receiver);
      }
      auto renderParams = Sound::RenderParameters::Create(std::move(params));
      return MakePtr<MultiRenderer>(std::move(delegates), std::move(renderParams), receiver);
    }
  private:
    void ApplyParameters()
    {
      if (SoundParams.IsChanged())
      {
        const uint_t bufSize = SoundParams->SamplesPerFrame();
        Target->SetBufferSize(bufSize);
      }
    }
  private:
    const RenderersArray Delegates;
    Parameters::TrackingHelper<Sound::RenderParameters> SoundParams;
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

    Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const override
    {
      return MultiRenderer::Create(params, Delegates, target);
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
