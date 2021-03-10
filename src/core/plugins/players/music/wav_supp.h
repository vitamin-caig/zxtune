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

// library includes
#include <binary/data.h>
#include <binary/input_stream.h>
#include <binary/view.h>
#include <sound/chunk.h>

namespace Module::Wav
{
  struct Properties
  {
    uint_t Frequency = 0;
    uint_t Channels = 0;
    uint_t Bits = 0;
    uint_t BlockSize = 0;
    uint_t BlockSizeSamples = 1;
    uint_t SamplesCountHint = 0;
    Binary::Data::Ptr Data;
  };

  class Model
  {
  public:
    using Ptr = std::shared_ptr<Model>;
    virtual ~Model() = default;

    virtual uint_t GetSamplerate() const = 0;
    virtual uint64_t GetTotalSamples() const = 0;

    virtual Sound::Chunk RenderNextFrame() = 0;
    virtual uint64_t Seek(uint64_t sample) = 0;
  };

  class BlockingModel : public Model
  {
  public:
    BlockingModel(Properties props)
      : Props(props)
      , Stream(*Props.Data)
    {
      Require(Props.BlockSize != 0);
      Require(Props.BlockSizeSamples != 0);
    }

    uint_t GetSamplerate() const override
    {
      return Props.Frequency;
    }

    uint64_t GetTotalSamples() const override
    {
      return Props.SamplesCountHint ? Props.SamplesCountHint
                                    : Props.BlockSizeSamples * (Props.Data->Size() / Props.BlockSize);
    }

    uint64_t Seek(uint64_t request) override
    {
      const auto block = request / Props.BlockSizeSamples;
      Stream.Seek(block * Props.BlockSize);
      return block * Props.BlockSizeSamples;
    }

  protected:
    const Properties Props;
    Binary::DataInputStream Stream;
  };

  Model::Ptr CreatePcmModel(Properties props);
  Model::Ptr CreateFloatPcmModel(Properties props);
  Model::Ptr CreateAdpcmModel(Properties props);
  Model::Ptr CreateImaAdpcmModel(Properties props);
  Model::Ptr CreateAtrac3Model(Properties props, Binary::View extraData);
  Model::Ptr CreateAtrac3PlusModel(Properties props);
  Model::Ptr CreateAtrac9Model(Properties props, Binary::View extraData);
}  // namespace Module::Wav
