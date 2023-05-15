/**
 *
 * @file
 *
 * @brief  FLAC subsystem API gate interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// std includes
#include <memory>
// platform-dependent includes
#include <FLAC/metadata.h>
#include <FLAC/stream_encoder.h>

namespace Sound
{
  namespace Flac
  {
    class Api
    {
    public:
      typedef std::shared_ptr<Api> Ptr;
      virtual ~Api() = default;

// clang-format off

      virtual FLAC__StreamEncoder* FLAC__stream_encoder_new(void) = 0;
      virtual void FLAC__stream_encoder_delete(FLAC__StreamEncoder *encoder) = 0;
      virtual FLAC__bool FLAC__stream_encoder_set_verify(FLAC__StreamEncoder *encoder, FLAC__bool value) = 0;
      virtual FLAC__bool FLAC__stream_encoder_set_channels(FLAC__StreamEncoder *encoder, unsigned value) = 0;
      virtual FLAC__bool FLAC__stream_encoder_set_bits_per_sample(FLAC__StreamEncoder *encoder, unsigned value) = 0;
      virtual FLAC__bool FLAC__stream_encoder_set_sample_rate(FLAC__StreamEncoder *encoder, unsigned value) = 0;
      virtual FLAC__bool FLAC__stream_encoder_set_compression_level(FLAC__StreamEncoder *encoder, unsigned value) = 0;
      virtual FLAC__bool FLAC__stream_encoder_set_blocksize(FLAC__StreamEncoder *encoder, unsigned value) = 0;
      virtual FLAC__bool FLAC__stream_encoder_set_metadata(FLAC__StreamEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks) = 0;
      virtual FLAC__StreamEncoderInitStatus FLAC__stream_encoder_init_stream(FLAC__StreamEncoder *encoder, FLAC__StreamEncoderWriteCallback write_callback, FLAC__StreamEncoderSeekCallback seek_callback, FLAC__StreamEncoderTellCallback tell_callback, FLAC__StreamEncoderMetadataCallback metadata_callback, void *client_data) = 0;
      virtual FLAC__bool FLAC__stream_encoder_finish(FLAC__StreamEncoder *encoder) = 0;
      virtual FLAC__bool FLAC__stream_encoder_process_interleaved(FLAC__StreamEncoder *encoder, const FLAC__int32 buffer[], unsigned samples) = 0;
      virtual FLAC__StreamMetadata* FLAC__metadata_object_new(FLAC__MetadataType type) = 0;
      virtual void FLAC__metadata_object_delete(FLAC__StreamMetadata *object) = 0;
      virtual FLAC__bool FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(FLAC__StreamMetadata_VorbisComment_Entry *entry, const char *field_name, const char *field_value) = 0;
      virtual FLAC__bool FLAC__metadata_object_vorbiscomment_append_comment(FLAC__StreamMetadata *object, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy) = 0;
      virtual FLAC__StreamEncoderState 	FLAC__stream_encoder_get_state(const FLAC__StreamEncoder *encoder) = 0;
// clang-format on
    };

    //throw exception in case of error
    Api::Ptr LoadDynamicApi();
  }
}  // namespace Sound
