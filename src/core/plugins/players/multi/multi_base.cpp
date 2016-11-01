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
#include "multi_base.h"
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
#include <functional>
//boost includes
#include <boost/bind.hpp>

namespace Module
{
  typedef std::vector<Analyzer::Ptr> AnalyzersArray;
  typedef std::vector<TrackState::Ptr> TrackStatesArray;
  typedef std::vector<Renderer::Ptr> RenderersArray;
  
  class MultiAnalyzer : public Module::Analyzer
  {
  public:
    explicit MultiAnalyzer(AnalyzersArray delegates)
      : Delegates(std::move(delegates))
      , MaxBands(4 * Delegates.size())//approx
    {
    }

    std::vector<ChannelState> GetState() const override
    {
      std::vector<ChannelState> result;
      result.reserve(MaxBands);
      for (const auto& delegate : Delegates)
      {
        const auto& portion = delegate->GetState();
        std::copy(portion.begin(), portion.end(), std::back_inserter(result));
      }
      MaxBands = std::max(MaxBands, result.size());
      return std::move(result);
    }
    
    static Ptr Create(const RenderersArray& renderers)
    {
      const auto count = renderers.size();
      Require(count > 1);
      AnalyzersArray delegates(count);
      std::transform(renderers.begin(), renderers.end(), delegates.begin(), std::mem_fn(&Renderer::GetAnalyzer));
      return MakePtr<MultiAnalyzer>(std::move(delegates));
    }
  private:
    const AnalyzersArray Delegates;
    mutable std::size_t MaxBands;
  };
 
  class MultiInformation : public Information
  {
  public:
    MultiInformation(Information::Ptr delegate, uint_t totalChannelsCount)
      : Delegate(std::move(delegate))
      , TotalChannelsCount(totalChannelsCount)
    {
    }
    
    uint_t PositionsCount() const override
    {
      return Delegate->PositionsCount();
    }
    uint_t LoopPosition() const override
    {
      return Delegate->LoopPosition();
    }
    uint_t PatternsCount() const override
    {
      return Delegate->PatternsCount();
    }
    uint_t FramesCount() const override
    {
      return Delegate->FramesCount();
    }
    uint_t LoopFrame() const override
    {
      return Delegate->LoopFrame();
    }
    uint_t ChannelsCount() const override
    {
      return TotalChannelsCount;
    }
    uint_t Tempo() const override
    {
      return Delegate->Tempo();
    }
    
    static Ptr Create(const Multi::HoldersArray& holders)
    {
      const auto count = holders.size();
      Require(count > 1);
      uint_t totalChannelsCount = 0;
      for (const auto& holder : holders)
      {
        totalChannelsCount += holder->GetModuleInformation()->ChannelsCount();
      }
      return MakePtr<MultiInformation>(holders.front()->GetModuleInformation(), totalChannelsCount);
    }
  private:
    const Information::Ptr Delegate;
    const uint_t TotalChannelsCount;
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
    
    void ApplyData(Sound::Chunk::Ptr data) override
    {
      if (DoneStreams++)
      {
        Buffer.Mix(*data);
      }
      else
      {
        Buffer.Fill(*data);
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
      
      Sound::Chunk::Ptr Convert(uint_t sources) const
      {
        auto result = MakePtr<Sound::Chunk>(Buffer.size());
        std::transform(Buffer.begin(), Buffer.end(), result->begin(), std::bind2nd(std::mem_fun_ref(&WideSample::Convert), sources));
        return std::move(result);
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
  
  class ForcedLoopParam : public Parameters::Accessor
  {
  public:
    uint_t Version() const override
    {
      return 1;
    }

    bool FindValue(const Parameters::NameType& name, Parameters::IntType& val) const override
    {
      if (name == Parameters::ZXTune::Sound::LOOPED)
      {
        val = 1;
        return true;
      }
      else
      {
        return false;
      }
    }

    bool FindValue(const Parameters::NameType& /*name*/, Parameters::StringType& /*val*/) const override
    {
      return false;
    }

    bool FindValue(const Parameters::NameType& /*name*/, Parameters::DataType& /*val*/) const override
    {
      return false;
    }

    void Process(class Parameters::Visitor& visitor) const override
    {
      visitor.SetValue(Parameters::ZXTune::Sound::LOOPED, 1);
    }
  };
  
  class MultiTrackState : public TrackState
  {
  public:
    explicit MultiTrackState(TrackStatesArray delegates)
      : Delegates(std::move(delegates))
    {
    }
    
    uint_t Position() const override
    {
      return Delegates.front()->Position();
    }
    uint_t Pattern() const override
    {
      return Delegates.front()->Pattern();
    }
    uint_t PatternSize() const override
    {
      return Delegates.front()->PatternSize();
    }
    uint_t Line() const override
    {
      return Delegates.front()->Line();
    }
    uint_t Tempo() const override
    {
      return Delegates.front()->Tempo();
    }
    uint_t Quirk() const override
    {
      return Delegates.front()->Quirk();
    }
    uint_t Frame() const override
    {
      return Delegates.front()->Frame();
    }
    uint_t Channels() const override
    {
      uint_t res = 0;
      for (const auto& delegate : Delegates)
      {
        res += delegate->Channels();
      }
      return res;
    }
    
    static Ptr Create(const RenderersArray& renderers)
    {
      const auto count = renderers.size();
      Require(count > 1);
      TrackStatesArray delegates(count);
      std::transform(renderers.begin(), renderers.end(), delegates.begin(), std::mem_fn(&Renderer::GetTrackState));
      return MakePtr<MultiTrackState>(std::move(delegates));
    }
  private:
    const TrackStatesArray Delegates;
  };
  
  class MultiRenderer : public Renderer
  {
  public:
    MultiRenderer(RenderersArray delegates, Sound::RenderParameters::Ptr renderParams, CompositeReceiver::Ptr target)
      : Delegates(std::move(delegates))
      , SoundParams(renderParams)
      , Target(std::move(target))
      , State(MultiTrackState::Create(Delegates))
      , Analysis(MultiAnalyzer::Create(Delegates))
    {
      ApplyParameters();
    }

    TrackState::Ptr GetTrackState() const override
    {
      return State;
    }

    Analyzer::Ptr GetAnalyzer() const override
    {
      return Analysis;
    }

    bool RenderFrame() override
    {
      ApplyParameters();
      bool result = true;
      for (std::size_t idx = 0, lim = Delegates.size(); idx != lim; ++idx)
      {
        result &= Delegates[idx]->RenderFrame();
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
      std::for_each(Delegates.begin(), Delegates.end(), std::mem_fn(&Renderer::Reset));
    }

    void SetPosition(uint_t frame) override
    {
      std::for_each(Delegates.begin(), Delegates.end(), boost::bind(&Renderer::SetPosition, _1, frame));
    }
    
    static Ptr Create(Parameters::Accessor::Ptr params, const Multi::HoldersArray& holders, Sound::Receiver::Ptr target)
    {
      const auto count = holders.size();
      Require(count > 1);
      const CompositeReceiver::Ptr receiver = MakePtr<CompositeReceiver>(target);
      const Parameters::Accessor::Ptr forcedLoop = MakePtr<ForcedLoopParam>();
      RenderersArray delegates(count);
      for (std::size_t idx = 0; idx != count; ++idx)
      {
        const auto& holder = holders[idx];
        auto delegateParams = idx == 0
          ? params
          : Parameters::CreateMergedAccessor(forcedLoop, params);
        delegates[idx] = holder->CreateRenderer(std::move(delegateParams), receiver);
      }
      const Sound::RenderParameters::Ptr renderParams = Sound::RenderParameters::Create(params);
      return MakePtr<MultiRenderer>(std::move(delegates), renderParams, receiver);
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
    const TrackState::Ptr State;
    const Analyzer::Ptr Analysis;
  };
  
  class MultiHolder : public Holder
  {
  public:
    MultiHolder(Parameters::Accessor::Ptr props, Multi::HoldersArray delegates)
      : Properties(std::move(props))
      , Delegates(std::move(delegates))
      , Info(MultiInformation::Create(Delegates))
    {
    }

    Information::Ptr GetModuleInformation() const override
    {
      return Info;
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
