/**
 *
 * @file
 *
 * @brief  Output stream interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// library includes
#include <binary/view.h>
#include <tools/data_streaming.h>

namespace Binary
{
  using OutputStream = DataReceiver<View>;

  class SeekableOutputStream : public OutputStream
  {
  public:
    using Ptr = std::shared_ptr<SeekableOutputStream>;

    virtual void Seek(uint64_t pos) = 0;
    virtual uint64_t Position() const = 0;
  };
}  // namespace Binary
