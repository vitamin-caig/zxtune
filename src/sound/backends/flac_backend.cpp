/*
Abstract:
  FLAC file backend implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "flac_api.h"
#include "enumerator.h"
#include "file_backend.h"
//common includes
#include <error_tools.h>
#include <logging.h>
#include <shared_library_gate.h>
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

#define FILE_TAG 6575CD3F

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  const std::string THIS_MODULE("Sound::Backend::Flac");

  const Char FLAC_BACKEND_ID[] = {'f', 'l', 'a', 'c', 0};
  
  typedef boost::shared_ptr<FLAC__StreamEncoder> FlacEncoderPtr;

  void CheckFlacCall(FLAC__bool res, Error::LocationRef loc)
  {
    if (!res)
    {
      throw Error(loc, BACKEND_PLATFORM_ERROR, Text::SOUND_ERROR_FLAC_BACKEND_UNKNOWN_ERROR);
    }
  }

  /*
  FLAC/stream_encoder.h

   Note that for either process call, each sample in the buffers should be a
   signed integer, right-justified to the resolution set by
   FLAC__stream_encoder_set_bits_per_sample().  For example, if the resolution
   is 16 bits per sample, the samples should all be in the range [-32768,32767].
  */
  const bool SamplesShouldBeConverted = !SAMPLE_SIGNED;

  inline FLAC__int32 ConvertSample(Sample in)
  {
    return SamplesShouldBeConverted
      ? FLAC__int32(in) - SAMPLE_MID
      : in;
  }

  class FlacMetadata
  {
  public:
    explicit FlacMetadata(Flac::Api::Ptr api)
      : Api(api)
      , Tags(Api->FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT), boost::bind(&Flac::Api::FLAC__metadata_object_delete, Api, _1))
    {
    }

    void AddTag(const String& name, const String& value)
    {
      const std::string nameC = IO::ConvertToFilename(name);
      const std::string valueC = IO::ConvertToFilename(value);
      FLAC__StreamMetadata_VorbisComment_Entry entry;
      CheckFlacCall(Api->FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, nameC.c_str(), valueC.c_str()), THIS_LINE);
      CheckFlacCall(Api->FLAC__metadata_object_vorbiscomment_append_comment(Tags.get(), entry, false), THIS_LINE);
    }

    void Encode(FLAC__StreamEncoder& encoder)
    {
      FLAC__StreamMetadata* meta[1] = {Tags.get()};
      CheckFlacCall(Api->FLAC__stream_encoder_set_metadata(&encoder, meta, ArraySize(meta)), THIS_LINE);
    }
  private:
    const Flac::Api::Ptr Api;
    const boost::shared_ptr<FLAC__StreamMetadata> Tags;
  };

  class FlacStream : public FileStream
  {
  public:
    FlacStream(Flac::Api::Ptr api, FlacEncoderPtr encoder, std::auto_ptr<std::ofstream> stream)
      : Api(api)
      , Encoder(encoder)
      , Meta(api)
      , Stream(stream)
    {
    }

    virtual void SetTitle(const String& title)
    {
      Meta.AddTag(Text::OGG_BACKEND_TITLE_TAG, title);
    }

    virtual void SetAuthor(const String& author)
    {
      Meta.AddTag(Text::OGG_BACKEND_AUTHOR_TAG, author);
    }

    virtual void SetComment(const String& comment)
    {
      Meta.AddTag(Text::OGG_BACKEND_COMMENT_TAG, comment);
    }

    virtual void FlushMetadata()
    {
      Meta.Encode(*Encoder);
      //real stream initializing should be performed after all set functions
      CheckFlacCall(FLAC__STREAM_ENCODER_INIT_STATUS_OK ==
        Api->FLAC__stream_encoder_init_stream(Encoder.get(), &WriteCallback, &SeekCallback, &TellCallback, 0, Stream.get()), THIS_LINE);
      Log::Debug(THIS_MODULE, "Stream initialized");
    }

    virtual void ApplyData(const ChunkPtr& data)
    {
      if (const std::size_t samples = data->size())
      {
        Buffer.resize(samples * data->front().size());
        std::transform(data->front().begin(), data->back().end(), &Buffer.front(), &ConvertSample);
        CheckFlacCall(Api->FLAC__stream_encoder_process_interleaved(Encoder.get(), &Buffer[0], samples), THIS_LINE);
      }
    }

    virtual void Flush()
    {
      CheckFlacCall(Api->FLAC__stream_encoder_finish(Encoder.get()), THIS_LINE);
      Log::Debug(THIS_MODULE, "Stream flushed");
    }
  private:
    static FLAC__StreamEncoderWriteStatus WriteCallback(const FLAC__StreamEncoder* /*encoder*/, const FLAC__byte buffer[],
      size_t bytes, unsigned /*samples*/, unsigned /*current_frame*/, void* client_data)
    {
      std::ofstream* const stream = static_cast<std::ofstream*>(client_data);
      stream->write(safe_ptr_cast<const char*>(buffer), bytes);
      return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
    }

    static FLAC__StreamEncoderSeekStatus SeekCallback(const FLAC__StreamEncoder* /*encoder*/,
      FLAC__uint64 absolute_byte_offset, void *client_data)
    {
      std::ofstream* const stream = static_cast<std::ofstream*>(client_data);
      stream->seekp(absolute_byte_offset, std::ios_base::beg);
      return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
    }

    static FLAC__StreamEncoderTellStatus TellCallback(const FLAC__StreamEncoder* /*encoder*/, FLAC__uint64* absolute_byte_offset,
      void *client_data)
    {
      std::ofstream* const stream = static_cast<std::ofstream*>(client_data);
      *absolute_byte_offset = stream->tellp();
      return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
    }

    void CheckFlacCall(FLAC__bool res, Error::LocationRef loc)
    {
      if (!res)
      {
        const FLAC__StreamEncoderState state = Api->FLAC__stream_encoder_get_state(Encoder.get());
        throw MakeFormattedError(loc, BACKEND_PLATFORM_ERROR, Text::SOUND_ERROR_FLAC_BACKEND_ERROR, state);
      }
    }
  private:
    const Flac::Api::Ptr Api;
    const FlacEncoderPtr Encoder;
    FlacMetadata Meta;
    const std::auto_ptr<std::ofstream> Stream;
    std::vector<FLAC__int32> Buffer;
  };

  class FlacParameters
  {
  public:
    explicit FlacParameters(Parameters::Accessor::Ptr params)
      : Params(params)
    {
    }

    boost::optional<uint_t> GetCompressionLevel() const
    {
      return GetOptionalParameter(Parameters::ZXTune::Sound::Backends::Flac::COMPRESSION);
    }

    boost::optional<uint_t> GetBlockSize() const
    {
      return GetOptionalParameter(Parameters::ZXTune::Sound::Backends::Flac::BLOCKSIZE);
    }
  private:
    boost::optional<uint_t> GetOptionalParameter(const Parameters::NameType& name) const
    {
      Parameters::IntType val = 0;
      if (Params->FindValue(name, val))
      {
        return val;
      }
      return boost::optional<uint_t>();
    }
  private:
    const Parameters::Accessor::Ptr Params;
  };

  class FlacFileFactory : public FileStreamFactory
  {
  public:
    FlacFileFactory(Flac::Api::Ptr api, Parameters::Accessor::Ptr params)
      : Api(api)
      , Params(params)
      , RenderingParameters(RenderParameters::Create(params))
    {
    }

    virtual String GetId() const
    {
      return FLAC_BACKEND_ID;
    }

    virtual FileStream::Ptr OpenStream(const String& fileName, bool overWrite) const
    {
      std::auto_ptr<std::ofstream> rawFile = IO::CreateFile(fileName, overWrite);
      const FlacEncoderPtr encoder(Api->FLAC__stream_encoder_new(), boost::bind(&Flac::Api::FLAC__stream_encoder_delete, Api, _1));
      SetupEncoder(*encoder);
      return FileStream::Ptr(new FlacStream(Api, encoder, rawFile));
    }
  private:
    void SetupEncoder(FLAC__StreamEncoder& encoder) const
    {
      CheckFlacCall(Api->FLAC__stream_encoder_set_verify(&encoder, true), THIS_LINE);
      CheckFlacCall(Api->FLAC__stream_encoder_set_channels(&encoder, OUTPUT_CHANNELS), THIS_LINE);
      CheckFlacCall(Api->FLAC__stream_encoder_set_bits_per_sample(&encoder, 8 * sizeof(Sample)), THIS_LINE);
      const uint_t samplerate = RenderingParameters->SoundFreq();
      Log::Debug(THIS_MODULE, "Setting samplerate to %1%Hz", samplerate);
      CheckFlacCall(Api->FLAC__stream_encoder_set_sample_rate(&encoder, samplerate), THIS_LINE);
      if (const boost::optional<uint_t> compression = Params.GetCompressionLevel())
      {
        Log::Debug(THIS_MODULE, "Setting compression level to %1%", *compression);
        CheckFlacCall(Api->FLAC__stream_encoder_set_compression_level(&encoder, *compression), THIS_LINE);
      }
      if (const boost::optional<uint_t> blocksize = Params.GetBlockSize())
      {
        Log::Debug(THIS_MODULE, "Setting block size to %1%", *blocksize);
        CheckFlacCall(Api->FLAC__stream_encoder_set_blocksize(&encoder, *blocksize), THIS_LINE);
      }
    }
  private:
    const Flac::Api::Ptr Api;
    const FlacParameters Params;
    const RenderParameters::Ptr RenderingParameters;
  };

  class FlacBackendCreator : public BackendCreator
  {
  public:
    explicit FlacBackendCreator(Flac::Api::Ptr api)
      : Api(api)
    {
    }

    virtual String Id() const
    {
      return FLAC_BACKEND_ID;
    }

    virtual String Description() const
    {
      return Text::FLAC_BACKEND_DESCRIPTION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_TYPE_FILE;
    }

    virtual Error CreateBackend(CreateBackendParameters::Ptr params, Backend::Ptr& result) const
    {
      try
      {
        const Parameters::Accessor::Ptr allParams = params->GetParameters();
        const FileStreamFactory::Ptr factory = boost::make_shared<FlacFileFactory>(Api, allParams);
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
    const Flac::Api::Ptr Api;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    void RegisterFLACBackend(BackendsEnumerator& enumerator)
    {
      try
      {
        const Flac::Api::Ptr api = Flac::LoadDynamicApi();
        Log::Debug(THIS_MODULE, "Detected FLAC library");
        const BackendCreator::Ptr creator(new FlacBackendCreator(api));
        enumerator.RegisterCreator(creator);
      }
      catch (const Error& e)
      {
        Log::Debug(THIS_MODULE, "%1%", Error::ToString(e));
      }
    }
  }
}
