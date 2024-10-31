/**
 *
 * @file
 *
 * @brief  Parameters tracking functionality
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "parameters/container.h"

namespace Parameters
{
  Container::Ptr CreatePreChangePropertyTrackedContainer(Container::Ptr delegate, Modifier& callback);
  Container::Ptr CreatePostChangePropertyTrackedContainer(Container::Ptr delegate, Modifier& callback);
}  // namespace Parameters
