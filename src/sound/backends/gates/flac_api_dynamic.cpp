/**
 *
 * @file
 *
 * @brief  FLAC subsystem API gate implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound/backends/gates/flac_api.h"
// common includes
#include <make_ptr.h>
// library includes
#include <debug/log.h>
#include <platform/shared_library_adapter.h>

namespace Sound
{
  namespace Flac
  {
    class LibraryName : public Platform::SharedLibrary::Name
    {
    public:
      LibraryName() {}

      StringView Base() const override
      {
        return "FLAC"_sv;
      }

      std::vector<StringView> PosixAlternatives() const override
      {
        return {"libFLAC.so.7"_sv, "libFLAC.so.8"_sv};
      }

      std::vector<StringView> WindowsAlternatives() const override
      {
        return {};
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

      
      FLAC__StreamEncoder* FLAC__stream_encoder_new(void) override
      {
        static const char NAME[] = "FLAC__stream_encoder_new";
        typedef FLAC__StreamEncoder* ( *FunctionType)();
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func();
      }
      
      void FLAC__stream_encoder_delete(FLAC__StreamEncoder *encoder) override
      {
        static const char NAME[] = "FLAC__stream_encoder_delete";
        typedef void ( *FunctionType)(FLAC__StreamEncoder *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(encoder);
      }
      
      FLAC__bool FLAC__stream_encoder_set_verify(FLAC__StreamEncoder *encoder, FLAC__bool value) override
      {
        static const char NAME[] = "FLAC__stream_encoder_set_verify";
        typedef FLAC__bool ( *FunctionType)(FLAC__StreamEncoder *, FLAC__bool);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(encoder, value);
      }
      
      FLAC__bool FLAC__stream_encoder_set_channels(FLAC__StreamEncoder *encoder, unsigned value) override
      {
        static const char NAME[] = "FLAC__stream_encoder_set_channels";
        typedef FLAC__bool ( *FunctionType)(FLAC__StreamEncoder *, unsigned);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(encoder, value);
      }
      
      FLAC__bool FLAC__stream_encoder_set_bits_per_sample(FLAC__StreamEncoder *encoder, unsigned value) override
      {
        static const char NAME[] = "FLAC__stream_encoder_set_bits_per_sample";
        typedef FLAC__bool ( *FunctionType)(FLAC__StreamEncoder *, unsigned);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(encoder, value);
      }
      
      FLAC__bool FLAC__stream_encoder_set_sample_rate(FLAC__StreamEncoder *encoder, unsigned value) override
      {
        static const char NAME[] = "FLAC__stream_encoder_set_sample_rate";
        typedef FLAC__bool ( *FunctionType)(FLAC__StreamEncoder *, unsigned);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(encoder, value);
      }
      
      FLAC__bool FLAC__stream_encoder_set_compression_level(FLAC__StreamEncoder *encoder, unsigned value) override
      {
        static const char NAME[] = "FLAC__stream_encoder_set_compression_level";
        typedef FLAC__bool ( *FunctionType)(FLAC__StreamEncoder *, unsigned);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(encoder, value);
      }
      
      FLAC__bool FLAC__stream_encoder_set_blocksize(FLAC__StreamEncoder *encoder, unsigned value) override
      {
        static const char NAME[] = "FLAC__stream_encoder_set_blocksize";
        typedef FLAC__bool ( *FunctionType)(FLAC__StreamEncoder *, unsigned);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(encoder, value);
      }
      
      FLAC__bool FLAC__stream_encoder_set_metadata(FLAC__StreamEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks) override
      {
        static const char NAME[] = "FLAC__stream_encoder_set_metadata";
        typedef FLAC__bool ( *FunctionType)(FLAC__StreamEncoder *, FLAC__StreamMetadata **, unsigned);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(encoder, metadata, num_blocks);
      }
      
      FLAC__StreamEncoderInitStatus FLAC__stream_encoder_init_stream(FLAC__StreamEncoder *encoder, FLAC__StreamEncoderWriteCallback write_callback, FLAC__StreamEncoderSeekCallback seek_callback, FLAC__StreamEncoderTellCallback tell_callback, FLAC__StreamEncoderMetadataCallback metadata_callback, void *client_data) override
      {
        static const char NAME[] = "FLAC__stream_encoder_init_stream";
        typedef FLAC__StreamEncoderInitStatus ( *FunctionType)(FLAC__StreamEncoder *, FLAC__StreamEncoderWriteCallback, FLAC__StreamEncoderSeekCallback, FLAC__StreamEncoderTellCallback, FLAC__StreamEncoderMetadataCallback, void *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(encoder, write_callback, seek_callback, tell_callback, metadata_callback, client_data);
      }
      
      FLAC__bool FLAC__stream_encoder_finish(FLAC__StreamEncoder *encoder) override
      {
        static const char NAME[] = "FLAC__stream_encoder_finish";
        typedef FLAC__bool ( *FunctionType)(FLAC__StreamEncoder *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(encoder);
      }
      
      FLAC__bool FLAC__stream_encoder_process_interleaved(FLAC__StreamEncoder *encoder, const FLAC__int32 buffer[], unsigned samples) override
      {
        static const char NAME[] = "FLAC__stream_encoder_process_interleaved";
        typedef FLAC__bool ( *FunctionType)(FLAC__StreamEncoder *, const FLAC__int32[], unsigned);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(encoder, buffer, samples);
      }
      
      FLAC__StreamMetadata* FLAC__metadata_object_new(FLAC__MetadataType type) override
      {
        static const char NAME[] = "FLAC__metadata_object_new";
        typedef FLAC__StreamMetadata* ( *FunctionType)(FLAC__MetadataType);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(type);
      }
      
      void FLAC__metadata_object_delete(FLAC__StreamMetadata *object) override
      {
        static const char NAME[] = "FLAC__metadata_object_delete";
        typedef void ( *FunctionType)(FLAC__StreamMetadata *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(object);
      }
      
      FLAC__bool FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(FLAC__StreamMetadata_VorbisComment_Entry *entry, const char *field_name, const char *field_value) override
      {
        static const char NAME[] = "FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair";
        typedef FLAC__bool ( *FunctionType)(FLAC__StreamMetadata_VorbisComment_Entry *, const char *, const char *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(entry, field_name, field_value);
      }
      
      FLAC__bool FLAC__metadata_object_vorbiscomment_append_comment(FLAC__StreamMetadata *object, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy) override
      {
        static const char NAME[] = "FLAC__metadata_object_vorbiscomment_append_comment";
        typedef FLAC__bool ( *FunctionType)(FLAC__StreamMetadata *, FLAC__StreamMetadata_VorbisComment_Entry, FLAC__bool);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(object, entry, copy);
      }
      
      FLAC__StreamEncoderState 	FLAC__stream_encoder_get_state(const FLAC__StreamEncoder *encoder) override
      {
        static const char NAME[] = "FLAC__stream_encoder_get_state";
        typedef FLAC__StreamEncoderState 	( *FunctionType)(const FLAC__StreamEncoder *);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(encoder);
      }
      
    private:
      const Platform::SharedLibraryAdapter Lib;
    };


    Api::Ptr LoadDynamicApi()
    {
      static const LibraryName NAME;
      auto lib = Platform::SharedLibrary::Load(NAME);
      return MakePtr<DynamicApi>(std::move(lib));
    }
  }  // namespace Flac
}  // namespace Sound
