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

//library includes
#include <binary/container.h>
#include <binary/format.h>
//std includes
#include <memory>

namespace Formats
{
  namespace Packed
  {
    class Container : public Binary::Container
    {
    public:
      typedef boost::shared_ptr<const Container> Ptr;

      virtual std::size_t PackedSize() const = 0;
    };

    class Decoder
    {
    public:
      typedef boost::shared_ptr<const Decoder> Ptr;
      virtual ~Decoder() {}

      virtual Binary::Format::Ptr GetFormat() const = 0;
      virtual bool Check(const Binary::Container& rawData) const = 0;
      virtual Container::Ptr Decode(const Binary::Container& rawData) const = 0;
    };
  }
}

#endif //__FORMATS_PACKED_H_DEFINED__
