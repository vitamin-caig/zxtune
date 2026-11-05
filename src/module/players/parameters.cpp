/**
 *
 * @file
 *
 * @brief  Players common parameters
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/parameters.h"

#include "core/parameters.h"

#include <utility>

namespace Module
{
  FrameDurationParameter::FrameDurationParameter(Parameters::Accessor::Ptr storage, Time::Microseconds def)
    : Delegate(std::move(storage))
    , Default(def)
    , Value(def)
  {
    Synchronize();
  }

  void FrameDurationParameter::Synchronize()
  {
    if (Delegate.IsChanged())
    {
      // Removing the parameter falls back to the family default
      Value = Delegate.Find<Parameters::ZXTune::Core::FrameDuration>().value_or(Default);
    }
  }
}  // namespace Module
