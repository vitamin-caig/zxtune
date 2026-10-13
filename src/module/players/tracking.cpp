/**
 *
 * @file
 *
 * @brief  Track modules support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/tracking.h"

#include <utility>

namespace Module
{
  FramedInformation CreateTrackInfoFixedChannels(const TrackModel& model, uint_t channels)
  {
    const auto& order = model.GetOrder();
    TrackLayout track = {
        .ChannelsCount = channels, .PositionsCount = order.GetSize(), .LoopPosition = order.GetLoopPosition()};
    ModelTrackCursor cursor(model);
    cursor.Seek(track.LoopPosition);
    const auto loopFrame = cursor.Get().At;
    cursor.Seek(track.PositionsCount);
    const auto totalFrame = cursor.Get().At;
    return {.Total = totalFrame, .Loop = loopFrame, .Track = std::move(track)};
  }
}  // namespace Module
