/**
*
* @file
*
* @brief  MP3 backend implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "file_backend.h"
#include "storage.h"
#include "gates/mp3_api.h"
//common includes
#include <error_tools.h>
#include <make_ptr.h>
//library includes
#include <binary/data_adapter.h>
#include <debug/log.h>
#include <l10n/api.h>
#include <math/numeric.h>
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/render_params.h>
//std includes
#include <algorithm>
//boost includes
#include <boost/bind.hpp>
//text includes
#include "text/backends.h"

#define FILE_TAG 3B251603

namespace
{
  const Debug::Stream Dbg("Sound::Backend::Mp3");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound_backends");
}

namespace Sound
{
namespace Mp3
{
  const String ID = Text::MP3_BACKEND_ID;
  const char* const DESCRIPTION = L10n::translate("MP3 support backend");

  const uint_t BITRATE_MIN = 32;
  const uint_t BITRATE_MAX = 320;
  const uint_t QUALITY_MIN = 0;
  const uint_t QUALITY_MAX = 9;

  typedef std::shared_ptr<lame_global_flags> LameContextPtr;

  void CheckLameCall(int res, Error::LocationRef loc)
  {
    if (res < 0)
    {
      throw MakeFormattedError(loc, translate("Error in MP3 backend: %1%."), res);
    }
  }

  const std::size_t INITIAL_ENCODED_BUFFER_SIZE = 1048576;

  class FileStream : public Sound::FileStream
  {
  public:
    FileStream(Api::Ptr api, LameContextPtr context, Binary::OutputStream::Ptr stream)
      : LameApi(api)
      , Stream(stream)
      , Context(context)
      , Encoded(INITIAL_ENCODED_BUFFER_SIZE)
    {
      Dbg("Stream initialized");
    }

    void SetTitle(const String& title) override
    {
      const std::string titleC = title;//TODO
      LameApi->id3tag_set_title(Context.get(), titleC.c_str());
    }

    void SetAuthor(const String& author) override
    {
      const std::string authorC = author;//TODO
      LameApi->id3tag_set_artist(Context.get(), authorC.c_str());
    }

    void SetComment(const String& comment) override
    {
      const std::string commentC = comment;//TODO
      LameApi->id3tag_set_comment(Context.get(), commentC.c_str());
    }

    void FlushMetadata() override
    {
    }

    void ApplyData(const Chunk::Ptr& data) override
    {
      //work with 16-bit
      static_assert(Sample::BITS == 16, "Incompatible sound sample bits count");
      static_assert(Sample::CHANNELS == 2, "Incompatible sound channels count");
      data->ToS16();
      while (const int res = LameApi->lame_encode_buffer_interleaved(Context.get(),
        safe_ptr_cast<short int*>(&data->front()), data->size(), &Encoded[0], Encoded.size()))
      {
        if (res > 0) //encoded
        {
          Stream->ApplyData(Binary::DataAdapter(&Encoded[0], res));
          break;
        }
        else if (-1 == res)//buffer too small
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
      while (const int res = LameApi->lame_encode_flush(Context.get(), &Encoded[0], Encoded.size()))
      {
        if (res > 0)
        {
          Stream->ApplyData(Binary::DataAdapter(&Encoded[0], res));
          break;
        }
        else if (-1 == res)//buffer too small
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
      Dbg("Increase buffer to %1% bytes", Encoded.size());
    }
  private:
    const Api::Ptr LameApi;
    const Binary::OutputStream::Ptr Stream;
    const LameContextPtr Context;
    Dump Encoded;
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
      : Params(params)
    {
    }

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
        throw MakeFormattedError(THIS_LINE,
          translate("MP3 backend error: invalid bitrate mode '%1%'."), mode);
      }
    }

    uint_t GetBitrate() const
    {
      Parameters::IntType bitrate = Parameters::ZXTune::Sound::Backends::Mp3::BITRATE_DEFAULT;
      if (Params->FindValue(Parameters::ZXTune::Sound::Backends::Mp3::BITRATE, bitrate) &&
        !Math::InRange<Parameters::IntType>(bitrate, BITRATE_MIN, BITRATE_MAX))
      {
        throw MakeFormattedError(THIS_LINE,
          translate("MP3 backend error: bitrate (%1%) is out of range (%2%..%3%)."), static_cast<int_t>(bitrate), BITRATE_MIN, BITRATE_MAX);
      }
      return static_cast<uint_t>(bitrate);
    }

    uint_t GetQuality() const
    {
      Parameters::IntType quality = Parameters::ZXTune::Sound::Backends::Mp3::QUALITY_DEFAULT;
      if (Params->FindValue(Parameters::ZXTune::Sound::Backends::Mp3::QUALITY, quality) &&
        !Math::InRange<Parameters::IntType>(quality, QUALITY_MIN, QUALITY_MAX))
      {
        throw MakeFormattedError(THIS_LINE,
          translate("MP3 backend error: quality (%1%) is out of range (%2%..%3%)."), static_cast<int_t>(quality), QUALITY_MIN, QUALITY_MAX);
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
        throw MakeFormattedError(THIS_LINE,
          translate("MP3 backend error: invalid channels mode '%1%'."), mode);
      }
    }
  private:
    const Parameters::Accessor::Ptr Params;
  };

  class FileStreamFactory : public Sound::FileStreamFactory
  {
  public:
    FileStreamFactory(Api::Ptr api, Parameters::Accessor::Ptr params)
      : LameApi(api)
      , Params(params)
    {
    }

    String GetId() const override
    {
      return ID;
    }

    FileStream::Ptr CreateStream(Binary::OutputStream::Ptr stream) const override
    {
      const LameContextPtr context = LameContextPtr(LameApi->lame_init(), boost::bind(&Api::lame_close, LameApi, _1));
      SetupContext(*context);
      return MakePtr<FileStream>(LameApi, context, stream);
    }
  private:
    void SetupContext(lame_global_flags& ctx) const
    {
      const StreamParameters stream(Params);
      const RenderParameters::Ptr sound = RenderParameters::Create(Params);

      const uint_t samplerate = sound->SoundFreq();
      Dbg("Setting samplerate to %1%Hz", samplerate);
      CheckLameCall(LameApi->lame_set_in_samplerate(&ctx, samplerate), THIS_LINE);
      CheckLameCall(LameApi->lame_set_out_samplerate(&ctx, samplerate), THIS_LINE);
      CheckLameCall(LameApi->lame_set_num_channels(&ctx, Sample::CHANNELS), THIS_LINE);
      CheckLameCall(LameApi->lame_set_bWriteVbrTag(&ctx, true), THIS_LINE);
      switch (stream.GetBitrateMode())
      {
      case MODE_CBR:
        {
          const uint_t bitrate = stream.GetBitrate();
          Dbg("Setting bitrate to %1%kbps", bitrate);
          CheckLameCall(LameApi->lame_set_VBR(&ctx, vbr_off), THIS_LINE);
          CheckLameCall(LameApi->lame_set_brate(&ctx, bitrate), THIS_LINE);
        }
        break;
      case MODE_ABR:
        {
          const uint_t bitrate = stream.GetBitrate();
          Dbg("Setting average bitrate to %1%kbps", bitrate);
          CheckLameCall(LameApi->lame_set_VBR(&ctx, vbr_abr), THIS_LINE);
          CheckLameCall(LameApi->lame_set_VBR_mean_bitrate_kbps(&ctx, bitrate), THIS_LINE);
        }
        break;
      case MODE_VBR:
        {
          const uint_t quality = stream.GetQuality();
          Dbg("Setting VBR quality to %1%", quality);
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
      : FlacApi(api)
    {
    }

    BackendWorker::Ptr CreateWorker(Parameters::Accessor::Ptr params, Module::Holder::Ptr /*holder*/) const override
    {
      const FileStreamFactory::Ptr factory = MakePtr<FileStreamFactory>(FlacApi, params);
      return CreateFileBackendWorker(params, factory);
    }
  private:
    const Api::Ptr FlacApi;
  };
}//Mp3
}//Sound

namespace Sound
{
  void RegisterMp3Backend(BackendsStorage& storage)
  {
    try
    {
      const Mp3::Api::Ptr api = Mp3::LoadDynamicApi();
      Dbg("Detected LAME library %1%", api->get_lame_version());
      const BackendWorkerFactory::Ptr factory = MakePtr<Mp3::BackendWorkerFactory>(api);
      storage.Register(Mp3::ID, Mp3::DESCRIPTION, CAP_TYPE_FILE, factory);
    }
    catch (const Error& e)
    {
      storage.Register(Mp3::ID, Mp3::DESCRIPTION, CAP_TYPE_FILE, e);
    }
  }
}
