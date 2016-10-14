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
      void SetProgram(const String& /*program*/) override {}
      void SetTitle(const String& /*title*/) override {}
      void SetAuthor(const String& /*author*/) override {}
      void SetStrings(const Strings::Array& /*strings*/) override {}
    };

    MetaBuilder& GetStubMetaBuilder()
    {
      static StubMetaBuilder instance;
      return instance;
    }
  }
}
