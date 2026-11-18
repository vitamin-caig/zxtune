/**
 *
 * @file
 *
 * @brief  Generic framed module renderer
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/iterator.h"
#include "module/players/parameters.h"

#include "module/renderer.h"

#include "make_ptr.h"

#include <cstdint>
#include <utility>

namespace Module
{
  template<class Device, class Chunk>
  Chunk DeduceRenderDataChunk(void (Device::*)(Chunk));

  template<class IteratorType, class DeviceType>
  class FramedRenderer : public Renderer
  {
    using Chunk = decltype(DeduceRenderDataChunk(&DeviceType::RenderData));

  public:
    FramedRenderer(FrameDurationParameter frameDuration, typename IteratorType::Ptr iterator,
                   typename DeviceType::Ptr device)
      : FrameDuration(std::move(frameDuration))
      , Iterator(std::move(iterator))
      , Device(std::move(device))
    {}

    State GetState() const override
    {
      return ToState(Iterator->GetState(), FrameDuration.Get());
    }

    Sound::Chunk Render() override
    {
      TransferChunk();
      FrameDuration.Synchronize();
      LastChunk.TimeStamp += FrameDuration.Get();
      return Device->RenderTill(LastChunk.TimeStamp);
    }

    void Reset() override
    {
      Iterator->Reset();
      Device->Reset();
      LastChunk.TimeStamp = {};
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      FrameDuration.Synchronize();
      const auto targetFrame = Time::Milliseconds(request.Get()).Divide<uint_t>(FrameDuration.Get());

      auto current = Iterator->GetState();
      if (targetFrame < current.At)
      {
        Reset();
        current = Iterator->GetState();
      }

      const auto frameDuration = FrameDuration.Get();
      for (uint64_t frame = current.At; frame < targetFrame && Iterator->GetState().LoopCount == current.LoopCount;
           ++frame)
      {
        LastChunk.Data = Iterator->Advance();
        LastChunk.TimeStamp += frameDuration;
        Device->UpdateState(LastChunk);
      }
    }

  private:
    void TransferChunk()
    {
      LastChunk.Data = Iterator->Advance();
      Device->RenderData(LastChunk);
    }

  private:
    FrameDurationParameter FrameDuration;
    const typename IteratorType::Ptr Iterator;
    const typename DeviceType::Ptr Device;
    Chunk LastChunk;
  };

  template<class IteratorPtr, class DevicePtr>
  Renderer::Ptr CreateFramedRenderer(Parameters::Accessor::Ptr params, Time::Microseconds defaultFrameDuration,
                                     IteratorPtr iterator, DevicePtr device)
  {
    using IteratorType = typename IteratorPtr::element_type;
    using DeviceType = typename DevicePtr::element_type;
    return MakePtr<FramedRenderer<IteratorType, DeviceType>>(
        FrameDurationParameter(std::move(params), defaultFrameDuration), std::move(iterator), std::move(device));
  }
}  // namespace Module
