/*
Abstract:
  TurboSound functionality helpers implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

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

  template<class T>
  inline T avg(T val1, T val2)
  {
    return (val1 + val2) / 2;
  }

  class TSMixerImpl : public TSMixer
  {
  public:
    explicit TSMixerImpl(Sound::MultichannelReceiver::Ptr delegate)
      : Delegate(delegate)
      , Stream()
    {
    }

    virtual void ApplyData(const std::vector<Sound::Sample>& data)
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
        MixCursor = CycledIterator<BufferType::iterator>(Buffer.begin(), Buffer.end());
      }
      else
      {
        Buffer.clear();
      }
    }
  private:
    void Store(const std::vector<Sound::Sample>& data)
    {
      Buffer.push_back(data);
    }

    void Mix(const std::vector<Sound::Sample>& data)
    {
      assert(MixCursor->size() == data.size());
      const SamplesVector::iterator bufIter = MixCursor->begin();
      std::transform(data.begin(), data.end(), bufIter, bufIter, &avg<Sound::Sample>);
      Delegate->ApplyData(*MixCursor);
      ++MixCursor;
    }
  private:
    const Sound::MultichannelReceiver::Ptr Delegate;
    uint_t Stream;
    typedef std::vector<Sound::Sample> SamplesVector;
    typedef std::vector<SamplesVector> BufferType;
    BufferType Buffer;
    CycledIterator<BufferType::iterator> MixCursor;
  };

  class TSPlayer : public Player
  {
  public:
    TSPlayer(Player::Ptr first, Player::Ptr second, TSMixer::Ptr mixer)
      : Mixer(mixer)
      , Player1(first)
      , Player2(second)
    {
    }

    virtual TrackState::Ptr GetTrackState() const
    {
      return CreateTSTrackState(Player1->GetTrackState(), Player2->GetTrackState());
    }

    virtual Analyzer::Ptr GetAnalyzer() const
    {
      return CreateTSAnalyzer(Player1->GetAnalyzer(), Player2->GetAnalyzer());
    }

    virtual Error RenderFrame(const Sound::RenderParameters& params,
                              PlaybackState& state)
    {
      PlaybackState state1, state2;
      Mixer->SetStream(0);
      if (const Error& e = Player1->RenderFrame(params, state1))
      {
        return e;
      }
      Mixer->SetStream(1);
      if (const Error& e = Player2->RenderFrame(params, state2))
      {
        return e;
      }
      state = state1 == MODULE_STOPPED || state2 == MODULE_STOPPED ? MODULE_STOPPED : MODULE_PLAYING;
      return Error();
    }

    virtual Error Reset()
    {
      if (const Error& e = Player1->Reset())
      {
        return e;
      }
      return Player2->Reset();
    }

    virtual Error SetPosition(uint_t frame)
    {
      if (const Error& e = Player1->SetPosition(frame))
      {
        return e;
      }
      return Player2->SetPosition(frame);
    }
  private:
    const TSMixer::Ptr Mixer;
    const Player::Ptr Player1;
    const Player::Ptr Player2;
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
      return boost::make_shared<TSMixerImpl>(delegate);
    }

    Player::Ptr CreateTSPlayer(Holder::Ptr first, Holder::Ptr second, Sound::MultichannelReceiver::Ptr target)
    {
      const TSMixer::Ptr mixer = CreateTSMixer(target);
      return CreateTSPlayer(first->CreatePlayer(mixer), second->CreatePlayer(mixer), mixer);
    }

    Player::Ptr CreateTSPlayer(Player::Ptr first, Player::Ptr second, TSMixer::Ptr mixer)
    {
      return boost::make_shared<TSPlayer>(first, second, mixer);
    }
  }
}
