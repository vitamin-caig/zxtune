/**
* 
* @file
*
* @brief  Streamed modules support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "streaming.h"
//common includes
#include <make_ptr.h>
//std includes
#include <utility>

namespace Module
{
  const uint_t STREAM_CHANNELS = 1;

  class StreamStateCursor : public TrackState
  {
  public:
    typedef std::shared_ptr<StreamStateCursor> Ptr;

    explicit StreamStateCursor(Information::Ptr info)
      : Info(std::move(info))
      , CurFrame()
    {
      Reset();
    }

    //status functions
    uint_t Position() const override
    {
      return 0;
    }

    uint_t Pattern() const override
    {
      return 0;
    }

    uint_t PatternSize() const override
    {
      return 0;
    }

    uint_t Line() const override
    {
      return 0;
    }

    uint_t Tempo() const override
    {
      return 1;
    }

    uint_t Quirk() const override
    {
      return 0;
    }

    uint_t Frame() const override
    {
      return CurFrame;
    }

    uint_t Channels() const override
    {
      return STREAM_CHANNELS;
    }

    //navigation
    bool IsValid() const
    {
      return CurFrame < Info->FramesCount();
    }

    void Reset()
    {
      CurFrame = 0;
    }

    void ResetToLoop()
    {
      CurFrame = Info->LoopFrame();
    }

    void NextFrame()
    {
      if (IsValid())
      {
        ++CurFrame;
      }
    }
  private:
    const Information::Ptr Info;
    uint_t CurFrame;
  };

  class StreamInfo : public Information
  {
  public:
    StreamInfo(uint_t frames, uint_t loopFrame)
      : TotalFrames(frames)
      , Loop(loopFrame)
    {
    }
    uint_t PositionsCount() const override
    {
      return 1;
    }
    uint_t LoopPosition() const override
    {
      return 0;
    }
    uint_t FramesCount() const override
    {
      return TotalFrames;
    }
    uint_t LoopFrame() const override
    {
      return Loop;
    }
    uint_t ChannelsCount() const override
    {
      return STREAM_CHANNELS;
    }
    uint_t Tempo() const override
    {
      return 1;
    }
  private:
    const uint_t TotalFrames;
    const uint_t Loop;
  };

  class StreamStateIterator : public StateIterator
  {
  public:
    explicit StreamStateIterator(Information::Ptr info)
      : Cursor(MakePtr<StreamStateCursor>(info))
    {
    }

    //iterator functions
    void Reset() override
    {
      Cursor->Reset();
    }

    bool IsValid() const override
    {
      return Cursor->IsValid();
    }

    void NextFrame(bool looped) override
    {
      Cursor->NextFrame();
      if (!Cursor->IsValid() && looped)
      {
        Cursor->ResetToLoop();
      }
    }

    TrackState::Ptr GetStateObserver() const override
    {
      return Cursor;
    }
  private:
    const StreamStateCursor::Ptr Cursor;
  };

  Information::Ptr CreateStreamInfo(uint_t frames, uint_t loopFrame)
  {
    return MakePtr<StreamInfo>(frames, loopFrame);
  }

  StateIterator::Ptr CreateStreamStateIterator(Information::Ptr info)
  {
    return MakePtr<StreamStateIterator>(info);
  }
}
