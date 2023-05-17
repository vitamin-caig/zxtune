/**
 *
 * @file
 *
 * @brief  TurboSound container support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/aym/turbosound.h"
#include "formats/chiptune/container.h"
// common includes
#include <byteorder.h>
#include <make_ptr.h>
// library includes
#include <binary/format_factories.h>
#include <math/numeric.h>

namespace Formats::Chiptune
{
  namespace TurboSound
  {
    const std::size_t MIN_SIZE = 256;
    const std::size_t MAX_MODULE_SIZE = 32767;
    const std::size_t MAX_SIZE = MAX_MODULE_SIZE * 2;

    struct Footer
    {
      uint8_t ID1[4];  //'PT3!' or other type
      le_uint16_t Size1;
      uint8_t ID2[4];  // same
      le_uint16_t Size2;
      uint8_t ID3[4];  //'02TS'
    };

    static_assert(sizeof(Footer) * alignof(Footer) == 16, "Invalid layout");

    const Char DESCRIPTION[] = "TurboSound";
    const auto FOOTER_FORMAT =
        "%0xxxxxxx%0xxxxxxx%0xxxxxxx21"  // uint8_t ID1[4];//'PT3!' or other type
        "?%0xxxxxxx"                     // uint16_t Size1;
        "%0xxxxxxx%0xxxxxxx%0xxxxxxx21"  // uint8_t ID2[4];//same
        "?%0xxxxxxx"                     // uint16_t Size2;
        "'0'2'T'S"                       // uint8_t ID3[4];//'02TS'
        ""_sv;

    class StubBuilder : public Builder
    {
    public:
      void SetFirstSubmoduleLocation(std::size_t /*offset*/, std::size_t /*size*/) override {}
      void SetSecondSubmoduleLocation(std::size_t /*offset*/, std::size_t /*size*/) override {}
    };

    class ModuleTraits
    {
    public:
      ModuleTraits(Binary::View data, std::size_t footerOffset)
        : FooterOffset(footerOffset)
        , Foot(data.SubView(footerOffset).As<Footer>())
        , FirstSize(Foot ? std::size_t(Foot->Size1) : 0)
        , SecondSize(Foot ? std::size_t(Foot->Size2) : 0)
      {}

      bool Matched() const
      {
        return Foot != nullptr && FooterOffset == FirstSize + SecondSize
               && Math::InRange(FooterOffset, MIN_SIZE, MAX_SIZE);
      }

      std::size_t NextOffset() const
      {
        if (Foot == nullptr)
        {
          return FooterOffset;
        }
        const std::size_t totalSize = FirstSize + SecondSize;
        if (totalSize < FooterOffset)
        {
          return FooterOffset - totalSize;
        }
        else
        {
          return FooterOffset + sizeof(*Foot);
        }
      }

      std::size_t GetFirstModuleSize() const
      {
        return FirstSize;
      }

      std::size_t GetSecondModuleSize() const
      {
        return SecondSize;
      }

      std::size_t GetTotalSize() const
      {
        return FooterOffset + sizeof(*Foot);
      }

    private:
      const std::size_t FooterOffset;
      const Footer* const Foot;
      const std::size_t FirstSize;
      const std::size_t SecondSize;
    };

    class FooterFormat : public Binary::Format
    {
    public:
      using Ptr = std::shared_ptr<const FooterFormat>;

      FooterFormat()
        : Delegate(Binary::CreateFormat(FOOTER_FORMAT))
      {}

      bool Match(Binary::View data) const override
      {
        const ModuleTraits traits = GetTraits(data);
        return traits.Matched();
      }

      std::size_t NextMatchOffset(Binary::View data) const override
      {
        const ModuleTraits traits = GetTraits(data);
        return traits.NextOffset();
      }

      ModuleTraits GetTraits(Binary::View data) const
      {
        return ModuleTraits(data, Delegate->NextMatchOffset(data));
      }

    private:
      const Binary::Format::Ptr Delegate;
    };

    class DecoderImpl : public Decoder
    {
    public:
      DecoderImpl()
        : Format(MakePtr<FooterFormat>())
      {}

      String GetDescription() const override
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

      Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        Builder& stub = GetStubBuilder();
        return Parse(rawData, stub);
      }

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& rawData, Builder& target) const override
      {
        const ModuleTraits& traits = Format->GetTraits(rawData);

        if (!traits.Matched())
        {
          return {};
        }

        target.SetFirstSubmoduleLocation(0, traits.GetFirstModuleSize());
        target.SetSecondSubmoduleLocation(traits.GetFirstModuleSize(), traits.GetSecondModuleSize());

        const std::size_t usedSize = traits.GetTotalSize();
        auto subData = rawData.GetSubcontainer(0, usedSize);
        // use whole container as a fixed data
        return CreateCalculatingCrcContainer(std::move(subData), 0, usedSize);
      }

    private:
      const FooterFormat::Ptr Format;
    };

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }

    Decoder::Ptr CreateDecoder()
    {
      return MakePtr<DecoderImpl>();
    }
  }  // namespace TurboSound

  Decoder::Ptr CreateTurboSoundDecoder()
  {
    return TurboSound::CreateDecoder();
  }
}  // namespace Formats::Chiptune
