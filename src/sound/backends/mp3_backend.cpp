/**
 *
 * @file
 *
 * @brief  MP3 backend implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound/backends/file_backend.h"
#include "sound/backends/gates/mp3_api.h"
#include "sound/backends/l10n.h"
#include "sound/backends/mp3.h"
#include "sound/backends/storage.h"
// common includes
#include <error_tools.h>
#include <make_ptr.h>
// library includes
#include <binary/dump.h>
#include <debug/log.h>
#include <math/numeric.h>
#include <sound/backends_parameters.h>
#include <sound/render_params.h>
// std includes
#include <algorithm>
#include <functional>

namespace Sound::Mp3
{
  const Debug::Stream Dbg("Sound::Backend::Mp3");

  const uint_t BITRATE_MIN = 32;
  const uint_t BITRATE_MAX = 320;
  const uint_t QUALITY_MIN = 0;
  const uint_t QUALITY_MAX = 9;

  typedef std::shared_ptr<lame_global_flags> LameContextPtr;

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

  enum BitrateMode
  {
    MODE_CBR,
    MODE_VBR,
    MODE_ABR
  };

  enum ChannelsMode
  {
    MODE_DEFAULT,
    MODE_STEREO,
    MODE_JOINTSTEREO,
    MODE_MONO
  };

  class StreamParameters
  {
  public:
    explicit StreamParameters(Parameters::Accessor::Ptr params)
      : Params(std::move(params))
    {}

    BitrateMode GetBitrateMode() const
    {
      Parameters::StringType mode = Parameters::ZXTune::Sound::Backends::Mp3::MODE_DEFAULT;
      Params->FindValue(Parameters::ZXTune::Sound::Backends::Mp3::MODE, mode);
      if (mode == Parameters::ZXTune::Sound::Backends::Mp3::MODE_CBR)
      {
        return MODE_CBR;
      }
      else if (mode == Parameters::ZXTune::Sound::Backends::Mp3::MODE_VBR)
      {
        return MODE_VBR;
      }
      else if (mode == Parameters::ZXTune::Sound::Backends::Mp3::MODE_ABR)
      {
        return MODE_ABR;
      }
      else
      {
        throw MakeFormattedError(THIS_LINE, translate("MP3 backend error: invalid bitrate mode '{}'."), mode);
      }
    }

    uint_t GetBitrate() const
    {
      Parameters::IntType bitrate = Parameters::ZXTune::Sound::Backends::Mp3::BITRATE_DEFAULT;
      if (Params->FindValue(Parameters::ZXTune::Sound::Backends::Mp3::BITRATE, bitrate)
          && !Math::InRange<Parameters::IntType>(bitrate, BITRATE_MIN, BITRATE_MAX))
      {
        throw MakeFormattedError(THIS_LINE, translate("MP3 backend error: bitrate ({0}) is out of range ({1}..{2})."),
                                 static_cast<int_t>(bitrate), BITRATE_MIN, BITRATE_MAX);
      }
      return static_cast<uint_t>(bitrate);
    }

    uint_t GetQuality() const
    {
      Parameters::IntType quality = Parameters::ZXTune::Sound::Backends::Mp3::QUALITY_DEFAULT;
      if (Params->FindValue(Parameters::ZXTune::Sound::Backends::Mp3::QUALITY, quality)
          && !Math::InRange<Parameters::IntType>(quality, QUALITY_MIN, QUALITY_MAX))
      {
        throw MakeFormattedError(THIS_LINE, translate("MP3 backend error: quality ({0}) is out of range ({1}..{2})."),
                                 static_cast<int_t>(quality), QUALITY_MIN, QUALITY_MAX);
      }
      return static_cast<uint_t>(quality);
    }

    ChannelsMode GetChannelsMode() const
    {
      Parameters::StringType mode = Parameters::ZXTune::Sound::Backends::Mp3::CHANNELS_DEFAULT;
      Params->FindValue(Parameters::ZXTune::Sound::Backends::Mp3::CHANNELS, mode);
      if (mode == Parameters::ZXTune::Sound::Backends::Mp3::CHANNELS_DEFAULT)
      {
        return MODE_DEFAULT;
      }
      else if (mode == Parameters::ZXTune::Sound::Backends::Mp3::CHANNELS_STEREO)
      {
        return MODE_STEREO;
      }
      else if (mode == Parameters::ZXTune::Sound::Backends::Mp3::CHANNELS_JOINTSTEREO)
      {
        return MODE_JOINTSTEREO;
      }
      else if (mode == Parameters::ZXTune::Sound::Backends::Mp3::CHANNELS_MONO)
      {
        return MODE_MONO;
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
      const LameContextPtr context =
          LameContextPtr(LameApi->lame_init(), std::bind(&Api::lame_close, LameApi, std::placeholders::_1));
      SetupContext(*context);
      return MakePtr<FileStream>(LameApi, context, stream);
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
      case MODE_CBR:
      {
        const uint_t bitrate = stream.GetBitrate();
        Dbg("Setting bitrate to {}kbps", bitrate);
        CheckLameCall(LameApi->lame_set_VBR(&ctx, vbr_off), THIS_LINE);
        CheckLameCall(LameApi->lame_set_brate(&ctx, bitrate), THIS_LINE);
      }
      break;
      case MODE_ABR:
      {
        const uint_t bitrate = stream.GetBitrate();
        Dbg("Setting average bitrate to {}kbps", bitrate);
        CheckLameCall(LameApi->lame_set_VBR(&ctx, vbr_abr), THIS_LINE);
        CheckLameCall(LameApi->lame_set_VBR_mean_bitrate_kbps(&ctx, bitrate), THIS_LINE);
      }
      break;
      case MODE_VBR:
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
      case MODE_DEFAULT:
        Dbg("Using default channels mode");
        break;
      case MODE_STEREO:
        Dbg("Using stereo mode");
        CheckLameCall(LameApi->lame_set_mode(&ctx, STEREO), THIS_LINE);
        break;
      case MODE_JOINTSTEREO:
        Dbg("Using joint stereo mode");
        CheckLameCall(LameApi->lame_set_mode(&ctx, JOINT_STEREO), THIS_LINE);
        break;
      case MODE_MONO:
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
