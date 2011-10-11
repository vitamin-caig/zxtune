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

namespace Devices
{
  namespace AYM
  {
    struct DataChunk;
  }
}

namespace Formats
{
  namespace Chiptune
  {
    namespace PSG
    {
      class ChunksSet
      {
      public:
        typedef boost::shared_ptr<const ChunksSet> Ptr;
        virtual ~ChunksSet() {}

        virtual std::size_t Count() const = 0;
        virtual const Devices::AYM::DataChunk& Get(std::size_t frameNum) const = 0;
      };

      class Builder
      {
      public:
        typedef boost::shared_ptr<Builder> Ptr;
        virtual ~Builder() {}

        virtual void AddChunks(std::size_t count) = 0;
        virtual void SetRegister(uint_t reg, uint_t val) = 0;

        virtual ChunksSet::Ptr Result() const = 0;
      };

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
      Builder& GetStubBuilder();
      Builder::Ptr CreateBuilder();
    }
  }
}
