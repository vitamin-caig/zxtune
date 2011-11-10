/*
Abstract:
  PSG modules format description

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace PSG
    {
      class Builder
      {
      public:
        typedef boost::shared_ptr<Builder> Ptr;
        virtual ~Builder() {}

        virtual void AddChunks(std::size_t count) = 0;
        virtual void SetRegister(uint_t reg, uint_t val) = 0;
      };

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
      Builder& GetStubBuilder();
    }
  }
}
