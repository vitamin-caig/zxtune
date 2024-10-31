/**
 *
 * @file
 *
 * @brief  WAV parser interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/view.h"
#include "formats/chiptune.h"

#include "types.h"

#include <array>

namespace Formats::Chiptune
{
  class MetaBuilder;

  namespace Wav
  {
    // currently supported formats
    enum Format : uint_t
    {
      PCM = 1,
      ADPCM = 2,
      IEEE_FLOAT = 3,
      ALAW = 6,
      MULAW = 7,
      IMA_ADPCM = 17,  // same as DVI_ADPCM

      ATRAC3 = 0x270,

      EXTENDED = 0xfffe
    };

    using Guid = std::array<uint8_t, 16>;

    const Guid ATRAC3PLUS = {
        {0xbf, 0xaa, 0x23, 0xe9, 0x58, 0xcb, 0x71, 0x44, 0xa1, 0x19, 0xff, 0xfa, 0x01, 0xe4, 0xce, 0x62}};
    const Guid ATRAC9 = {
        {0xd2, 0x42, 0xe1, 0x47, 0xba, 0x36, 0x8d, 0x4d, 0x88, 0xfc, 0x61, 0x65, 0x4f, 0x8c, 0x83, 0x6c}};

    class Builder
    {
    public:
      virtual ~Builder() = default;

      virtual MetaBuilder& GetMetaBuilder() = 0;

      virtual void SetProperties(uint_t format, uint_t frequency, uint_t channels, uint_t bits, uint_t blockSize) = 0;
      // Called when format is Format::EXTENDED
      virtual void SetExtendedProperties(uint_t validBitsOrBlockSize, uint_t channelsMask, const Guid& formatId,
                                         Binary::View restData) = 0;
      virtual void SetExtraData(Binary::View data) = 0;
      virtual void SetSamplesData(Binary::Container::Ptr data) = 0;
      virtual void SetSamplesCountHint(uint_t count) = 0;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
    Builder& GetStubBuilder();
  }  // namespace Wav

  Decoder::Ptr CreateWAVDecoder();
}  // namespace Formats::Chiptune
