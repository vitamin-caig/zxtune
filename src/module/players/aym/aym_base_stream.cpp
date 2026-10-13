/**
 *
 * @file
 *
 * @brief  AYM-based stream chiptunes support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/aym/aym_base_stream.h"

#include "module/players/streaming.h"

#include "make_ptr.h"

#include <utility>

namespace Module::AYM
{
  class StreamedChiptune : public Chiptune
  {
  public:
    StreamedChiptune(Time::Microseconds frameDuration, StreamModel::Ptr model)
      : FrameDuration(frameDuration)
      , Data(std::move(model))
    {}

    Time::Microseconds GetFrameDuration() const override
    {
      return FrameDuration;
    }

    FramedInformation GetInformation() const override
    {
      return Data->GetInformation();
    }

    DataIterator::Ptr CreateDataIterator(TrackParameters::Ptr /*trackParams*/) const override
    {
      return MakePtr<StreamIterator<StreamModel>>(Data);
    }

  private:
    const Time::Microseconds FrameDuration;
    const StreamModel::Ptr Data;
  };

  Chiptune::Ptr CreateStreamedChiptune(Time::Microseconds frameDuration, StreamModel::Ptr model)
  {
    return MakePtr<StreamedChiptune>(frameDuration, std::move(model));
  }

  Chiptune::Ptr CreateStreamedChiptune(StreamModel::Ptr model)
  {
    return CreateStreamedChiptune(BASE_FRAME_DURATION, std::move(model));
  }
}  // namespace Module::AYM
