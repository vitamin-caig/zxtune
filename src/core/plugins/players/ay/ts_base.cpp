/*
Abstract:
  TurboSound functionality helpers implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

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

    virtual uint_t AbsoluteFrame() const
    {
      return First->AbsoluteFrame();
    }

    virtual uint64_t AbsoluteTick() const
    {
      return First->AbsoluteTick();
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
  }
}
