/**
 *
 * @file
 *
 * @brief  FLAC subsystem API gate implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "sound/backends/gates/flac_api.h"

#include <debug/log.h>
#include <platform/shared_library_adapter.h>

#include <make_ptr.h>
#include <string_view.h>

namespace Sound::Flac
{
  class LibraryName : public Platform::SharedLibrary::Name
  {
  public:
    LibraryName() = default;

    StringView Base() const override
    {
      return "FLAC"sv;
    }

    std::vector<StringView> PosixAlternatives() const override
    {
      return {"libFLAC.so.7"sv, "libFLAC.so.8"sv};
    }

    std::vector<StringView> WindowsAlternatives() const override
    {
      return {"libFLAC.dll"sv};
    }
  };

  class DynamicApi : public Api
  {
  public:
    explicit DynamicApi(Platform::SharedLibrary::Ptr lib)
      : Lib(std::move(lib))
    {
      Debug::Log("Sound::Backend::Flac", "Library loaded");
    }

    ~DynamicApi() override
    {
      Debug::Log("Sound::Backend::Flac", "Library unloaded");
    }

    // clang-format off

    FLAC__StreamEncoder* FLAC__stream_encoder_new() override
    {
      using FunctionType = decltype(&::FLAC__stream_encoder_new);
      const auto func = Lib.GetSymbol<FunctionType>("FLAC__stream_encoder_new");
      return func();
    }

    void FLAC__stream_encoder_delete(FLAC__StreamEncoder *encoder) override
    {
      using FunctionType = decltype(&::FLAC__stream_encoder_delete);
      const auto func = Lib.GetSymbol<FunctionType>("FLAC__stream_encoder_delete");
      return func(encoder);
    }

    FLAC__bool FLAC__stream_encoder_set_verify(FLAC__StreamEncoder *encoder, FLAC__bool value) override
    {
      using FunctionType = decltype(&::FLAC__stream_encoder_set_verify);
      const auto func = Lib.GetSymbol<FunctionType>("FLAC__stream_encoder_set_verify");
      return func(encoder, value);
    }

    FLAC__bool FLAC__stream_encoder_set_channels(FLAC__StreamEncoder *encoder, unsigned value) override
    {
      using FunctionType = decltype(&::FLAC__stream_encoder_set_channels);
      const auto func = Lib.GetSymbol<FunctionType>("FLAC__stream_encoder_set_channels");
      return func(encoder, value);
    }

    FLAC__bool FLAC__stream_encoder_set_bits_per_sample(FLAC__StreamEncoder *encoder, unsigned value) override
    {
      using FunctionType = decltype(&::FLAC__stream_encoder_set_bits_per_sample);
      const auto func = Lib.GetSymbol<FunctionType>("FLAC__stream_encoder_set_bits_per_sample");
      return func(encoder, value);
    }

    FLAC__bool FLAC__stream_encoder_set_sample_rate(FLAC__StreamEncoder *encoder, unsigned value) override
    {
      using FunctionType = decltype(&::FLAC__stream_encoder_set_sample_rate);
      const auto func = Lib.GetSymbol<FunctionType>("FLAC__stream_encoder_set_sample_rate");
      return func(encoder, value);
    }

    FLAC__bool FLAC__stream_encoder_set_compression_level(FLAC__StreamEncoder *encoder, unsigned value) override
    {
      using FunctionType = decltype(&::FLAC__stream_encoder_set_compression_level);
      const auto func = Lib.GetSymbol<FunctionType>("FLAC__stream_encoder_set_compression_level");
      return func(encoder, value);
    }

    FLAC__bool FLAC__stream_encoder_set_blocksize(FLAC__StreamEncoder *encoder, unsigned value) override
    {
      using FunctionType = decltype(&::FLAC__stream_encoder_set_blocksize);
      const auto func = Lib.GetSymbol<FunctionType>("FLAC__stream_encoder_set_blocksize");
      return func(encoder, value);
    }

    FLAC__bool FLAC__stream_encoder_set_metadata(FLAC__StreamEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks) override
    {
      using FunctionType = decltype(&::FLAC__stream_encoder_set_metadata);
      const auto func = Lib.GetSymbol<FunctionType>("FLAC__stream_encoder_set_metadata");
      return func(encoder, metadata, num_blocks);
    }

    FLAC__StreamEncoderInitStatus FLAC__stream_encoder_init_stream(FLAC__StreamEncoder *encoder, FLAC__StreamEncoderWriteCallback write_callback, FLAC__StreamEncoderSeekCallback seek_callback, FLAC__StreamEncoderTellCallback tell_callback, FLAC__StreamEncoderMetadataCallback metadata_callback, void *client_data) override
    {
      using FunctionType = decltype(&::FLAC__stream_encoder_init_stream);
      const auto func = Lib.GetSymbol<FunctionType>("FLAC__stream_encoder_init_stream");
      return func(encoder, write_callback, seek_callback, tell_callback, metadata_callback, client_data);
    }

    FLAC__bool FLAC__stream_encoder_finish(FLAC__StreamEncoder *encoder) override
    {
      using FunctionType = decltype(&::FLAC__stream_encoder_finish);
      const auto func = Lib.GetSymbol<FunctionType>("FLAC__stream_encoder_finish");
      return func(encoder);
    }

    FLAC__bool FLAC__stream_encoder_process_interleaved(FLAC__StreamEncoder *encoder, const FLAC__int32 buffer[], unsigned samples) override
    {
      using FunctionType = decltype(&::FLAC__stream_encoder_process_interleaved);
      const auto func = Lib.GetSymbol<FunctionType>("FLAC__stream_encoder_process_interleaved");
      return func(encoder, buffer, samples);
    }

    FLAC__StreamMetadata* FLAC__metadata_object_new(FLAC__MetadataType type) override
    {
      using FunctionType = decltype(&::FLAC__metadata_object_new);
      const auto func = Lib.GetSymbol<FunctionType>("FLAC__metadata_object_new");
      return func(type);
    }

    void FLAC__metadata_object_delete(FLAC__StreamMetadata *object) override
    {
      using FunctionType = decltype(&::FLAC__metadata_object_delete);
      const auto func = Lib.GetSymbol<FunctionType>("FLAC__metadata_object_delete");
      return func(object);
    }

    FLAC__bool FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(FLAC__StreamMetadata_VorbisComment_Entry *entry, const char *field_name, const char *field_value) override
    {
      using FunctionType = decltype(&::FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair);
      const auto func = Lib.GetSymbol<FunctionType>("FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair");
      return func(entry, field_name, field_value);
    }

    FLAC__bool FLAC__metadata_object_vorbiscomment_append_comment(FLAC__StreamMetadata *object, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy) override
    {
      using FunctionType = decltype(&::FLAC__metadata_object_vorbiscomment_append_comment);
      const auto func = Lib.GetSymbol<FunctionType>("FLAC__metadata_object_vorbiscomment_append_comment");
      return func(object, entry, copy);
    }

    FLAC__StreamEncoderState 	FLAC__stream_encoder_get_state(const FLAC__StreamEncoder *encoder) override
    {
      using FunctionType = decltype(&::FLAC__stream_encoder_get_state);
      const auto func = Lib.GetSymbol<FunctionType>("FLAC__stream_encoder_get_state");
      return func(encoder);
    }

    // clang-format on
  private:
    const Platform::SharedLibraryAdapter Lib;
  };

  Api::Ptr LoadDynamicApi()
  {
    static const LibraryName NAME;
    auto lib = Platform::SharedLibrary::Load(NAME);
    return MakePtr<DynamicApi>(std::move(lib));
  }
}  // namespace Sound::Flac
