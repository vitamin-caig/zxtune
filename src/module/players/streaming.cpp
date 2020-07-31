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
#include "module/players/streaming.h"
//common includes
#include <make_ptr.h>
//library includes
#include <sound/loop.h>
//std includes
#include <utility>

namespace Module
{
  class FramedStreamStateCursor : public State
  {
  public:
    typedef std::shared_ptr<FramedStreamStateCursor> Ptr;

    explicit FramedStreamStateCursor(FramedStream stream)
      : Stream(std::move(stream))
      , CurFrame()
      , Loops()
    {
      Reset();
    }

    //status functions
    uint_t Frame() const override
    {
      return CurFrame;
    }

    uint_t LoopCount() const override
    {
      return Loops;
    }

    //navigation
    bool IsValid() const
    {
      return CurFrame < Stream.TotalFrames;
    }

    void Reset()
    {
      CurFrame = 0;
      Loops = 0;
    }

    void ResetToLoop()
    {
      CurFrame = Stream.LoopFrame;
      ++Loops;
    }

    void NextFrame()
    {
      if (IsValid())
      {
        ++CurFrame;
      }
    }
  private:
    const FramedStream Stream;
    uint_t CurFrame;
    uint_t Loops;
  };

  class FramedStreamInfo : public Information
  {
  public:
    FramedStreamInfo(FramedStream stream)
      : Stream(std::move(stream))
    {
    }

    uint_t FramesCount() const override
    {
      return Stream.TotalFrames;
    }

    uint_t LoopFrame() const override
    {
      return Stream.LoopFrame;
    }
  private:
    const FramedStream Stream;
  };

  class FramedStreamStateIterator : public StateIterator
  {
  public:
    explicit FramedStreamStateIterator(FramedStream stream)
      : Cursor(MakePtr<FramedStreamStateCursor>(std::move(stream)))
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

    void NextFrame(const Sound::LoopParameters& looped) override
    {
      Cursor->NextFrame();
      if (!Cursor->IsValid() && looped(Cursor->LoopCount()))
      {
        Cursor->ResetToLoop();
      }
    }

    State::Ptr GetStateObserver() const override
    {
      return Cursor;
    }
  private:
    const FramedStreamStateCursor::Ptr Cursor;
  };

  Information::Ptr CreateStreamInfo(FramedStream stream)
  {
    return MakePtr<FramedStreamInfo>(std::move(stream));
  }

  StateIterator::Ptr CreateStreamStateIterator(FramedStream stream)
  {
    return MakePtr<FramedStreamStateIterator>(std::move(stream));
  }
}
