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

namespace Binary
{
  typedef DataReceiver<Data> OutputStream;
}

#endif //BINARY_OUTPUT_STREAM_H_DEFINED
