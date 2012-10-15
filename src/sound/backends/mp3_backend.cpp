/*
Abstract:
  Mp3 file backend implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "mp3_api.h"
#include "enumerator.h"
#include "file_backend.h"
//common includes
#include <debug_log.h>
#include <error_tools.h>
#include <tools.h>
//library includes
#include <binary/data_adapter.h>
#include <l10n/api.h>
#include <math/numeric.h>
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/render_params.h>
//std includes
#include <algorithm>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <sound/text/backends.h>

#define FILE_TAG 3B251603

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const Debug::Stream Dbg("Sound::Backend::Mp3");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound");

  const uint_t BITRATE_MIN = 32;
  const uint_t BITRATE_MAX = 320;
  const uint_t QUALITY_MIN = 0;
  const uint_t QUALITY_MAX = 9;

  typedef boost::shared_ptr<lame_global_flags> LameContextPtr;

  void CheckLameCall(int res, Error::LocationRef loc)
  {
    if (res < 0)
    {
      throw MakeFormattedError(loc, translate("Error in MP3 backend: %1%."), res);
    }
  }

  const std::size_t INITIAL_ENCODED_BUFFER_SIZE = 1048576;

  //work with 16-bit
  BOOST_STATIC_ASSERT(sizeof(Sample) == 2);

  const bool SamplesShouldBeConverted = !SAMPLE_SIGNED;

  class Mp3Stream : public FileStream
  {
  public:
    Mp3Stream(Mp3::Api::Ptr api, LameContextPtr context, Binary::OutputStream::Ptr stream)
      : Api(api)
      , Stream(stream)
      , Context(context)
      , Encoded(INITIAL_ENCODED_BUFFER_SIZE)
    {
      Dbg("Stream initialized");
    }

    virtual void SetTitle(const String& title)
    {
      const std::string titleC = title;//TODO
      Api->id3tag_set_title(Context.get(), titleC.c_str());
    }

    virtual void SetAuthor(const String& author)
    {
      const std::string authorC = author;//TODO
      Api->id3tag_set_artist(Context.get(), authorC.c_str());
    }

    virtual void SetComment(const String& comment)
    {
      const std::string commentC = comment;//TODO
      Api->id3tag_set_comment(Context.get(), commentC.c_str());
    }

    virtual void FlushMetadata()
    {
    }

    virtual void ApplyData(const ChunkPtr& data)
    {
      if (SamplesShouldBeConverted)
      {
        std::transform(data->front().begin(), data->back().end(), data->front().begin(), &ToSignedSample);
      }
      while (const int res = Api->lame_encode_buffer_interleaved(Context.get(),
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

    virtual void Flush()
    {
      while (const int res = Api->lame_encode_flush(Context.get(), &Encoded[0], Encoded.size()))
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
    const Mp3::Api::Ptr Api;
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

  class Mp3Parameters
  {
  public:
    explicit Mp3Parameters(Parameters::Accessor::Ptr params)
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

  const String ID = Text::MP3_BACKEND_ID;
  const char* const DESCRIPTION = L10n::translate("MP3 support backend");

  class Mp3FileFactory : public FileStreamFactory
  {
  public:
    Mp3FileFactory(Mp3::Api::Ptr api, Parameters::Accessor::Ptr params)
      : Api(api)
      , Params(params)
      , RenderingParameters(RenderParameters::Create(params))
    {
    }

    virtual String GetId() const
    {
      return ID;
    }

    virtual FileStream::Ptr CreateStream(Binary::OutputStream::Ptr stream) const
    {
      const LameContextPtr context = LameContextPtr(Api->lame_init(), boost::bind(&Mp3::Api::lame_close, Api, _1));
      SetupContext(*context);
      return boost::make_shared<Mp3Stream>(Api, context, stream);
    }
  private:
    void SetupContext(lame_global_flags& ctx) const
    {
      const uint_t samplerate = RenderingParameters->SoundFreq();
      Dbg("Setting samplerate to %1%Hz", samplerate);
      CheckLameCall(Api->lame_set_in_samplerate(&ctx, samplerate), THIS_LINE);
      CheckLameCall(Api->lame_set_out_samplerate(&ctx, samplerate), THIS_LINE);
      CheckLameCall(Api->lame_set_num_channels(&ctx, OUTPUT_CHANNELS), THIS_LINE);
      CheckLameCall(Api->lame_set_bWriteVbrTag(&ctx, true), THIS_LINE);
      switch (Params.GetBitrateMode())
      {
      case MODE_CBR:
        {
          const uint_t bitrate = Params.GetBitrate();
          Dbg("Setting bitrate to %1%kbps", bitrate);
          CheckLameCall(Api->lame_set_VBR(&ctx, vbr_off), THIS_LINE);
          CheckLameCall(Api->lame_set_brate(&ctx, bitrate), THIS_LINE);
        }
        break;
      case MODE_ABR:
        {
          const uint_t bitrate = Params.GetBitrate();
          Dbg("Setting average bitrate to %1%kbps", bitrate);
          CheckLameCall(Api->lame_set_VBR(&ctx, vbr_abr), THIS_LINE);
          CheckLameCall(Api->lame_set_VBR_mean_bitrate_kbps(&ctx, bitrate), THIS_LINE);
        }
        break;
      case MODE_VBR:
        {
          const uint_t quality = Params.GetQuality();
          Dbg("Setting VBR quality to %1%", quality);
          CheckLameCall(Api->lame_set_VBR(&ctx, vbr_default), THIS_LINE);
          CheckLameCall(Api->lame_set_VBR_q(&ctx, quality), THIS_LINE);
        }
        break;
      default:
        assert(!"Invalid mode");
      }
      switch (Params.GetChannelsMode())
      {
      case MODE_DEFAULT:
        Dbg("Using default channels mode");
        break;
      case MODE_STEREO:
        Dbg("Using stereo mode");
        CheckLameCall(Api->lame_set_mode(&ctx, STEREO), THIS_LINE);
        break;
      case MODE_JOINTSTEREO:
        Dbg("Using joint stereo mode");
        CheckLameCall(Api->lame_set_mode(&ctx, JOINT_STEREO), THIS_LINE);
        break;
      case MODE_MONO:
        Dbg("Using mono mode");
        CheckLameCall(Api->lame_set_mode(&ctx, MONO), THIS_LINE);
        break;
      default:
        assert(!"Invalid mode");
      }
      CheckLameCall(Api->lame_init_params(&ctx), THIS_LINE);
      Api->id3tag_init(&ctx);
    }
  private:
    const Mp3::Api::Ptr Api;
    const Mp3Parameters Params;
    const RenderParameters::Ptr RenderingParameters;
  };

  class Mp3BackendCreator : public BackendCreator
  {
  public:
    explicit Mp3BackendCreator(Mp3::Api::Ptr api)
      : Api(api)
    {
    }

    virtual String Id() const
    {
      return ID;
    }

    virtual String Description() const
    {
      return translate(DESCRIPTION);
    }

    virtual uint_t Capabilities() const
    {
      return CAP_TYPE_FILE;
    }

    virtual Error Status() const
    {
      return Error();
    }

    virtual Backend::Ptr CreateBackend(CreateBackendParameters::Ptr params) const
    {
      try
      {
        const Parameters::Accessor::Ptr allParams = params->GetParameters();
        const FileStreamFactory::Ptr factory = boost::make_shared<Mp3FileFactory>(Api, allParams);
        const BackendWorker::Ptr worker = CreateFileBackendWorker(allParams, factory);
        return Sound::CreateBackend(params, worker);
      }
      catch (const Error& e)
      {
        throw MakeFormattedError(THIS_LINE,
          translate("Failed to create backend '%1%'."), Id()).AddSuberror(e);
      }
    }
  private:
    const Mp3::Api::Ptr Api;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    void RegisterMp3Backend(BackendsEnumerator& enumerator)
    {
      try
      {
        const Mp3::Api::Ptr api = Mp3::LoadDynamicApi();
        Dbg("Detected LAME library %1%", api->get_lame_version());
        const BackendCreator::Ptr creator = boost::make_shared<Mp3BackendCreator>(api);
        enumerator.RegisterCreator(creator);
      }
      catch (const Error& e)
      {
        enumerator.RegisterCreator(CreateUnavailableBackendStub(ID, DESCRIPTION, CAP_TYPE_FILE, e));
      }
    }
  }
}
