/**
* 
* @file
*
* @brief  TurboFM Compiled support interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>
//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace TFC
    {
      class Builder
      {
      public:
        typedef boost::shared_ptr<Builder> Ptr;
        virtual ~Builder() {}

        virtual void SetVersion(const String& version) = 0;
        virtual void SetIntFreq(uint_t freq) = 0;
        virtual void SetTitle(const String& title) = 0;
        virtual void SetAuthor(const String& author) = 0;
        virtual void SetComment(const String& comment) = 0;

        //! building channel->frame
        virtual void StartChannel(uint_t idx) = 0;
        virtual void StartFrame() = 0;
        virtual void SetSkip(uint_t count) = 0;
        virtual void SetLoop() = 0;
        virtual void SetSlide(uint_t slide) = 0;
        virtual void SetKeyOff() = 0;
        virtual void SetFreq(uint_t freq) = 0;
        virtual void SetRegister(uint_t reg, uint_t val) = 0;
        virtual void SetKeyOn() = 0;
      };

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
      Builder& GetStubBuilder();
    }
  }
}
