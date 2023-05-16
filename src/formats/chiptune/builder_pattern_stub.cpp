/**
 *
 * @file
 *
 * @brief  Pattern builder stub implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/builder_pattern.h"

namespace Formats::Chiptune
{
  class StubPatternBuilder : public PatternBuilder
  {
  public:
    void Finish(uint_t /*size*/) override {}
    void StartLine(uint_t /*index*/) override {}
    void SetTempo(uint_t /*tempo*/) override {}
  };

  PatternBuilder& GetStubPatternBuilder()
  {
    static StubPatternBuilder instance;
    return instance;
  }
}  // namespace Formats::Chiptune
