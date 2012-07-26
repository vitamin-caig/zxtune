/*
Abstract:
  Ogg api gate interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef SOUND_BACKENDS_OGG_API_H_DEFINED
#define SOUND_BACKENDS_OGG_API_H_DEFINED

//platform-specific includes
#include <ogg/ogg.h>
//boost includes
#include <boost/shared_ptr.hpp>

namespace ZXTune
{
  namespace Sound
  {
    namespace Ogg
    {
      class Api
      {
      public:
        typedef boost::shared_ptr<Api> Ptr;
        virtual ~Api() {}

        
        virtual int ogg_stream_init(ogg_stream_state *os, int serialno) = 0;
        virtual int ogg_stream_clear(ogg_stream_state *os) = 0;
        virtual int ogg_stream_packetin(ogg_stream_state *os, ogg_packet *op) = 0;
        virtual int ogg_stream_pageout(ogg_stream_state *os, ogg_page *og) = 0;
        virtual int ogg_stream_flush(ogg_stream_state *os, ogg_page *og) = 0;
        virtual int ogg_page_eos(const ogg_page *og) = 0;
      };

      //throw exception in case of error
      Api::Ptr LoadDynamicApi();

    }
  }
}

#endif //SOUND_BACKENDS_OGG_API_H_DEFINED
