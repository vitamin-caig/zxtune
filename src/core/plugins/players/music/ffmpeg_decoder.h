/**
 *
 * @file
 *
 * @brief  FFmpeg adapter interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <binary/view.h>
#include <sound/chunk.h>

#include <memory>

namespace Module::FFmpeg
{
  class Decoder
  {
  public:
    using Ptr = std::unique_ptr<Decoder>;
    virtual ~Decoder() = default;

    virtual void Decode(Binary::View frame, Sound::Chunk* output = nullptr) = 0;
  };

  Decoder::Ptr CreateAtrac3Decoder(uint_t channels, uint_t blockSize, Binary::View config);
  Decoder::Ptr CreateAtrac3PlusDecoder(uint_t channels, uint_t blockSize);
  Decoder::Ptr CreateAtrac9Decoder(uint_t blockSize, Binary::View config);
}  // namespace Module::FFmpeg
