/**
 *
 * @file
 *
 * @brief  Base64 functions implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <binary/base64.h>
#include <tools/iterators.h>

#include <contract.h>
#include <string_view.h>

namespace Binary::Base64
{
  const uint_t BIN_GROUP_SIZE = 3;
  const uint_t TXT_GROUP_SIZE = 4;

  const char ENCODE_TABLE[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  const char STUB_SYMBOL = '=';

  const uint8_t INVALID = 0x40;
  const uint8_t SKIPPED = 0x41;
  const uint8_t PADDING = 0x42;

  // clang-format off
    const uint8_t DECODE_TABLE[128] =
    {
      INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
      INVALID, SKIPPED, SKIPPED, INVALID, INVALID, SKIPPED, INVALID, INVALID,
      INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
      INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
      SKIPPED, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
      INVALID, INVALID, INVALID, 0x3e,    INVALID, INVALID, INVALID, 0x3f,
      0x34,    0x35,    0x36,    0x37,    0x38,    0x39,    0x3a,    0x3b,
      0x3c,    0x3d,    INVALID, INVALID, INVALID, PADDING, INVALID, INVALID,
      INVALID, 0x00,    0x01,    0x02,    0x03,    0x04,    0x05,    0x06,
      0x07,    0x08,    0x09,    0x0a,    0x0b,    0x0c,    0x0d,    0x0e,
      0x0f,    0x10,    0x11,    0x12,    0x13,    0x14,    0x15,    0x16,
      0x17,    0x18,    0x19,    INVALID, INVALID, INVALID, INVALID, INVALID,
      INVALID, 0x1a,    0x1b,    0x1c,    0x1d,    0x1e,    0x1f,    0x20,
      0x21,    0x22,    0x23,    0x24,    0x25,    0x26,    0x27,    0x28,
      0x29,    0x2a,    0x2b,    0x2c,    0x2d,    0x2e,    0x2f,    0x30,
      0x31,    0x32,    0x33,    INVALID, INVALID, INVALID, INVALID, INVALID
    };
  // clang-format on

  inline void Encode3Bytes(const uint8_t* in, char* out)
  {
    const uint_t buf = (uint_t(in[0]) << 16) | (uint_t(in[1]) << 8) | in[2];
    out[0] = ENCODE_TABLE[(buf >> 18) & 63];
    out[1] = ENCODE_TABLE[(buf >> 12) & 63];
    out[2] = ENCODE_TABLE[(buf >> 6) & 63];
    out[3] = ENCODE_TABLE[(buf >> 0) & 63];
  }

  inline void Encode2Bytes(const uint8_t* in, char* out)
  {
    const uint_t buf = (uint_t(in[0]) << 8) | in[1];
    out[0] = ENCODE_TABLE[(buf >> 10) & 63];
    out[1] = ENCODE_TABLE[(buf >> 4) & 63];
    out[2] = ENCODE_TABLE[(buf << 2) & 63];
    out[3] = STUB_SYMBOL;
  }

  inline void Encode1Byte(const uint8_t* in, char* out)
  {
    const uint_t buf = *in;
    out[0] = ENCODE_TABLE[(buf >> 2) & 63];
    out[1] = ENCODE_TABLE[(buf << 4) & 63];
    out[2] = STUB_SYMBOL;
    out[3] = STUB_SYMBOL;
  }

  using StrIterator = RangeIterator<const char*>;

  inline uint8_t ReadAcceptableChar(StrIterator& in)
  {
    for (;;)
    {
      Require(in);
      const uint_t c = *in;
      ++in;
      Require(c < 128);
      const uint8_t st = DECODE_TABLE[c];
      Require(st != INVALID);
      if (st != SKIPPED)
      {
        return st;
      }
    }
  }

  inline uint8_t ReadSignificantChar(StrIterator& in)
  {
    const uint8_t c = ReadAcceptableChar(in);
    Require(c != PADDING);
    return c;
  }

  inline uint_t ReadGroup(StrIterator& in, uint_t chars)
  {
    uint32_t buf = 0;
    for (uint_t idx = 0; idx != chars; ++idx)
    {
      buf <<= 6;
      buf |= ReadSignificantChar(in);
    }
    return buf;
  }

  inline void Decode3Bytes(StrIterator& in, uint8_t* out)
  {
    const uint32_t buf = ReadGroup(in, TXT_GROUP_SIZE);
    out[0] = static_cast<uint8_t>(buf >> 16);
    out[1] = static_cast<uint8_t>(buf >> 8);
    out[2] = static_cast<uint8_t>(buf >> 0);
  }

  inline void Decode2Bytes(StrIterator& in, uint8_t* out)
  {
    const uint32_t buf = ReadGroup(in, TXT_GROUP_SIZE - 1);
    Require(PADDING == ReadAcceptableChar(in));
    out[0] = static_cast<uint8_t>(buf >> 10);
    out[1] = static_cast<uint8_t>(buf >> 2);
  }

  inline void Decode1Byte(StrIterator& in, uint8_t* out)
  {
    const uint32_t buf = ReadGroup(in, TXT_GROUP_SIZE - 2);
    Require(PADDING == ReadAcceptableChar(in));
    Require(PADDING == ReadAcceptableChar(in));
    out[0] = static_cast<uint8_t>(buf >> 4);
  }
}  // namespace Binary::Base64

namespace Binary::Base64
{
  std::size_t CalculateConvertedSize(std::size_t size)
  {
    return TXT_GROUP_SIZE * ((size + BIN_GROUP_SIZE - 1) / BIN_GROUP_SIZE);
  }

  char* Encode(const uint8_t* inBegin, const uint8_t* inEnd, char* outBegin, const char* outEnd)
  {
    std::size_t rest = inEnd - inBegin;
    const uint8_t* in = inBegin;
    char* out = outBegin;
    Require(outEnd - outBegin >= static_cast<std::ptrdiff_t>(CalculateConvertedSize(rest)));
    while (rest >= BIN_GROUP_SIZE)
    {
      Encode3Bytes(in, out);
      in += BIN_GROUP_SIZE;
      out += TXT_GROUP_SIZE;
      rest -= BIN_GROUP_SIZE;
    }
    if (2 == rest)
    {
      Encode2Bytes(in, out);
      out += TXT_GROUP_SIZE;
    }
    else if (1 == rest)
    {
      Encode1Byte(in, out);
      out += TXT_GROUP_SIZE;
    }
    return out;
  }

  uint8_t* Decode(const char* inBegin, const char* inEnd, uint8_t* outBegin, const uint8_t* outEnd)
  {
    StrIterator in(inBegin, inEnd);
    uint8_t* out = outBegin;
    std::size_t rest = outEnd - outBegin;
    while (rest >= BIN_GROUP_SIZE)
    {
      Decode3Bytes(in, out);
      out += BIN_GROUP_SIZE;
      rest -= BIN_GROUP_SIZE;
    }
    if (2 == rest)
    {
      Decode2Bytes(in, out);
    }
    else if (1 == rest)
    {
      Decode1Byte(in, out);
    }
    out += rest;
    return out;
  }

  Dump Decode(StringView input)
  {
    const std::size_t inSize = input.size();
    Require(0 == inSize % TXT_GROUP_SIZE);
    const std::size_t padPos = input.find(STUB_SYMBOL);
    const std::size_t outSize =
        BIN_GROUP_SIZE * (inSize / TXT_GROUP_SIZE) - (padPos == inSize - 1) - 2 * (padPos == inSize - 2);
    const char* in = input.data();
    Dump result(outSize);
    uint8_t* out = result.data();
    Decode(in, in + inSize, out, out + outSize);
    return result;
  }
}  // namespace Binary::Base64
