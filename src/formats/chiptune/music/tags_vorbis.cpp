/**
 *
 * @file
 *
 * @brief  Vorbis tags parser implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/music/tags_vorbis.h"
// common includes
#include <byteorder.h>
// library includes
#include <binary/base64.h>
#include <strings/casing.h>
#include <strings/sanitize.h>
// std includes
#include <array>

namespace Formats::Chiptune::Vorbis
{
  StringView ReadString(Binary::DataInputStream& payload)
  {
    const std::size_t size = payload.Read<le_uint32_t>();
    const auto* const utf8 = safe_ptr_cast<const char*>(payload.ReadData(size).Start());
    return {utf8, size};
  }

  void ParseCommentField(StringView field, MetaBuilder& target)
  {
    const auto eqPos = field.find('=');
    if (eqPos == StringView::npos)
    {
      target.SetComment(Strings::SanitizeMultiline(field));
      return;
    }
    const auto name = field.substr(0, eqPos);
    const auto value = field.substr(eqPos + 1);
    if (Strings::EqualNoCaseAscii(name, "TITLE"_sv))
    {
      target.SetTitle(Strings::Sanitize(value));
    }
    else if (Strings::EqualNoCaseAscii(name, "ARTIST"_sv) || Strings::EqualNoCaseAscii(name, "PERFORMER"_sv))
    {
      target.SetAuthor(Strings::Sanitize(value));
    }
    else if (Strings::EqualNoCaseAscii(name, "COPYRIGHT"_sv) || Strings::EqualNoCaseAscii(name, "DESCRIPTION"_sv))
    {
      target.SetComment(Strings::SanitizeMultiline(value));
    }
    else if (Strings::EqualNoCaseAscii(name, "COVERART"_sv))
    {
      try
      {
        target.SetPicture(Binary::Base64::Decode(value));
      }
      catch (const std::exception&)
      {}
    }
  }

  void ParseComment(Binary::DataInputStream& payload, MetaBuilder& target)
  {
    try
    {
      /*const auto vendor = */ ReadString(payload);
      if (uint_t items = payload.Read<le_uint32_t>())
      {
        while (items--)
        {
          ParseCommentField(ReadString(payload), target);
        }
      }
      Require(1 == payload.ReadByte());
    }
    catch (const std::exception&)
    {}
  }
}  // namespace Formats::Chiptune::Vorbis
