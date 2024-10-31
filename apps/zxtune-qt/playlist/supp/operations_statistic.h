/**
 *
 * @file
 *
 * @brief Playlist statistic operations interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "apps/zxtune-qt/playlist/supp/controller.h"
#include "apps/zxtune-qt/playlist/supp/model.h"
#include "apps/zxtune-qt/playlist/supp/operations.h"

#include "time/duration.h"

#include "string_view.h"

#include <cstddef>
#include <memory>

namespace Playlist::Item
{
  class StatisticTextNotification : public Playlist::TextNotification
  {
  public:
    using Ptr = std::shared_ptr<StatisticTextNotification>;

    virtual void AddInvalid() = 0;
    virtual void AddValid() = 0;
    virtual void AddType(StringView type) = 0;
    virtual void AddDuration(const Time::Milliseconds& duration) = 0;
    virtual void AddSize(std::size_t size) = 0;
    virtual void AddPath(StringView path) = 0;
  };

  TextResultOperation::Ptr CreateCollectStatisticOperation(StatisticTextNotification::Ptr result);
  TextResultOperation::Ptr CreateCollectStatisticOperation(Playlist::Model::IndexSet::Ptr items,
                                                           StatisticTextNotification::Ptr result);
}  // namespace Playlist::Item
