/*
Abstract:
  Flac file backend implementation

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
#include <tools.h>
//library includes
#include <binary/data_adapter.h>
#include <debug/log.h>
#include <l10n/api.h>
#include <sound/backend_attrs.h>
#include <sound/backends_parameters.h>
#include <sound/render_params.h>
//std includes
#include <algorithm>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//text includes
#include "text/backends.h"

#define FILE_TAG 6575CD3F

namespace
{
  const Debug::Stream Dbg("Sound::Backend::Flac");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound_backends");
}

namespace Sound
{
namespace Flac
{
  typedef boost::shared_ptr<FLAC__StreamEncoder> EncoderPtr;

  void CheckFlacCall(FLAC__bool res, Error::LocationRef loc)
  {
    if (!res)
    {
      throw Error(loc, translate("Error in FLAC backend."));
    }
  }

  /*
  FLAC/stream_encoder.h

   Note that for either process call, each sample in the buffers should be a
   signed integer, right-justified to the resolution set by
   FLAC__stream_encoder_set_bits_per_sample().  For example, if the resolution
   is 16 bits per sample, the samples should all be in the range [-32768,32767].
  */
  typedef std::pair<FLAC__int32, FLAC__int32> FlacSample;

  inline FlacSample ConvertSample(Sample in)
  {
    BOOST_STATIC_ASSERT(Sample::MID == 0);
    return FlacSample(in.Left(), in.Right());
  }

  class MetaData
  {
  public:
    explicit MetaData(Api::Ptr api)
      : FlacApi(api)
      , Tags(FlacApi->FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT), boost::bind(&Api::FLAC__metadata_object_delete, FlacApi, _1))
    {
    }

    void AddTag(const String& name, const String& value)
    {
      const std::string& nameC = name;//TODO
      const std::string& valueC = value;//TODO
      FLAC__StreamMetadata_VorbisComment_Entry entry;
      CheckFlacCall(FlacApi->FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, nameC.c_str(), valueC.c_str()), THIS_LINE);
      CheckFlacCall(FlacApi->FLAC__metadata_object_vorbiscomment_append_comment(Tags.get(), entry, false), THIS_LINE);
    }

    void Encode(FLAC__StreamEncoder& encoder)
    {
      FLAC__StreamMetadata* meta[1] = {Tags.get()};
      CheckFlacCall(FlacApi->FLAC__stream_encoder_set_metadata(&encoder, meta, ArraySize(meta)), THIS_LINE);
    }
  private:
    const Api::Ptr FlacApi;
    const boost::shared_ptr<FLAC__StreamMetadata> Tags;
  };

  class FileStream : public Sound::FileStream
  {
  public:
    FileStream(Api::Ptr api, EncoderPtr encoder, Binary::OutputStream::Ptr stream)
      : FlacApi(api)
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
      if (const Binary::SeekableOutputStream::Ptr seekableStream = boost::dynamic_pointer_cast<Binary::SeekableOutputStream>(Stream))
      {
        Dbg("Using seekable stream for FLAC output");
        CheckFlacCall(FLAC__STREAM_ENCODER_INIT_STATUS_OK ==
          FlacApi->FLAC__stream_encoder_init_stream(Encoder.get(), &WriteCallback, &SeekCallback, &TellCallback, 0, seekableStream.get()), THIS_LINE);
      }
      else
      {
        Dbg("Using non-seekable stream for FLAC output");
        CheckFlacCall(FLAC__STREAM_ENCODER_INIT_STATUS_OK ==
          FlacApi->FLAC__stream_encoder_init_stream(Encoder.get(), &WriteCallback, 0, 0, 0, Stream.get()), THIS_LINE);
      }
      Dbg("Stream initialized");
    }

    virtual void ApplyData(const Chunk::Ptr& data)
    {
      if (const std::size_t samples = data->size())
      {
        Buffer.resize(samples);
        std::transform(data->begin(), data->end(), &Buffer.front(), &ConvertSample);
        CheckFlacCall(FlacApi->FLAC__stream_encoder_process_interleaved(Encoder.get(), &Buffer[0].first, samples), THIS_LINE);
      }
    }

    virtual void Flush()
    {
      CheckFlacCall(FlacApi->FLAC__stream_encoder_finish(Encoder.get()), THIS_LINE);
      Dbg("Stream flushed");
    }
  private:
    static FLAC__StreamEncoderWriteStatus WriteCallback(const FLAC__StreamEncoder* /*encoder*/, const FLAC__byte buffer[],
      size_t bytes, unsigned /*samples*/, unsigned /*current_frame*/, void* client_data)
    {
      Binary::OutputStream* const stream = static_cast<Binary::OutputStream*>(client_data);
      stream->ApplyData(Binary::DataAdapter(buffer, bytes));
      return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
    }

    static FLAC__StreamEncoderSeekStatus SeekCallback(const FLAC__StreamEncoder* /*encoder*/,
      FLAC__uint64 absolute_byte_offset, void *client_data)
    {
      Binary::SeekableOutputStream* const stream = static_cast<Binary::SeekableOutputStream*>(client_data);
      stream->Seek(absolute_byte_offset);
      return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
    }

    static FLAC__StreamEncoderTellStatus TellCallback(const FLAC__StreamEncoder* /*encoder*/, FLAC__uint64* absolute_byte_offset,
      void *client_data)
    {
      Binary::SeekableOutputStream* const stream = static_cast<Binary::SeekableOutputStream*>(client_data);
      *absolute_byte_offset = stream->Position();
      return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
    }

    void CheckFlacCall(FLAC__bool res, Error::LocationRef loc)
    {
      if (!res)
      {
        const FLAC__StreamEncoderState state = FlacApi->FLAC__stream_encoder_get_state(Encoder.get());
        throw MakeFormattedError(loc, translate("Error in FLAC backend (code %1%)."), state);
      }
    }
  private:
    const Api::Ptr FlacApi;
    const EncoderPtr Encoder;
    MetaData Meta;
    const Binary::OutputStream::Ptr Stream;
    std::vector<FlacSample> Buffer;
  };

  class StreamParameters
  {
  public:
    explicit StreamParameters(Parameters::Accessor::Ptr params)
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

  const String ID = Text::FLAC_BACKEND_ID;
  const char* const DESCRIPTION = L10n::translate("FLAC support backend.");

  class FileStreamFactory : public Sound::FileStreamFactory
  {
  public:
    FileStreamFactory(Api::Ptr api, Parameters::Accessor::Ptr params)
      : FlacApi(api)
      , Params(params)
    {
    }

    virtual String GetId() const
    {
      return ID;
    }

    virtual FileStream::Ptr CreateStream(Binary::OutputStream::Ptr stream) const
    {
      const EncoderPtr encoder(FlacApi->FLAC__stream_encoder_new(), boost::bind(&Api::FLAC__stream_encoder_delete, FlacApi, _1));
      SetupEncoder(*encoder);
      return boost::make_shared<FileStream>(FlacApi, encoder, stream);
    }
  private:
    void SetupEncoder(FLAC__StreamEncoder& encoder) const
    {
      const StreamParameters stream(Params);
      const RenderParameters::Ptr sound = RenderParameters::Create(Params);
      CheckFlacCall(FlacApi->FLAC__stream_encoder_set_verify(&encoder, true), THIS_LINE);
      CheckFlacCall(FlacApi->FLAC__stream_encoder_set_channels(&encoder, Sample::CHANNELS), THIS_LINE);
      CheckFlacCall(FlacApi->FLAC__stream_encoder_set_bits_per_sample(&encoder, Sample::BITS), THIS_LINE);
      const uint_t samplerate = sound->SoundFreq();
      Dbg("Setting samplerate to %1%Hz", samplerate);
      CheckFlacCall(FlacApi->FLAC__stream_encoder_set_sample_rate(&encoder, samplerate), THIS_LINE);
      if (const boost::optional<uint_t> compression = stream.GetCompressionLevel())
      {
        Dbg("Setting compression level to %1%", *compression);
        CheckFlacCall(FlacApi->FLAC__stream_encoder_set_compression_level(&encoder, *compression), THIS_LINE);
      }
      if (const boost::optional<uint_t> blocksize = stream.GetBlockSize())
      {
        Dbg("Setting block size to %1%", *blocksize);
        CheckFlacCall(FlacApi->FLAC__stream_encoder_set_blocksize(&encoder, *blocksize), THIS_LINE);
      }
    }
  private:
    const Api::Ptr FlacApi;
    const Parameters::Accessor::Ptr Params;
  };

  class BackendCreator : public Sound::BackendCreator
  {
  public:
    explicit BackendCreator(Api::Ptr api)
      : FlacApi(api)
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
        const FileStreamFactory::Ptr factory = boost::make_shared<FileStreamFactory>(FlacApi, allParams);
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
    const Api::Ptr FlacApi;
  };
}//Flac
}//Sound

namespace Sound
{
  void RegisterFlacBackend(BackendsEnumerator& enumerator)
  {
    try
    {
      const Flac::Api::Ptr api = Flac::LoadDynamicApi();
      Dbg("Detected Flac library");
      const BackendCreator::Ptr creator = boost::make_shared<Flac::BackendCreator>(api);
      enumerator.RegisterCreator(creator);
    }
    catch (const Error& e)
    {
      enumerator.RegisterCreator(CreateUnavailableBackendStub(Flac::ID, Flac::DESCRIPTION, CAP_TYPE_FILE, e));
    }
  }
}
