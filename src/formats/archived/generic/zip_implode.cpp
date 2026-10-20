/**
 *
 * @file
 *
 * @brief  PKWare implode (ZIP compression method 6) decompress function implementation
 *
 * @note   Based on Info-ZIP explode.c by Mark Adler
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/archived/generic/zip_implode.h"

#include "binary/data_builder.h"

#include "error.h"

#include <optional>
#include <span>
#include <vector>

namespace Formats::Archived::Zip
{
  namespace Implode
  {
    //! Bit flags of the ZIP general purpose flag word that select implode mode
    enum
    {
      IMPLODE_LITERALS_CODED = 0x4,
      IMPLODE_DICTIONARY_8K = 0x2
    };

    //! Maximum bit length of any code
    const std::size_t MAX_BITS = 16;
    //! Mark of an unused table entry
    const uint8_t INVALID_CODE = 99;

    const uint16_t LENGTH_BASE_2[64] = {
        2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
        24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
        46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65,
    };
    const uint16_t LENGTH_BASE_3[64] = {
        3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
        25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46,
        47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66,
    };
    const uint8_t LENGTH_EXTRA[64] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8,
    };
    const uint16_t DISTANCE_BASE_4K[64] = {
        1,    65,   129,  193,  257,  321,  385,  449,  513,  577,  641,  705,  769,  833,  897,  961,
        1025, 1089, 1153, 1217, 1281, 1345, 1409, 1473, 1537, 1601, 1665, 1729, 1793, 1857, 1921, 1985,
        2049, 2113, 2177, 2241, 2305, 2369, 2433, 2497, 2561, 2625, 2689, 2753, 2817, 2881, 2945, 3009,
        3073, 3137, 3201, 3265, 3329, 3393, 3457, 3521, 3585, 3649, 3713, 3777, 3841, 3905, 3969, 4033,
    };
    const uint16_t DISTANCE_BASE_8K[64] = {
        1,    129,  257,  385,  513,  641,  769,  897,  1025, 1153, 1281, 1409, 1537, 1665, 1793, 1921,
        2049, 2177, 2305, 2433, 2561, 2689, 2817, 2945, 3073, 3201, 3329, 3457, 3585, 3713, 3841, 3969,
        4097, 4225, 4353, 4481, 4609, 4737, 4865, 4993, 5121, 5249, 5377, 5505, 5633, 5761, 5889, 6017,
        6145, 6273, 6401, 6529, 6657, 6785, 6913, 7041, 7169, 7297, 7425, 7553, 7681, 7809, 7937, 8065,
    };

    struct TableEntry
    {
      uint8_t ExtraBits = 0;
      uint8_t Bits = 0;
      uint16_t Value = 0;

      bool IsInvalid() const
      {
        return ExtraBits == INVALID_CODE;
      }
    };

    //! Decoding table built from a set of code lengths; uses flat lookup indexed by reversed code
    class HuffmanTable
    {
    public:
      HuffmanTable(std::span<const unsigned> lengths, std::size_t simpleCodes, std::span<const uint16_t> bases = {},
                   std::span<const uint8_t> extras = {})
      {
        const std::size_t count = lengths.size();
        std::size_t lengthCounts[MAX_BITS + 1] = {};
        for (const unsigned len : lengths)
        {
          ++lengthCounts[len];
        }
        if (lengthCounts[0] == count)
        {
          throw Error(THIS_LINE, "Implode: empty code");
        }
        for (MaxLen = MAX_BITS; !lengthCounts[MaxLen]; --MaxLen)
        {}
        std::size_t minLen;
        for (minLen = 1; !lengthCounts[minLen]; ++minLen)
        {}
        // Pad an incomplete code set with invalid entries at the longest length, as in huft_build
        const std::size_t actualTotal = count - lengthCounts[0];
        std::size_t remaining = std::size_t{1} << minLen;
        for (std::size_t len = minLen; len < MaxLen; ++len)
        {
          if (remaining < lengthCounts[len])
          {
            throw Error(THIS_LINE, "Implode: invalid code lengths");
          }
          remaining = (remaining - lengthCounts[len]) << 1;
        }
        if (remaining < lengthCounts[MaxLen])
        {
          throw Error(THIS_LINE, "Implode: invalid code lengths");
        }
        lengthCounts[MaxLen] = remaining;
        // Generate starting offsets into the value table for each length
        std::size_t offsets[MAX_BITS + 1] = {};
        std::size_t total = 0;
        for (std::size_t len = 1; len <= MAX_BITS; ++len)
        {
          offsets[len] = total;
          total += lengthCounts[len];
        }
        // Only the real codes are stored; padded codes are resolved as invalid during decoding
        std::vector<std::size_t> ordered(actualTotal);
        for (std::size_t i = 0; i < count; ++i)
        {
          if (const std::size_t len = lengths[i])
          {
            ordered[offsets[len]++] = i;
          }
        }

        // Flat lookup table; indexed by the inverted low bits of the stream, as in huft_build
        const std::size_t tableSize = std::size_t{1} << MaxLen;
        Entries.assign(tableSize, TableEntry{});
        // Codes are assigned by a backwards increment of the code value, matching huft_build
        std::size_t code = 0;
        std::size_t value = 0;
        for (std::size_t len = 1; len <= MaxLen; ++len)
        {
          const std::size_t step = std::size_t{1} << len;
          const std::size_t mask = step - 1;
          for (std::size_t n = 0; n < lengthCounts[len]; ++n, ++value)
          {
            TableEntry entry;
            entry.Bits = static_cast<uint8_t>(len);
            if (value >= ordered.size())
            {
              // Padding entry of an incomplete code set
              entry.ExtraBits = INVALID_CODE;
            }
            else
            {
              const std::size_t symbol = ordered[value];
              if (symbol < simpleCodes)
              {
                entry.ExtraBits = symbol < 256 ? 32 : 31;
                entry.Value = static_cast<uint16_t>(symbol);
              }
              else
              {
                entry.ExtraBits = extras[symbol - simpleCodes];
                entry.Value = bases[symbol - simpleCodes];
              }
            }
            // The decoder matches the inverted bits, so fill the complemented patterns
            for (std::size_t idx = (~code) & mask; idx < tableSize; idx += step)
            {
              Entries[idx] = entry;
            }
            std::size_t j;
            for (j = step >> 1; code & j; j >>= 1)
            {
              code ^= j;
            }
            code ^= j;
          }
        }
      }

      uint_t Bits() const
      {
        return MaxLen;
      }

      //! Decodes a symbol from already peeked bits (LSB-first)
      TableEntry Decode(uint_t value) const
      {
        return Entries[value & ((uint_t{1} << MaxLen) - 1)];
      }

    private:
      std::vector<TableEntry> Entries;
      uint_t MaxLen = 0;
    };

    //! Implode bitstream reader; codes are stored least significant bit first
    class Bitstream
    {
    public:
      explicit Bitstream(Binary::View data)
        : Data(data.As<uint8_t>())
        , Size(data.Size())
      {}

      uint_t GetByte()
      {
        return GetBits(8);
      }

      uint_t GetBits(std::size_t count)
      {
        if (Pos + count > Size * 8)
        {
          throw Error(THIS_LINE, "Implode: out of input");
        }
        const uint_t result = PeekBits(count);
        Pos += count;
        return result;
      }

      //! Peeks bits without advancing; missing trailing bits are treated as zeroes
      uint_t PeekBits(std::size_t count) const
      {
        const std::size_t startByte = Pos >> 3;
        const std::size_t startBit = Pos & 7;
        // Only the first byte is offset by the current bit position, subsequent ones are aligned
        uint_t result = startByte < Size ? Data[startByte] >> startBit : 0;
        for (std::size_t i = 1, produced = 8 - startBit; produced < count; ++i, produced += 8)
        {
          const uint_t byte = startByte + i < Size ? Data[startByte + i] : 0;
          result |= byte << produced;
        }
        return result & ((uint_t{1} << count) - 1);
      }

      void DropBits(std::size_t count)
      {
        Pos += count;
      }

    private:
      const uint8_t* const Data;
      const std::size_t Size;
      std::size_t Pos = 0;
    };

    //! Implode stream decoder, based on the Info-ZIP explode algorithm
    class Decoder
    {
    public:
      Decoder(Binary::View packed, unsigned flags)
        : Stream(packed)
        , DistanceExtraBits(0 != (flags & IMPLODE_DICTIONARY_8K) ? 7 : 6)
      {
        if (0 != (flags & IMPLODE_LITERALS_CODED))
        {
          LiteralTable.emplace(ReadCodeLengths(256), 256);
          LengthTable.emplace(ReadCodeLengths(64), 0, LENGTH_BASE_3, LENGTH_EXTRA);
        }
        else
        {
          LengthTable.emplace(ReadCodeLengths(64), 0, LENGTH_BASE_2, LENGTH_EXTRA);
        }
        if (DistanceExtraBits == 7)
        {
          DistanceTable.emplace(ReadCodeLengths(64), 0, DISTANCE_BASE_8K, LENGTH_EXTRA);
        }
        else
        {
          DistanceTable.emplace(ReadCodeLengths(64), 0, DISTANCE_BASE_4K, LENGTH_EXTRA);
        }
      }

      void Decompress(std::size_t unpackedSize, Binary::DataBuilder& output)
      {
        std::size_t produced = 0;
        while (produced < unpackedSize)
        {
          if (Stream.GetBits(1))
          {
            output.AddByte(static_cast<uint8_t>(DecodeLiteral()));
            ++produced;
          }
          else
          {
            const std::size_t distance = DecodeDistance();
            const std::size_t length = DecodeLength();
            const std::size_t till = std::min(unpackedSize, produced + length);
            for (; produced < till; ++produced)
            {
              // Distances may point before the output start; the format interprets that area as zeroes
              output.AddByte(distance <= produced ? output.Get<uint8_t>(produced - distance) : 0);
            }
          }
        }
      }

    private:
      //! Reads the compact code lengths representation: pairs of (count-1, length-1)
      std::vector<unsigned> ReadCodeLengths(std::size_t expected)
      {
        std::vector<unsigned> result;
        result.reserve(expected);
        const std::size_t pairs = Stream.GetByte() + 1;
        for (std::size_t i = 0; i < pairs; ++i)
        {
          const uint_t raw = Stream.GetByte();
          const std::size_t bits = (raw & 0xf) + 1;
          const std::size_t count = (raw >> 4) + 1;
          if (result.size() + count > expected)
          {
            throw Error(THIS_LINE, "Implode: too many code lengths");
          }
          result.insert(result.end(), count, static_cast<unsigned>(bits));
        }
        if (result.size() != expected)
        {
          throw Error(THIS_LINE, "Implode: not enough code lengths");
        }
        return result;
      }

      uint_t DecodeLiteral()
      {
        return LiteralTable ? DecodeCode(*LiteralTable).Value : Stream.GetByte();
      }

      std::size_t DecodeDistance()
      {
        const std::size_t low = Stream.GetBits(DistanceExtraBits);
        // The table value already holds the shifted high bits, low bits are uncoded
        return DecodeCode(*DistanceTable).Value + low;
      }

      std::size_t DecodeLength()
      {
        const TableEntry entry = DecodeCode(*LengthTable);
        std::size_t result = entry.Value;
        if (const std::size_t extra = entry.ExtraBits)
        {
          result += Stream.GetBits(extra);
        }
        return result;
      }

      TableEntry DecodeCode(const HuffmanTable& table)
      {
        const uint_t peek = Stream.PeekBits(table.Bits());
        const TableEntry entry = table.Decode(peek);
        if (entry.IsInvalid())
        {
          throw Error(THIS_LINE, "Implode: invalid code");
        }
        Stream.DropBits(entry.Bits);
        return entry;
      }

    private:
      Bitstream Stream;
      const std::size_t DistanceExtraBits;
      std::optional<HuffmanTable> LiteralTable;
      std::optional<HuffmanTable> LengthTable;
      std::optional<HuffmanTable> DistanceTable;
    };
  }  // namespace Implode

  Binary::Container::Ptr Deimplode(Binary::View packed, std::size_t unpackedSize, unsigned flags)
  {
    Binary::DataBuilder out(unpackedSize);
    Implode::Decoder(packed, flags).Decompress(unpackedSize, out);
    return out.CaptureResult();
  }
}  // namespace Formats::Archived::Zip
