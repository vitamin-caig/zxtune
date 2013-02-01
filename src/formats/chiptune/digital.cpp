/*
Abstract:
  Base implementation for simple digital trackers

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "digital.h"

namespace Formats
{
namespace Chiptune
{
  namespace Digital
  {
    class StubBuilder : public Builder
    {
    public:
      virtual void SetTitle(const String& /*title*/) {}
      virtual void SetProgram(const String& /*program*/) {}
      virtual void SetInitialTempo(uint_t /*tempo*/) {}
      virtual void SetSample(uint_t /*index*/, std::size_t /*loop*/, Binary::Data::Ptr /*content*/, bool /*is4Bit*/) {}
      virtual void SetPositions(const std::vector<uint_t>& /*positions*/, uint_t /*loop*/) {}
      virtual void StartPattern(uint_t /*index*/) {}
      virtual void StartLine(uint_t /*index*/) {}
      virtual void SetTempo(uint_t /*tempo*/) {}
      virtual void StartChannel(uint_t /*index*/) {}
      virtual void SetRest() {}
      virtual void SetNote(uint_t /*note*/) {}
      virtual void SetSample(uint_t /*sample*/) {}
    };

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }
  }//namespace Digital
} //namespace Chiptune
} //namespace Formats
