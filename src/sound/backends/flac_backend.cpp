/*
Abstract:
  FLAC file backend implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifdef FLAC_SUPPORT

//local includes
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
//platform-specific includes
#include <FLAC/metadata.h>
#include <FLAC/stream_encoder.h>
//std includes
#include <algorithm>
//boost includes
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

  struct FlacLibraryTraits
  {
    static std::string GetName()
    {
      return "FLAC";
    }

    static void Startup()
    {
      Log::Debug(THIS_MODULE, "Library loaded");
    }

    static void Shutdown()
    {
      Log::Debug(THIS_MODULE, "Library unloaded");
    }
  };

  typedef SharedLibraryGate<FlacLibraryTraits> FlacLibrary;

  typedef boost::shared_ptr<FLAC__StreamEncoder> FlacEncoderPtr;

  void CheckFlacCall(FLAC__bool res, Error::LocationRef loc)
  {
    if (!res)
    {
      //TODO: clarify error using FLAC__stream_encoder_get_state
      throw Error(loc, BACKEND_PLATFORM_ERROR, Text::SOUND_ERROR_FLAC_BACKEND_ERROR);
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

  class FlacMetadata
  {
  public:
    FlacMetadata()
      : Tags(::FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT), &::FLAC__metadata_object_delete)
    {
    }

    void AddTag(const String& name, const String& value)
    {
      const std::string nameC = IO::ConvertToFilename(name);
      const std::string valueC = IO::ConvertToFilename(value);
      FLAC__StreamMetadata_VorbisComment_Entry entry;
      CheckFlacCall(::FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, nameC.c_str(), valueC.c_str()), THIS_LINE);
      CheckFlacCall(::FLAC__metadata_object_vorbiscomment_append_comment(Tags.get(), entry, false), THIS_LINE);
    }

    void Encode(FLAC__StreamEncoder& encoder)
    {
      FLAC__StreamMetadata* meta[1] = {Tags.get()};
      CheckFlacCall(::FLAC__stream_encoder_set_metadata(&encoder, meta, ArraySize(meta)), THIS_LINE);
    }
  private:
    const boost::shared_ptr<FLAC__StreamMetadata> Tags;
  };

  class FlacStream : public FileStream
  {
  public:
    FlacStream(FlacEncoderPtr encoder, std::auto_ptr<std::ofstream> stream)
      : Encoder(encoder)
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
        ::FLAC__stream_encoder_init_stream(Encoder.get(), &WriteCallback, &SeekCallback, &TellCallback, 0, Stream.get()), THIS_LINE);
      Log::Debug(THIS_MODULE, "Stream initialized");
    }

    virtual void ApplyData(const ChunkPtr& data)
    {
      if (SamplesShouldBeConverted)
      {
        std::transform(data->front().begin(), data->back().end(), data->front().begin(), &ToSignedSample);
      }
      if (const std::size_t samples = data->size())
      {
        Buffer.resize(samples * OUTPUT_CHANNELS);
        std::copy(data->front().begin(), data->back().end(), Buffer.begin());
        CheckFlacCall(::FLAC__stream_encoder_process_interleaved(Encoder.get(), &Buffer[0], samples), THIS_LINE);
      }
    }

    virtual void Flush()
    {
      CheckFlacCall(::FLAC__stream_encoder_finish(Encoder.get()), THIS_LINE);
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
  private:
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
    explicit FlacFileFactory(Parameters::Accessor::Ptr params)
      : Params(params)
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
      const FlacEncoderPtr encoder(::FLAC__stream_encoder_new(), &::FLAC__stream_encoder_delete);
      SetupEncoder(*encoder);
      return FileStream::Ptr(new FlacStream(encoder, rawFile));
    }
  private:
    void SetupEncoder(FLAC__StreamEncoder& encoder) const
    {
      CheckFlacCall(::FLAC__stream_encoder_set_verify(&encoder, true), THIS_LINE);
      CheckFlacCall(::FLAC__stream_encoder_set_channels(&encoder, OUTPUT_CHANNELS), THIS_LINE);
      CheckFlacCall(::FLAC__stream_encoder_set_bits_per_sample(&encoder, 8 * sizeof(Sample)), THIS_LINE);
      const uint_t samplerate = RenderingParameters->SoundFreq();
      Log::Debug(THIS_MODULE, "Setting samplerate to %1%Hz", samplerate);
      CheckFlacCall(::FLAC__stream_encoder_set_sample_rate(&encoder, samplerate), THIS_LINE);
      if (const boost::optional<uint_t> compression = Params.GetCompressionLevel())
      {
        Log::Debug(THIS_MODULE, "Setting compression level to %1%", *compression);
        CheckFlacCall(::FLAC__stream_encoder_set_compression_level(&encoder, *compression), THIS_LINE);
      }
      if (const boost::optional<uint_t> blocksize = Params.GetBlockSize())
      {
        Log::Debug(THIS_MODULE, "Setting block size to %1%", *blocksize);
        CheckFlacCall(::FLAC__stream_encoder_set_blocksize(&encoder, *blocksize), THIS_LINE);
      }
    }
  private:
    const FlacParameters Params;
    const RenderParameters::Ptr RenderingParameters;
  };

  class FLACBackendCreator : public BackendCreator
  {
  public:
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
        const FileStreamFactory::Ptr factory = boost::make_shared<FlacFileFactory>(allParams);
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
  };
}

namespace ZXTune
{
  namespace Sound
  {
    void RegisterFLACBackend(BackendsEnumerator& enumerator)
    {
      if (FlacLibrary::Instance().IsAccessible())
      {
        Log::Debug(THIS_MODULE, "Detected FLAC library");
        const BackendCreator::Ptr creator(new FLACBackendCreator());
        enumerator.RegisterCreator(creator);
      }
      else
      {
        Log::Debug(THIS_MODULE, "%1%", Error::ToString(FlacLibrary::Instance().GetLoadError()));
      }
    }
  }
}

//global namespace
#define STR(a) #a
#define FLAC_FUNC(func) FlacLibrary::Instance().GetSymbol(&func, STR(func))

FLAC__StreamEncoder* FLAC__stream_encoder_new(void)
{
  return FLAC_FUNC(FLAC__stream_encoder_new)();
}

void FLAC__stream_encoder_delete(FLAC__StreamEncoder *encoder)
{
  return FLAC_FUNC(FLAC__stream_encoder_delete)(encoder);
}

FLAC__bool FLAC__stream_encoder_set_verify(FLAC__StreamEncoder *encoder, FLAC__bool value)
{
  return FLAC_FUNC(FLAC__stream_encoder_set_verify)(encoder, value);
}

FLAC__bool FLAC__stream_encoder_set_channels(FLAC__StreamEncoder *encoder, unsigned value)
{
  return FLAC_FUNC(FLAC__stream_encoder_set_channels)(encoder, value);
}

FLAC__bool FLAC__stream_encoder_set_bits_per_sample(FLAC__StreamEncoder *encoder, unsigned value)
{
  return FLAC_FUNC(FLAC__stream_encoder_set_bits_per_sample)(encoder, value);
}

FLAC__bool FLAC__stream_encoder_set_sample_rate(FLAC__StreamEncoder *encoder, unsigned value)
{
  return FLAC_FUNC(FLAC__stream_encoder_set_sample_rate)(encoder, value);
}

FLAC__bool FLAC__stream_encoder_set_compression_level(FLAC__StreamEncoder *encoder, unsigned value)
{
  return FLAC_FUNC(FLAC__stream_encoder_set_compression_level)(encoder, value);
}

FLAC__bool FLAC__stream_encoder_set_blocksize(FLAC__StreamEncoder *encoder, unsigned value)
{
  return FLAC_FUNC(FLAC__stream_encoder_set_blocksize)(encoder, value);
}

FLAC__bool FLAC__stream_encoder_set_metadata(FLAC__StreamEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks)
{
  return FLAC_FUNC(FLAC__stream_encoder_set_metadata)(encoder, metadata, num_blocks);
}

FLAC__StreamEncoderInitStatus FLAC__stream_encoder_init_stream(FLAC__StreamEncoder *encoder,
  FLAC__StreamEncoderWriteCallback write_callback, FLAC__StreamEncoderSeekCallback seek_callback,
  FLAC__StreamEncoderTellCallback tell_callback, FLAC__StreamEncoderMetadataCallback metadata_callback, void *client_data)
{
  return FLAC_FUNC(FLAC__stream_encoder_init_stream)(encoder, write_callback, seek_callback, tell_callback, metadata_callback, client_data);
}

FLAC__bool FLAC__stream_encoder_finish(FLAC__StreamEncoder *encoder)
{
  return FLAC_FUNC(FLAC__stream_encoder_finish)(encoder);
}

FLAC__bool FLAC__stream_encoder_process_interleaved(FLAC__StreamEncoder *encoder, const FLAC__int32 buffer[],
  unsigned samples)
{
  return FLAC_FUNC(FLAC__stream_encoder_process_interleaved)(encoder, buffer, samples);
}

FLAC__StreamMetadata* FLAC__metadata_object_new(FLAC__MetadataType type)
{
  return FLAC_FUNC(FLAC__metadata_object_new)(type);
}

void FLAC__metadata_object_delete(FLAC__StreamMetadata *object)
{
  return FLAC_FUNC(FLAC__metadata_object_delete)(object);
}

FLAC__bool FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(FLAC__StreamMetadata_VorbisComment_Entry *entry,
  const char *field_name, const char *field_value)
{
  return FLAC_FUNC(FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair)(entry, field_name, field_value);
}

FLAC__bool FLAC__metadata_object_vorbiscomment_append_comment(FLAC__StreamMetadata *object,
  FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy)
{
  return FLAC_FUNC(FLAC__metadata_object_vorbiscomment_append_comment)(object, entry, copy);
}

#else //not supported

namespace ZXTune
{
  namespace Sound
  {
    void RegisterFLACBackend(class BackendsEnumerator& /*enumerator*/)
    {
      //do nothing
    }
  }
}
#endif
