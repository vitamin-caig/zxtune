/**
 *
 * @file
 *
 * @brief Version fields factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <memory>

namespace Strings
{
  class FieldsSource;
}  // namespace Strings

namespace Platform::Version
{
  std::unique_ptr<Strings::FieldsSource> CreateVersionFieldsSource();
}  // namespace Platform::Version
