/**
* 
* @file
*
* @brief  Metadata builder stub implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "builder_meta.h"

namespace Formats
{
  namespace Chiptune
  {
    class StubMetaBuilder : public MetaBuilder
    {
    public:
      virtual void SetProgram(const String& /*program*/) {}
      virtual void SetTitle(const String& /*title*/) {}
      virtual void SetAuthor(const String& /*author*/) {}
    };

    MetaBuilder& GetStubMetaBuilder()
    {
      static StubMetaBuilder instance;
      return instance;
    }
  }
}
