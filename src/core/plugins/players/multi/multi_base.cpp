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
//library includes
#include <parameters/merged_accessor.h>
#include <parameters/visitor.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
//std includes
#include <algorithm>
//boost includes
#include <boost/bind.hpp>
#include <boost/mem_fn.hpp>
#include <boost/make_shared.hpp>

namespace Module
{
  typedef std::vector<Analyzer::Ptr> AnalyzersArray;
  typedef std::vector<TrackState::Ptr> TrackStatesArray;
  typedef std::vector<Renderer::Ptr> RenderersArray;
  
  class MultiAnalyzer : public Module::Analyzer
  {
  public:
    MultiAnalyzer(AnalyzersArray::const_iterator begin, AnalyzersArray::const_iterator end)
      : Delegates(begin, end)
      , MaxBands(4 * Delegates.size())//approx
    {
    }

    virtual void GetState(std::vector<ChannelState>& channels) const
    {
      std::vector<ChannelState> result;
      result.reserve(MaxBands);
      for (AnalyzersArray::const_iterator it = Delegates.begin(), lim = Delegates.end(); it != lim; ++it)
      {
        std::vector<ChannelState> portion;
        (*it)->GetState(portion);
        std::copy(portion.begin(), portion.end(), std::back_inserter(result));
      }
      MaxBands = std::max(MaxBands, result.size());
      channels.swap(result);
    }
    
    static Ptr Create(const RenderersArray& renderers)
    {
      Require(!renderers.empty());
      AnalyzersArray delegates(renderers.size());
      std::transform(renderers.begin(), renderers.end(), delegates.begin(), boost::mem_fn(&Renderer::GetAnalyzer));
      return delegates.size() == 1
           ? delegates.front()
           : boost::make_shared<MultiAnalyzer>(delegates.begin(), delegates.end());
    }
  private:
    const AnalyzersArray Delegates;
    mutable std::size_t MaxBands;
  };
 
  class MultiInformation : public Information
  {
  public:
    MultiInformation(Information::Ptr delegate, uint_t totalChannelsCount)
      : Delegate(delegate)
      , TotalChannelsCount(totalChannelsCount)
    {
    }
    
    virtual uint_t PositionsCount() const
    {
      return Delegate->PositionsCount();
    }
    virtual uint_t LoopPosition() const
    {
      return Delegate->LoopPosition();
    }
    virtual uint_t PatternsCount() const
    {
      return Delegate->PatternsCount();
    }
    virtual uint_t FramesCount() const
    {
      return Delegate->FramesCount();
    }
    virtual uint_t LoopFrame() const
    {
      return Delegate->LoopFrame();
    }
    virtual uint_t ChannelsCount() const
    {
      return TotalChannelsCount;
    }
    virtual uint_t Tempo() const
    {
      return Delegate->Tempo();
    }
    
    static Ptr Create(const Multi::HoldersArray& holders)
    {
      Require(!holders.empty());
      if (holders.size() == 1)
      {
        return holders.front()->GetModuleInformation();
      }
      else
      {
        uint_t totalChannelsCount = 0;
        for (Multi::HoldersArray::const_iterator it = holders.begin(), lim = holders.end(); it != lim; ++it)
        {
          totalChannelsCount += (*it)->GetModuleInformation()->ChannelsCount();
        }
        return boost::make_shared<MultiInformation>(holders.front()->GetModuleInformation(), totalChannelsCount);
      }
    }
  private:
    const Information::Ptr Delegate;
    const uint_t TotalChannelsCount;
  };

  class CompositeReceiver : public Sound::Receiver
  {
  public:
    typedef boost::shared_ptr<CompositeReceiver> Ptr;
    
    CompositeReceiver(Sound::Receiver::Ptr delegate, uint_t bufferSize)
      : Delegate(delegate)
      , Buffer(bufferSize)
      , DoneStreams()
    {
    }
    
    virtual void ApplyData(const Sound::Chunk::Ptr& data)
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
    
    virtual void Flush()
    {
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
        BOOST_STATIC_ASSERT(Sound::Sample::MID == 0);
        return Sound::Sample(Left / divisor, Right / divisor);
      }
    private:
      Sound::Sample::WideType Left;
      Sound::Sample::WideType Right;
    };
    
    class CumulativeChunk
    {
    public:
      explicit CumulativeChunk(uint_t size)
        : Buffer(size)
      {
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
        const Sound::Chunk::Ptr result = boost::make_shared<Sound::Chunk>(Buffer.size());
        std::transform(Buffer.begin(), Buffer.end(), result->begin(), std::bind2nd(std::mem_fun_ref(&WideSample::Convert), sources));
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
  
  class ForcedLoopParam : public Parameters::Accessor
  {
  public:
    virtual uint_t Version() const
    {
      return 1;
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::IntType& val) const
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

    virtual bool FindValue(const Parameters::NameType& /*name*/, Parameters::StringType& /*val*/) const
    {
      return false;
    }

    virtual bool FindValue(const Parameters::NameType& /*name*/, Parameters::DataType& /*val*/) const
    {
      return false;
    }

    virtual void Process(class Parameters::Visitor& visitor) const
    {
      visitor.SetValue(Parameters::ZXTune::Sound::LOOPED, 1);
    }
  };
  
  class MultiTrackState : public TrackState
  {
  public:
    MultiTrackState(TrackStatesArray::const_iterator begin, TrackStatesArray::const_iterator end)
      : Delegates(begin, end)
    {
    }
    
    virtual uint_t Position() const
    {
      return Delegates.front()->Position();
    }
    virtual uint_t Pattern() const
    {
      return Delegates.front()->Pattern();
    }
    virtual uint_t PatternSize() const
    {
      return Delegates.front()->PatternSize();
    }
    virtual uint_t Line() const
    {
      return Delegates.front()->Line();
    }
    virtual uint_t Tempo() const
    {
      return Delegates.front()->Tempo();
    }
    virtual uint_t Quirk() const
    {
      return Delegates.front()->Quirk();
    }
    virtual uint_t Frame() const
    {
      return Delegates.front()->Frame();
    }
    virtual uint_t Channels() const
    {
      uint_t res = 0;
      for (TrackStatesArray::const_iterator it = Delegates.begin(), lim = Delegates.end(); it != lim; ++it)
      {
        res += (*it)->Channels();
      }
      return res;
    }
    
    static Ptr Create(const RenderersArray& renderers)
    {
      Require(!renderers.empty());
      TrackStatesArray delegates(renderers.size());
      std::transform(renderers.begin(), renderers.end(), delegates.begin(), boost::mem_fn(&Renderer::GetTrackState));
      return delegates.size() == 1
           ? delegates.front()
           : boost::make_shared<MultiTrackState>(delegates.begin(), delegates.end());
    }
  private:
    const TrackStatesArray Delegates;
  };
  
  class MultiRenderer : public Renderer
  {
  public:
    MultiRenderer(RenderersArray::const_iterator begin, RenderersArray::const_iterator end, CompositeReceiver::Ptr target)
      : Delegates(begin, end)
      , Target(target)
      , State(MultiTrackState::Create(Delegates))
      , Analysis(MultiAnalyzer::Create(Delegates))
    {
    }

    virtual TrackState::Ptr GetTrackState() const
    {
      return State;
    }

    virtual Analyzer::Ptr GetAnalyzer() const
    {
      return Analysis;
    }

    virtual bool RenderFrame()
    {
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

    virtual void Reset()
    {
      std::for_each(Delegates.begin(), Delegates.end(), boost::mem_fn(&Renderer::Reset));
    }

    virtual void SetPosition(uint_t frame)
    {
      std::for_each(Delegates.begin(), Delegates.end(), boost::bind(&Renderer::SetPosition, _1, frame));
    }
    
    static Ptr Create(Parameters::Accessor::Ptr params, const Multi::HoldersArray& holders, Sound::Receiver::Ptr target)
    {
      Require(!holders.empty());
      if (holders.size() == 1)
      {
        return holders.front()->CreateRenderer(params, target);
      }
      else
      {
        const Sound::RenderParameters::Ptr renderParams = Sound::RenderParameters::Create(params);
        const std::size_t size = holders.size();
        const std::size_t bufSize = renderParams->SamplesPerFrame();
        const CompositeReceiver::Ptr receiver = boost::make_shared<CompositeReceiver>(target, bufSize);
        const Parameters::Accessor::Ptr forcedLoop = boost::make_shared<ForcedLoopParam>();
        RenderersArray delegates(size);
        for (std::size_t idx = 0; idx != size; ++idx)
        {
          const Holder::Ptr holder = holders[idx];
          const Parameters::Accessor::Ptr delegateParams = idx == 0
            ? params
            : Parameters::CreateMergedAccessor(forcedLoop, params);
          delegates[idx] = holder->CreateRenderer(delegateParams, receiver);
        }
        return boost::make_shared<MultiRenderer>(delegates.begin(), delegates.end(), receiver);
      }
    }
  private:
    const RenderersArray Delegates;
    const CompositeReceiver::Ptr Target;
    const TrackState::Ptr State;
    const Analyzer::Ptr Analysis;
  };
  
  class MultiHolder : public Holder
  {
  public:
    MultiHolder(Parameters::Accessor::Ptr props, Multi::HoldersArray::const_iterator begin, Multi::HoldersArray::const_iterator end)
      : Properties(props)
      , Delegates(begin, end)
      , Info(MultiInformation::Create(Delegates))
    {
    }

    virtual Information::Ptr GetModuleInformation() const
    {
      return Info;
    }

    virtual Parameters::Accessor::Ptr GetModuleProperties() const
    {
      return Properties;
    }

    virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const
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
           : boost::make_shared<MultiHolder>(params, holders.begin(), holders.end());
    }
  }
}
