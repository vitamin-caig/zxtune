/**
 *
 * @file
 *
 * @brief  DSF parser implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/emulation/dreamcastsoundformat.h"

#include <binary/format_factories.h>

#include <make_ptr.h>

namespace Formats::Chiptune
{
  namespace DreamcastSoundFormat
  {
    const auto DESCRIPTION = "Dreamcast Sound Format"sv;

    const auto FORMAT =
        "'P'S'F"
        "12"
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
  }  // namespace DreamcastSoundFormat

  Decoder::Ptr CreateDSFDecoder()
  {
    return MakePtr<DreamcastSoundFormat::Decoder>();
  }
}  // namespace Formats::Chiptune
