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

#include <parameters/container.h>

namespace Parameters
{
  // All accessed properties are prioritized by the first one
  // All the changes are bypassed to the second
  Container::Ptr CreateMergedContainer(Accessor::Ptr first, Container::Ptr second);
}  // namespace Parameters
