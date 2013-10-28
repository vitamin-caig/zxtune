/**
*
* @file     analysis/scanner.h
* @brief    Data analysing service interface
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef ANALYSIS_SCANNER_H_DEFINED
#define ANALYSIS_SCANNER_H_DEFINED

//library includes
#include <binary/container.h>
#include <formats/archived.h>
#include <formats/chiptune.h>
#include <formats/image.h>
#include <formats/packed.h>

namespace Analysis
{
  class Scanner
  {
  public:
    typedef boost::shared_ptr<const Scanner> Ptr;
    typedef boost::shared_ptr<Scanner> RWPtr;

    virtual ~Scanner() {}

    virtual void AddDecoder(Formats::Archived::Decoder::Ptr decoder) = 0;
    virtual void AddDecoder(Formats::Packed::Decoder::Ptr decoder) = 0;
    virtual void AddDecoder(Formats::Image::Decoder::Ptr decoder) = 0;
    virtual void AddDecoder(Formats::Chiptune::Decoder::Ptr decoder) = 0;

    class Target
    {
    public:
      virtual ~Target() {}

      virtual void Apply(const Formats::Archived::Decoder& decoder, std::size_t offset, Formats::Archived::Container::Ptr data) = 0;
      virtual void Apply(const Formats::Packed::Decoder& decoder, std::size_t offset, Formats::Packed::Container::Ptr data) = 0;
      virtual void Apply(const Formats::Image::Decoder& decoder, std::size_t offset, Formats::Image::Container::Ptr data) = 0;
      virtual void Apply(const Formats::Chiptune::Decoder& decoder, std::size_t offset, Formats::Chiptune::Container::Ptr data) = 0;
      virtual void Apply(std::size_t offset, Binary::Container::Ptr data) = 0;
    };

    virtual void Scan(Binary::Container::Ptr source, Target& target) const = 0;
  };

  Scanner::RWPtr CreateScanner();
}

#endif //ANALYSIS_SCANNER_H_DEFINED
