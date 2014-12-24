/**
* 
* @file
*
* @brief  MIDI tracks format support interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>
//library includes
#include <formats/chiptune.h>
#include <time/stamp.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace MIDI
    {
      typedef Time::Microseconds TimeBase;
      
      class Builder
      {
      public:
        typedef boost::shared_ptr<Builder> Ptr;
        virtual ~Builder() {}

        // Initial tick timebase in case of SMPTE format
        virtual void SetTimeBase(TimeBase tickPeriod) = 0;
        // Ticks per quarter note in case of PPQN format
        virtual void SetTicksPerQN(uint_t ticks) = 0;
        // Quarter note period in case of PPQN format
        virtual void SetTempo(TimeBase qnPeriod) = 0;
        
        virtual void StartTrack(uint_t idx) = 0;
        virtual void StartEvent(uint32_t ticksDelta) = 0;
        virtual void SetMessage(uint8_t status, uint8_t data) = 0;
        virtual void SetMessage(uint8_t status, uint8_t data1, uint8_t data2) = 0;
        //! sysex message without prefix
        virtual void SetMessage(const Dump& sysex) = 0;
        virtual void SetTitle(const String& title) = 0;
        virtual void FinishTrack() = 0;
      };

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
      Builder& GetStubBuilder();
    }
  }
}
