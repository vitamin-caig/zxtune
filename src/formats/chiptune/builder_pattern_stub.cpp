/**
* 
* @file
*
* @brief  Pattern builder stub implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "builder_pattern.h"

namespace Formats
{
  namespace Chiptune
  {
    class StubPatternBuilder : public PatternBuilder
    {
    public:
      virtual void Finish(uint_t /*size*/) {}
      virtual void StartLine(uint_t /*index*/) {}
      virtual void SetTempo(uint_t /*tempo*/) {}
    };

    PatternBuilder& GetStubPatternBuilder()
    {
      static StubPatternBuilder instance;
      return instance;
    }
  }
}
