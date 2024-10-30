/**
 *
 * @file
 *
 * @brief  Stub l10n api
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <string_type.h>

namespace L10n
{
  class TranslateFunctor
  {
  public:
    explicit TranslateFunctor(const char* /*domain*/) {}

    String operator()(const char* text) const
    {
      return text;
    }

    String operator()(const char* single, const char* plural, int count) const
    {
      return count == 1 ? single : plural;
    }

    String operator()(const char* /*context*/, const char* text) const
    {
      return text;
    }

    String operator()(const char* /*context*/, const char* single, const char* plural, int count) const
    {
      return count == 1 ? single : plural;
    }
  };
}  // namespace L10n
