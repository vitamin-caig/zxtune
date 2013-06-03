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
  }
}
