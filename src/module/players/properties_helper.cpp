/**
 *
 * @file
 *
 * @brief  Module properties builder implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/properties_helper.h"
// library includes
#include <binary/crc.h>
#include <module/attributes.h>
#include <sound/sound_parameters.h>
#include <strings/join.h>
#include <strings/trim.h>
// common includes
#include <string_view.h>

namespace Module
{
  void PropertiesHelper::SetNonEmptyProperty(StringView name, StringView value)
  {
    if (!value.empty())
    {
      Delegate.SetValue(name, value);
    }
  }

  void PropertiesHelper::SetBinaryProperty(StringView name, Binary::View value)
  {
    Delegate.SetValue(name, value);
  }

  void PropertiesHelper::SetType(StringView type)
  {
    Delegate.SetValue(ATTR_TYPE, type);
  }

  void PropertiesHelper::SetContainer(StringView container)
  {
    SetNonEmptyProperty(ATTR_CONTAINER, container);
  }

  void PropertiesHelper::SetSource(const Formats::Chiptune::Container& source)
  {
    Delegate.SetValue(ATTR_SIZE, source.Size());
    Delegate.SetValue(ATTR_CRC, Binary::Crc32(source));
    Delegate.SetValue(ATTR_FIXEDCRC, source.FixedChecksum());
  }

  void PropertiesHelper::SetAuthor(StringView author)
  {
    SetNonEmptyProperty(ATTR_AUTHOR, author);
  }

  void PropertiesHelper::SetTitle(StringView title)
  {
    SetNonEmptyProperty(ATTR_TITLE, title);
  }

  void PropertiesHelper::SetComment(StringView comment)
  {
    SetNonEmptyProperty(ATTR_COMMENT, comment);
  }

  void PropertiesHelper::SetProgram(StringView program)
  {
    SetNonEmptyProperty(ATTR_PROGRAM, program);
  }

  void PropertiesHelper::SetComputer(StringView computer)
  {
    SetNonEmptyProperty(ATTR_COMPUTER, computer);
  }

  void PropertiesHelper::SetStrings(const Strings::Array& strings)
  {
    // TODO: Join(begin, end, delimiter)
    const auto joined = Strings::Join(strings, "\n"sv);
    const auto trimmed = Strings::Trim(joined, '\n');
    SetNonEmptyProperty(ATTR_STRINGS, trimmed);
  }

  void PropertiesHelper::SetVersion(uint_t major, uint_t minor)
  {
    assert(minor < 10);
    const uint_t version = 10 * major + minor;
    Delegate.SetValue(ATTR_VERSION, version);
  }

  void PropertiesHelper::SetVersion(StringView version)
  {
    SetNonEmptyProperty(ATTR_VERSION, version);
  }

  void PropertiesHelper::SetDate(StringView date)
  {
    SetNonEmptyProperty(ATTR_DATE, date);
  }

  void PropertiesHelper::SetPlatform(StringView platform)
  {
    Delegate.SetValue(ATTR_PLATFORM, platform);
  }

  void PropertiesHelper::SetFadein(Time::Milliseconds fadein)
  {
    using namespace Parameters::ZXTune::Sound;
    Delegate.SetValue(FADEIN, FADEIN_PRECISION * fadein.Get() / fadein.PER_SECOND);
  }

  void PropertiesHelper::SetFadeout(Time::Milliseconds fadeout)
  {
    using namespace Parameters::ZXTune::Sound;
    Delegate.SetValue(FADEOUT, FADEOUT_PRECISION * fadeout.Get() / fadeout.PER_SECOND);
  }
}  // namespace Module
