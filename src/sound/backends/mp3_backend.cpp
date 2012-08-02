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
#include <error_tools.h>
#include <logging.h>
#include <tools.h>
//library includes
#include <io/fs_tools.h>
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/error_codes.h>
#include <sound/render_params.h>
//std includes
#include <algorithm>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <sound/text/backends.h>
#include <sound/text/sound.h>

#define FILE_TAG 3B251603

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const std::string THIS_MODULE("Sound::Backend::Mp3");

  const uint_t BITRATE_MIN = 32;
  const uint_t BITRATE_MAX = 320;
  const uint_t QUALITY_MIN = 0;
  const uint_t QUALITY_MAX = 9;

  typedef boost::shared_ptr<lame_global_flags> LameContextPtr;

  void CheckLameCall(int res, Error::LocationRef loc)
  {
    if (res < 0)
    {
      throw MakeFormattedError(loc, BACKEND_PLATFORM_ERROR, Text::SOUND_ERROR_MP3_BACKEND_ERROR, res);
    }
  }

  const std::size_t INITIAL_ENCODED_BUFFER_SIZE = 1048576;

  //work with 16-bit
  BOOST_STATIC_ASSERT(sizeof(Sample) == 2);

  const bool SamplesShouldBeConverted = !SAMPLE_SIGNED;

  class Mp3Stream : public FileStream
  {
  public:
    Mp3Stream(Mp3::Api::Ptr api, LameContextPtr context, std::auto_ptr<std::ofstream> stream)
      : Api(api)
      , Stream(stream)
      , Context(context)
      , Encoded(INITIAL_ENCODED_BUFFER_SIZE)
    {
      Log::Debug(THIS_MODULE, "Stream initialized");
    }

    virtual void SetTitle(const String& title)
    {
      const std::string titleC = IO::ConvertToFilename(title);
      Api->id3tag_set_title(Context.get(), titleC.c_str());
    }

    virtual void SetAuthor(const String& author)
    {
      const std::string authorC = IO::ConvertToFilename(author);
      Api->id3tag_set_artist(Context.get(), authorC.c_str());
    }

    virtual void SetComment(const String& comment)
    {
      const std::string commentC = IO::ConvertToFilename(comment);
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
          Stream->write(safe_ptr_cast<const char*>(&Encoded[0]), static_cast<std::streamsize>(res));
          break;
        }
        else if (-1 == res)//buffer too small
        {
          Encoded.resize(Encoded.size() * 2);
          Log::Debug(THIS_MODULE, "Increase buffer to %1% bytes", Encoded.size());
        }
        else if (-2 == res)//malloc problem
        {
          throw Error(THIS_LINE, BACKEND_NO_MEMORY, Text::SOUND_ERROR_BACKEND_NO_MEMORY);
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
          Stream->write(safe_ptr_cast<const char*>(&Encoded[0]), static_cast<std::streamsize>(res));
          break;
        }
        else if (-1 == res)//buffer too small
        {
          Encoded.resize(Encoded.size() * 2);
          Log::Debug(THIS_MODULE, "Increase buffer to %1% bytes", Encoded.size());
        }
        else if (-2 == res)//malloc problem
        {
          throw Error(THIS_LINE, BACKEND_NO_MEMORY, Text::SOUND_ERROR_BACKEND_NO_MEMORY);
        }
        else
        {
          CheckLameCall(res, THIS_LINE);
        }
      }
      Log::Debug(THIS_MODULE, "Stream flushed");
    }
  private:
    const Mp3::Api::Ptr Api;
    const std::auto_ptr<std::ofstream> Stream;
    const LameContextPtr Context;
    Dump Encoded;
  };

  enum Mode
  {
    MODE_CBR,
    MODE_VBR,
    MODE_ABR
  };

  class Mp3Parameters
  {
  public:
    explicit Mp3Parameters(Parameters::Accessor::Ptr params)
      : Params(params)
    {
    }

    Mode GetMode() const
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
        throw MakeFormattedError(THIS_LINE, BACKEND_INVALID_PARAMETER,
          Text::SOUND_ERROR_MP3_BACKEND_INVALID_MODE, mode);
      }
    }

    uint_t GetBitrate() const
    {
      Parameters::IntType bitrate = Parameters::ZXTune::Sound::Backends::Mp3::BITRATE_DEFAULT;
      if (Params->FindValue(Parameters::ZXTune::Sound::Backends::Mp3::BITRATE, bitrate) &&
        !in_range<Parameters::IntType>(bitrate, BITRATE_MIN, BITRATE_MAX))
      {
        throw MakeFormattedError(THIS_LINE, BACKEND_INVALID_PARAMETER,
          Text::SOUND_ERROR_MP3_BACKEND_INVALID_BITRATE, static_cast<int_t>(bitrate), BITRATE_MIN, BITRATE_MAX);
      }
      return static_cast<uint_t>(bitrate);
    }

    uint_t GetQuality() const
    {
      Parameters::IntType quality = Parameters::ZXTune::Sound::Backends::Mp3::QUALITY_DEFAULT;
      if (Params->FindValue(Parameters::ZXTune::Sound::Backends::Mp3::QUALITY, quality) &&
        !in_range<Parameters::IntType>(quality, QUALITY_MIN, QUALITY_MAX))
      {
        throw MakeFormattedError(THIS_LINE, BACKEND_INVALID_PARAMETER,
          Text::SOUND_ERROR_MP3_BACKEND_INVALID_QUALITY, static_cast<int_t>(quality), QUALITY_MIN, QUALITY_MAX);
      }
      return static_cast<uint_t>(quality);
    }
  private:
    const Parameters::Accessor::Ptr Params;
  };

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
      return Text::MP3_BACKEND_ID;
    }

    virtual FileStream::Ptr OpenStream(const String& fileName, bool overWrite) const
    {
      std::auto_ptr<std::ofstream> rawFile = IO::CreateFile(fileName, overWrite);
      const LameContextPtr context = LameContextPtr(Api->lame_init(), boost::bind(&Mp3::Api::lame_close, Api, _1));
      SetupContext(*context);
      return FileStream::Ptr(new Mp3Stream(Api, context, rawFile));
    }
  private:
    void SetupContext(lame_global_flags& ctx) const
    {
      const uint_t samplerate = RenderingParameters->SoundFreq();
      Log::Debug(THIS_MODULE, "Setting samplerate to %1%Hz", samplerate);
      CheckLameCall(Api->lame_set_in_samplerate(&ctx, samplerate), THIS_LINE);
      CheckLameCall(Api->lame_set_out_samplerate(&ctx, samplerate), THIS_LINE);
      CheckLameCall(Api->lame_set_num_channels(&ctx, OUTPUT_CHANNELS), THIS_LINE);
      CheckLameCall(Api->lame_set_bWriteVbrTag(&ctx, true), THIS_LINE);
      switch (Params.GetMode())
      {
      case MODE_CBR:
        {
          const uint_t bitrate = Params.GetBitrate();
          Log::Debug(THIS_MODULE, "Setting bitrate to %1%kbps", bitrate);
          CheckLameCall(Api->lame_set_VBR(&ctx, vbr_off), THIS_LINE);
          CheckLameCall(Api->lame_set_brate(&ctx, bitrate), THIS_LINE);
        }
        break;
      case MODE_ABR:
        {
          const uint_t bitrate = Params.GetBitrate();
          Log::Debug(THIS_MODULE, "Setting average bitrate to %1%kbps", bitrate);
          CheckLameCall(Api->lame_set_VBR(&ctx, vbr_abr), THIS_LINE);
          CheckLameCall(Api->lame_set_VBR_mean_bitrate_kbps(&ctx, bitrate), THIS_LINE);
        }
        break;
      case MODE_VBR:
        {
          const uint_t quality = Params.GetQuality();
          Log::Debug(THIS_MODULE, "Setting VBR quality to %1%", quality);
          CheckLameCall(Api->lame_set_VBR(&ctx, vbr_default), THIS_LINE);
          CheckLameCall(Api->lame_set_VBR_q(&ctx, quality), THIS_LINE);
        }
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
      return Text::MP3_BACKEND_ID;
    }

    virtual String Description() const
    {
      return Text::MP3_BACKEND_DESCRIPTION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_TYPE_FILE;
    }

    virtual Error Status() const
    {
      return Error();
    }

    virtual Error CreateBackend(CreateBackendParameters::Ptr params, Backend::Ptr& result) const
    {
      try
      {
        const Parameters::Accessor::Ptr allParams = params->GetParameters();
        const FileStreamFactory::Ptr factory = boost::make_shared<Mp3FileFactory>(Api, allParams);
        const BackendWorker::Ptr worker = CreateFileBackendWorker(allParams, factory);
        result = Sound::CreateBackend(params, worker);
        return Error();
      }
      catch (const Error& e)
      {
        return MakeFormattedError(THIS_LINE, BACKEND_FAILED_CREATE,
          Text::SOUND_ERROR_BACKEND_FAILED, Id()).AddSuberror(e);
      }
      catch (const std::bad_alloc&)
      {
        return Error(THIS_LINE, BACKEND_NO_MEMORY, Text::SOUND_ERROR_BACKEND_NO_MEMORY);
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
        Log::Debug(THIS_MODULE, "Detected LAME library %1%", api->get_lame_version());
        const BackendCreator::Ptr creator(new Mp3BackendCreator(api));
        enumerator.RegisterCreator(creator);
      }
      catch (const Error& e)
      {
        enumerator.RegisterCreator(CreateUnavailableBackendStub(Text::MP3_BACKEND_ID, Text::MP3_BACKEND_DESCRIPTION, CAP_TYPE_FILE, e));
      }
    }
  }
}
