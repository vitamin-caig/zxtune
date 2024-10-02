/**
 *
 * @file
 *
 * @brief  Module properties helper interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <string_view.h>
// library includes
#include <formats/chiptune.h>
#include <parameters/modifier.h>
#include <strings/array.h>
#include <time/duration.h>

namespace Module
{
  class PropertiesHelper
  {
  public:
    explicit PropertiesHelper(Parameters::Modifier& delegate)
      : Delegate(delegate)
    {}

    // Generic
    void SetNonEmptyProperty(StringView name, StringView value);
    void SetBinaryProperty(StringView name, Binary::View value);

    // Common
    void SetType(StringView type);
    void SetContainer(StringView container);
    void SetSource(const Formats::Chiptune::Container& source);

    // Meta
    void SetAuthor(StringView author);
    void SetTitle(StringView title);
    void SetComment(StringView comment);
    void SetProgram(StringView program);
    void SetComputer(StringView computer);
    void SetStrings(const Strings::Array& strings);
    void SetVersion(uint_t major, uint_t minor);
    void SetVersion(StringView version);
    void SetDate(StringView date);
    void SetPlatform(StringView platform);

    // Sound
    void SetFadein(Time::Milliseconds fadein);
    void SetFadeout(Time::Milliseconds fadeout);

  protected:
    Parameters::Modifier& Delegate;
  };
}  // namespace Module
