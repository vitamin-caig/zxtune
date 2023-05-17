/**
 *
 * @file
 *
 * @brief  Multitrack container support interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// library includes
#include <formats/chiptune.h>

namespace Formats::Chiptune
{
  namespace MultiTrackContainer
  {
    class Builder
    {
    public:
      virtual ~Builder() = default;

      /*
        Can be called to any of the entity (Module -> Track)
        Workflow:
          SetProperty() <- global props
          StartTrack(0)
          SetProperty() <- track0 props
          SetData()
          SetProperty() <- track0_data0 props
          SetData()
          SetProperty() <- track0_data1 props
          StartTrack(1)
          SetProperty() <- track1 props
          ...
      */
      virtual void SetAuthor(StringView author) = 0;
      virtual void SetTitle(StringView title) = 0;
      virtual void SetAnnotation(StringView annotation) = 0;
      // arbitrary property
      virtual void SetProperty(StringView name, StringView value) = 0;

      virtual void StartTrack(uint_t idx) = 0;
      virtual void SetData(Binary::Container::Ptr data) = 0;
    };

    Builder& GetStubBuilder();

    class ContainerBuilder : public Builder
    {
    public:
      using Ptr = std::shared_ptr<ContainerBuilder>;
      virtual Binary::Data::Ptr GetResult() = 0;
    };

    ContainerBuilder::Ptr CreateBuilder();
    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
  }  // namespace MultiTrackContainer

  Decoder::Ptr CreateMultiTrackContainerDecoder();
}  // namespace Formats::Chiptune
