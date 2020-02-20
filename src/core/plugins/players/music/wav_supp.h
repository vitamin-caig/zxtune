/**
* 
* @file
*
* @brief  WAV player internals interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <binary/data.h>
#include <binary/view.h>
#include <sound/chunk.h>
#include <time/duration.h>

namespace Module
{
namespace Wav
{
  struct Properties
  {
    uint_t Frequency = 0;
    uint_t Channels = 0;
    uint_t Bits = 0;
    uint_t BlockSize = 0;
    uint_t BlockSizeSamples = 0;
    uint_t SamplesCountHint = 0;
    Time::Microseconds FrameDuration;
    Binary::Data::Ptr Data;
  };

  class Model
  {
  public:
    using Ptr = std::shared_ptr<const Model>;
    virtual ~Model() = default;
    
    virtual uint_t GetFrequency() const = 0;
    virtual uint_t GetFramesCount() const = 0;
    virtual uint_t GetSamplesPerFrame() const = 0;
    virtual Sound::Chunk RenderFrame(uint_t idx) const = 0;
  };
  
  Model::Ptr CreatePcmModel(const Properties& props);
  Model::Ptr CreateFloatPcmModel(const Properties& props);
  Model::Ptr CreateAdpcmModel(const Properties& props);
  Model::Ptr CreateImaAdpcmModel(const Properties& props);
  Model::Ptr CreateAtrac3Model(const Properties& props, Binary::View extraData);
  Model::Ptr CreateAtrac3PlusModel(const Properties& props);
  Model::Ptr CreateAtrac9Model(const Properties& props, Binary::View extraData);
}
}
