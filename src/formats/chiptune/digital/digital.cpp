/**
* 
* @file
*
* @brief  Simple digital trackers support implementation
*
* @author vitamin.caig@gmail.com
*
**/

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
      MetaBuilder& GetMetaBuilder() override
      {
        return GetStubMetaBuilder();
      }

      void SetInitialTempo(uint_t /*tempo*/) override {}
      void SetSamplesFrequency(uint_t /*freq*/) override {}
      void SetSample(uint_t /*index*/, std::size_t /*loop*/, Binary::Data::Ptr /*content*/, bool /*is4Bit*/) override {}
      void SetPositions(const std::vector<uint_t>& /*positions*/, uint_t /*loop*/) override {}

      PatternBuilder& StartPattern(uint_t /*index*/) override
      {
        return GetStubPatternBuilder();
      }

      void StartChannel(uint_t /*index*/) override {}
      void SetRest() override {}
      void SetNote(uint_t /*note*/) override {}
      void SetSample(uint_t /*sample*/) override {}
    };

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }
  }//namespace Digital
} //namespace Chiptune
} //namespace Formats
