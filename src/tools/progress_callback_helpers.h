/**
 *
 * @file
 *
 * @brief  Progress callback helpers
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <string_view.h>
// library includes
#include <math/scale.h>
#include <tools/progress_callback.h>

//! @brief Namespace is used for logging and other informational purposes
namespace Log
{
  //! @brief Progress receiver
  class PercentProgressCallback : public ProgressCallback
  {
  public:
    PercentProgressCallback(uint_t total, ProgressCallback& delegate)
      : ToPercent(total, 100)
      , Delegate(delegate)
    {}

    void OnProgress(uint_t current) override
    {
      Delegate.OnProgress(ToPercent(current));
    }

    void OnProgress(uint_t current, StringView message) override
    {
      Delegate.OnProgress(ToPercent(current), message);
    }

  private:
    const Math::ScaleFunctor<uint_t> ToPercent;
    ProgressCallback& Delegate;
  };

  class NestedProgressCallback : public ProgressCallback
  {
  public:
    NestedProgressCallback(uint_t total, uint_t done, ProgressCallback& delegate)
      : ToPercent(total, 100)
      , Start(ToPercent(done))
      , Range(ToPercent(done + 1) - Start)
      , Delegate(delegate)
    {}

    void OnProgress(uint_t current) override
    {
      Delegate.OnProgress(Scale(current));
    }

    void OnProgress(uint_t current, StringView message) override
    {
      Delegate.OnProgress(Scale(current), message);
    }

  private:
    uint_t Scale(uint_t nested) const
    {
      return Start + Math::Scale(nested, 100, Range);
    }

  private:
    const Math::ScaleFunctor<uint_t> ToPercent;
    const uint_t Start;
    const uint_t Range;
    ProgressCallback& Delegate;
  };
}  // namespace Log
