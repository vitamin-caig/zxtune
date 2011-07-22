/*
Abstract:
  TurboSound functionality helpers implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ay_base.h"
//common includes
#include <error.h>
#include <iterator.h>
//local includes
#include "ts_base.h"
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  class TSTrackState : public TrackState
  {
  public:
    TSTrackState(TrackState::Ptr first, TrackState::Ptr second)
      : First(first), Second(second)
    {
    }

    virtual uint_t Position() const
    {
      return First->Position();
    }

    virtual uint_t Pattern() const
    {
      return First->Pattern();
    }

    virtual uint_t PatternSize() const
    {
      return First->PatternSize();
    }

    virtual uint_t Line() const
    {
      return First->Line();
    }

    virtual uint_t Tempo() const
    {
      return First->Tempo();
    }

    virtual uint_t Quirk() const
    {
      return First->Quirk();
    }

    virtual uint_t Frame() const
    {
      return First->Frame();
    }

    virtual uint_t Channels() const
    {
      return First->Channels() + Second->Channels();
    }
  private:
    const TrackState::Ptr First;
    const TrackState::Ptr Second;
  };

  class TSAnalyzer : public Analyzer
  {
  public:
    TSAnalyzer(Analyzer::Ptr first, Analyzer::Ptr second)
      : First(first), Second(second)
    {
    }

    virtual uint_t ActiveChannels() const
    {
      return First->ActiveChannels() + Second->ActiveChannels();
    }

    virtual void BandLevels(std::vector<std::pair<uint_t, uint_t> >& bandLevels) const
    {
      std::vector<std::pair<uint_t, uint_t> > firstLevels, secondLevels;
      First->BandLevels(firstLevels);
      Second->BandLevels(secondLevels);
      bandLevels.resize(firstLevels.size() + secondLevels.size());
      std::copy(secondLevels.begin(), secondLevels.end(),
        std::copy(firstLevels.begin(), firstLevels.end(), bandLevels.begin()));
    }
  private:
    const Analyzer::Ptr First;
    const Analyzer::Ptr Second;
  };

  template<class T>
  inline T avg(T val1, T val2)
  {
    return (val1 + val2) / 2;
  }

  template<class MultiSample>
  class DoubleReceiverImpl : public DoubleReceiver<MultiSample>
  {
  public:
    explicit DoubleReceiverImpl(typename DataReceiver<MultiSample>::Ptr delegate)
      : Delegate(delegate)
      , Stream()
    {
    }

    virtual void ApplyData(const MultiSample& data)
    {
      if (Stream)
      {
        Mix(data);
      }
      else
      {
        Store(data);
      }
    }

    virtual void Flush()
    {
      if (Stream)
      {
        Delegate->Flush();
      }
    }

    virtual void SetStream(uint_t idx)
    {
      if ((Stream = idx))
      {
        MixCursor = CycledIterator<typename BufferType::iterator>(Buffer.begin(), Buffer.end());
      }
      else
      {
        Buffer.clear();
      }
    }
  private:
    void Store(const MultiSample& data)
    {
      Buffer.push_back(data);
    }

    void Mix(const MultiSample& data)
    {
      assert(MixCursor->size() == data.size());
      const typename MultiSample::iterator bufIter = MixCursor->begin();
      std::transform(data.begin(), data.end(), bufIter, bufIter, &avg<typename MultiSample::value_type>);
      Delegate->ApplyData(*MixCursor);
      ++MixCursor;
    }
  private:
    const typename DataReceiver<MultiSample>::Ptr Delegate;
    uint_t Stream;
    typedef std::vector<MultiSample> BufferType;
    BufferType Buffer;
    CycledIterator<typename BufferType::iterator> MixCursor;
  };

  template<class Type>
  class TSRenderer : public Renderer
  {
  public:
    TSRenderer(Renderer::Ptr first, Renderer::Ptr second, typename DoubleReceiver<Type>::Ptr mixer)
      : Mixer(mixer)
      , Renderer1(first)
      , Renderer2(second)
    {
    }

    virtual TrackState::Ptr GetTrackState() const
    {
      return CreateTSTrackState(Renderer1->GetTrackState(), Renderer2->GetTrackState());
    }

    virtual Analyzer::Ptr GetAnalyzer() const
    {
      return CreateTSAnalyzer(Renderer1->GetAnalyzer(), Renderer2->GetAnalyzer());
    }

    virtual bool RenderFrame()
    {
      Mixer->SetStream(0);
      const bool res1 = Renderer1->RenderFrame();
      Mixer->SetStream(1);
      const bool res2 = Renderer2->RenderFrame();
      return res1 && res2;
    }

    virtual void Reset()
    {
      Renderer1->Reset();
      Renderer2->Reset();
    }

    virtual void SetPosition(uint_t frame)
    {
      Renderer1->SetPosition(frame);
      Renderer2->SetPosition(frame);
    }
  private:
    const typename DoubleReceiver<Type>::Ptr Mixer;
    const Renderer::Ptr Renderer1;
    const Renderer::Ptr Renderer2;
  };
}

namespace ZXTune
{
  namespace Module
  {
    TrackState::Ptr CreateTSTrackState(TrackState::Ptr first, TrackState::Ptr second)
    {
      return boost::make_shared<TSTrackState>(first, second);
    }

    Analyzer::Ptr CreateTSAnalyzer(Analyzer::Ptr first, Analyzer::Ptr second)
    {
      return boost::make_shared<TSAnalyzer>(first, second);
    }

    TSMixer::Ptr CreateTSMixer(Sound::MultichannelReceiver::Ptr delegate)
    {
      typedef DoubleReceiverImpl<std::vector<Sound::Sample> > Impl;
      return boost::make_shared<Impl>(delegate);
    }

    AYMTSMixer::Ptr CreateTSMixer(Devices::AYM::Receiver::Ptr delegate)
    {
      typedef DoubleReceiverImpl<Devices::AYM::MultiSample> Impl;
      return boost::make_shared<Impl>(delegate);
    }

    Renderer::Ptr CreateTSRenderer(Parameters::Accessor::Ptr params, Holder::Ptr first, Holder::Ptr second, Sound::MultichannelReceiver::Ptr target)
    {
      const TSMixer::Ptr mixer = CreateTSMixer(target);
      typedef TSRenderer<std::vector<Sound::Sample> > Impl;
      return boost::make_shared<Impl>(first->CreateRenderer(params, mixer), second->CreateRenderer(params, mixer), mixer);
    }

    Renderer::Ptr CreateTSRenderer(Renderer::Ptr first, Renderer::Ptr second, AYMTSMixer::Ptr mixer)
    {
      typedef TSRenderer<Devices::AYM::MultiSample> Impl;
      return boost::make_shared<Impl>(first, second, mixer);
    }
  }
}
