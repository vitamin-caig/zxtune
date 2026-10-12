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

#include "core/core_parameters.h"

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
      using namespace Parameters::ZXTune::Core;
      using Storage = Time::Duration<Time::BaseUnit<Parameters::IntType, FRAMEDURATION_PRECISION>>;
      // Removing the parameter falls back to the family default
      if (const auto val = Delegate->FindInteger(FRAMEDURATION))
      {
        Value = Storage(*val);
      }
      else
      {
        Value = Default;
      }
    }
  }
}  // namespace Module
