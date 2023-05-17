/**
 *
 * @file
 *
 * @brief  FLAC backend implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound/backends/file_backend.h"
#include "sound/backends/flac.h"
#include "sound/backends/gates/flac_api.h"
#include "sound/backends/l10n.h"
#include "sound/backends/storage.h"
// common includes
#include <error_tools.h>
#include <make_ptr.h>
// library includes
#include <debug/log.h>
#include <sound/backends_parameters.h>
#include <sound/render_params.h>
// std includes
#include <algorithm>
#include <optional>

namespace Sound::Flac
{
  const Debug::Stream Dbg("Sound::Backend::Flac");

  using EncoderPtr = std::shared_ptr<FLAC__StreamEncoder>;

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
  using FlacSample = std::pair<FLAC__int32, FLAC__int32>;

  inline FlacSample ConvertSample(Sample in)
  {
    static_assert(Sample::MID == 0, "Incompatible sound sample type");
    return FlacSample(in.Left(), in.Right());
  }

  class MetaData
  {
  public:
    explicit MetaData(Api::Ptr api)
      : FlacApi(std::move(api))
      , Tags(FlacApi->FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT),
             [api = FlacApi](auto&& arg) { api->FLAC__metadata_object_delete(arg); })
    {}

    void AddTag(const String& name, const String& value)
    {
      FLAC__StreamMetadata_VorbisComment_Entry entry;
      CheckFlacCall(
          FlacApi->FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, name.c_str(), value.c_str()),
          THIS_LINE);
      CheckFlacCall(FlacApi->FLAC__metadata_object_vorbiscomment_append_comment(Tags.get(), entry, false), THIS_LINE);
    }

    void Encode(FLAC__StreamEncoder& encoder)
    {
      const std::size_t METADATAS_COUNT = 1;
      FLAC__StreamMetadata* meta[METADATAS_COUNT] = {Tags.get()};
      CheckFlacCall(FlacApi->FLAC__stream_encoder_set_metadata(&encoder, meta, METADATAS_COUNT), THIS_LINE);
    }

  private:
    const Api::Ptr FlacApi;
    const std::shared_ptr<FLAC__StreamMetadata> Tags;
  };

  class FileStream : public Sound::FileStream
  {
  public:
    FileStream(Api::Ptr api, EncoderPtr encoder, Binary::OutputStream::Ptr stream)
      : FlacApi(api)
      , Encoder(std::move(encoder))
      , Meta(api)
      , Stream(std::move(stream))
    {}

    void SetTitle(const String& title) override
    {
      Meta.AddTag(File::TITLE_TAG, title);
    }

    void SetAuthor(const String& author) override
    {
      Meta.AddTag(File::AUTHOR_TAG, author);
    }

    void SetComment(const String& comment) override
    {
      Meta.AddTag(File::COMMENT_TAG, comment);
    }

    void FlushMetadata() override
    {
      Meta.Encode(*Encoder);
      // real stream initializing should be performed after all set functions
      if (const auto seekableStream = std::dynamic_pointer_cast<Binary::SeekableOutputStream>(Stream))
      {
        Dbg("Using seekable stream for FLAC output");
        CheckFlacCall(FLAC__STREAM_ENCODER_INIT_STATUS_OK
                          == FlacApi->FLAC__stream_encoder_init_stream(Encoder.get(), &WriteCallback, &SeekCallback,
                                                                       &TellCallback, nullptr, seekableStream.get()),
                      THIS_LINE);
      }
      else
      {
        Dbg("Using non-seekable stream for FLAC output");
        CheckFlacCall(FLAC__STREAM_ENCODER_INIT_STATUS_OK
                          == FlacApi->FLAC__stream_encoder_init_stream(Encoder.get(), &WriteCallback, nullptr, nullptr,
                                                                       nullptr, Stream.get()),
                      THIS_LINE);
      }
      Dbg("Stream initialized");
    }

    void ApplyData(Chunk data) override
    {
      if (const std::size_t samples = data.size())
      {
        Buffer.resize(samples);
        std::transform(data.begin(), data.end(), Buffer.data(), &ConvertSample);
        CheckFlacCall(FlacApi->FLAC__stream_encoder_process_interleaved(Encoder.get(), &Buffer[0].first, samples),
                      THIS_LINE);
      }
    }

    void Flush() override
    {
      CheckFlacCall(FlacApi->FLAC__stream_encoder_finish(Encoder.get()), THIS_LINE);
      Dbg("Stream flushed");
    }

  private:
    static FLAC__StreamEncoderWriteStatus WriteCallback(const FLAC__StreamEncoder* /*encoder*/,
                                                        const FLAC__byte buffer[], size_t bytes, unsigned /*samples*/,
                                                        unsigned /*current_frame*/, void* client_data)
    {
      auto* const stream = static_cast<Binary::OutputStream*>(client_data);
      stream->ApplyData(Binary::View(buffer, bytes));
      return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
    }

    static FLAC__StreamEncoderSeekStatus SeekCallback(const FLAC__StreamEncoder* /*encoder*/,
                                                      FLAC__uint64 absolute_byte_offset, void* client_data)
    {
      auto* const stream = static_cast<Binary::SeekableOutputStream*>(client_data);
      stream->Seek(absolute_byte_offset);
      return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
    }

    static FLAC__StreamEncoderTellStatus TellCallback(const FLAC__StreamEncoder* /*encoder*/,
                                                      FLAC__uint64* absolute_byte_offset, void* client_data)
    {
      auto* const stream = static_cast<Binary::SeekableOutputStream*>(client_data);
      *absolute_byte_offset = stream->Position();
      return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
    }

    void CheckFlacCall(FLAC__bool res, Error::LocationRef loc)
    {
      if (!res)
      {
        const FLAC__StreamEncoderState state = FlacApi->FLAC__stream_encoder_get_state(Encoder.get());
        throw MakeFormattedError(loc, translate("Error in FLAC backend (code {})."), static_cast<int>(state));
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
      : Params(std::move(params))
    {}

    std::optional<uint_t> GetCompressionLevel() const
    {
      return GetOptionalParameter(Parameters::ZXTune::Sound::Backends::Flac::COMPRESSION);
    }

    std::optional<uint_t> GetBlockSize() const
    {
      return GetOptionalParameter(Parameters::ZXTune::Sound::Backends::Flac::BLOCKSIZE);
    }

  private:
    std::optional<uint_t> GetOptionalParameter(Parameters::Identifier name) const
    {
      Parameters::IntType val = 0;
      if (Params->FindValue(name, val))
      {
        return val;
      }
      return {};
    }

  private:
    const Parameters::Accessor::Ptr Params;
  };

  class FileStreamFactory : public Sound::FileStreamFactory
  {
  public:
    FileStreamFactory(Api::Ptr api, Parameters::Accessor::Ptr params)
      : FlacApi(std::move(api))
      , Params(std::move(params))
    {}

    BackendId GetId() const override
    {
      return BACKEND_ID;
    }

    FileStream::Ptr CreateStream(Binary::OutputStream::Ptr stream) const override
    {
      auto encoder = EncoderPtr(FlacApi->FLAC__stream_encoder_new(),
                                [api = FlacApi](auto&& arg) { api->FLAC__stream_encoder_delete(arg); });
      SetupEncoder(*encoder);
      return MakePtr<FileStream>(FlacApi, std::move(encoder), std::move(stream));
    }

  private:
    void SetupEncoder(FLAC__StreamEncoder& encoder) const
    {
      const StreamParameters stream(Params);
      CheckFlacCall(FlacApi->FLAC__stream_encoder_set_verify(&encoder, true), THIS_LINE);
      CheckFlacCall(FlacApi->FLAC__stream_encoder_set_channels(&encoder, Sample::CHANNELS), THIS_LINE);
      CheckFlacCall(FlacApi->FLAC__stream_encoder_set_bits_per_sample(&encoder, Sample::BITS), THIS_LINE);
      const uint_t samplerate = GetSoundFrequency(*Params);
      Dbg("Setting samplerate to {}Hz", samplerate);
      CheckFlacCall(FlacApi->FLAC__stream_encoder_set_sample_rate(&encoder, samplerate), THIS_LINE);
      if (const auto compression = stream.GetCompressionLevel())
      {
        Dbg("Setting compression level to {}", *compression);
        CheckFlacCall(FlacApi->FLAC__stream_encoder_set_compression_level(&encoder, *compression), THIS_LINE);
      }
      if (const auto blocksize = stream.GetBlockSize())
      {
        Dbg("Setting block size to {}", *blocksize);
        CheckFlacCall(FlacApi->FLAC__stream_encoder_set_blocksize(&encoder, *blocksize), THIS_LINE);
      }
    }

  private:
    const Api::Ptr FlacApi;
    const Parameters::Accessor::Ptr Params;
  };

  class BackendWorkerFactory : public Sound::BackendWorkerFactory
  {
  public:
    explicit BackendWorkerFactory(Api::Ptr api)
      : FlacApi(std::move(api))
    {}

    BackendWorker::Ptr CreateWorker(Parameters::Accessor::Ptr params, Module::Holder::Ptr holder) const override
    {
      auto factory = MakePtr<FileStreamFactory>(FlacApi, params);
      return CreateFileBackendWorker(std::move(params), holder->GetModuleProperties(), std::move(factory));
    }

  private:
    const Api::Ptr FlacApi;
  };
}  // namespace Sound::Flac

namespace Sound
{
  void RegisterFlacBackend(BackendsStorage& storage)
  {
    try
    {
      auto api = Flac::LoadDynamicApi();
      Flac::Dbg("Detected Flac library");
      auto factory = MakePtr<Flac::BackendWorkerFactory>(std::move(api));
      storage.Register(Flac::BACKEND_ID, Flac::BACKEND_DESCRIPTION, CAP_TYPE_FILE, std::move(factory));
    }
    catch (const Error& e)
    {
      storage.Register(Flac::BACKEND_ID, Flac::BACKEND_DESCRIPTION, CAP_TYPE_FILE, e);
    }
  }
}  // namespace Sound
