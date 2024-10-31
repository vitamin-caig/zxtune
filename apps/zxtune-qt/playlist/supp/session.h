/**
 *
 * @file
 *
 * @brief Sessions support interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "apps/zxtune-qt/playlist/supp/container.h"

namespace Playlist
{
  class Session
  {
  public:
    using Ptr = std::shared_ptr<Session>;

    virtual ~Session() = default;

    virtual bool Empty() const = 0;
    virtual void Load(Container::Ptr container) = 0;
    virtual void Save(Controller::Iterator::Ptr it) = 0;

    // creator
    static Ptr Create();
  };
}  // namespace Playlist
