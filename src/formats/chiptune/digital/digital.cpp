/**
 *
 * @file
 *
 * @brief  Simple digital trackers support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/digital/digital.h"

#include "formats/chiptune/builder_meta.h"
#include "formats/chiptune/builder_pattern.h"

namespace Formats::Chiptune::Digital
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
    void SetSample(uint_t /*index*/, std::size_t /*loop*/, Binary::View /*content*/, bool /*is4Bit*/) override {}
    void SetPositions(Positions /*positions*/) override {}

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
}  // namespace Formats::Chiptune::Digital
