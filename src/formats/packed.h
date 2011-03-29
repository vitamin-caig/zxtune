/**
*
* @file     formats/packed.h
* @brief    Interface for packed data accessors
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __FORMATS_PACKED_H_DEFINED__
#define __FORMATS_PACKED_H_DEFINED__

//common includes
#include <detector.h>
//std includes
#include <memory>

namespace Formats
{
  namespace Packed
  {
    class Decoder
    {
    public:
      typedef std::auto_ptr<const Decoder> Ptr;
      virtual ~Decoder() {}

      virtual DataFormat::Ptr GetFormat() const = 0;
      virtual bool Check(const void* data, std::size_t availSize) const = 0;
      virtual std::auto_ptr<Dump> Decode(const void* data, std::size_t availSize, std::size_t& usedSize) const = 0;
    };
  }
}

#endif //__FORMATS_PACKED_H_DEFINED__
