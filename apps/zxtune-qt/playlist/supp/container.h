/**
 *
 * @file
 *
 * @brief Playlist container interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "apps/zxtune-qt/playlist/supp/controller.h"
// library includes
#include <parameters/accessor.h>
// qt includes
#include <QtCore/QObject>

namespace Playlist
{
  class Container : public QObject
  {
    Q_OBJECT
  public:
    using Ptr = std::shared_ptr<Container>;

    // creator
    static Ptr Create(Parameters::Accessor::Ptr parameters);

    virtual Controller::Ptr CreatePlaylist(const QString& name) const = 0;
    virtual void OpenPlaylist(const QString& filename) = 0;
  signals:
    void PlaylistCreated(Playlist::Controller::Ptr);
  };

  void Save(Controller& ctrl, const QString& filename, uint_t flags);
}  // namespace Playlist
