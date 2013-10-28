/**
*
* @file      binary/output_stream.h
* @brief     Output stream interface
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef BINARY_OUTPUT_STREAM_H_DEFINED
#define BINARY_OUTPUT_STREAM_H_DEFINED

//common includes
#include <data_streaming.h>
#include <types.h>
//library includes
#include <binary/data.h>

namespace Binary
{
  typedef DataReceiver<Data> OutputStream;

  class SeekableOutputStream : public OutputStream
  {
  public:
    typedef boost::shared_ptr<SeekableOutputStream> Ptr;

    virtual void Seek(uint64_t pos) = 0;
    virtual uint64_t Position() const = 0;
  };
}

#endif //BINARY_OUTPUT_STREAM_H_DEFINED
