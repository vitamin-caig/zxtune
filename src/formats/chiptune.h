/**
*
* @file     formats/chiptune.h
* @brief    Interface for chiptunes data accessors
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __FORMATS_CHIPTUNE_H_DEFINED__
#define __FORMATS_CHIPTUNE_H_DEFINED__

//common includes
#include <parameters.h>
//library includes
#include <binary/container.h>
#include <binary/format.h>
//std includes
#include <memory>

namespace Formats
{
  namespace Chiptune
  {
    class Container : public Binary::Container
    {
    public:
      typedef boost::shared_ptr<const Container> Ptr;

      virtual uint_t FixedChecksum() const = 0;
    };

    class Decoder
    {
    public:
      typedef boost::shared_ptr<const Decoder> Ptr;
      virtual ~Decoder() {}

      virtual String GetDescription() const = 0;

      virtual Binary::Format::Ptr GetFormat() const = 0;

      virtual bool Check(const Binary::Container& rawData) const = 0;
      virtual Container::Ptr Decode(const Binary::Container& rawData) const = 0;
    };
  }
}

#endif //__FORMATS_CHIPTUNE_H_DEFINED__
