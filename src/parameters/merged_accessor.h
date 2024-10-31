/**
 *
 * @file
 *
 * @brief  Factories for accessor-based adapters
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <parameters/accessor.h>

namespace Parameters
{
  // All accessed properties are prioritized by the first one
  Accessor::Ptr CreateMergedAccessor(Accessor::Ptr first, Accessor::Ptr second);
  Accessor::Ptr CreateMergedAccessor(Accessor::Ptr first, Accessor::Ptr second, Accessor::Ptr third);
}  // namespace Parameters
