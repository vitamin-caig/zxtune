/**
 *
 * @file
 *
 * @brief  SSF parser implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/emulation/segasaturnsoundformat.h"

#include "binary/format.h"
#include "binary/format_factories.h"
#include "binary/view.h"

#include "make_ptr.h"
#include "string_view.h"

#include <memory>

namespace Formats::Chiptune
{
  namespace SegaSaturnSoundFormat
  {
    const auto DESCRIPTION = "Sega Saturn Sound Format"sv;
    const auto FORMAT =
        "'P'S'F"
        "11"
        ""sv;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateMatchOnlyFormat(FORMAT))
      {}

      StringView GetDescription() const override
      {
        return DESCRIPTION;
      }

      Binary::Format::Ptr GetFormat() const override
      {
        return Format;
      }

      bool Check(Binary::View rawData) const override
      {
        return Format->Match(rawData);
      }

      Formats::Chiptune::Container::Ptr Decode(const Binary::Container& /*rawData*/) const override
      {
        return {};  // TODO
      }

    private:
      const Binary::Format::Ptr Format;
    };
  }  // namespace SegaSaturnSoundFormat

  Decoder::Ptr CreateSSFDecoder()
  {
    return MakePtr<SegaSaturnSoundFormat::Decoder>();
  }
}  // namespace Formats::Chiptune
