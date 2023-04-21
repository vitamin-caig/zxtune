/**
 *
 * @file
 *
 * @brief  VorbisEnc subsystem API gate interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// std includes
#include <memory>
// platform-specific includes
#include <vorbis/vorbisenc.h>

namespace Sound
{
  namespace VorbisEnc
  {
    class Api
    {
    public:
      typedef std::shared_ptr<Api> Ptr;
      virtual ~Api() = default;

      
      virtual int vorbis_encode_init(vorbis_info *vi, long channels, long rate, long max_bitrate, long nominal_bitrate, long min_bitrate) = 0;
      virtual int vorbis_encode_init_vbr(vorbis_info *vi, long channels, long rate, float base_quality) = 0;
    };

    //throw exception in case of error
    Api::Ptr LoadDynamicApi();

  }
}  // namespace Sound
