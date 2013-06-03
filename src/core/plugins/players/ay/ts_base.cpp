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
#include "ts_base.h"
//common includes
#include <error.h>
#include <iterator.h>
//library includes
#include <sound/mixer_factory.h>
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

    virtual void GetState(std::vector<Analyzer::ChannelState>& channels) const
    {
      std::vector<Analyzer::ChannelState> firstLevels, secondLevels;
      First->GetState(firstLevels);
      Second->GetState(secondLevels);
      channels.resize(firstLevels.size() + secondLevels.size());
      std::copy(secondLevels.begin(), secondLevels.end(),
        std::copy(firstLevels.begin(), firstLevels.end(), channels.begin()));
    }
  private:
    const Analyzer::Ptr First;
    const Analyzer::Ptr Second;
  };

  template<class T>
  inline T Avg(T val1, T val2)
  {
    return (val1 + val2) / 2;
  }

  template<class T, std::size_t N>
  boost::array<T, N> Avg(const boost::array<T, N>& val1, const boost::array<T, N>& val2)
  {
    boost::array<T, N> res;
    for (uint_t chan = 0; chan != N; ++chan)
    {
      res[chan] = Avg(val1[chan], val2[chan]);
    }
    return res;
  }

  template<class SampleType>
  class DoubleMixer
  {
  public:
    typedef boost::shared_ptr<DoubleMixer> Ptr;

    DoubleMixer()
      : InCursor()
      , OutCursor()
    {
    }

    void Store(const SampleType& in)
    {
      Buffer[InCursor++] = in;
    }

    SampleType Mix(const SampleType& in)
    {
      return Avg(static_cast<const SampleType&>(Buffer[OutCursor++]), in);
    }
  private:
    boost::array<SampleType, 65536> Buffer;
    uint16_t InCursor;
    uint16_t OutCursor;
  };

  template<class SampleType>
  class FirstReceiver : public DataReceiver<SampleType>
  {
  public:
    explicit FirstReceiver(typename DoubleMixer<SampleType>::Ptr mixer)
      : Mixer(mixer)
    {
    }

    virtual void ApplyData(const SampleType& data)
    {
      Mixer->Store(data);
    }

    virtual void Flush()
    {
    }
  private:
    const typename DoubleMixer<SampleType>::Ptr Mixer;
  };

  template<class SampleType>
  class SecondReceiver : public DataReceiver<SampleType>
  {
  public:
    SecondReceiver(typename DoubleMixer<SampleType>::Ptr mixer, typename DataReceiver<SampleType>::Ptr delegate)
      : Mixer(mixer)
      , Delegate(delegate)
    {
    }

    virtual void ApplyData(const SampleType& data)
    {
      Delegate->ApplyData(Mixer->Mix(data));
    }

    virtual void Flush()
    {
      Delegate->Flush();
    }
  private:
    const typename DoubleMixer<SampleType>::Ptr Mixer;
    const typename DataReceiver<SampleType>::Ptr Delegate;
  };

  template<class SampleType>
  boost::array<typename DataReceiver<SampleType>::Ptr, 2> CreateTSMixer(typename DataReceiver<SampleType>::Ptr target)
  {
    const typename DoubleMixer<SampleType>::Ptr mixer = boost::make_shared<DoubleMixer<SampleType> >();
    const boost::array<typename DataReceiver<SampleType>::Ptr, 2> res =
    {
      {
        boost::make_shared<FirstReceiver<SampleType> >(mixer),
        boost::make_shared<SecondReceiver<SampleType> >(mixer, target)
      }
    };
    return res;
  }

  class TSRenderer : public Renderer
  {
  public:
    TSRenderer(Renderer::Ptr first, Renderer::Ptr second, TrackState::Ptr state)
      : Renderer1(first)
      , Renderer2(second)
      , State(state)
    {
    }

    virtual TrackState::Ptr GetTrackState() const
    {
      return State;
    }

    virtual Analyzer::Ptr GetAnalyzer() const
    {
      return boost::make_shared<TSAnalyzer>(Renderer1->GetAnalyzer(), Renderer2->GetAnalyzer());
    }

    virtual bool RenderFrame()
    {
      const bool res1 = Renderer1->RenderFrame();
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
    const Renderer::Ptr Renderer1;
    const Renderer::Ptr Renderer2;
    const TrackState::Ptr State;
  };
}

namespace ZXTune
{
  namespace Module
  {
    Renderer::Ptr CreateTSRenderer(Renderer::Ptr first, Renderer::Ptr second)
    {
      return CreateTSRenderer(first, second, boost::make_shared<TSTrackState>(first->GetTrackState(), second->GetTrackState()));
    }

    Renderer::Ptr CreateTSRenderer(Renderer::Ptr first, Renderer::Ptr second, TrackState::Ptr state)
    {
      return boost::make_shared<TSRenderer>(first, second, state);
    }

    boost::array<Devices::AYM::Receiver::Ptr, 2> CreateTSAYMixer(Devices::AYM::Receiver::Ptr target)
    {
      return CreateTSMixer<Devices::AYM::MultiSample>(target);
    }
  }
}
