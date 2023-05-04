/**
 *
 * @file
 *
 * @brief  Localization support API
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "library.h"

namespace L10n
{
  /*
    @brief Simple function-like helper
    @code
      //implicit ctor to avoid adding domain to strings if specified by constant
      const L10n::TranslateFunctor translate = L10n::TranslateFunctor(currentDomain);
      //simple message
      translate("just a message");
      //message with single and plural forms
      translate("single form", "plural form", count)
      //message with context
      translate("some context", "ambiguous simple message");
      //single and plural form message with context
      translate("other context", "single for of ambiguous message", "plural form of ambiguous message", count);
    @endcode
    @note Name of the functor ('translate') should not be changed due to messages' collecting utility settings
  */
  class TranslateFunctor
  {
  public:
    explicit TranslateFunctor(StringView domain)
      : Delegate(Library::Instance().GetVocabulary(domain))
    {}

    String operator()(const char* text) const
    {
      return Delegate->GetText(text);
    }

    String operator()(const char* single, const char* plural, int count) const
    {
      return Delegate->GetText(single, plural, count);
    }

    String operator()(const char* context, const char* text) const
    {
      return Delegate->GetText(context, text);
    }

    String operator()(const char* context, const char* single, const char* plural, int count) const
    {
      return Delegate->GetText(context, single, plural, count);
    }

  private:
    const Vocabulary::Ptr Delegate;
  };
}  // namespace L10n
