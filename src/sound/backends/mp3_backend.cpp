/**
 *
 * @file
 *
 * @brief  MP3 backend implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "sound/backends/file_backend.h"
#include "sound/backends/gates/mp3_api.h"
#include "sound/backends/l10n.h"
#include "sound/backends/mp3.h"
#include "sound/backends/storage.h"

#include "binary/dump.h"
#include "debug/log.h"
#include "math/numeric.h"
#include "sound/backends_parameters.h"
#include "sound/render_params.h"

#include "error_tools.h"
#include "make_ptr.h"

namespace Sound::Mp3
{
  const Debug::Stream Dbg("Sound::Backend::Mp3");

  const uint_t BITRATE_MIN = 32;
  const uint_t BITRATE_MAX = 320;
  const uint_t QUALITY_MIN = 0;
  const uint_t QUALITY_MAX = 9;

  using LameContextPtr = std::shared_ptr<lame_global_flags>;

  void CheckLameCall(int res, Error::LocationRef loc)
  {
    if (res < 0)
    {
      throw MakeFormattedError(loc, translate("Error in MP3 backend: {}."), res);
    }
  }

  const std::size_t INITIAL_ENCODED_BUFFER_SIZE = 1048576;

  class FileStream : public Sound::FileStream
  {
  public:
    FileStream(Api::Ptr api, LameContextPtr context, Binary::OutputStream::Ptr stream)
      : LameApi(std::move(api))
      , Stream(std::move(stream))
      , Context(std::move(context))
      , Encoded(INITIAL_ENCODED_BUFFER_SIZE)
    {
      Dbg("Stream initialized");
    }

    void SetTitle(const String& title) override
    {
      LameApi->id3tag_set_title(Context.get(), title.c_str());
    }

    void SetAuthor(const String& author) override
    {
      LameApi->id3tag_set_artist(Context.get(), author.c_str());
    }

    void SetComment(const String& comment) override
    {
      LameApi->id3tag_set_comment(Context.get(), comment.c_str());
    }

    void FlushMetadata() override {}

    void ApplyData(Chunk data) override
    {
      // work with 16-bit
      static_assert(Sample::BITS == 16, "Incompatible sound sample bits count");
      static_assert(Sample::CHANNELS == 2, "Incompatible sound channels count");
      data.ToS16();
      while (const int res = LameApi->lame_encode_buffer_interleaved(
                 Context.get(), safe_ptr_cast<short int*>(data.data()), data.size(), Encoded.data(), Encoded.size()))
      {
        if (res > 0)  // encoded
        {
          Stream->ApplyData(Binary::View(Encoded.data(), res));
          break;
        }
        else if (-1 == res)  // buffer too small
        {
          ResizeBuffer();
        }
        else
        {
          CheckLameCall(res, THIS_LINE);
        }
      }
    }

    void Flush() override
    {
      while (const int res = LameApi->lame_encode_flush(Context.get(), Encoded.data(), Encoded.size()))
      {
        if (res > 0)
        {
          Stream->ApplyData(Binary::View(Encoded.data(), res));
          break;
        }
        else if (-1 == res)  // buffer too small
        {
          ResizeBuffer();
        }
        else
        {
          CheckLameCall(res, THIS_LINE);
        }
      }
      Dbg("Stream flushed");
    }

  private:
    void ResizeBuffer()
    {
      Encoded.resize(Encoded.size() * 2);
      Dbg("Increase buffer to {} bytes", Encoded.size());
    }

  private:
    const Api::Ptr LameApi;
    const Binary::OutputStream::Ptr Stream;
    const LameContextPtr Context;
    Binary::Dump Encoded;
  };

  enum class BitrateMode
  {
    CBR,
    VBR,
    ABR
  };

  enum class ChannelsMode
  {
    DEFAULT,
    STEREO,
    JOINTSTEREO,
    MONO
  };

  class StreamParameters
  {
  public:
    explicit StreamParameters(Parameters::Accessor::Ptr params)
      : Params(std::move(params))
    {}

    BitrateMode GetBitrateMode() const
    {
      using namespace Parameters::ZXTune::Sound::Backends::Mp3;
      const auto mode = Parameters::GetString(*Params, MODE, MODE_DEFAULT);
      if (mode == MODE_CBR)
      {
        return BitrateMode::CBR;
      }
      else if (mode == MODE_VBR)
      {
        return BitrateMode::VBR;
      }
      else if (mode == MODE_ABR)
      {
        return BitrateMode::ABR;
      }
      else
      {
        throw MakeFormattedError(THIS_LINE, translate("MP3 backend error: invalid bitrate mode '{}'."), mode);
      }
    }

    uint_t GetBitrate() const
    {
      using namespace Parameters::ZXTune::Sound::Backends::Mp3;
      const auto bitrate = Parameters::GetInteger<uint_t>(*Params, BITRATE, BITRATE_DEFAULT);
      if (!Math::InRange(bitrate, BITRATE_MIN, BITRATE_MAX))
      {
        throw MakeFormattedError(THIS_LINE, translate("MP3 backend error: bitrate ({0}) is out of range ({1}..{2})."),
                                 bitrate, BITRATE_MIN, BITRATE_MAX);
      }
      return bitrate;
    }

    uint_t GetQuality() const
    {
      using namespace Parameters::ZXTune::Sound::Backends::Mp3;
      const auto quality = Parameters::GetInteger<uint_t>(*Params, QUALITY, QUALITY_DEFAULT);
      if (!Math::InRange(quality, QUALITY_MIN, QUALITY_MAX))
      {
        throw MakeFormattedError(THIS_LINE, translate("MP3 backend error: quality ({0}) is out of range ({1}..{2})."),
                                 quality, QUALITY_MIN, QUALITY_MAX);
      }
      return quality;
    }

    ChannelsMode GetChannelsMode() const
    {
      using namespace Parameters::ZXTune::Sound::Backends::Mp3;
      const auto mode = Parameters::GetString(*Params, CHANNELS, CHANNELS_DEFAULT);
      if (mode == CHANNELS_DEFAULT)
      {
        return ChannelsMode::DEFAULT;
      }
      else if (mode == CHANNELS_STEREO)
      {
        return ChannelsMode::STEREO;
      }
      else if (mode == CHANNELS_JOINTSTEREO)
      {
        return ChannelsMode::JOINTSTEREO;
      }
      else if (mode == CHANNELS_MONO)
      {
        return ChannelsMode::MONO;
      }
      else
      {
        throw MakeFormattedError(THIS_LINE, translate("MP3 backend error: invalid channels mode '{}'."), mode);
      }
    }

  private:
    const Parameters::Accessor::Ptr Params;
  };

  class FileStreamFactory : public Sound::FileStreamFactory
  {
  public:
    FileStreamFactory(Api::Ptr api, Parameters::Accessor::Ptr params)
      : LameApi(std::move(api))
      , Params(std::move(params))
    {}

    BackendId GetId() const override
    {
      return BACKEND_ID;
    }

    FileStream::Ptr CreateStream(Binary::OutputStream::Ptr stream) const override
    {
      auto context = LameContextPtr(LameApi->lame_init(), [api = LameApi](auto&& arg) { api->lame_close(arg); });
      SetupContext(*context);
      return MakePtr<FileStream>(LameApi, std::move(context), std::move(stream));
    }

  private:
    void SetupContext(lame_global_flags& ctx) const
    {
      const StreamParameters stream(Params);

      const uint_t samplerate = GetSoundFrequency(*Params);
      Dbg("Setting samplerate to {}Hz", samplerate);
      CheckLameCall(LameApi->lame_set_in_samplerate(&ctx, samplerate), THIS_LINE);
      CheckLameCall(LameApi->lame_set_out_samplerate(&ctx, samplerate), THIS_LINE);
      CheckLameCall(LameApi->lame_set_num_channels(&ctx, Sample::CHANNELS), THIS_LINE);
      CheckLameCall(LameApi->lame_set_bWriteVbrTag(&ctx, true), THIS_LINE);
      switch (stream.GetBitrateMode())
      {
      case BitrateMode::CBR:
      {
        const uint_t bitrate = stream.GetBitrate();
        Dbg("Setting bitrate to {}kbps", bitrate);
        CheckLameCall(LameApi->lame_set_VBR(&ctx, vbr_off), THIS_LINE);
        CheckLameCall(LameApi->lame_set_brate(&ctx, bitrate), THIS_LINE);
      }
      break;
      case BitrateMode::ABR:
      {
        const uint_t bitrate = stream.GetBitrate();
        Dbg("Setting average bitrate to {}kbps", bitrate);
        CheckLameCall(LameApi->lame_set_VBR(&ctx, vbr_abr), THIS_LINE);
        CheckLameCall(LameApi->lame_set_VBR_mean_bitrate_kbps(&ctx, bitrate), THIS_LINE);
      }
      break;
      case BitrateMode::VBR:
      {
        const uint_t quality = stream.GetQuality();
        Dbg("Setting VBR quality to {}", quality);
        CheckLameCall(LameApi->lame_set_VBR(&ctx, vbr_default), THIS_LINE);
        CheckLameCall(LameApi->lame_set_VBR_q(&ctx, quality), THIS_LINE);
      }
      break;
      default:
        assert(!"Invalid mode");
      }
      switch (stream.GetChannelsMode())
      {
      case ChannelsMode::DEFAULT:
        Dbg("Using default channels mode");
        break;
      case ChannelsMode::STEREO:
        Dbg("Using stereo mode");
        CheckLameCall(LameApi->lame_set_mode(&ctx, STEREO), THIS_LINE);
        break;
      case ChannelsMode::JOINTSTEREO:
        Dbg("Using joint stereo mode");
        CheckLameCall(LameApi->lame_set_mode(&ctx, JOINT_STEREO), THIS_LINE);
        break;
      case ChannelsMode::MONO:
        Dbg("Using mono mode");
        CheckLameCall(LameApi->lame_set_mode(&ctx, MONO), THIS_LINE);
        break;
      default:
        assert(!"Invalid mode");
      }
      CheckLameCall(LameApi->lame_init_params(&ctx), THIS_LINE);
      LameApi->id3tag_init(&ctx);
    }

  private:
    const Api::Ptr LameApi;
    const Parameters::Accessor::Ptr Params;
  };

  class BackendWorkerFactory : public Sound::BackendWorkerFactory
  {
  public:
    explicit BackendWorkerFactory(Api::Ptr api)
      : LameApi(std::move(api))
    {}

    BackendWorker::Ptr CreateWorker(Parameters::Accessor::Ptr params, Module::Holder::Ptr holder) const override
    {
      auto factory = MakePtr<FileStreamFactory>(LameApi, params);
      return CreateFileBackendWorker(std::move(params), holder->GetModuleProperties(), std::move(factory));
    }

  private:
    const Api::Ptr LameApi;
  };
}  // namespace Sound::Mp3

namespace Sound
{
  void RegisterMp3Backend(BackendsStorage& storage)
  {
    try
    {
      auto api = Mp3::LoadDynamicApi();
      Mp3::Dbg("Detected LAME library {}", api->get_lame_version());
      auto factory = MakePtr<Mp3::BackendWorkerFactory>(std::move(api));
      storage.Register(Mp3::BACKEND_ID, Mp3::BACKEND_DESCRIPTION, CAP_TYPE_FILE, std::move(factory));
    }
    catch (const Error& e)
    {
      storage.Register(Mp3::BACKEND_ID, Mp3::BACKEND_DESCRIPTION, CAP_TYPE_FILE, e);
    }
  }
}  // namespace Sound
