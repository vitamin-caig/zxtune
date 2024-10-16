/**
 *
 * @file
 *
 * @brief  OGG subsystem API gate interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// std includes
#include <memory>
// platform-specific includes
#include <ogg/ogg.h>

namespace Sound::Ogg
{
  class Api
  {
  public:
    using Ptr = std::shared_ptr<Api>;
    virtual ~Api() = default;

    // clang-format off

    virtual int ogg_stream_init(ogg_stream_state *os, int serialno) = 0;
    virtual int ogg_stream_clear(ogg_stream_state *os) = 0;
    virtual int ogg_stream_packetin(ogg_stream_state *os, ogg_packet *op) = 0;
    virtual int ogg_stream_pageout(ogg_stream_state *os, ogg_page *og) = 0;
    virtual int ogg_stream_flush(ogg_stream_state *os, ogg_page *og) = 0;
    virtual int ogg_page_eos(const ogg_page *og) = 0;
    // clang-format on
  };

  // throw exception in case of error
  Api::Ptr LoadDynamicApi();
}  // namespace Sound::Ogg
