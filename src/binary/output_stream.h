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

//common includes
#include <data_streaming.h>
#include <types.h>
//library includes
#include <binary/data_view.h>

namespace Binary
{
  typedef DataReceiver<DataView> OutputStream;

  class SeekableOutputStream : public OutputStream
  {
  public:
    typedef std::shared_ptr<SeekableOutputStream> Ptr;

    virtual void Seek(uint64_t pos) = 0;
    virtual uint64_t Position() const = 0;
  };
}
