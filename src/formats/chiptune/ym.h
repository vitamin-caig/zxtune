/*
Abstract:
  YM/VTX modules format description

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef FORMATS_CHIPTUNE_YM_H_DEFINED
#define FORMATS_CHIPTUNE_YM_H_DEFINED

//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace YM
    {
      class Builder
      {
      public:
        typedef boost::shared_ptr<Builder> Ptr;
        virtual ~Builder() {}

        //YMx
        virtual void SetVersion(const String& version) = 0;
        // Default: false
        virtual void SetChipType(bool ym) = 0;
        // Default: abc.
        // 0 - mono, 1- abc, 2- acb, 3- bac, 4- bca, 5- cab, 6- cba
        virtual void SetStereoMode(uint_t mode) = 0;
        // Default: 0
        virtual void SetLoop(uint_t loop) = 0;
        virtual void SetDigitalSample(uint_t idx, const Dump& data) = 0;
        virtual void SetClockrate(uint64_t freq) = 0;
        // Default: 50
        virtual void SetIntFreq(uint_t freq) = 0;
        virtual void SetTitle(const String& title) = 0;
        virtual void SetAuthor(const String& author) = 0;
        virtual void SetComment(const String& comment) = 0;
        virtual void SetYear(uint_t year) = 0;
        virtual void SetProgram(const String& program) = 0;
        virtual void SetEditor(const String& editor) = 0;

        virtual void AddData(const Dump& registers) = 0;
      };

      Formats::Chiptune::Container::Ptr ParseUnpacked(const Binary::Container& data, Builder& target);
      Formats::Chiptune::Container::Ptr ParseVTX(const Binary::Container& data, Builder& target);
      Builder& GetStubBuilder();
    }
  }
}

#endif //FORMATS_CHIPTUNE_YM_H_DEFINED
