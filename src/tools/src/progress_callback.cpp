/**
*
* @file
*
* @brief  Progress callback helpers implementation
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <progress_callback.h>

namespace
{
  class StubProgressCallback : public Log::ProgressCallback
  {
  public:
    virtual void OnProgress(uint_t /*current*/)
    {
    }

    virtual void OnProgress(uint_t /*current*/, const String& /*message*/)
    {
    }
  };

  inline uint_t ScaleToPercent(uint_t total, uint_t current)
  {
    return static_cast<uint_t>(uint64_t(current) * 100 / total);
  }

  //TODO: use template method or functor
  class PercentProgressCallback : public Log::ProgressCallback
  {
  public:
    PercentProgressCallback(uint_t total, Log::ProgressCallback& delegate)
      : Total(total)
      , Delegate(delegate)
    {
    }

    virtual void OnProgress(uint_t current)
    {
      const int_t curProg = ScaleToPercent(Total, current);
      Delegate.OnProgress(curProg);
    }

    virtual void OnProgress(uint_t current, const String& message)
    {
      const int_t curProg = ScaleToPercent(Total, current);
      Delegate.OnProgress(curProg, message);
    }
  private:
    const uint_t Total;
    Log::ProgressCallback& Delegate;
  };

  class NestedPercentProgressCallback : public Log::ProgressCallback
  {
  public:
    NestedPercentProgressCallback(uint_t total, uint_t current, Log::ProgressCallback& delegate)
      : Start(ScaleToPercent(total, current))
      , Range(ScaleToPercent(total, current + 1) - Start)
      , Delegate(delegate)
    {
    }

    virtual void OnProgress(uint_t current)
    {
      const int_t curProg = ScaleToProgress(current);
      Delegate.OnProgress(curProg);
    }

    virtual void OnProgress(uint_t current, const String& message)
    {
      const int_t curProg = ScaleToProgress(current);
      Delegate.OnProgress(curProg, message);
    }
  private:
    uint_t ScaleToProgress(uint_t position) const
    {
      return Start + static_cast<uint_t>(uint64_t(position) * Range / 100);
    }
  private:
    const uint_t Start;
    const uint_t Range;
    Log::ProgressCallback& Delegate;
  };
}

namespace Log
{
  ProgressCallback& ProgressCallback::Stub()
  {
    static StubProgressCallback stub;
    return stub;
  }

  ProgressCallback::Ptr CreatePercentProgressCallback(uint_t total, ProgressCallback& delegate)
  {
    return ProgressCallback::Ptr(new PercentProgressCallback(total, delegate));
  }

  ProgressCallback::Ptr CreateNestedPercentProgressCallback(uint_t total, uint_t current, ProgressCallback& delegate)
  {
    return ProgressCallback::Ptr(new NestedPercentProgressCallback(total, current, delegate));
  }
}
