/**
 *
 * @file
 *
 * @brief  Generic decoder for format-matching descriptions implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/common/decoder.h"

#include "formats/chiptune/common/container.h"

#include "binary/format_factories.h"

#include "make_ptr.h"

namespace Formats::Chiptune
{
  class FormatDecoder : public Decoder
  {
  public:
    FormatDecoder(StringView format, StringView description)
      : Format(format)
      , Description(description)
      , Fmt(Binary::CreateMatchOnlyFormat(format))
    {}

    StringView GetDescription() const override
    {
      return Description;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Fmt;
    }

    bool Check(Binary::View rawData) const override
    {
      return Fmt->Match(rawData);
    }

    Container::Ptr Decode(const Binary::Container& rawData) const override
    {
      if (Check(rawData))
      {
        return CreateCalculatingCrcContainer(rawData);
      }
      return {};
    }

  private:
    const StringView Format;
    const StringView Description;
    const Binary::Format::Ptr Fmt;
  };

  Decoder::Ptr CreateFormatDecoder(StringView format, StringView description)
  {
    return MakePtr<FormatDecoder>(format, description);
  }
}  // namespace Formats::Chiptune
