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

//local includes
#include "operations.h"

namespace Playlist
{
  namespace Item
  {
    class StatisticTextNotification : public Playlist::TextNotification
    {
    public:
      typedef boost::shared_ptr<StatisticTextNotification> Ptr;

      virtual void AddInvalid() = 0;
      virtual void AddValid(const String& type, const Time::MillisecondsDuration& duration, std::size_t size) = 0;
    };

    TextResultOperation::Ptr CreateCollectStatisticOperation(StatisticTextNotification::Ptr result);
    TextResultOperation::Ptr CreateCollectStatisticOperation(Playlist::Model::IndexSetPtr items, StatisticTextNotification::Ptr result);
  }
}
