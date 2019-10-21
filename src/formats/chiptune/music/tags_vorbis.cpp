/**
*
* @file
*
* @brief  Vorbis tags parser implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "formats/chiptune/music/tags_vorbis.h"
//common includes
#include <byteorder.h>
//library includes
#include <strings/encoding.h>
#include <strings/trim.h>
//std includes
#include <array>
#include <cctype>

namespace Formats
{
namespace Chiptune
{
  namespace Vorbis
  {
    StringView ReadString(Binary::DataInputStream& payload)
    {
      const auto size = fromLE(payload.ReadField<uint32_t>());
      const auto utf8 = safe_ptr_cast<const char*>(payload.ReadRawData(size));
      return StringView(utf8, size);
    }

    String Decode(StringView str)
    {
      //do not trim before- it may break some encodings
      auto decoded = Strings::ToAutoUtf8(str);
      auto trimmed = Strings::TrimSpaces(decoded);
      return decoded.size() == trimmed.size() ? decoded : trimmed.to_string();
    }

    bool CompareTag(StringView str, StringView tag)
    {
      if (str.size() != tag.size())
      {
        return false;
      }
      for (std::size_t idx = 0, lim = str.size(); idx != lim; ++idx)
      {
        if (std::toupper(str[idx]) != tag[idx])
        {
          return false;
        }
      }
      return true;
    }

    void ParseCommentField(StringView field, MetaBuilder& target)
    {
      const auto eqPos = field.find('=');
      Require(eqPos != StringView::npos);
      const auto name = field.substr(0, eqPos);
      const auto value = field.substr(eqPos + 1);
      Strings::Array strings;
      if (CompareTag(name, "TITLE"))
      {
        target.SetTitle(Decode(value));
      }
      else if (CompareTag(name, "ARTIST") || CompareTag(name, "PERFORMER"))
      {
        target.SetAuthor(Decode(value));
      }
      else if (CompareTag(name, "COPYRIGHT") || CompareTag(name, "DESCRIPTION"))
      {
        strings.emplace_back(Decode(value));
      }
      //TODO: meta.SetComment
      if (!strings.empty())
      {
        target.SetStrings(strings);
      }
    }

    void ParseComment(Binary::DataInputStream& payload, MetaBuilder& target)
    {
      try
      {
        /*const auto vendor = */ReadString(payload);
        if (auto items = fromLE(payload.ReadField<uint32_t>()))
        {
          while (items--)
          {
            ParseCommentField(ReadString(payload), target);
          }
        }
        Require(1 == payload.ReadField<uint8_t>());
      }
      catch (const std::exception&)
      {
      }
    }
  } //namespace Vorbis
}
}
