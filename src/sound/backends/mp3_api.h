/*
Abstract:
  Mp3 api gate interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef SOUND_BACKENDS_MP3_API_H_DEFINED
#define SOUND_BACKENDS_MP3_API_H_DEFINED

//platform-specific includes
#include <lame/lame.h>
//boost includes
#include <boost/shared_ptr.hpp>

namespace ZXTune
{
  namespace Sound
  {
    namespace Mp3
    {
      class Api
      {
      public:
        typedef boost::shared_ptr<Api> Ptr;
        virtual ~Api() {}

        
        virtual const char* get_lame_version() = 0;
        virtual lame_t lame_init() = 0;
        virtual int lame_close(lame_t ctx) = 0;
        virtual int lame_set_in_samplerate(lame_t ctx, int rate) = 0;
        virtual int lame_set_out_samplerate(lame_t ctx, int rate) = 0;
        virtual int lame_set_bWriteVbrTag(lame_t ctx, int flag) = 0;
        virtual int lame_set_num_channels(lame_t ctx, int chans) = 0;
        virtual int lame_set_brate(lame_t ctx, int brate) = 0;
        virtual int lame_set_VBR(lame_t ctx, vbr_mode mode) = 0;
        virtual int lame_set_VBR_q(lame_t ctx, int quality) = 0;
        virtual int lame_set_VBR_mean_bitrate_kbps(lame_t ctx, int brate) = 0;
        virtual int lame_init_params(lame_t ctx) = 0;
        virtual int lame_encode_buffer_interleaved(lame_t ctx, short int* pcm, int samples, unsigned char* dst, int dstSize) = 0;
        virtual int lame_encode_flush(lame_t ctx, unsigned char* dst, int dstSize) = 0;
        virtual void id3tag_init(lame_t ctx) = 0;
        virtual void id3tag_add_v2(lame_t ctx) = 0;
        virtual void id3tag_set_title(lame_t ctx, const char* title) = 0;
        virtual void id3tag_set_artist(lame_t ctx, const char* artist) = 0;
        virtual void id3tag_set_comment(lame_t ctx, const char* comment) = 0;
      };

      //throw exception in case of error
      Api::Ptr LoadDynamicApi();

    }
  }
}

#endif //SOUND_BACKENDS_MP3_API_H_DEFINED
