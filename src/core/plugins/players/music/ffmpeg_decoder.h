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

//library includes
#include <binary/view.h>
#include <sound/chunk.h>
//std includes
#include <memory>

namespace Module
{
namespace FFmpeg
{
  class Decoder
  {
  public:
    using Ptr = std::unique_ptr<Decoder>;
    virtual ~Decoder() = default;

    virtual void Decode(Binary::View frame, Sound::Chunk* output = nullptr) = 0;
  };
}
}
